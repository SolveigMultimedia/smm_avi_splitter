/*
Copyright (c) 2015 Solveig Multimedia www.solveigmm.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:



The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.



THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include "stdafx.h"
#include "avisplitter.h"
#include "proppages.h"


//////////////////////////////// filter registration tables
const AMOVIESETUP_MEDIATYPE 
CAVISplitter::m_sudType[] = 
{
	{
		&MEDIATYPE_Stream,
		&MEDIASUBTYPE_NULL
	},
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_NULL
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_NULL
	}
};
const AMOVIESETUP_PIN 
CAVISplitter::m_sudPin[] = 
{
	{
		L"Input",           // pin name
		FALSE,              // is rendered?    
		FALSE,              // is output?
		FALSE,              // zero instances allowed?
		FALSE,              // many instances allowed?
		&CLSID_NULL,        // connects to filter (for bridge pins)
		NULL,               // connects to pin (for bridge pins)
		1,                  // count of registered media types
		&m_sudType[0]       // list of registered media types    
	},
};

//static 
const AMOVIESETUP_FILTER 
CAVISplitter::m_sudFilter =
{
	&__uuidof(CAVISplitter),		// filter clsid
	L"SolveigMM AVI Splitter",		// filter name
	MERIT_NORMAL,					// MERIT_NORMAL would be here if this weren't a sample
	1,								// count of registered pins
	m_sudPin						// list of pins to register
};


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CAVISplitter
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// constructor method used by class factory
CUnknown* WINAPI CAVISplitter::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
	CAVISplitter* splitter = new CAVISplitter(pUnk, phr);
	*phr = splitter->Init();
	return splitter;
}

CAVISplitter::CAVISplitter(LPUNKNOWN pUnk, HRESULT* phr) 
:	CBaseFilter(NAME("CAVISplitter"), pUnk, &m_csFilter, *m_sudFilter.clsID),
	CPersistStream(pUnk, phr),
	m_in_pin(NULL),
	m_tStart(0),
	m_tStop(MAX_TIME),       // less than MAX_TIME so we can add one second to it
	m_dRate(1.0),
	m_bLocating(false),
	m_nTries(0),
	m_llFileSize(0),
	m_sync_time(0),
	m_pSeekingPin(NULL),
    m_totalDuration(0)
{
		
}
// creates internal class objects
HRESULT CAVISplitter::Init()
{
	HRESULT hr = S_OK;
	// Initialize output pin's creator object
	m_pins_creator.Init(this);

	// Create intput pin here
	m_in_pin = new CStreamInputPin(this, &m_csFilter, &hr);

	// Set initial state of pulling thread to tm_exit
	m_thread_state = tm_exit;

	// Set initial state of worker thread id to 0
	m_threadid = 0;

	return hr;

}

CAVISplitter::~CAVISplitter()
{
	BreakInputConnect();
	delete m_in_pin;
}

STDMETHODIMP CAVISplitter::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
	if (iid == IID_ISpecifyPropertyPages)
		return GetInterface((ISpecifyPropertyPages *) this, ppv);
    else if (iid == IID_ISMMModuleConfig)
        return GetInterface((ISMMModuleConfig*)this, ppv);
	else if (iid == IID_IPersistStream)
		return GetInterface((CPersistStream*)this, ppv);
	return CBaseFilter::NonDelegatingQueryInterface(iid, ppv);
}
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// CBaseFilter methods

int CAVISplitter::GetPinCount()
{
	return m_outpins.size() + 1; // Number of output pins + one input pin
}

CBasePin *CAVISplitter::GetPin(int n)
{
	if (n == 0)
		return m_in_pin;
	
	// Get output pins from output pins array
	if (n > (int)m_outpins.size())
		return NULL;
	return m_outpins[n-1];
}

STDMETHODIMP CAVISplitter::Pause()
{
	// StartThread/Stop Threads called from Active/Inactive of input pins
	// so just pass to base class
	HRESULT hr = CBaseFilter::Pause();
	hr = StartThread();
	return hr;
}

STDMETHODIMP CAVISplitter::Stop()
{
	HRESULT hr = CBaseFilter::Stop();
	DeselectSeekingPin();
	hr = StopThread();    
	return hr;
}
//////////////////////////////////////////////////////////////////////////
HRESULT CAVISplitter::CompleteConnect(IPin* pPeer)
{
	// called when input is connected to 
	// validate the input stream, and locate 
	// media type and timing information.

	// we use pull mode
	IAsyncReaderPtr rdr = pPeer;
	if (rdr == NULL)
	{
		return E_NOINTERFACE;
	}

	try{
		m_scanner.Reset();
		m_scanner.SetScannerCallback(this);
		m_scanner.CanProcess(rdr );

		ReCreateOuputPins();
	}
	catch( char *s)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
	catch(...)
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}	


	return S_OK;
}

HRESULT CAVISplitter::BreakInputConnect()
{
	m_scanner.Reset();

	m_pins_creator.Reset();
	
	DeleteOuputPins();
    m_totalDuration = 0;
	return S_OK;
}
void CAVISplitter::DeleteOuputPins()
{
	// disconnect and delete all outputPins
	for (size_t  i = 0; i < m_outpins.size();i++)
	{
		CAVIOutputPin *p = m_outpins[i];
		if (p)
		{
			IPinPtr pPeer;
			p->ConnectedTo(&pPeer);
			if (pPeer != NULL)
			{
				p->Disconnect();
				pPeer->Disconnect();
			}
			p->Release();
		}
	}

	m_outpins.clear();
}

HRESULT CAVISplitter::BeginFlush()
{

	// flush each output pin
	for( int n = 0; n < (int)m_outpins.size(); n++)
	{
		CAVIOutputPin *p = m_outpins[n];
		if (p && p->IsConnected())
			p->DeliverBeginFlush();
	}
	return S_OK;
}

HRESULT CAVISplitter::EndFlush()
{
	// Pass to all output pins
	for( int n = 0; n < (int)m_outpins.size(); n++)
	{
		CAVIOutputPin *p = m_outpins[n];
		if (p && p->IsConnected())
			p->DeliverEndFlush();
	}
	return S_OK;
}

HRESULT CAVISplitter::EndOfStream()
{	
	return S_OK;
}

DWORD  CAVISplitter::ThreadProc()
{
	m_threadid = GetCurrentThreadId();
	while (1) 
	{
		DWORD cmd = GetRequest();
		switch (cmd)
		{
		case tm_exit:
            stopPinsProcessingThreads();
			Reply(S_OK);
            return 0;

		case tm_pause:
			// we are paused already
            stopPinsProcessingThreads();
			Reply(S_OK);
            break;

		case tm_start:
			startPinsProcessingThreads();
            Reply(S_OK);
            break;
		}

	}
	return 0;
}

// filter seeking ----------------------------------------------

REFERENCE_TIME CAVISplitter::GetDuration()
{

    if (m_totalDuration == 0)
    {
        for (size_t i = 0 ; i < m_outpins.size();i++)
        {
            if (m_outpins[i]->IsVideo())
            {
                m_totalDuration = m_outpins[i]->GetTotalDuration();
                break;
            }
            m_totalDuration = max(m_totalDuration,m_outpins[i]->GetTotalDuration());
        }
    }
	if (m_totalDuration > 0)
		return m_totalDuration;

	return MAX_TIME;
}

void CAVISplitter::GetSeekingParams(REFERENCE_TIME* ptStart, REFERENCE_TIME* ptStop, double* pdRate)
{
	if (ptStart != NULL)
	{
		*ptStart = m_tStart;
	}
	if (ptStop != NULL)
	{
		if (m_tStop == MAX_TIME)
		{
			m_tStop = GetDuration();
		}

		*ptStop = m_tStop;
	}
	if (pdRate != NULL)
	{
		*pdRate = m_dRate;
	}
}
HRESULT CAVISplitter::Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	// We must stop the pushing thread before we change
	// the seek parameters -- especially for VBR seeking, 
	// where we might re-seek based on the incoming data.	
	CAutoLock lock(&m_csSeeking);

	if (tStop == 0 || tStop < tStart)
		tStop = GetDuration();

	bool bShouldRestart = Suspend();
	m_tStart = tStart;
	m_tStop = tStop;
	m_dRate = dRate;
	m_scanner.Seek(m_tStart);
	if (bShouldRestart)
		Resume();
	
	return S_OK;
}

void CAVISplitter::SetSeekStartValue(REFERENCE_TIME& tStart)
{
	m_tStart = tStart;
}


HRESULT CAVISplitter::SetRate(double dRate)
{
	CAutoLock lock(&m_csSeeking);
	m_dRate = dRate;
	return S_OK;
}
bool CAVISplitter::Suspend()
{
	 // the parser cannot set the seek params until the pushing
    // thread is stopped
    ThreadMsg msg = m_thread_state;

    bool bShouldRestart = false;

    if (msg == tm_start) {

		BeginFlush();
        if (m_threadid != GetCurrentThreadId())
        {
            bShouldRestart = true;
            PauseThread();
        }
		EndFlush();
    }

    return bShouldRestart;
}

void CAVISplitter::Resume()
{
	StartThread();
}

HRESULT CAVISplitter::StartThread()
{
    CAutoLock lock(&m_AccessLock);
	
    HRESULT hr = S_OK;
    if (!ThreadExists()) 
	{
        // commit allocator
        // Start thread
        if (!Create()) {
            return E_FAIL;
        }
    }

	if (m_thread_state != tm_start)
	{
		m_thread_state = tm_start;
		hr = (HRESULT) CallWorker(m_thread_state);
	}
    return hr;
}

HRESULT CAVISplitter::PauseThread()
{
	CAutoLock lock(&m_AccessLock);

	if (!ThreadExists()) {
		return E_UNEXPECTED;
	}

	HRESULT hr = S_OK;

	// Need to flush to ensure the thread is not blocked
	// in WaitForNext
	m_scanner.BeginFlush();
	m_thread_state = tm_pause;
	hr = CallWorker(tm_pause);
	//WriteToLog("<CAVISplitter::PauseThread> CallWorker(tm_pause)");
	m_scanner.EndFlush();
	return hr;
}

HRESULT CAVISplitter::StopThread()
{
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists()) {
        return S_FALSE;
    }
	HRESULT hr = S_OK;

    // need to flush to ensure the thread is not blocked
    // in WaitForNext
    m_scanner.BeginFlush();    
    m_thread_state = tm_exit;
    hr = CallWorker(tm_exit);
	//WriteToLog("<CAVISplitter::StopThread> CallWorker(tm_exit)");
    m_scanner.EndFlush();

    // wait for thread to completely exit
    Close();
    return S_OK;
}

void CAVISplitter::stopPinsProcessingThreads()
{
    for (size_t i = 0; i < m_outpins.size();i++)
	{
		CAVIOutputPin *p = m_outpins[i];
       
		if (p && p->IsConnected())
        {
            p->StopProcess();
        }
			
	}
}

void CAVISplitter::startPinsProcessingThreads()
{
	if (!hasAnyOutpinConnected())
		return;

    for (size_t i = 0; i < m_outpins.size();i++)
	{
		CAVIOutputPin *p = m_outpins[i];
       
		if (p && p->IsConnected())
        {
            p->StartProcess();
        }
			
	}
}

CAVIScanner *CAVISplitter::GetAVIScanner()
{
	return &m_scanner;
}

CCritSec *CAVISplitter::GetFilterCS()
{
	return &m_csFilter;
}

void CAVISplitter::AddOutputPin( CAVIOutputPin *p)
{
	m_outpins.push_back(p);
}

STDMETHODIMP  CAVISplitter::SetValue( /*[in]*/ const GUID *pParamID, /*[in]*/ const VARIANT *pValue )
{
	
	
	
	return E_NOTIMPL;
}

STDMETHODIMP CAVISplitter::GetValue( /*[in]*/ const GUID *pParamID, /*[out]*/ VARIANT *pValue )
{
	
	return E_NOTIMPL;
}


#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
	if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
	if (FAILED(hr)) return hr;

HRESULT CAVISplitter::WriteToStream(IStream *pStream)
{
	HRESULT hr;
	
	return NOERROR;

} 

HRESULT CAVISplitter::ReadFromStream(IStream *pStream)
{
	HRESULT hr;

	return NOERROR;

}
STDMETHODIMP CAVISplitter::GetClassID(CLSID *pClsid)
{
	return CBaseFilter::GetClassID(pClsid);

} 
STDMETHODIMP CAVISplitter::IsDirty( void )
{
	return CPersistStream::IsDirty();
}

STDMETHODIMP CAVISplitter::Load( IStream *pStm )
{
	return CPersistStream::Load(pStm );
}

STDMETHODIMP CAVISplitter::Save( IStream *pStm , BOOL fClearDirty)
{
	return CPersistStream::Save(pStm,fClearDirty );
}

STDMETHODIMP CAVISplitter::GetSizeMax( ULARGE_INTEGER *pcbSize)
{
	return CPersistStream::GetSizeMax(pcbSize );
}

void CAVISplitter::SetTimeFormatForAllPins(const GUID* pFormat)
{
	for (size_t i = 0; i < m_outpins.size(); i++)
	{
		m_outpins[i]->SetCurrentTimeFormat(pFormat);
	}
}

bool CAVISplitter::hasConnectedVideoPin()
{
	for (size_t i = 0; i < m_outpins.size(); i++)
	{
		CAVIVideoPin* p = dynamic_cast<CAVIVideoPin*>(m_outpins[i]);
		if (p && p->IsConnected())
			return true;
	}
	return false;
}
bool CAVISplitter::SelectSeekingPin(CAVIOutputPin* pPin)
{
	CAutoLock lock(&m_csSeeking);
	if (m_pSeekingPin == NULL)
	{
		m_pSeekingPin = pPin;
	}
	return (m_pSeekingPin == pPin);
}
void CAVISplitter::DeselectSeekingPin()
{
	CAutoLock lock(&m_csSeeking);
    m_pSeekingPin = NULL;
}


void CAVISplitter::ReCreateOuputPins()
{
	DeleteOuputPins();

	m_pins_creator.CreatePins();
    for (size_t i = 0; i < m_outpins.size(); i++)
    {
        m_outpins[i]->SetIndex(m_scanner.getIndex(i));
        AVIStreamHeader strh = m_scanner.getStreamHeader(i);       
        m_outpins[i]->SetStreamHeader(strh);
    }
    
}

STDMETHODIMP CAVISplitter::GetPages( CAUUID *pPages )
{
	CheckPointer(pPages,E_POINTER);

	pPages->cElems = 2;
	pPages->pElems = (GUID *) CoTaskMemAlloc( sizeof(GUID)  * pPages->cElems );
	if (pPages->pElems == NULL) {
		return E_OUTOFMEMORY;
	}
	pPages->pElems[0] = __uuidof(CAboutPropPage);
	pPages->pElems[1] = __uuidof(CPropertyPropPage);
	return NOERROR;
}
void CAVISplitter::OnTrack(const CMediaType& mt)
{
	m_pins_creator.AddTrack(mt);
}


bool CAVISplitter::hasAnyOutpinConnected()
{
	bool ret = false;
	for (size_t i = 0; i < m_outpins.size();i++)
	{
		CAVIOutputPin *p = m_outpins[i];
		if (p && p->IsConnected())
			return true;
	}
	return ret;
}





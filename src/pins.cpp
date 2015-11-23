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
#include "pins.h"
#include "avisplitter.h"
#include "pins_creator.h"
#include <math.h>
#define ADTS_FIXEDHEADER_LENGTH 4
#define ADTS_HEADER_LENGTH 7

#define QUEUE_SIZE 2
#define AUDIO_BUFFERS_COUNT  1
#define VIDEO_BUFFERS_COUNT	 1// should be anough for waiting kframe
#define MEM_BUFFER_SIZE 2*1024*1024
#define MEM_VIDEO_BUFFER_SIZE 20*1024*1024
#define AVC_ANNEXB_NALU_SIZE 4

//#include "cprofiler.h"
//CProfiler g_profiler;
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CStreamInputPin
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CStreamInputPin::CStreamInputPin(CAVISplitter* pFilter, CCritSec* pLock, HRESULT* phr)
: CBasePin(NAME("CStreamInputPin"), pFilter, pLock, phr, L"Input", PINDIR_INPUT),
  m_pSplitter(pFilter),
  m_bFlushing(false)
{
}

CStreamInputPin::~CStreamInputPin()
{
}

// base pin overrides
HRESULT  CStreamInputPin::CheckMediaType(const CMediaType* pmt)
{
	return S_OK; // controlled in CompleteConnect by reading first 4 bytes
}

HRESULT CStreamInputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if (iPosition != 0)
	{
		return VFW_S_NO_MORE_ITEMS;
	}
	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Stream);
	return S_OK;
}

HRESULT CStreamInputPin::CompleteConnect(IPin* pPeer)
{
	// validate input with parser
	HRESULT hr = m_pSplitter->CompleteConnect(pPeer);
	if (SUCCEEDED(hr))
		hr = CBasePin::CompleteConnect(pPeer);
	return hr;
}
STDMETHODIMP CStreamInputPin::BeginFlush()
{
	// pass the flush call downstream via the filter
	m_bFlushing = true;
	return m_pSplitter->BeginFlush();
}

STDMETHODIMP CStreamInputPin::EndFlush()
{
	m_bFlushing = false;
	return m_pSplitter->EndFlush();
}

STDMETHODIMP CStreamInputPin::Disconnect()
{
	HRESULT hr = CBasePin::Disconnect();
	if (FAILED(hr))
		return hr;

	hr = m_pSplitter->BreakInputConnect();
	return hr;
}

HRESULT CStreamInputPin::Active()
{
	return 	S_OK;//m_pSplitter->StartThread();
}

HRESULT CStreamInputPin::Inactive()
{
	return m_pSplitter->StopThread();	
}


STDMETHODIMP CStreamInputPin::EndOfStream()
{
	return m_pSplitter->EndOfStream();
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CAVIOutputPin
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CAVIOutputPin::CAVIOutputPin(	CAVISplitter* pParser, 	CCritSec* pLock, 	HRESULT* phr, 	LPCWSTR pName)
: CBaseOutputPin(NAME("CAVIOutputPin"), pParser, pLock, phr, pName),
	m_pSplitter(pParser),
	m_bDiscont(true),
	m_currentTimeFormat(TIME_FORMAT_MEDIA_TIME),
	m_bEOSDelivered(false),
    m_pIndex(NULL)
{
}
CAVIOutputPin::~CAVIOutputPin()
{
}

STDMETHODIMP CAVIOutputPin::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
	if (iid == IID_IMediaSeeking)
	{
		return GetInterface((IMediaSeeking*)this, ppv);
	} else
	{
		return CBaseOutputPin::NonDelegatingQueryInterface(iid, ppv);
	}
}

// base class overrides for connection establishment
HRESULT CAVIOutputPin::CheckMediaType(const CMediaType* pmt)
{
	if (*pmt == m_mt)
		return S_OK;
	return S_FALSE;
}

HRESULT CAVIOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	HRESULT hr = S_OK;
	if (iPosition > 0)
		hr = VFW_S_NO_MORE_ITEMS;
	else
		*pmt = m_mt;
	return hr;
}
void CAVIOutputPin::SetTrackInfo(const TrackInfo& ti)
{
	m_trackInfo = ti;
}

HRESULT CAVIOutputPin::SetMediaType(const CMediaType* pmt)
{
	return CBaseOutputPin::SetMediaType(pmt);
}

HRESULT CAVIOutputPin::DecideAllocator(IMemInputPin* pPin, IMemAllocator ** pAlloc)
{
	HRESULT hr = S_OK;

	*pAlloc = new CSampleBuffer(NULL, &hr);
    if (!(*pAlloc))
       return E_OUTOFMEMORY;   

	(*pAlloc)->AddRef();

    if (SUCCEEDED(hr)) 
	{
        ALLOCATOR_PROPERTIES propRequest;
        ZeroMemory(&propRequest, sizeof(propRequest));

        hr = DecideBufferSize(*pAlloc, &propRequest);

        if (SUCCEEDED(hr)) 
		{
            // tell downstream pins that modification
            // in-place is not permitted
            hr = pPin->NotifyAllocator(*pAlloc, TRUE);
			
            if (SUCCEEDED(hr)) 
			{
                return NOERROR;
            }
        }
    }
	if (FAILED(hr))
	{
		(*pAlloc)->Release();
		 *pAlloc = NULL;
	}
    return hr;
}

HRESULT CAVIOutputPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop)
{
	pprop->cBuffers = QUEUE_SIZE;
    pprop->cbBuffer = m_AVIStreamHeader.dwSuggestedBufferSize?m_AVIStreamHeader.dwSuggestedBufferSize:MEM_BUFFER_SIZE;//MEM_BUFFER_SIZE/QUEUE_SIZE; 
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;
	ALLOCATOR_PROPERTIES propActual;
	return pAlloc->SetProperties(pprop, &propActual);
}

void CAVIOutputPin::SetIndex(const STREAM_INDEX& index)
{
    m_pIndex = &index;
}
void CAVIOutputPin::SetStreamHeader(AVIStreamHeader& strh)
{
    m_AVIStreamHeader = strh;
}
AVIStreamHeader& CAVIOutputPin::GetStreamHeader()
{
    return m_AVIStreamHeader;
}

void CAVIOutputPin::StopProcess()
{
    CallWorker(tm_stop);
    Close();
}
void CAVIOutputPin::StartProcess()
{
    BOOL b = Create();
    int i = 0;

}
int CAVIOutputPin::GetIndexStartPosEntry(const REFERENCE_TIME& tStart)
{
    return 0;   
}
void CAVIOutputPin::GetFirstSampleData(BYTE** pData, DWORD& dataLen)
{
    CAVIScanner* scanner = m_pSplitter->GetAVIScanner();
    if ((*m_pIndex).size())
    {        
        const INDEXENTRY& entry = (*m_pIndex)[0];
        *pData = new BYTE[entry.dwChunkLength];
        dataLen = entry.dwChunkLength;
        scanner->ReadData(entry.chunkOffset, entry.dwChunkLength, *pData);  
    }
}

REFERENCE_TIME CAVIOutputPin::GetTotalDuration()
{
    if (!m_pIndex)
        return 0;
    REFERENCE_TIME ret = 0;
    if (m_mt.majortype == MEDIATYPE_Video || m_mt.majortype == MEDIATYPE_Interleaved || m_mt.majortype == MEDIATYPE_Text)
    {           
        double retd = (1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate)*m_pIndex->size();
         ret =  (REFERENCE_TIME)floor(retd+0.5);
    }
    else if (m_mt.majortype == MEDIATYPE_Audio)
    {
        if (m_mt.formattype == FORMAT_WaveFormatEx)
        {
            WAVEFORMATEX* pwfx =  (WAVEFORMATEX*)m_mt.Format();
            if (!pwfx->nAvgBytesPerSec || !m_pIndex->size())
                return 0;
            ULONGLONG totalBytes = (*m_pIndex)[m_pIndex->size()-1].cumlen +(*m_pIndex)[m_pIndex->size()-1].dwChunkLength;
              double retd = 0;
            if (m_AVIStreamHeader.dwSampleSize == 0)
            { // vbr       
                retd = m_pIndex->size()*1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate;              
            }
            else                      
                retd = ((double)totalBytes)/pwfx->nAvgBytesPerSec*1e7;
            ret =  (REFERENCE_TIME)floor(retd+0.5);
        }
    }
    return ret;
}

REFERENCE_TIME CAVIOutputPin::GetCurTime(int sampleIdx)
{
     if (sampleIdx>=m_pIndex->size())
           return GetTotalDuration();

    if (m_mt.majortype == MEDIATYPE_Video || m_mt.majortype == MEDIATYPE_Interleaved)
    {
        return (REFERENCE_TIME)sampleIdx*1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate;
    }
    else if (m_mt.majortype == MEDIATYPE_Audio)
    {
        if (m_mt.formattype == FORMAT_WaveFormatEx)
        {
            WAVEFORMATEX* pwfx =  (WAVEFORMATEX*)m_mt.Format();
            if (!pwfx->nAvgBytesPerSec)
                return 0;
            REFERENCE_TIME ret = 0;
            if (m_AVIStreamHeader.dwSampleSize == 0)
            { // vbr       

                ret = (REFERENCE_TIME)sampleIdx*1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate;
            }
            else
                ret = (REFERENCE_TIME)((*m_pIndex)[sampleIdx].cumlen*1e7)/pwfx->nAvgBytesPerSec;
            return ret;
        }
    }
    return 0;
}

DWORD CAVIOutputPin::ThreadProc()
{
    CAVIScanner* scanner = m_pSplitter->GetAVIScanner();
    bool bFirst = true;
    REFERENCE_TIME tStart = 0;
    REFERENCE_TIME tStop = 0;
    REFERENCE_TIME duration = 0;
    bool bDeliverEOS = true;
    DWORD req = 0;
    HRESULT hr = S_OK;
    REFERENCE_TIME tSeekStart = 0;
    REFERENCE_TIME tSeekStop = 0;

     double dRate;
    m_pSplitter->GetSeekingParams(&tSeekStart, &tSeekStop, &dRate);
    DeliverNewSegment(tSeekStart, tSeekStop, dRate);
    int startPos = GetIndexStartPosEntry(tSeekStart);
    for (int i = startPos; i < m_pIndex->size(); i++)
    {
        const INDEXENTRY& entry = (*m_pIndex)[i];
        if (!entry.dwChunkLength)
            continue;
        CComPtr<IMediaSample> pSample;
        hr = GetNewSample(&pSample, entry.dwChunkLength);
        
        // Break out without calling EndOfStream if we're asked to
	    // do something different
	    if (CheckRequest(&req)) 
        {
            bDeliverEOS = false;          
            break;
        }


        if (bFirst)
        {
            bFirst = false;
            pSample->SetDiscontinuity(TRUE);
           

        }
        if (entry.dwFlags & AVIIF_INDEX)
            pSample->SetSyncPoint(TRUE);
        tStart = GetCurTime(i) - tSeekStart;
        tStop = GetCurTime(i+1)- tSeekStart;
       
        pSample->SetTime(&tStart,&tStop);
        if (tStart < 0 && tStop <= 0)
            pSample->SetPreroll(TRUE);

        BYTE* pbData = 0;
        pSample->GetPointer(&pbData);
        scanner->ReadData(entry.chunkOffset, entry.dwChunkLength, pbData);   
        hr = Deliver(pSample);
        if (FAILED(hr) || hr == S_FALSE)
        {
            while(true)
            {
                if (CheckRequest(&req)) 
                {
                    bDeliverEOS = false;          
                    break;
                }
                Sleep(100);
            }           
        }
    }
    if (bDeliverEOS)
        DeliverEndOfStream();

    req =GetRequest();
    Reply(S_OK);
    return 0;
}
HRESULT CAVIOutputPin::Active()
{
	resetFlags();

	HRESULT hr = S_OK;
	if (IsConnected())
	{
		hr = CBaseOutputPin::Active();
	
	}
	return hr;
}

HRESULT CAVIOutputPin::Inactive()
{
	HRESULT hr = CBaseOutputPin::Inactive(); 
	m_bEOSDelivered = false;
	
	return hr;
}

STDMETHODIMP CAVIOutputPin::Disconnect()
{
	m_pSplitter->DeselectSeekingPin();
	return CBaseOutputPin::Disconnect();
}





HRESULT CAVIOutputPin::DeliverBeginFlush()
{
    HRESULT hr = CBaseOutputPin::DeliverBeginFlush();
	return hr;
}

HRESULT CAVIOutputPin::DeliverEndFlush()
{
    HRESULT hr = CBaseOutputPin::DeliverEndFlush();
	resetFlags();
	return hr;
}
HRESULT CAVIOutputPin::DeliverEndOfStream()
{
    m_bEOSDelivered = true;
    return CBaseOutputPin::DeliverEndOfStream();
}

HRESULT CAVIOutputPin::GetNewSample(IMediaSample** pSample, uint32 len)
{
	HRESULT hr = Allocator()->GetSample(pSample, len);
	return hr;
}

bool CAVIOutputPin::IsAudio()
{
	return (*m_mt.Type() == MEDIATYPE_Audio) ? true : false;
}

bool CAVIOutputPin::IsVideo()
{
	return (*m_mt.Type() == MEDIATYPE_Video) ? true : false;
}
// -- output pin seeking methods -------------------------

STDMETHODIMP CAVIOutputPin::GetCapabilities(DWORD * pCapabilities)
{

	// Some versions of DShow have an aggregation bug that
	// affects playback with Media Player. To work around this,
	// we need to report the capabilities and time format the
	// same on all pins, even though only one
	// can seek at once.
	*pCapabilities =        AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards |
		AM_SEEKING_CanGetDuration |
		//AM_SEEKING_CanGetCurrentPos |
		AM_SEEKING_CanGetStopPos;
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::CheckCapabilities(DWORD * pCapabilities)
{
	DWORD dwActual;
	GetCapabilities(&dwActual);
	if (*pCapabilities & (~dwActual))
	{
		return S_FALSE;
	}
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::IsFormatSupported(const GUID * pFormat)
{
	// Some versions of DShow have an aggregation bug that
	// affects playback with Media Player. To work around this,
	// we need to report the capabilities and time format the
	// same on all pins, even though only one
	// can seek at once.
	if (*pFormat == TIME_FORMAT_MEDIA_TIME ||
		(*pFormat == TIME_FORMAT_FRAME && dynamic_cast<CAVIVideoPin*>(this) != NULL)) // only for video
	{
		return S_OK;
	}
	return S_FALSE;

}
STDMETHODIMP CAVIOutputPin::QueryPreferredFormat(GUID * pFormat)
{
	// Some versions of DShow have an aggregation bug that
	// affects playback with Media Player. To work around this,
	// we need to report the capabilities and time format the
	// same on all pins, even though only one
	// can seek at once.
	*pFormat = TIME_FORMAT_MEDIA_TIME;
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::GetTimeFormat(GUID *pFormat)
{
	*pFormat = m_currentTimeFormat;
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::SetTimeFormat(const GUID * pFormat)
{
	if (IsFormatSupported(pFormat) != S_OK)
		return  E_FAIL;

	if (m_currentTimeFormat != *pFormat)
		m_pSplitter->SetTimeFormatForAllPins(pFormat);	

	return S_OK;
}

void CAVIOutputPin::SetCurrentTimeFormat(const GUID* pFormat)
{
	m_currentTimeFormat = *pFormat;
}


STDMETHODIMP CAVIOutputPin::IsUsingTimeFormat(const GUID * pFormat)
{
	GUID guidActual;
	HRESULT hr = GetTimeFormat(&guidActual);

	if (SUCCEEDED(hr) && (guidActual == *pFormat))
	{
		return S_OK;
	} else
	{
		return S_FALSE;
	}
}

STDMETHODIMP CAVIOutputPin::ConvertTimeFormat(
									   LONGLONG* pTarget, 
									   const GUID* pTargetFormat,
									   LONGLONG Source, 
									   const GUID* pSourceFormat)
{
	// format guids can be null to indicate current format

	// since we only support TIME_FORMAT_MEDIA_TIME, we don't really
	// offer any conversions.
	// no conversion need

	GUID tF = m_currentTimeFormat;
	GUID sF = m_currentTimeFormat;	
	if (pTargetFormat)
		tF = *pTargetFormat;
	if (pSourceFormat)
		sF = *pSourceFormat;

	if (sF == tF)
	{
		*pTarget = Source;
		return S_OK;
	}

	if (sF == TIME_FORMAT_MEDIA_TIME)
	{
		if (tF == TIME_FORMAT_FRAME)
		{
			REFERENCE_TIME dpf = m_pSplitter->GetAVIScanner()->GetDefaultDurationPerFrame(m_trackInfo.track_number);
			*pTarget = (dpf!=0)?((double)Source/dpf):0;// prevent divistion by zero
			return S_OK;
		}
	}
	else if (sF == TIME_FORMAT_FRAME)
	{
		if (tF == TIME_FORMAT_MEDIA_TIME)
		{
			REFERENCE_TIME dpf = m_pSplitter->GetAVIScanner()->GetDefaultDurationPerFrame(m_trackInfo.track_number);
			*pTarget = dpf*Source;// prevent divistion by zero
			return S_OK;
		}
	}
/*	if (pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
	{
		if (pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
		{
			*pTarget = Source;
			return S_OK;
		}
	}*/

	return E_INVALIDARG;
}
void CAVIOutputPin::setPrerolling(bool prerolling)
{
	m_bPrerolling = prerolling;
}

STDMETHODIMP CAVIOutputPin::GetDuration(LONGLONG *pDuration)
{
	LONGLONG refTimeDuration = m_pSplitter->GetDuration();
	//*pDuration = refTimeDuration;
	//convert from reftime to current format
	ConvertTimeFormat(pDuration, NULL, refTimeDuration,&TIME_FORMAT_MEDIA_TIME);
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::GetStopPosition(LONGLONG *pStop)
{
	REFERENCE_TIME tStart, tStop;
	double dRate;
	m_pSplitter->GetSeekingParams(&tStart, &tStop, &dRate);
	//convert from reftime to current format
	ConvertTimeFormat(pStop, NULL, tStop, &TIME_FORMAT_MEDIA_TIME);
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::GetCurrentPosition(LONGLONG *pCurrent)
{
	// this method is not supposed to report the previous start
	// position, but rather where we are now. This is normally
	// implemented by renderers, not parsers
	return E_NOTIMPL;
}
void CAVIOutputPin::resetFlags()
{
	// next sample will be after a discontinuity
	m_bDiscont = true;
	m_bEOSDelivered = false;
}




STDMETHODIMP CAVIOutputPin::SetPositions( LONGLONG * pCurrent,   DWORD dwCurrentFlags, LONGLONG * pStop,  DWORD dwStopFlags)
{
	if (!IsConnected()) // only one connected pin can do this
		return S_OK;

	if (!(m_pSplitter->SelectSeekingPin(this))) // will be deselected on Inactive
		return S_OK;


	LONGLONG refTimeCurrent = 0;
	LONGLONG refTimeStop = 0;
	ConvertTimeFormat(&refTimeCurrent, &TIME_FORMAT_MEDIA_TIME, *pCurrent,NULL);
	*pCurrent = refTimeCurrent;
	ConvertTimeFormat(&refTimeStop, &TIME_FORMAT_MEDIA_TIME, *pStop,NULL);

	{
		char buf[512];
		sprintf(buf, "CAVIOutputPin::SetPositions %I64d\n", refTimeCurrent);
	}

	// check inputs
	REFERENCE_TIME duration = m_pSplitter->GetDuration();
	
	if (refTimeCurrent > duration)
	{
		refTimeCurrent = duration;
		ConvertTimeFormat(pCurrent, NULL,refTimeCurrent ,&TIME_FORMAT_MEDIA_TIME);
	}
	
	if (refTimeStop > duration)
	{
		refTimeStop = duration;
		ConvertTimeFormat(pStop, NULL,refTimeStop ,&TIME_FORMAT_MEDIA_TIME);
	}

	// fetch current properties
	REFERENCE_TIME tStart, tStop;
	double dRate;
	m_pSplitter->GetSeekingParams(&tStart, &tStop, &dRate);
	dwCurrentFlags &= AM_SEEKING_PositioningBitsMask;
	if (dwCurrentFlags)
	{
		applySeekPosition(tStart, refTimeCurrent, tStop, refTimeStop, dwCurrentFlags);
	}
	// splitter will call SetPositionsValues for all  pins
	HRESULT hr = m_pSplitter->Seek(tStart, tStop, dRate);
	return hr;
}



void CAVIOutputPin::applySeekPosition(REFERENCE_TIME& tStart, const REFERENCE_TIME& newtStartValue, 
										   REFERENCE_TIME& tStop, const REFERENCE_TIME& newtStopValue,
										   DWORD flagsToApply)
{
	
	if (flagsToApply == AM_SEEKING_AbsolutePositioning)
	{
		char buf[512];
		sprintf(buf,"AM_SEEKING_AbsolutePositioning %I64d", newtStartValue);

		tStart = newtStartValue;
		tStop = newtStopValue;
	}
	else if (flagsToApply == AM_SEEKING_RelativePositioning || 
			flagsToApply == AM_SEEKING_IncrementalPositioning)
	{
		tStart += newtStartValue;
		tStop += newtStopValue;
	}	
	else
		throw "CAVIOutputPin::applySeekPosition unknown flags";
}


STDMETHODIMP CAVIOutputPin::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
	REFERENCE_TIME tStart, tStop;
	double dRate;
	m_pSplitter->GetSeekingParams(&tStart, &tStop, &dRate);
	//convert from reftime to current format
	ConvertTimeFormat(pCurrent, NULL, tStart, &TIME_FORMAT_MEDIA_TIME);
	ConvertTimeFormat(pStop, NULL, tStop, &TIME_FORMAT_MEDIA_TIME);
	//*pCurrent = tStart;
	//*pStop = tStop;
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest)
{
	if (pEarliest != NULL)
	{
		*pEarliest = 0;
	}
	if (pLatest != NULL)
	{
		*pLatest = m_pSplitter->GetDuration();
	}
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::SetRate(double dRate)
{
	HRESULT hr = S_OK;
	hr = m_pSplitter->SetRate(dRate);
	return hr;
}



STDMETHODIMP CAVIOutputPin::GetRate(double * pdRate)
{
	REFERENCE_TIME tStart, tStop;
	double dRate;
	m_pSplitter->GetSeekingParams(&tStart, &tStop, &dRate);
	*pdRate = dRate;
	return S_OK;
}

STDMETHODIMP CAVIOutputPin::GetPreroll(LONGLONG * pllPreroll)
{
	// don't need to allow any preroll time for us
	*pllPreroll = 0;
	return S_OK;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CAVIVideoPin
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CAVIVideoPin::CAVIVideoPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name):
	CAVIOutputPin(parser, lock, hr, name)

{

}



CAVIVideoPin::~CAVIVideoPin()
{

}

HRESULT CAVIVideoPin::Active()
{
   
    m_seqHParser.Init( ((VIDEOINFOHEADER*)m_mt.pbFormat)->bmiHeader.biCompression);
    return CAVIOutputPin::Active();
}

HRESULT CAVIVideoPin::Inactive()
{
	return CAVIOutputPin::Inactive();
}
HRESULT CAVIVideoPin::Deliver(IMediaSample *pSample)
{
     CComPtr<IMediaSample> pSampleDeliver = pSample;
    if (pSample->IsDiscontinuity() == S_OK)
    {
        if (m_seqHParser.isInited())
        {
             BYTE* pBuffer = NULL;
            pSample->GetPointer(&pBuffer);
            long len = pSample->GetActualDataLength();
            if (!m_seqHParser.ParseFrame(pBuffer, len))
            {
                // get it from the first sample
                const INDEXENTRY& entry = (*m_pIndex)[0];
                BYTE* pTempData = new BYTE[entry.dwChunkLength];
                 CAVIScanner* scanner = m_pSplitter->GetAVIScanner();
                scanner->ReadData(entry.chunkOffset, entry.dwChunkLength, pTempData); 
                BYTE* pDecoderConfig = NULL;
                int decoderConfigLen = 0;
                if (m_seqHParser.ParseFrame(pTempData, entry.dwChunkLength))
                {
                    decoderConfigLen = m_seqHParser.VisualObjSeq().obj_len+m_seqHParser.Pic_Parameter_Set().obj_len;
                    pDecoderConfig = new BYTE[decoderConfigLen];
                    memcpy(pDecoderConfig, m_seqHParser.VisualObjSeq().obj, m_seqHParser.VisualObjSeq().obj_len);
                    if (m_seqHParser.Pic_Parameter_Set().obj_len)
                        memcpy(pDecoderConfig+m_seqHParser.VisualObjSeq().obj_len, m_seqHParser.Pic_Parameter_Set().obj, m_seqHParser.Pic_Parameter_Set().obj_len);

                     long newsampleLen = decoderConfigLen+len;
                    CComPtr<IMediaSample> pNewSample;
                    HRESULT hr = GetNewSample(&pNewSample, newsampleLen);
                    ASSERT(hr == S_OK);
                    if (SUCCEEDED(hr))
                    {
                        CComPtr<IMediaSample2> pSmp = NULL;
                        pSample->QueryInterface(&pSmp);
                        AM_SAMPLE2_PROPERTIES props_in = {0};
                        hr = pSmp->GetProperties(sizeof( AM_SAMPLE2_PROPERTIES ), (BYTE*)&props_in);

                        CComPtr<IMediaSample2> pSmp2 = NULL;
                        pNewSample->QueryInterface(&pSmp2);

                        AM_SAMPLE2_PROPERTIES props_out = {0};
                        hr = pSmp2->GetProperties(sizeof( AM_SAMPLE2_PROPERTIES ), (BYTE*)&props_out);
                      
                      
                        props_out.dwTypeSpecificFlags = props_in.dwTypeSpecificFlags;
                        props_out.dwSampleFlags = props_in.dwSampleFlags;
                        props_out.lActual = decoderConfigLen+len;
                        props_out.tStart = props_in.tStart;
                        props_out.tStop = props_in.tStop;
                        props_out.dwStreamId = props_in.dwStreamId;
                        if (props_in.pMediaType)
                            props_out.pMediaType = CreateMediaType(props_in.pMediaType);

                        memcpy(props_out.pbBuffer, pDecoderConfig, decoderConfigLen);
                        memcpy(props_out.pbBuffer+decoderConfigLen, pBuffer, len);
                        hr = pSmp2->SetProperties(sizeof( AM_SAMPLE2_PROPERTIES ), (BYTE*)&props_out);
                        pSampleDeliver = pNewSample; 
                    }

                    delete pDecoderConfig;

                }
                delete pTempData;

            }

        }      
    }
    return CAVIOutputPin::Deliver(pSampleDeliver);
}



int CAVIVideoPin::GetIndexStartPosEntry(const REFERENCE_TIME& tStart)
{
    int dstIndex =0;
    if (m_mt.majortype == MEDIATYPE_Video)
    {
        if (m_mt.formattype == FORMAT_VideoInfo)
        {
            VIDEOINFOHEADER* pvih =  (VIDEOINFOHEADER*)m_mt.Format();
            if (!pvih->AvgTimePerFrame)
            {
            
                return 0;
            }
            dstIndex = (int)(tStart/(1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate));
        }
    }
    if (dstIndex <0 || dstIndex>= m_pIndex->size())
    {

        return 0 ;
    }
    // now search back for keyframe

    for (int i = dstIndex; i > 0; i--)
    {
        if ((*m_pIndex)[i].dwFlags & AVIIF_INDEX)
        {         
            return i;
        }
    }
    return 0;   
}


HRESULT CAVIVideoPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop)
{
	
    pprop->cbBuffer = m_AVIStreamHeader.dwSuggestedBufferSize?m_AVIStreamHeader.dwSuggestedBufferSize:MEM_VIDEO_BUFFER_SIZE;//MEM_VIDEO_BUFFER_SIZE/QUEUE_SIZE; 
    pprop->cBuffers = MEM_VIDEO_BUFFER_SIZE/pprop->cbBuffer;
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;
	ALLOCATOR_PROPERTIES propActual;
	return pAlloc->SetProperties(pprop, &propActual);
}


HRESULT CAVIVideoPin::DeliverEndOfStream()
{
	HRESULT hr = S_OK;	
	hr = CAVIOutputPin::DeliverEndOfStream();
	return hr;
}
void CAVIVideoPin::resetFlags()
{
	CAVIOutputPin::resetFlags();

}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CAVIAudioPin
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CAVIAudioPin::CAVIAudioPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name):
	CAVIOutputPin(parser, lock, hr, name)
{
}

CAVIAudioPin::~CAVIAudioPin()
{
	
}

HRESULT CAVIAudioPin::Active()
{

	return CAVIOutputPin::Active();
}

HRESULT CAVIAudioPin::Inactive()
{
	HRESULT hr = CAVIOutputPin::Inactive();
	return hr;
}
int CAVIAudioPin::GetIndexStartPosEntry(const REFERENCE_TIME& tStart)
{
    uint64 dstCumLen =0;
    if (m_mt.formattype == FORMAT_WaveFormatEx)
    {
        WAVEFORMATEX* pwfx =  (WAVEFORMATEX*)m_mt.Format();
        if (m_AVIStreamHeader.dwSampleSize == 0)
        { // vbr       
               int sampleIdx = ((double)tStart)*((double)m_AVIStreamHeader.dwRate/m_AVIStreamHeader.dwScale)/1e7;               
               return sampleIdx;
        }
        if (!pwfx->nAvgBytesPerSec)
        {

            return 0;
        }

        dstCumLen =(double)pwfx->nAvgBytesPerSec*tStart/1e7;
    }

    for (int i = 0; i < m_pIndex->size(); i++)
    {
        if ((*m_pIndex)[i].cumlen > dstCumLen)
        {
            int ret = (i>0)?i-1:0;       
            return ret;
        }
    }
    if (dstCumLen)
    {
        return m_pIndex->size() -1;
    }
    return 0;   
}
HRESULT CAVIAudioPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop)
{
	pprop->cBuffers = QUEUE_SIZE;
    pprop->cbBuffer = m_AVIStreamHeader.dwSuggestedBufferSize?m_AVIStreamHeader.dwSuggestedBufferSize:MEM_BUFFER_SIZE;//MEM_VIDEO_BUFFER_SIZE/QUEUE_SIZE; 
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;
	ALLOCATOR_PROPERTIES propActual;
	return pAlloc->SetProperties(pprop, &propActual);
}



//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// CAVISubtitlePin
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CAVISubtitlePin::CAVISubtitlePin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name):
	CAVIOutputPin(parser, lock, hr, name)
{
}


int CAVISubtitlePin::GetIndexStartPosEntry(const REFERENCE_TIME& tStart)
{
      int dstIndex = tStart/(1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate);
    return dstIndex;   

}
HRESULT CAVISubtitlePin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop)
{
	pprop->cBuffers = 1;
    pprop->cbBuffer = m_AVIStreamHeader.dwSuggestedBufferSize?m_AVIStreamHeader.dwSuggestedBufferSize:3*1024*1024;//MEM_VIDEO_BUFFER_SIZE/QUEUE_SIZE; 
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;
	ALLOCATOR_PROPERTIES propActual;
	return pAlloc->SetProperties(pprop, &propActual);
}

CAVIInterleavedPin::CAVIInterleavedPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name):
	CAVIOutputPin(parser, lock, hr, name)
{
}

int CAVIInterleavedPin::GetIndexStartPosEntry(const REFERENCE_TIME& tStart)
{
      int dstIndex = tStart/(1e7*m_AVIStreamHeader.dwScale/ m_AVIStreamHeader.dwRate);
    return dstIndex;   

}
HRESULT CAVIInterleavedPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop)
{
	pprop->cBuffers = 5;
    pprop->cbBuffer = m_AVIStreamHeader.dwSuggestedBufferSize?m_AVIStreamHeader.dwSuggestedBufferSize:MEM_VIDEO_BUFFER_SIZE*2;//MEM_VIDEO_BUFFER_SIZE/QUEUE_SIZE; 
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;
	ALLOCATOR_PROPERTIES propActual;
	return pAlloc->SetProperties(pprop, &propActual);
}

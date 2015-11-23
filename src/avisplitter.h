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

#pragma once
#include "aviscanner.h"
#include "pins.h"
#include "pins_creator.h"
class DECLSPEC_UUID("C589E763-6942-43aa-8C5B-C501492AF638")
CAVISplitter
:	public CBaseFilter, 
	public CAVICallback, //callbacks called by parser
	public CAMThread, // parsing thread
	public ISpecifyPropertyPages,//property page
	public ISMMModuleConfig,
	public CPersistStream
	
{
public:
	// filter registration tables
	static const AMOVIESETUP_MEDIATYPE m_sudType[];
	static const AMOVIESETUP_PIN m_sudPin[];
	static const AMOVIESETUP_FILTER m_sudFilter;

	// constructor method used by class factory
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv);

	// CBaseFilter methods
	int GetPinCount();
	CBasePin *GetPin(int n);
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

	// called from input pin
	HRESULT CompleteConnect(IPin* pPeer);
	HRESULT BreakInputConnect();
	HRESULT BeginFlush();
	HRESULT EndFlush();
	HRESULT EndOfStream();
	// CAVICallback implementation
	void OnTrack(const CMediaType& mt);
	REFERENCE_TIME GetDuration();
	void GetSeekingParams(REFERENCE_TIME* ptStart, REFERENCE_TIME* ptStop, double* pdRate);
	HRESULT Seek(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	void SetSeekStartValue(REFERENCE_TIME& tStart);
	HRESULT DeliverPrerolledPinsFrames(REFERENCE_TIME& timeFrom);
	void SetPrerollingForAllPins();
	void ResetPrerollingForAllPins();


	HRESULT SetRate(double dRate);

	// Pulling Thread rountines
	HRESULT StartThread();
	HRESULT StopThread();

	// Returns pointer to AVI scanner object
	CAVIScanner *GetAVIScanner();
	// Returns pointer to filter's critical section object
	CCritSec *GetFilterCS();

	// Adds pin to the list of output pins
	void AddOutputPin( CAVIOutputPin *p);

   // implementing of IModuleConfig
   //
   STDMETHODIMP SetValue( /*[in]*/ const GUID *pParamID, /*[in]*/ const VARIANT *pValue );
   STDMETHODIMP GetValue( /*[in]*/ const GUID *pParamID, /*[out]*/ VARIANT *pValue );
   
   STDMETHODIMP CommitChanges( VARIANT *pReason ){return E_NOTIMPL;}
   

	//CPersistStream functions
	HRESULT WriteToStream(IStream *pStream);
	HRESULT ReadFromStream(IStream *pStream);
	STDMETHODIMP GetClassID(CLSID *pClsid);
	//
   // IPersistStream to use CPresistStream::
   //
   STDMETHODIMP IsDirty( void );
   STDMETHODIMP Load( IStream *pStm );
   STDMETHODIMP Save( IStream *pStm , BOOL fClearDirty);
   STDMETHODIMP GetSizeMax( ULARGE_INTEGER *pcbSize);

	void SetTimeFormatForAllPins(const GUID* pFormat);
	bool hasConnectedVideoPin();

	bool SelectSeekingPin(CAVIOutputPin* pPin);
	void DeselectSeekingPin();



	bool GetADTS_AACOutput();
protected:
	// ISpecifyPropertyPages interface
	STDMETHODIMP GetPages( CAUUID *pPages );
private:
	// construct only via class factory
	CAVISplitter(LPUNKNOWN pUnk, HRESULT* phr);
	HRESULT Init(); // creates internal class objects
	~CAVISplitter();

	bool hasAnyOutpinConnected();
    void checkMediaTypes(CAVIOutputPin* pPin);

	// Pulling thread procedure
	DWORD ThreadProc();
	// Suspend worker thread from pulling, eg during seek
	HRESULT PauseThread();
	// Suspend/Resume pulling thread
	bool Suspend();
	void Resume();
	// Main method for reading data using IAsyncReader,
	// process it and deliver downstream
	void startPinsProcessingThreads(); 
    void stopPinsProcessingThreads();

	void DeleteOuputPins();
	void ReCreateOuputPins();

	// Thread controlling messages
	enum ThreadMsg 
	{
		tm_pause,       // stop pulling and wait for next message
		tm_start,       // start pulling
		tm_exit,        // stop and exit
	};
	// Thread controlling exceptions
	enum PullThreadExc
	{
		te_new_request = 1000
	};

	// Filter's critical section object 
	CCritSec m_csFilter;
	// Input pin
	CStreamInputPin* m_in_pin;
	// Output pins creator object
	COutPinsCreator m_pins_creator;

	// Output pins
	vector<CAVIOutputPin *>m_outpins;
	
	// Ebml scanner object 
	CAVIScanner m_scanner;


	// for timing baseline and duration
	LONGLONG m_firstPTS;
	LONGLONG m_lastPTS;
	LONGLONG m_llFileSize;

	// for seeking
	CCritSec m_csSeeking;
	CAVIOutputPin* m_pSeekingPin;
	REFERENCE_TIME m_tStart;
	REFERENCE_TIME m_tStop;
	double m_dRate;

	// VBR seeking -- multiple tries, refining bitrate
	bool m_bLocating;
	int m_nTries;
	// when locating, last known position and time
	LONGLONG m_posLast;
	REFERENCE_TIME m_tsLast;
	// current target offset (file location of this seek)
	LONGLONG m_posThis;
	// Syncro time
	REFERENCE_TIME m_sync_time;
	//uint32 m_sync_track;
	// Identifier of worker thread
	DWORD m_threadid;
	// Pulling thread state
	ThreadMsg m_thread_state;
    REFERENCE_TIME m_totalDuration;

};


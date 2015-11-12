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
// ------------------------------------------------------------------
// CStreamInputPin
// ------------------------------------------------------------------

// The input pin connects to the file source using the IAsyncReader interface to pull
// data. It accepts an input of MEDIATYPE_Stream and delivers it to the parser for
// demultiplexing and delivery. 
//
// Since we are not using IMemInputPin, it does not make sense to derive from 
// CBaseInputPin. Instead, we use the CPullPin class internally -- this provides a 
// worker thread that fetches data from the upstream filter.
#include "commonTypes.h"
#include <vector>
#include "SampleBuffer.h"
class CAVISplitter;
#include "pins_creator.h"
class CStreamInputPin : public CBasePin
{
public:
    CStreamInputPin(CAVISplitter* pFilter, CCritSec* pLock, HRESULT* phr);
    ~CStreamInputPin();

    // base pin overrides
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
	HRESULT CompleteConnect(IPin* pPeer);
	STDMETHODIMP Disconnect();

	STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
	
	HRESULT Active();
    HRESULT Inactive();
    STDMETHODIMP EndOfStream();

private:
    CAVISplitter* m_pSplitter;
    bool m_bFlushing;
};



// ------------------------------------------------------------------
// CAVIOutputPin
// ------------------------------------------------------------------

// The output pins deliver demultiplexed data
// downstream.
// We use a COutputQueueM object with a worker thread to ensure that
// one blocking pin does not prevent the rest of the parser from 
// continuing (at least until the input buffers run out).
class CAVIOutputPin : public CBaseOutputPin,  public IMediaSeeking, CAMThread
{
public:
    CAVIOutputPin(CAVISplitter* pParser, CCritSec* pLock, HRESULT* phr, LPCWSTR pName);
	virtual ~CAVIOutputPin();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv);

	// normally, pins share the filter's lifetime, but we will release the pins
	// when the input is disconnected, so they need to have a normal COM lifetime
	STDMETHODIMP_(ULONG) NonDelegatingAddRef()
	{
		return CUnknown::NonDelegatingAddRef();
	}
	STDMETHODIMP_(ULONG) NonDelegatingRelease()
	{
		return CUnknown::NonDelegatingRelease();
	}
    // base class overrides for connection establishment
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT SetMediaType(const CMediaType* pmt);
	virtual HRESULT DecideAllocator(IMemInputPin* pPin, IMemAllocator ** pAlloc);
    virtual HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop);

    // this group of methods deal with the COutputQueueM
    virtual HRESULT Active();
    virtual HRESULT Inactive();
	
	STDMETHODIMP Disconnect();

    HRESULT DeliverBeginFlush();
    HRESULT DeliverEndFlush();
    HRESULT DeliverEndOfStream();
    void StartProcess();
    void StopProcess();
    void SetIndex(const STREAM_INDEX& index);
    void SetStreamHeader(AVIStreamHeader& strh);

    DWORD ThreadProc();
    REFERENCE_TIME GetCurTime(int sampleIdx);
    REFERENCE_TIME GetTotalDuration();
    virtual int GetIndexStartPosEntry(const REFERENCE_TIME& tStart);
// IMediaSeeking
public:
    STDMETHODIMP GetCapabilities(DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities(DWORD * pCapabilities );
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP SetPositions(LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags );
	STDMETHODIMP SetTimeFormat(const GUID * pFormat);
	STDMETHODIMP GetPositions(LONGLONG * pCurrent,LONGLONG * pStop );
    STDMETHODIMP GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG * pllPreroll);
	


	
	
	void SetTrackInfo(const TrackInfo& ti);
	TrackInfo GetTrackInfo(){return m_trackInfo;}

	void SetCurrentTimeFormat(const GUID* pFormat);
	bool isPrerolling(){return m_bPrerolling;}
	void setPrerolling(bool prerolling);
		// Actual sample delivering
	bool IsAudio();
	bool IsVideo();
protected:
	// IQualityControl override
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q){return S_OK;}


	virtual void resetFlags();


	bool isEOSDelivered(){return m_bEOSDelivered;}

	virtual HRESULT GetNewSample(IMediaSample** pSample, uint32 len);
	// Pointer to splitter
	CAVISplitter* m_pSplitter;

	bool m_bEOSDelivered;
	TrackInfo m_trackInfo;
	
    const STREAM_INDEX* m_pIndex;
    AVIStreamHeader m_AVIStreamHeader;
private:
	CSampleBuffer* Allocator(){return (CSampleBuffer *)m_pAllocator;}
	// Thread controlling messages
	enum ThreadMsg 
	{
		tm_start,      
		tm_stop,      
	};

	void applySeekPosition(REFERENCE_TIME& tStart/*inout*/, const REFERENCE_TIME& newtStartValue, REFERENCE_TIME& tStop/*inout*/, const REFERENCE_TIME& newtStopValue, DWORD flagsToApply);
	bool m_bDiscont;

	GUID m_currentTimeFormat;
	bool m_bPrerolling;
  
  

};

// ------------------------------------------------------------------
// CAVIVideoPin
// ------------------------------------------------------------------
class CAVIVideoPin : public CAVIOutputPin
{
public:
	CAVIVideoPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name);
	~CAVIVideoPin();
	virtual HRESULT DeliverEndOfStream();


	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop);
	virtual HRESULT Inactive();
    virtual int GetIndexStartPosEntry(const REFERENCE_TIME& tStart);
    virtual HRESULT Deliver(IMediaSample *);
protected:
	virtual void resetFlags();
	
private:
    bool findH264SPSPPS(const BYTE* data, int dataSize, BYTE** pOutput, int& outputLen);
    int  IsAVCStartCode(const BYTE * pBuf);

};


// ------------------------------------------------------------------
// CAVIAudioPin
// ------------------------------------------------------------------
class CAVIAudioPin : public CAVIOutputPin
{
public:
	CAVIAudioPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name);
	~CAVIAudioPin();
	virtual HRESULT Active();
	virtual HRESULT Inactive();
    virtual int GetIndexStartPosEntry(const REFERENCE_TIME& tStart);
    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop);
protected:
private:

};

// ------------------------------------------------------------------
// CAVISubtitlePin
// ------------------------------------------------------------------
class CAVISubtitlePin : public CAVIOutputPin
{
public:
	CAVISubtitlePin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name);	
      virtual int GetIndexStartPosEntry(const REFERENCE_TIME& tStart);
	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop);
};

class CAVIInterleavedPin : public CAVIOutputPin
{
public:
	CAVIInterleavedPin(CAVISplitter* parser, CCritSec* lock, HRESULT* hr, LPCWSTR name);
	
    virtual int GetIndexStartPosEntry(const REFERENCE_TIME& tStart);
	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pprop);
protected:

private:
};


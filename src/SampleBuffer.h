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

// ring samples buffer on preallocated memory
//
#pragma once
#include "amfilter.h"
#include "wxutil.h"
#include <set>
#include <map>
using namespace std;

class CSampleBuffer:public CMemAllocator
{
public:
	CSampleBuffer(LPUNKNOWN  pUnk, HRESULT   *phr);
	virtual ~CSampleBuffer(void);
	STDMETHODIMP GetSample(IMediaSample** pSample/*output*/, LONG size, bool bBlockIfNoBuffer = true);
	STDMETHODIMP ReleaseBuffer(IMediaSample * pSample);
	void Init();
	size_t GetBusyCount();
      STDMETHODIMP GetProperties(
		    __out ALLOCATOR_PROPERTIES* pProps);
private:
	

	CCritSec m_pointersCS;
	struct SB_INTERVAL
	{
		SB_INTERVAL(BYTE* _b, BYTE* _e)
		{
			b= _b;
			e = _e;
		}
		BYTE* b;
		BYTE* e;
	};
	typedef pair<BYTE*,SB_INTERVAL> SB_INTERVAL_PAIR;
	map<BYTE*, SB_INTERVAL> m_busyIntervals;
	CAMEvent m_changeBufEvent;
	BYTE* m_rbb;
	BYTE* m_rbe;
};

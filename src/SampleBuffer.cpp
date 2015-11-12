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
#include "SampleBuffer.h"

CSampleBuffer::CSampleBuffer(LPUNKNOWN  pUnk, HRESULT   *phr)
:CMemAllocator(NAME("CSubAllocator"), pUnk, phr),m_changeBufEvent(FALSE)
{
	m_rbb = NULL;
	m_rbe = NULL;
}

CSampleBuffer::~CSampleBuffer(void)
{
}

void CSampleBuffer::Init()
{
	m_rbb = m_pBuffer;
	if (m_rbb)
		m_rbe = m_pBuffer+m_lSize*m_lCount;
}


size_t CSampleBuffer::GetBusyCount()
{
	CAutoLock lock(&m_pointersCS);
	return m_busyIntervals.size();
}
STDMETHODIMP CSampleBuffer::GetSample(IMediaSample** pSample/*output*/, LONG size, bool bBlockIfNoBuffer)
{
	HRESULT hr = S_OK;
	while(true)
	{
		{ // lock block
			CAutoLock lock(&m_pointersCS);
			if (!m_busyIntervals.size()) // first time
			{
				Init();
				if (m_rbe-m_rbb < size)
				{
					return E_FAIL;
				}
					

				m_busyIntervals.insert(SB_INTERVAL_PAIR(m_rbb, SB_INTERVAL(m_rbb, m_rbb+size)));
				CMediaSample* newSample  = new CMediaSample(NAME("CMediaSample"),this,&hr,m_rbb,size);
				newSample->AddRef();
				*pSample = newSample;
				m_rbb+=size;
				break;
			}
			else
			{
				ASSERT(m_rbe >= m_rbb);
				if (m_rbe - m_rbb < size)// need wrap
				{
					if ((m_busyIntervals.begin()->first > m_pBuffer) && (m_rbe == m_pBuffer+m_lSize*m_lCount))
					{
						m_rbb = m_pBuffer;
						m_rbe = m_busyIntervals.begin()->first;
					}
				}

				ASSERT(m_rbe >= m_rbb);
				if (m_rbe - m_rbb >= size)
				{
					m_busyIntervals.insert(SB_INTERVAL_PAIR(m_rbb, SB_INTERVAL(m_rbb, m_rbb+size)));
					CMediaSample* newSample  = new CMediaSample(NAME("CMediaSample"),this,&hr,m_rbb,size);
					newSample->AddRef();
					*pSample = newSample; 
					m_rbb+=size;
					break;
				}
			}
			if (!bBlockIfNoBuffer)
				return S_FALSE;

		}
		// wait until it will
		
		WaitForSingleObject(m_changeBufEvent, INFINITE);
	}
	return hr;
}

STDMETHODIMP CSampleBuffer::GetProperties(
		    __out ALLOCATOR_PROPERTIES* pProps)
{
    return CMemAllocator::GetProperties(pProps);
}

STDMETHODIMP CSampleBuffer::ReleaseBuffer(IMediaSample * pSample)
{
	CAutoLock lock(&m_pointersCS);
	HRESULT hr = S_OK;
	BYTE* samplePointer;
	pSample->GetPointer(&samplePointer);
	map<BYTE*, SB_INTERVAL>::iterator it = m_busyIntervals.find(samplePointer);
	ASSERT(it != m_busyIntervals.end());
	if (m_rbe == samplePointer)
	{
		map<BYTE*, SB_INTERVAL>::iterator next = it;
		next++;
		if (next == m_busyIntervals.end())
		{// if it last make m_rbe end of true buffer
			m_rbe = m_pBuffer+m_lSize*m_lCount;
		}
		else 
		{
			m_rbe = next->first;
		}
	}	
	if (it != m_busyIntervals.end())
		m_busyIntervals.erase(it);
	delete pSample;
	m_changeBufEvent.Set();
	return hr;
}


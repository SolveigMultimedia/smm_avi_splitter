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
#include "acallback.h"
inline DWORD SWAP_ULONG(const BYTE* pData)
{
	return DWORD((pData[0] << 24) |
				 (pData[1] << 16) |
				 (pData[2] << 8) | pData[3]);
}
// ----------------------------------------
// CFileIOÑallBack
// ----------------------------------------

CFileIOCallBack::CFileIOCallBack():
	m_file(NULL),
	m_pos(0),
	m_length(0)
{

}
CFileIOCallBack::~CFileIOCallBack()
{
	closeFile();
}
void CFileIOCallBack::closeFile()
{	CAutoLock lock(&m_accessLock);
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
	}
}
void CFileIOCallBack::SetFileName(LPTSTR fileName)
{
	closeFile();
	{
		CAutoLock lock(&m_accessLock);
		//fileName = FILE_NAME;
		m_file = _tfopen(fileName, _T("rb"));
	}
	GetLength();
}
void CFileIOCallBack::BeginFlush()
{
	// do nothing
}
void CFileIOCallBack::EndFlush()
{
	// do nothing
}
uint64 CFileIOCallBack::GetLength()
{
	CAutoLock lock(&m_accessLock);
	if (!m_file) throw "CFileIOÑallBack::GetLength no file opened";
	long oldpos = ftell (m_file);
	fseek (m_file , 0 , SEEK_END);
	uint64 lSize = ftell (m_file);
	fseek (m_file,oldpos, SEEK_SET);
	m_length = lSize;
	return m_length;
}
bool CFileIOCallBack::validateSeekOffset( int64 offset)
{
	bool ret = true;
	if (offset > (int64)m_length)
		ret = false;

	if ( offset  < 0 )
		ret = false;
	return ret;
}

uint32 CFileIOCallBack::read(void*Buffer,size_t Size)
{
	CAutoLock lock(&m_accessLock);

	if (!m_file) throw "CFileIOÑallBack::read no file opened";

	if ( m_pos > m_length)
	{
		//m_pos = -1;
		return 0;
	}
	fseek(m_file, (long)m_pos, SEEK_SET);
	Size = fread(Buffer, 1, Size, m_file);
	m_pos += Size;
	return Size;
}

size_t CFileIOCallBack::write(const void*Buffer,size_t Size)
{
	CAutoLock lock(&m_accessLock);
	// Not make sence
	return 0;
}
void CFileIOCallBack::setFilePointer(int64 Offset,seek_mode Mode)
{
	CAutoLock lock(&m_accessLock);
	int64 absolutePos = 0;
	if (Mode ==  seek_beginning )
		absolutePos = Offset;
	else if ( Mode == seek_current )
		absolutePos = m_pos + Offset;
	else if (Mode == seek_end )
		absolutePos = m_length - Offset;	
	else
		throw "CFileIOÑallBack::setFilePointer invalid seek mode";

	if (validateSeekOffset(absolutePos))
		m_pos = absolutePos;

}
uint64 CFileIOCallBack::getFilePointer()
{
	CAutoLock lock(&m_accessLock);
	return m_pos;
}

void CFileIOCallBack::close()
{
	closeFile();
}


// ----------------------------------------
// CAsyncCallback
// ----------------------------------------
CAsyncCallback::CAsyncCallback()	:
	m_ardr(0),
	m_pos(0),
	m_length(0)
{
	m_bufferCache = NULL;
	m_bufferCacheSize = 2*1024*1024;
	//m_bufferCache = new BYTE[m_bufferCacheSize];
	m_bCacheBufferReady=false;
}

CAsyncCallback::~CAsyncCallback()
{
	if (m_bufferCache)
		delete m_bufferCache;
}

void CAsyncCallback::SetAsyncReader( IAsyncReader *ardr )
{
		m_ardr = ardr;
		GetLength();
		m_bCacheBufferReady = false;
}

void CAsyncCallback::BeginFlush()
{
	if (!m_ardr) throw "<CAsyncCallback::BeginFlush> invalid pointer to IAsyncReader interface";
	m_ardr->BeginFlush();
}

void CAsyncCallback::EndFlush()
{
	if (!m_ardr) throw "<CAsyncCallback::EndFlush> invalid pointer to IAsyncReader interface";
    m_ardr->EndFlush();
}

uint64 CAsyncCallback::GetLength()
{
	if (!m_ardr) throw "<CAsyncCallback::read> invalid pointer to IAsyncReader interface";
	uint64	actual;
	m_ardr->Length((LONGLONG *)&m_length, (LONGLONG *)&actual);
	return m_length;
}




bool CAsyncCallback::validateSeekOffset( int64 offset)
{
	bool ret = true;
		 if (offset > (int64)m_length)
			 ret = false;
			 //throw after_end_of_stream;
			 //throw "<CAsyncCallback::setFilePointer> invalid seek offset. Resulting position is greater than stream length ";

		 if ( offset  < 0 )
			 ret = false;
			 //throw before_begin_of_stream;
			 //throw "<CAsyncCallback::setFilePointer> invalid seek offset. Resulting position is smaller than 0 ";
 return ret;
}
uint32 CAsyncCallback::read(void*Buffer,size_t Size)
 {
	 CAutoLock lock(&m_readCS);
	 //g_profiler.start("CAsyncCallback::read");
	if (!m_ardr) throw "<CAsyncCallback::read> invalid pointer to IAsyncReader interface";
	if ( m_pos > m_length)
	{
		//m_pos = -1;
		return 0;
	}

	if (m_bufferCache)
	{
		if (!m_bCacheBufferReady)
		{
			 //g_profiler.start("CAsyncCallback::read fromfile");
			HRESULT hr = m_ardr->SyncRead(m_pos, m_bufferCacheSize, m_bufferCache);
			m_bufferCacheStartPos =m_pos;
			m_bCacheBufferReady = true;
			//g_profiler.stop("CAsyncCallback::read fromfile");
		}
		ASSERT(m_bCacheBufferReady);
		if (m_bCacheBufferReady )
		{
			if ((m_pos >=m_bufferCacheStartPos && m_pos < m_bufferCacheStartPos+m_bufferCacheSize) &&// if start pos inside cache
				((m_pos-m_bufferCacheStartPos)+Size < m_bufferCacheSize))// check that requested size is inside cache
			{ 
				 //g_profiler.start("CAsyncCallback::read fromcache");
				BYTE* startPosInCache = (BYTE*)m_bufferCache+(m_pos-m_bufferCacheStartPos);
				memcpy(Buffer, startPosInCache,Size);
				//g_profiler.stop("CAsyncCallback::read fromcache");
			}
			else
			{ // 
				if (Size > m_bufferCacheSize)
				{// check if cache size is enough
					delete m_bufferCache;
					m_bufferCacheSize = Size;
					m_bufferCache = new BYTE[m_bufferCacheSize];
				}
				// fill the cache with new pos
				 //g_profiler.start("CAsyncCallback::read fromfile");
				HRESULT hr = m_ardr->SyncRead(m_pos, m_bufferCacheSize, m_bufferCache);
				m_bufferCacheStartPos =m_pos;
				BYTE* startPosInCache = (BYTE*)m_bufferCache;
				memcpy(Buffer, startPosInCache,Size);
				//g_profiler.stop("CAsyncCallback::read fromfile");
			}
		}
			
	}
	else
	{
		HRESULT hr = m_ardr->SyncRead(m_pos, Size, (BYTE *)Buffer);

		if (FAILED(hr))
			throw "<CAsyncCallback::read> failed to read input data using IAsyncReader::SyncRead method";
	}
	m_pos += Size;
	 //g_profiler.stop("CAsyncCallback::read");
	return Size;
}


 void CAsyncCallback::setFilePointer(int64 Offset,seek_mode Mode)
 {
	 int64 absolutePos = 0;
	 if (Mode ==  seek_beginning )
		 absolutePos = Offset;
	 else if ( Mode == seek_current )
		 absolutePos = m_pos + Offset;
	 else if (Mode == seek_end )
		 absolutePos = m_length - Offset;	
	 else
		throw "<CAsyncCallback::setFilePointer> invalid seek mode";

	 if (validateSeekOffset(absolutePos))
		m_pos = absolutePos;
 }

 size_t CAsyncCallback::write(const void*Buffer,size_t Size)
 {
	 // Not make sence
	 return 0;
 }

 uint64 CAsyncCallback::getFilePointer()
 {
	 return m_pos;
 }

 void CAsyncCallback::close()
 {
	 // Do nothing
 }

 void CAsyncCallback::Clear()
 {
 }
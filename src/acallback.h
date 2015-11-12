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

#include "commonTypes.h"

#define BUFF_SIZE 262144 //256K
//#define BUFF_SIZE 8192 //8K

enum seek_mode
{
	seek_beginning=SEEK_SET
	,seek_end=SEEK_END
	,seek_current=SEEK_CUR
};

class CFileIOCallBack
{
public:
	CFileIOCallBack();
	virtual ~CFileIOCallBack();
	void SetFileName(LPTSTR fileName);
	// Begin flush / End flush pair
	void BeginFlush();
	void EndFlush();
	// Gets length of input stream
	uint64 GetLength();
	// IOCallback implementation
	virtual uint32 read(void*Buffer,size_t Size);
	virtual size_t write(const void*Buffer,size_t Size);
	virtual void setFilePointer(int64 Offset,seek_mode Mode=seek_beginning);
	virtual uint64 getFilePointer();
	virtual void close();

private:
	void closeFile();
	// Validation of seek position
	bool validateSeekOffset( int64 offset);
	// Current position
	uint64 m_pos;

	// Stream length
	uint64 m_length;
	FILE* m_file;
	CCritSec m_accessLock;
};

class CAsyncCallback 
{
public:
	// Exceptions
	enum ACallbackExc
	{
		after_end_of_stream = 2000,
		before_begin_of_stream
	};

	CAsyncCallback(void);
	virtual ~CAsyncCallback(void);

	// Set IAsyncReader pointer
	void SetAsyncReader( IAsyncReader *ardr );
	// Begin flush / End flush pair
	void BeginFlush();
	void EndFlush();
	// Gets length of input stream
	uint64 GetLength();

public:
	// IOCallback implementation
	virtual uint32 read(void*Buffer,size_t Size);
	virtual size_t write(const void*Buffer,size_t Size);
	virtual void setFilePointer(int64 Offset,seek_mode Mode=seek_beginning);
	virtual uint64 getFilePointer();
	virtual void close();

private:
	// Validation of seek position
	bool validateSeekOffset( int64 offset);

	// Clear buffer
	void Clear();
	// 

	// Pointer to async reader 
	IAsyncReader *m_ardr;
	// Current position
	uint64 m_pos;

	// Stream length
	uint64 m_length;
	BYTE* m_bufferCache;
	uint64 m_bufferCacheStartPos;
	size_t m_bufferCacheSize;
	bool m_bCacheBufferReady;
	CCritSec m_readCS;
};

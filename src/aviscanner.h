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
// buffer management and start code location
#include <vector>
#include "commonTypes.h"
#include "acallback.h"
#include "mcallback.h"
#include <vector>
using namespace std;



class CAVIScanner
{
public:
   
	enum PROCESS_EXIT_REASON
	{
		PER_END_OF_FILE = 0,
		PER_INTERRUPTED
	};

    CAVIScanner();
	~CAVIScanner();

	// Set scanner callback
	void SetScannerCallback( CAVICallback *callback );
	// Resets object
	void Reset();
	// Stream initialization
	void InitializeStream( IAsyncReader *rdr );
	// Can we process data or not
	void CanProcess(IAsyncReader *rdr);
	// Main processing
	PROCESS_EXIT_REASON Process();
		// Returns stream duration
	// Seek to specified stream time
	void Seek(uint64 stream_time );
	// Begin flush / End flash pair
	void BeginFlush();
	void EndFlush();
    uint64 GetDefaultDurationPerFrame(uint32 trackNum);
    STREAM_INDEX& getIndex(int streamNum);
    AVIStreamHeader&  getStreamHeader(int streamNum);
    void ReadData(int64 offset, DWORD len, BYTE* pOut);
protected:
	// Pointer to callback object which methods will be called by scanner when certain evens occurs
	CAVICallback *m_scallback;


private:
    void OnTrack(CMediaType& mt);
	uint64 m_seek_time;
    DWORD GetFourCC( const CString &strFourCC );
    bool checkRiffList();
    bool getNextLIST( const CString& strfourcc,LIST& list);
    bool getNextCHUNK(const CString& strfourcc,const uint64& StopPos, CHUNK& chunk);
    bool process_hdrl(const LIST& hdlr);
    bool process_strl(const LIST& strl);
    bool process_vids(const CHUNK& strf,const AVIStreamHeader streamheader);
    bool process_auds(const CHUNK& strf,const AVIStreamHeader streamheader);
    bool process_dv1(const CHUNK& strf,const AVIStreamHeader streamheader);
    bool process_txts(const CHUNK& strf,const AVIStreamHeader streamheader);


    bool readidx1(const CHUNK& idx1,uint64 moviDataPos);
    bool load_OldIndex(const LIST& hdlr);
    bool getAVIH(const CHUNK& avih, MainAVIHeader& mainheader);
    bool getStreamHeader(const CHUNK& strh, AVIStreamHeader& streamheader);
    bool loadODMLIndex(const CHUNK& indx);
	// Async reader callback object. Used to read data from input ping using IAsyncReader interface
	CAsyncCallback m_acallback;
	
	
    vector<STREAM_INDEX> m_indexes;
    vector<AVIStreamHeader> m_streamHeaders;
    CCritSec           m_readCS;
    bool m_bODMLIndexInUse;

};



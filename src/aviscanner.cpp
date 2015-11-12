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
#include "aviscanner.h"
#define is_id(e, ref) (dynamic_cast<ref*>(e) != NULL)

#define in_parent(p) (m_acallback.getFilePointer() < (p->GetElementPosition() + p->ElementSize()))

// ------------------------------------------------------------
// CAVIScanner
// ------------------------------------------------------------
CAVIScanner::CAVIScanner(): 
  m_seek_time(0),
  m_bODMLIndexInUse(false)
{
}

CAVIScanner::~CAVIScanner()
{
	Reset();
}

void CAVIScanner::Reset()
{
	
}


void CAVIScanner::SetScannerCallback( CAVICallback *callback )
{
	m_scallback = callback;
}


void CAVIScanner::InitializeStream( IAsyncReader *rdr )
{
	m_acallback.SetAsyncReader(rdr);
	m_acallback.setFilePointer(0, seek_beginning);
    m_bODMLIndexInUse = false;


}	
bool CAVIScanner::checkRiffList()
{
    DWORD dwLIST;
    uint32 ret = m_acallback.read(&dwLIST, sizeof(dwLIST));
    if (ret != 4)
        return false;
    DWORD dwSize;
    ret = m_acallback.read(&dwSize, sizeof(dwSize));
    if (ret != 4)
        return false;
     DWORD dwFourCC;
    ret = m_acallback.read(&dwFourCC, sizeof(dwFourCC));
    if (ret != 4)
        return false;
 
    if (dwLIST == GetFourCC("RIFF") && dwFourCC == GetFourCC("AVI "))
        return true;
    return false;
}
DWORD CAVIScanner::GetFourCC( const CString &strFourCC )
{
    DWORD fccResult = 0;

    if( strFourCC.GetLength() != 4 )
        return 0;

    fccResult =   ( strFourCC.GetAt( 0 )	   )
        | ( strFourCC.GetAt( 1 ) << 8  )
        | (	strFourCC.GetAt( 2 ) << 16 )
        | (	strFourCC.GetAt( 3 ) << 24 );

    return fccResult;
}
void CAVIScanner::CanProcess( IAsyncReader *rdr)
{
	//g_profiler.start("CAVIScanner::CanProcess");
	InitializeStream(rdr);

    m_indexes.clear();
    m_streamHeaders.clear();
    if (!checkRiffList())
	    throw "<CAVIScanner::CanProcess> No RIFF element found";
    //  seach for hdlr LIST
    LIST hdlr;
    if (!getNextLIST( "hdrl",hdlr))
         throw "<CAVIScanner::CanProcess> No hdrl element found";
       
    if (!process_hdrl(hdlr))
        throw "<CAVIScanner::CanProcess> unable to processHdrl";

    if (!m_bODMLIndexInUse)
    {
        if (!load_OldIndex(hdlr))
            throw "<CAVIScanner::CanProcess> unable to load_index";
    }
}
bool CAVIScanner::load_OldIndex(const LIST& hdlr)
{
    m_acallback.setFilePointer(hdlr.dataPos, seek_beginning);
    LIST movi;
    if (!getNextLIST( "movi",movi))
        return false;
    m_acallback.setFilePointer(movi.dataPos+movi.dwSize, seek_beginning);
    CHUNK idx1;
    if (getNextCHUNK("idx1",(uint64)-1, idx1))
    {
        readidx1(idx1,movi.dataPos+4);
        int i = 0;
    }
    return true;
}
bool CAVIScanner::readidx1(const CHUNK& idx1, uint64 moviDataPos)
{
    m_acallback.setFilePointer(idx1.dataPos, seek_beginning);
    int indexEntriesCount = idx1.dwSize/sizeof(AVIINDEXENTRY);
    vector<AVIINDEXENTRY> avientries;
    avientries.resize(indexEntriesCount);
    INDEXENTRY entry;
    vector<uint64> cumLen;
    vector<DWORD> streamentriesCount;
    cumLen.resize(m_indexes.size(),0);
    streamentriesCount.resize(m_indexes.size(),0);

    uint32 ret = m_acallback.read(&(avientries[0]), sizeof(AVIINDEXENTRY)*indexEntriesCount);
    if (ret != sizeof(AVIINDEXENTRY)*indexEntriesCount)
    {
            return false;
    }
    for (size_t i =0; i < m_indexes.size(); i++)
    {
        m_indexes[i].resize(indexEntriesCount);
    }
    bool bOffsetInIndexFromMoviStart = true;
    for (int i = 0; i < indexEntriesCount; i++)
    {       
        AVIINDEXENTRY& avientry = avientries[i];
        if (i == 0)
        {
            if (avientry.dwChunkOffset+4 == (moviDataPos)) 
                bOffsetInIndexFromMoviStart = false;

        }
         entry.chunkOffset =avientry.dwChunkOffset;

        if (bOffsetInIndexFromMoviStart)
            entry.chunkOffset += moviDataPos;
        else
            entry.chunkOffset += 8;
      
           
        entry.dwChunkLength = avientry.dwChunkLength;
        entry.dwFlags = avientry.dwFlags;
        
        size_t streamNum = ((avientry.ckid & 0xff) - '0') * 10;
        streamNum += ((avientry.ckid >> 8) & 0xff) - '0';
        if (streamNum >=0 && (streamNum < m_indexes.size()))
        {           
            entry.cumlen = cumLen[streamNum];
            m_indexes[streamNum][streamentriesCount[streamNum]] = entry;
            streamentriesCount[streamNum]++;
            cumLen[streamNum]+=entry.dwChunkLength;
        }
    }
    for (size_t i =0; i < m_indexes.size(); i++)
    {
        m_indexes[i].resize(streamentriesCount[i]);
    }

    return true;
    
}
bool CAVIScanner::getAVIH(const CHUNK& avih, MainAVIHeader& mainheader)
{
    m_acallback.setFilePointer(avih.dataPos, seek_beginning);
    uint32 ret = m_acallback.read(&mainheader, sizeof(MainAVIHeader));
    if (ret != sizeof(MainAVIHeader))
        return false;
    return true;

}
bool CAVIScanner::process_hdrl(const LIST& hdlr)
{
    
    m_acallback.setFilePointer(hdlr.dataPos, seek_beginning);
    CHUNK avih;
    if (!getNextCHUNK("avih",hdlr.dataPos+hdlr.dwSize,avih))
    {
        return false;
    }
    MainAVIHeader mainheader;
    if (!getAVIH(avih, mainheader))
        return false;

    // now process strl and create tracks
    for (size_t i = 0; i < mainheader.dwStreams; i++)
    {
        LIST strl;
        if (getNextLIST("strl",strl))
        {
            process_strl(strl);                
            m_acallback.setFilePointer(strl.dataPos+strl.dwSize, seek_beginning);
        }
        
    }
    return true;
}
bool CAVIScanner::getNextCHUNK( const CString& strfourcc,const uint64& StopPos, CHUNK& chunk)
  {
     DWORD fourcc=  GetFourCC(strfourcc);
    DWORD fcc;
    DWORD listfcc = GetFourCC("LIST");    
    do
    {
        uint32 ret = m_acallback.read(&fcc, sizeof(fcc));
        if (ret != sizeof(fcc))
            break;
        if (fcc == listfcc)
        {// skip list
            DWORD dwSize;
            ret = m_acallback.read(&dwSize, sizeof(dwSize));
            if (ret != 4)
                break;
            m_acallback.setFilePointer(dwSize-4, seek_current);
            if (m_acallback.getFilePointer() > StopPos)
                break;
            
        }
        else
        { 
           
            DWORD dwSize;           
            ret = m_acallback.read(&dwSize, sizeof(dwSize));
            if (ret != 4)
                break;
            if (fcc == fourcc)
            {


                chunk.dwFourCC = fcc;
                chunk.dwSize = dwSize;
                chunk.dataPos = m_acallback.getFilePointer();
                return true;
            }
            m_acallback.setFilePointer(dwSize, seek_current);
            if (m_acallback.getFilePointer() > StopPos)
                break;
        }

    }
    while(true);
    return false;
  }
bool CAVIScanner::getNextLIST( const CString& strfourcc,LIST& list)

{
     DWORD fourcc = GetFourCC(strfourcc);
    DWORD fcc;
    DWORD listfcc = GetFourCC("LIST");    
    do
    {
        uint32 ret = m_acallback.read(&fcc, sizeof(fcc));
        if (ret != sizeof(fcc))
            break;
        if (fcc == listfcc)
        {
            DWORD dwSize;
            DWORD dwFourCC;
            ret = m_acallback.read(&dwSize, sizeof(dwSize));
            if (ret != 4)
                break;
            ret = m_acallback.read(&dwFourCC, sizeof(dwFourCC));
            if (ret != 4)
                break;
            if (dwFourCC == fourcc)
            {
                list.dwFourCC = dwFourCC;               
                list.dwSize = dwSize -4;
                list.dataPos = m_acallback.getFilePointer();
                return true;
            }
            m_acallback.setFilePointer(dwSize-4, seek_current);
        }
        else
        { // skip chunk data
             DWORD dwSize;           
            ret = m_acallback.read(&dwSize, sizeof(dwSize));
             if (ret != 4)
                break;
             m_acallback.setFilePointer(dwSize, seek_current);
        }

    }
    while(true);
    return false;
    
}

bool CAVIScanner::process_strl(const LIST& strl)
{
    m_acallback.setFilePointer(strl.dataPos, seek_beginning);
    CHUNK strh;
    if (!getNextCHUNK("strh",strl.dataPos+strl.dwSize,strh))
    {
        return false;
    }
    AVIStreamHeader streamheader;
    if (!getStreamHeader(strh, streamheader))
        return false;
    m_streamHeaders.push_back(streamheader);
    m_acallback.setFilePointer(strl.dataPos, seek_beginning);
    CHUNK strf;
    if (!getNextCHUNK("strf",strl.dataPos+strl.dwSize,strf))
    {
        return false;
    }
 
    if (streamheader.fccType == GetFourCC("vids"))
    {
        process_vids(strf,streamheader);
    }
    else if (streamheader.fccType == GetFourCC("auds"))
    {
        process_auds(strf,streamheader);
    }
    else if (streamheader.fccType == GetFourCC("iavs"))
    {
        process_dv1(strf,streamheader);
    }
    else if (streamheader.fccType == GetFourCC("txts"))
    {
        process_txts(strf,streamheader);
    }
    else     
        return false;
    
    m_acallback.setFilePointer(strl.dataPos, seek_beginning);
    CHUNK indx;
    if (getNextCHUNK("indx",strl.dataPos+strl.dwSize,indx))
    {// check presence of odml superindex
        loadODMLIndex(indx);
    }

    return true;
}
bool CAVIScanner::loadODMLIndex(const CHUNK& indx)
{

     m_acallback.setFilePointer(indx.dataPos, seek_beginning);
     AVISUPERINDEX superindex;
     {
     uint32 ret = m_acallback.read(&superindex, sizeof(superindex));
     if (ret != sizeof(superindex))
         return false;
     }
     if (!superindex.nEntriesInUse)
         return false;

     vector<AVISUPERINDEXENTRY> superindexEntries;
     superindexEntries.resize( superindex.nEntriesInUse);
     uint64 cumlen = 0;

     {
     uint32 ret = m_acallback.read(&superindexEntries[0], sizeof(AVISUPERINDEXENTRY)*superindex.nEntriesInUse);
     if (ret != sizeof(AVISUPERINDEXENTRY)*superindex.nEntriesInUse)
         return false;
     }

    
     // now collect all std index chunks
     vector<AVISTDINDEXENTRY> stdindexEntries;
      AVISTDINDEX stdindex;
     for (size_t i = 0; i < superindex.nEntriesInUse; i++)
     {
        AVISUPERINDEXENTRY& sEntry =  superindexEntries[i];
        m_acallback.setFilePointer(sEntry.qwOffset, seek_beginning);
      
        uint32 ret = m_acallback.read(&stdindex, sizeof(AVISTDINDEX));
        if (ret != sizeof(stdindex))
           return false;

        
        if (stdindexEntries.size() != stdindex.nEntriesInUse)
            stdindexEntries.resize(stdindex.nEntriesInUse);

        {
            uint32 ret = m_acallback.read(&(stdindexEntries[0]), sizeof(AVISTDINDEXENTRY)*stdindex.nEntriesInUse);
            if (ret != sizeof(AVISTDINDEXENTRY)*stdindex.nEntriesInUse)
                return false;
        }

        STREAM_INDEX& streamindx = m_indexes[m_indexes.size()-1];
        
        int startposinIndex = streamindx.size();
        streamindx.resize(streamindx.size()+stdindexEntries.size());
        for (size_t j = 0; j < stdindexEntries.size(); j++)
        {
           AVISTDINDEXENTRY& stdentry =  stdindexEntries[j];
           INDEXENTRY& streamindxEntry = streamindx[ startposinIndex+j];
           streamindxEntry.dwFlags = stdentry.dwSize & 0x80000000? 0:AVIIF_INDEX;
           streamindxEntry.chunkOffset = stdindex.qwBaseOffset+stdentry.dwOffset;
           streamindxEntry.dwChunkLength = stdentry.dwSize&0x7FFFFFFF;           
           streamindxEntry.cumlen = cumlen;
           cumlen+=streamindxEntry.dwChunkLength;
        }
        

     }


     //calc memory of the indexes
     {
         uint64 indexesmemSize = 0;
        for (size_t i = 0;i < m_indexes.size();i++)
        {
            indexesmemSize+=m_indexes[i].size()*sizeof(INDEXENTRY);
        }
        int i = 0;
     }
     m_bODMLIndexInUse = true;
     return true;
}
bool CAVIScanner::process_vids(const CHUNK& strf,const AVIStreamHeader streamheader)
{
    m_acallback.setFilePointer(strf.dataPos, seek_beginning);
   
    BYTE* pFormatData = new BYTE[strf.dwSize];

    uint32 ret = m_acallback.read(pFormatData, strf.dwSize);
    if (ret != strf.dwSize)
    {
        delete pFormatData;
        return false;
    }

    int extraDataLen = strf.dwSize-sizeof(BITMAPINFOHEADER);
    BITMAPINFOHEADER* pbih = (BITMAPINFOHEADER*)pFormatData;
    CMediaType mt;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = TRUE;
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = FOURCCMap(pbih->biCompression);
    mt.formattype = FORMAT_VideoInfo;
    mt.lSampleSize = streamheader.dwSampleSize;
    BYTE* pFormat = mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)+extraDataLen);
    
    VIDEOINFOHEADER vih;
    memset(&vih, 0, sizeof(VIDEOINFOHEADER));
    vih.AvgTimePerFrame = (REFERENCE_TIME)(1e7*streamheader.dwScale/ streamheader.dwRate);
    vih.bmiHeader = *pbih;
    memcpy(pFormat, &vih, sizeof(VIDEOINFOHEADER));
    if (extraDataLen)
        memcpy(pFormat+sizeof(VIDEOINFOHEADER), pFormatData+sizeof(BITMAPINFOHEADER), extraDataLen);
     if (pbih->biCompression == MAKEFOURCC('X','2','6','4') || pbih->biCompression == MAKEFOURCC('x','2','6','4') || pbih->biCompression == MAKEFOURCC('h','2','6','4'))
     {
         vih.bmiHeader.biCompression = MAKEFOURCC('H','2','6','4');
         mt.subtype = FOURCCMap(vih.bmiHeader.biCompression);
     }
      

    delete pFormatData;
   
    OnTrack(mt);
    return true;
}

bool CAVIScanner:: process_txts(const CHUNK& strf,const AVIStreamHeader streamheader)
{
    m_acallback.setFilePointer(strf.dataPos, seek_beginning);        
    CMediaType mt;
    mt.majortype = MEDIATYPE_Text;
    mt.bFixedSizeSamples = FALSE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = streamheader.dwSampleSize?streamheader.dwSampleSize:1;
    mt.subtype = FOURCCMap((DWORD)streamheader.fccHandler);
    mt.formattype = GUID_NULL;   
    OnTrack(mt);
    return true;
}

bool CAVIScanner:: process_dv1(const CHUNK& strf,const AVIStreamHeader streamheader)
{
    m_acallback.setFilePointer(strf.dataPos, seek_beginning);

    BYTE* pFormatData = new BYTE[strf.dwSize];

  
    uint32 ret = m_acallback.read(pFormatData, strf.dwSize);
    if (ret != strf.dwSize)
    {
        delete pFormatData;
        return false;
    }
  
    DVINFO* pdvinfo = (DVINFO*)pFormatData;
    CMediaType mt;
    mt.majortype = MEDIATYPE_Interleaved;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = streamheader.dwSampleSize;
    mt.subtype = FOURCCMap((DWORD)streamheader.fccHandler);
    mt.formattype = FORMAT_DvInfo;
    BYTE* pFormat = mt.AllocFormatBuffer(sizeof(DVINFO));
    memcpy(pFormat, pdvinfo, sizeof(DVINFO));
   
    delete pFormatData;
    OnTrack(mt);
    return true;
}

bool CAVIScanner::process_auds(const CHUNK& strf,const AVIStreamHeader streamheader)
{
    m_acallback.setFilePointer(strf.dataPos, seek_beginning);

    BYTE* pFormatData = new BYTE[strf.dwSize];

  
    uint32 ret = m_acallback.read(pFormatData, strf.dwSize);
    if (ret != strf.dwSize)
    {
        delete pFormatData;
        return false;
    }
    WAVEFORMATEX* pwf = (WAVEFORMATEX*)pFormatData;
    CMediaType mt;
    mt.majortype = MEDIATYPE_Audio;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = streamheader.dwSampleSize;

    mt.subtype = FOURCCMap((DWORD)pwf->wFormatTag);
    mt.formattype = FORMAT_WaveFormatEx;
    int formatLen = max(strf.dwSize,sizeof(WAVEFORMATEX));
    BYTE* pFormat = mt.AllocFormatBuffer(formatLen);
    memset(pFormat, 0, formatLen);
    memcpy(pFormat, pwf, strf.dwSize); 
    delete pFormatData;
    OnTrack(mt);
    return true;
}

void CAVIScanner::OnTrack(CMediaType& mt)
{
    STREAM_INDEX index;
    m_indexes.push_back(index);
    m_scallback->OnTrack(mt);
}

bool CAVIScanner::getStreamHeader(const CHUNK& strh, AVIStreamHeader& streamheader)
{
    m_acallback.setFilePointer(strh.dataPos, seek_beginning);
    uint32 ret = m_acallback.read(&streamheader, sizeof(AVIStreamHeader));
    if (ret != sizeof(AVIStreamHeader))
        return false;
    return true;
}

CAVIScanner::PROCESS_EXIT_REASON  CAVIScanner::Process()
{
	//g_profiler.start("processingonesecond");
	PROCESS_EXIT_REASON retReason = PER_END_OF_FILE;
    
	
	return retReason;
}

uint64 CAVIScanner::GetDefaultDurationPerFrame(uint32 trackNum)
{
	uint64 duration = 0;
	
	return duration;
}
STREAM_INDEX& CAVIScanner::getIndex(int streamNum)
{
    return m_indexes[streamNum];
}
AVIStreamHeader&  CAVIScanner::getStreamHeader(int streamNum)
{
    return m_streamHeaders[streamNum];
}

void CAVIScanner::ReadData(int64 offset, DWORD len, BYTE* pOut)
{
    CAutoLock lock(&m_readCS);

    m_acallback.setFilePointer(offset, seek_beginning);
    m_acallback.read(pOut, len);
}
void  CAVIScanner::BeginFlush()
{
	m_acallback.BeginFlush();
}
void  CAVIScanner::EndFlush()
{
	m_acallback.EndFlush();
}

void CAVIScanner::Seek( uint64 stream_time )
{
	m_seek_time = stream_time;	
}
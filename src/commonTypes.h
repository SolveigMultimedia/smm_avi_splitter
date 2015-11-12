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
typedef int int32;
typedef  unsigned int uint32;
typedef LONGLONG    int64;
typedef  ULONGLONG uint64;
typedef unsigned char uint8;
#include <vector>
using namespace std;
#define AVIIF_INDEX             0x10
#define AVI_INDEX_OF_INDEXES 0x00 // when each entry in aIndex array points to an index chunk
#define AVI_INDEX_OF_CHUNKS 0x01 // when each entry in aIndex array points to a chunk in the file
#define AVI_INDEX_IS_DATA 0x80 // when each entry is aIndex is


// buffer ptr + buffer len
typedef struct tag_Block_Frame
{
	tag_Block_Frame()
	{
		splice = false;
		trackNum = 0;
		frameTime= -1;
		frameDuration = -1;
		data = NULL;
		dataSize = 0;
	}
	tag_Block_Frame(bool _splice, uint32 _trackNum,
					REFERENCE_TIME _frameTime,
					REFERENCE_TIME _frameDuration,
					uint8* _data, 
					uint32 _dataSize)
	{
		splice = _splice;
		trackNum = _trackNum;
		frameTime = _frameTime;
		frameDuration = _frameDuration;
		data = _data;
		dataSize = _dataSize;
	};
	bool splice;
	uint32 trackNum;
	REFERENCE_TIME frameTime;
	REFERENCE_TIME frameDuration;
	uint8* data;
	uint32 dataSize;

}BLOCK_FRAME;
typedef enum track_type {
    track_video       = 0x01, ///< Rectangle-shaped non-transparent pictures aka video
    track_audio       ,
    track_subtitle,
    track_interleaved

} track_type;
bool operator ==(const BLOCK_FRAME& f1, const BLOCK_FRAME& f2);


typedef struct {
FOURCC fccType;
FOURCC fccHandler;
DWORD dwFlags;
WORD wPriority;
WORD wLanguage;
DWORD dwInitialFrames;
DWORD dwScale;
DWORD dwRate; /* dwRate / dwScale == samples/second */
DWORD dwStart;
DWORD dwLength; /* In units above... */
DWORD dwSuggestedBufferSize;
DWORD dwQuality;
DWORD dwSampleSize;
RECT rcFrame;
} AVIStreamHeader;

typedef struct
{
DWORD dwMicroSecPerFrame; // frame display rate (or 0)
DWORD dwMaxBytesPerSec; // max. transfer rate
DWORD dwPaddingGranularity; // pad to multiples of this
// size;
DWORD dwFlags; // the ever-present flags
DWORD dwTotalFrames; // # frames in file
DWORD dwInitialFrames;
DWORD dwStreams;
DWORD dwSuggestedBufferSize;
DWORD dwWidth;
DWORD dwHeight;
DWORD dwReserved[4];
} MainAVIHeader;

typedef struct _tagList{
    _tagList()
    {dwSize= 0; dwFourCC=0; dataPos =0;}
    DWORD dwSize;
    DWORD dwFourCC;
    uint64 dataPos;
} LIST;

typedef struct _tagCHUNK {
    _tagCHUNK()
    {dwSize= 0; dwFourCC=0; dataPos = 0;}
    DWORD dwFourCC;
    DWORD dwSize;
    uint64 dataPos;
} CHUNK;

typedef struct {
DWORD ckid;
DWORD dwFlags;
DWORD dwChunkOffset;
DWORD dwChunkLength;
} AVIINDEXENTRY;

#pragma pack(1)
typedef struct _avisuperindex_chunk {
WORD wLongsPerEntry;
BYTE bIndexSubType;
BYTE bIndexType;
DWORD nEntriesInUse;
DWORD dwChunkId;
DWORD dwReserved[3];
} AVISUPERINDEX;

#pragma pack(1)
typedef struct _avisuperindex_entry {
__int64 qwOffset;
DWORD dwSize;
DWORD dwDuration;
}AVISUPERINDEXENTRY;

#pragma pack(1)
typedef struct _avistdindex_chunk {
    _avistdindex_chunk()
    {
        fcc = 0;
        cb = 0;
        wLongsPerEntry = 0;
        bIndexSubType = 0;
        bIndexType = 0;
        nEntriesInUse = 0;
        dwChunkId = 0;
        qwBaseOffset = 0;
        dwReserved3 = 0;
    }
FOURCC fcc;
DWORD cb;
WORD wLongsPerEntry;
BYTE bIndexSubType;
BYTE bIndexType;
DWORD nEntriesInUse;
DWORD dwChunkId;
__int64 qwBaseOffset;
DWORD dwReserved3;
} AVISTDINDEX;

typedef struct _avistdindex_entry {
DWORD dwOffset;
DWORD dwSize;
}  AVISTDINDEXENTRY;


typedef struct tag_INDEXENTRY {
    tag_INDEXENTRY()
    {

        dwFlags = 0;
        chunkOffset = 0;
        dwChunkLength  = 0;
        cumlen = 0;

    }
    tag_INDEXENTRY(const AVIINDEXENTRY& entry, uint64 offset)
    {
        dwFlags = entry.dwFlags;
        chunkOffset = offset+entry.dwChunkOffset;
        dwChunkLength = entry.dwChunkLength;
    }
DWORD dwFlags;
uint64 chunkOffset;
DWORD dwChunkLength;
uint64 cumlen;
} INDEXENTRY;


 typedef vector<INDEXENTRY> STREAM_INDEX;
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


class CAVISplitter;
class CAVIOutputPin;
#include "mtype.h"
#include <string>
#include <map>
#include <vector>
#include "commonTypes.h"
using namespace std;

struct TrackInfo
{
	TrackInfo()
	{
		track_number = 0;
		type = 0;
	}
	

uint32 track_number;
uint32 type;
CMediaType mediaType;
};

class COutPinsCreator
{
public:
	COutPinsCreator(void);
	virtual ~COutPinsCreator(void);
	void Init( CAVISplitter *splitter);
	void AddTrack( const CMediaType& mt);
	void Reset();
	void CreatePins();
	void CreateDynamicPins();
	size_t GetTracksCount();

	bool getTrackInfo(uint32 track_number,TrackInfo& ti );
private:
	// internal pins creation routines
	CAVIOutputPin* CreatePin(const TrackInfo& ti);

	CAVISplitter* m_splitter;
	//Last created video, audio and subtitle pins number
	typedef  pair<uint32, TrackInfo> TI_PAIR;
	map<uint32, TrackInfo> m_tracks;
};

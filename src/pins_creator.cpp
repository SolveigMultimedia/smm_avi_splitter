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
#include "pins_creator.h"
#include "pins.h"
#include "avisplitter.h"

COutPinsCreator::COutPinsCreator(void):m_splitter(NULL)
{
	Reset();
}

COutPinsCreator::~COutPinsCreator(void)
{
}



void COutPinsCreator::Init( CAVISplitter *splitter )
{
	m_splitter = splitter;
}



void COutPinsCreator::AddTrack(const CMediaType& mt)
{
    TrackInfo ti;
    ti.track_number =   m_tracks.size();
    ti.mediaType = mt;
    if (mt.majortype == MEDIATYPE_Video)
    {
        ti.type = track_video;
    }
    else if(mt.majortype == MEDIATYPE_Audio)
    {
        ti.type = track_audio;
    }
    else if(mt.majortype == MEDIATYPE_Text)
    {
        ti.type= track_subtitle;
    }
    else if (mt.majortype == MEDIATYPE_Interleaved)
    {
        ti.type = track_interleaved;
    }
	m_tracks.insert(TI_PAIR(ti.track_number, ti));	
}




CAVIOutputPin* COutPinsCreator::CreatePin(const TrackInfo& ti)
{
	CAVIOutputPin  *p= NULL;
	HRESULT hr = S_OK;

	// prepare pin name
	int currentPinNum = 0;
    CString pinName= GetPinNameByMediaType(ti.mediaType);
	// create pins itself
	switch(ti.type)
	{
	case track_video:
		p = new CAVIVideoPin(m_splitter, m_splitter->GetFilterCS(), &hr, pinName);
		break;
	case track_audio:
		p = new CAVIAudioPin(m_splitter, m_splitter->GetFilterCS(), &hr, pinName);
		break;
	case track_subtitle:
		p = new CAVISubtitlePin(m_splitter, m_splitter->GetFilterCS(), &hr, pinName);
		break;
    case track_interleaved:
        p = new CAVIInterleavedPin(m_splitter, m_splitter->GetFilterCS(), &hr, pinName);
        break;
	/*default:
		throw "COutPinsCreator::CreatePin  unknown type";*/
	}
	
	if (p)
	{
		p->SetTrackInfo(ti);
		p->SetMediaType(&ti.mediaType);
		// initial addref, release in CAVISplitter::BreakInputConnect
		p->AddRef(); 
		m_splitter->AddOutputPin(p);
	}

	return p;
}

size_t COutPinsCreator::GetTracksCount()
{
	return m_tracks.size();
}


bool COutPinsCreator::getTrackInfo(uint32 track_number,TrackInfo& ti )
{
	map<uint32, TrackInfo>::iterator it = m_tracks.find(track_number);
	if (it == m_tracks.end())
		return false;
	ti = it->second;
	return true;
}
void COutPinsCreator::CreatePins()
{

	CAVIOutputPin  *p = NULL;
	HRESULT hr = S_OK;
	// create pins
	for (map<uint32, TrackInfo>::iterator it = m_tracks.begin(); it != m_tracks.end(); it++)
	{
		TrackInfo& ti = it->second;
		p = CreatePin(ti);
	}

}

void COutPinsCreator::Reset()
{
	// clear track infos
	m_tracks.clear();
}
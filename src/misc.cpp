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
#include <mmreg.h>

CString GetFormatNameByMediaType(const AM_MEDIA_TYPE& mt)
{
	CString ret; 
	if (mt.majortype == MEDIATYPE_Video)
	{		
		{
			if (mt.subtype == FOURCCMap('1VLF'))
				ret +=_T("H.263");
			else if (mt.subtype == FOURCCMap('4VLF'))
				ret +=_T("VP6");
			else if ((mt.subtype == FOURCCMap('1CVA') ||
				mt.subtype == FOURCCMap('1cva') ||
				mt.subtype == FOURCCMap('462H') ||
				mt.subtype == FOURCCMap('462h') ||
				mt.subtype == FOURCCMap('462X') ||
				mt.subtype == FOURCCMap('462x') ||
				mt.subtype == FOURCCMap('462x') ))
				ret +=_T("AVC");		
			else
			{
				CString code;
				FOURCCMap	fourcc(&mt.subtype);
				DWORD		videoInfo = fourcc.GetFOURCC();

				char temp;
				temp = char( videoInfo & 0x000000FF );
				code += temp;
				temp = char( videoInfo >> 8 & 0x000000FF );
				code += temp;
				temp = char( videoInfo >> 16 & 0x000000FF );
				code += temp;
				temp = char( videoInfo >> 24 & 0x000000FF );
				code += temp;
				ret+=code;
			}
		}
	}
	else if (mt.majortype == MEDIATYPE_Audio)
	{
		{
			if (mt.subtype == FOURCCMap(WAVE_FORMAT_PCM))
				ret +=_T("PCM");
			else if (mt.subtype == FOURCCMap(MAKEFOURCC('A','S','W','F'))	)
				ret +=_T("ADPCM(ShockWave)");				
			else if (mt.subtype == FOURCCMap(WAVE_FORMAT_MPEGLAYER3))
				ret +=_T("MP3");			
			else if (mt.subtype == MEDIASUBTYPE_MPEG1AudioPayload ||
					 mt.subtype == MEDIASUBTYPE_MPEG1Packet		  ||
					 mt.subtype == MEDIASUBTYPE_MPEG1Payload	 ||
					 mt.subtype == MEDIASUBTYPE_MPEG1Audio )
				ret +=_T("MPA");			
			else if (mt.subtype == FOURCCMap(MAKEFOURCC('N','E','L','L')))
				ret +=_T("NELLYMOSER 8HZ MONO");			
			else if (mt.subtype == FOURCCMap(0x00FF))
				ret +=_T("AAC");						
			else if (mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
				ret +=_T("LPCM");
			else if (mt.subtype == FOURCCMap(WAVE_FORMAT_ADPCM))
				ret +=_T("MS-ADPCM");
			else if (mt.subtype == FOURCCMap(WAVE_FORMAT_DVI_ADPCM))
				ret +=_T("Intel-ADPCM");
			else if (mt.subtype == FOURCCMap(DWORD('rmas')))
				ret +=_T("SAMR");
			else if (mt.subtype == MEDIASUBTYPE_DOLBY_AC3)
				ret +=_T("AC3");
		}
	}
	return ret;
}

CString GetPinNameByMediaType(const AM_MEDIA_TYPE& mt)
{
	CString ret; 
	if (mt.majortype == MEDIATYPE_Video)
	{
		ret +=_T("Video");
		ret +=_T(" ");		
	}
	else if (mt.majortype == MEDIATYPE_Audio)
	{
		ret +=_T("Audio");
		ret +=_T(" ");
	}
	else if (mt.majortype == MEDIATYPE_Text)
	{
		ret +=_T("Text");
		ret +=_T(" ");
	}
    else if (mt.majortype == MEDIATYPE_Interleaved)
	{
		ret +=_T("Interleaved");
		ret +=_T(" ");
	}
	ret+=GetFormatNameByMediaType(mt);
	return ret;
}



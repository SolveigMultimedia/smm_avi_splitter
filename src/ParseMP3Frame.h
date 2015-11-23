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

//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PARSEFRAME_H__7107AF1B_F772_4843_B2FB_DD8664C2C24D__INCLUDED_)
#define AFX_PARSEFRAME_H__7107AF1B_F772_4843_B2FB_DD8664C2C24D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <streams.h>// needed for REFERENCE_TIME definition

struct FrameInfo
{
	FrameInfo():
	nStCodePos(-1),
	nFrameNum(0),
	nFrameSize(0),
	rtTime(0),
	bitrate(0),
	freq(0),
    channel_Mode(0)
	{
	}

	int nStCodePos;
	int nFrameNum;
    int channel_Mode;
	int	nFrameSize;
	double	rtTime;
	DWORD bitrate;
	int freq;
};

class CParseFrame  
{
public:
	CParseFrame();
	virtual ~CParseFrame();

	int	GetBitRate			( void );

	int	getFrameSync		( void )
	{
		return ( ( bithdr >> 21 ) & 2047 ); 
	};

    int	getVersionIndex		( void )
	{ 
		return ( ( bithdr >> 19 ) & 3 );  
	};

    int	getLayerIndex		( void )
	{ 
		return ( ( bithdr >> 17 ) & 3 ); 
	};

    int	getProtectionBit	( void )
	{ 
		return ( ( bithdr >> 16 ) & 1 );  
	};

    int	getBitrateIndex		( void )
	{ 
		return ( ( bithdr >> 12 ) & 15 ); 
	};

    int	getFrequencyIndex	( void )
	{ 
		return ((bithdr>>10) & 3);  
	};

    int	getPaddingBit		( void )
	{ 
		return ( ( bithdr >> 9 ) & 1);  
	};

    int	getPrivateBit		( void )
	{
		return ( ( bithdr >> 8 ) & 1);  
	};

    int	getModeIndex		( void )
	{ 
		return ( ( bithdr >> 6 ) & 3);  
	};

    int	getModeExtIndex		( void )
	{ 
		return ( ( bithdr >> 4 ) & 3);  
	};

    int	getCoprightBit		( void )
	{ 
		return ( ( bithdr >> 3 ) & 1);  
	};

    int	getOrginalBit		( void )
	{ 
		return ( ( bithdr >> 2 ) & 1);  
	};

    int	getEmphasisIndex	( void )
	{ 
		return ( ( bithdr ) & 3);  
	};

    float		getVersion			( void );
	void		loadHeader			( BYTE *c );//(char c[4]);
	int			getFrequency		( void );
	int			getLayer			( void );
	int			getFrameNum			( void );
    int         getChannelMode();
	FrameInfo	FindFirstStartCode	( BYTE *pbBuf, long lLength, long lStartPos, const FrameInfo& prevFrameHeader );

	
	BOOL		isValidHeaderAudio	( void );

private:
	unsigned long bithdr;

protected:
	int   m_nLastFrameNum;
	BOOL  m_bFirstFrame;

};

#endif // !defined(AFX_PARSEFRAME_H__7107AF1B_F772_4843_B2FB_DD8664C2C24D__INCLUDED_)

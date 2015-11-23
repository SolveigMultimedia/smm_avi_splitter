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
#include "stdafx.h"
#include "ParseMP3Frame.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CParseFrame::CParseFrame():
m_nLastFrameNum(NULL),
m_bFirstFrame(TRUE)
{

}

CParseFrame::~CParseFrame()
{

}

int CParseFrame::GetBitRate()
{
 const int table[2][3][16] = {
        {         //MPEG 2 & 2.5
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, //Layer III
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, //Layer II
            {0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,0}  //Layer I
        },{       //MPEG 1
            {0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}, //Layer III
            {0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,0}, //Layer II
            {0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,0}  //Layer I
        }
    };

    // the bitrate is not only dependent of the bitrate index,
    // the bitrate also varies with the MPEG version and Layer version
    return table[(getVersionIndex() & 1)][(getLayerIndex() -1)][getBitrateIndex()];
}

void CParseFrame::loadHeader(BYTE *c)//(char c[4])
{
     bithdr = (unsigned long)(
                              ( (*(c) & 255) << 24) |
                              ( (*(c+1) & 255) << 16) |
                              ( (*(c+2) & 255) <<  8) |
                              ( (*(c+3) & 255)      )
                            ); 
}

int CParseFrame::getFrequency()
{
	// a table to convert the indexes into
    // something informative...
    static int table[4][3] = {

//      {32000, 16000,  8000}, //MPEG 2.5
		{11025, 12000,  8000}, //MPEG 2.5
        {    0,     0,     0}, //reserved
        {22050, 24000, 16000}, //MPEG 2
        {44100, 48000, 32000}  //MPEG 1

    };

    // the frequency is not only dependent of the bitrate index,
    // the bitrate also varies with the MPEG version
    return table[getVersionIndex()][getFrequencyIndex()];
}

int CParseFrame::getLayer()
{
	  return ( 4 - getLayerIndex() );
}

float CParseFrame::getVersion() {

    // a table to convert the indexes into
    // something informative...
    float table[4] = {
                      2.5, 0.0, 2.0, 1.0
                     };

    // return modified value
    return table[getVersionIndex()];
}

BOOL CParseFrame::isValidHeaderAudio()
{
	return ( ( ( getFrameSync()      & 2047) == 2047 ) &&
             ( ( getVersionIndex()   &    3) != 1 ) &&
             ( ( getLayerIndex()     &    3) != 0 ) && 


             // due to lack of support of the .mp3 format
             // no "public" .mp3's should contain information
             // like this anyway... :)
             ( ( getBitrateIndex()   &   15) !=   0 ) &&
             ( ( getBitrateIndex()   &   15) !=  15 ) &&
             ( ( getFrequencyIndex() &    3) !=   3 ) &&
             ( ( getEmphasisIndex()  &    3) !=   2 )    );
}
int CParseFrame::getChannelMode()
{
    int mode = ( ( bithdr >> 6 ) & 3 ); 
    return mode;
}
int	CParseFrame::getFrameNum()
{
	int   iCountFr	= 0;
	float fVer		= getVersion();
	
	if ( fVer == 1.0 )
	{
		switch( getLayer() ) 
		{
		case 1 :  iCountFr = 384; 
			break;
		case 2 :  iCountFr = 1152;
			break;
		case 3 :  iCountFr = 1152;
			break;
		default:
			break;
		} 
	}
	else
		if (fVer == 2.0)
		{
			switch(getLayer()) 
			{
			case 1 : iCountFr = 384; 
				break;
			case 2 : iCountFr = 1152;
				break;
			case 3 : iCountFr = 576;
				break;
			default:
				break;
			} 
			
		}
		else
			if (fVer == 2.5)
			{
				switch(getLayer()) 
				{
					case 1 : iCountFr = 384; 
						break;
					case 2 : iCountFr = 1152;
						break;
					case 3 : iCountFr = 576; 
						break;
					default:
						break;
				}
			}
	return 	(m_nLastFrameNum = iCountFr);		
}

FrameInfo CParseFrame::FindFirstStartCode( BYTE *pbBuf, long lLength, long lStartPos, const FrameInfo& prevFrameHeader )
{
	FrameInfo fInfo;
	
	for( int nCountSize = lStartPos; lLength - nCountSize >= 4 ; nCountSize++)
	{
		loadHeader( &pbBuf[nCountSize] );

		BOOL a1  = isValidHeaderAudio();
		

		if ( a1 == TRUE )
		{            			
			fInfo.nFrameNum		= getFrameNum();
			fInfo.channel_Mode = getChannelMode();
			fInfo.bitrate		= GetBitRate() * 1000;	
            fInfo.freq =  getFrequency();
			fInfo.nFrameSize	= ( ( fInfo.nFrameNum * fInfo.bitrate ) / fInfo.freq ) / 8 + getPaddingBit();
			fInfo.rtTime		=   ( ( 1e7 * (double)fInfo.nFrameNum ) / (double) fInfo.freq ); // this is not the same as the system does
            if( 0 != prevFrameHeader.freq && fInfo.freq != prevFrameHeader.freq )
            {
                continue;
            }          
            if (prevFrameHeader.nStCodePos != -1 && prevFrameHeader.channel_Mode != fInfo.channel_Mode)
                continue;

            fInfo.nStCodePos	= nCountSize;
			

			break;
		}
	}
	return fInfo;
}
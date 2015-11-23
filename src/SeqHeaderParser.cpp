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
#include "SeqHeaderParser.h"


#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

#define BSWAP_DW(x) ((((x) << 24) & 0xff000000) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00) | (((x) >> 24) & 0xff))



// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

	FOURCC _MPEG4_FOURCC []= 
	{
		FCC('XVID'),
		FCC('XVIX'),
		FCC('DIVX'),
		FCC('MP4S'),
		FCC('MP4V'),
		FCC('M4S2'),
		FCC('DX50'),
		FCC('BLZ0'),
		FCC('3IV2'),
		FCC('3IVX'),
		FCC('RMP4'),
		FCC('DM4V'),
		FCC('MP42'),
		FCC('DIV2'),
		FCC('MP41'),
		FCC('MPG4'),
		FCC('DIV1'),
		FCC('DIV1')
	};
	
	FOURCC _AVC_FOURCC []= 
	{
		FCC('H264'),
		FCC('X264'),
		FCC('VSSH'),
		FCC('DAVC'),
		FCC('AVC1')
	};

CSeqHeaderSimpleParser::CSeqHeaderSimpleParser ( void ) :
m_FrameType		( FrameType_NonSet ),
m_bMPEG4ByFOurcc( FALSE ),
m_bAVCByFOurcc	( FALSE )
{

	memset( &m_visual_object_sequence, 0, sizeof(MPG_OBJ) );
	memset( &m_pic_parameter_set_rbsp, 0, sizeof(MPG_OBJ) );
	
}

CSeqHeaderSimpleParser::~CSeqHeaderSimpleParser(void)
{
}
void CSeqHeaderSimpleParser::Init(DWORD fourcc)
{
    memset( &m_visual_object_sequence, 0, sizeof(MPG_OBJ) );
	memset( &m_pic_parameter_set_rbsp, 0, sizeof(MPG_OBJ) );

    if (!CheckIfFourccIsAVC(fourcc))
        CheckIfFourccIsMPEG4(fourcc);
}
bool CSeqHeaderSimpleParser::isInited()
{
   	return m_bMPEG4ByFOurcc || m_bAVCByFOurcc;
}
bool CSeqHeaderSimpleParser::ParseFrame(BYTE *pb, long length)
{
	if( IsVideoAVCByFourcc() )
		return ParseAVCFrame(pb,length);


	int sc_pos = FindStartCode( vop_start_code, pb, length, SEARCH_FORWARD);
	if( sc_pos == -1 ){
		return false;
	}

	m_FrameType = (FrameType )((pb[sc_pos + 4] >> 6) & 0x03);

	int sc_seq_strt_pos = FindStartCode( visual_object_sequence_start_code, pb, length, SEARCH_FORWARD);
	if( sc_seq_strt_pos == -1 ){
		return false;
	}

	int len = sc_pos - sc_seq_strt_pos;
	if( len > 0 ){
		memcpy( m_visual_object_sequence.obj, &pb[sc_seq_strt_pos], len );
		m_visual_object_sequence.obj_len = len;
        return true;
	}
    return false;
}


bool CSeqHeaderSimpleParser::ParseAVCFrame(BYTE *pb, long length)
{

	BYTE * pBufer = pb;
	BYTE * pBuferStart = pBufer;
	long locLen	  = length;

	// we should copy seq_par_set adn pic_parameter_set_rbsp to insern them to AVI / ASF first sample
	
	int sc_pos  = 0;
	int sec_pos = 0;

	
	{
		int scSize = 0;
		sc_pos = FindAVCStartCode( seq_parameter_set_rbsp, pBufer, locLen, SEARCH_FORWARD, scSize);
		if( sc_pos == -1 )
			return false;

		int shift = sc_pos+scSize;
		pBufer +=  shift;		
		locLen -= shift;
		
		sec_pos = FindAVCStartCode( nal_unit_type_unspecified, pBufer, locLen , SEARCH_FORWARD,scSize);
		if( sec_pos == -1 )
			sec_pos = length;
		else
		sec_pos += shift; // we shifted buffer to sc_pos previously so we hast to compensate that

		memcpy( m_visual_object_sequence.obj, &pBuferStart[sc_pos], sec_pos -  sc_pos);
		m_visual_object_sequence.obj_len = sec_pos - sc_pos;
	}
	length -= (pBufer-pBuferStart);
	pBuferStart = pBufer;
	
	{
		int scSize = 0;
		sc_pos = FindAVCStartCode( pic_parameter_set_rbsp, pBufer, locLen, SEARCH_FORWARD,scSize );
		if( sc_pos == -1 )
			return false;

		int shift = sc_pos+scSize;
		pBufer += shift ;
		locLen -= shift;

		sec_pos = FindAVCStartCode( nal_unit_type_unspecified, pBufer, locLen , SEARCH_FORWARD,scSize );
		if( sec_pos == -1 )
			sec_pos = length;
		else		
		sec_pos += shift; // we shifted buffer to sc_pos previously so we hast to compensate that

		memcpy( m_pic_parameter_set_rbsp.obj, &pBuferStart[sc_pos], sec_pos -  sc_pos);
		m_pic_parameter_set_rbsp.obj_len = sec_pos - sc_pos;
	}
    return true;
}


BOOL CSeqHeaderSimpleParser::CheckIfFourccIsAVC( DWORD fourcc )
{
	DWORD size = sizeof(_AVC_FOURCC ) / sizeof(_AVC_FOURCC[0] ); 

	TCHAR
		fcc1[ sizeof(_AVC_FOURCC[0])+1 ] = {0},
		fcc2[ sizeof(_AVC_FOURCC[0])+1 ] = {0};
	Dword4ccToString( fourcc, fcc1 );
	for(DWORD n = 0; n <  size; n++ )
	{
		Dword4ccToString( _AVC_FOURCC[n], fcc2 );
		if( !_tcsicmp( fcc1, fcc2 ) )
		{
			m_bAVCByFOurcc = TRUE;
			break;
		}
	}
	return m_bAVCByFOurcc;
}

BOOL CSeqHeaderSimpleParser::CheckIfFourccIsMPEG4 ( DWORD fourcc )
{
	m_bMPEG4ByFOurcc = 	IsFourCC_MPEG4(fourcc);
	return m_bMPEG4ByFOurcc;
}

BOOL CSeqHeaderSimpleParser::IsFourCC_MPEG4( FOURCC fourcc )
{
	DWORD size = sizeof(_MPEG4_FOURCC ) / sizeof(_MPEG4_FOURCC[0] ); 
	TCHAR
		fcc1[ sizeof(_MPEG4_FOURCC[0])+1 ] = {0},
		fcc2[ sizeof(_MPEG4_FOURCC[0])+1 ] = {0};
	Dword4ccToString( fourcc, fcc1 );
	for(DWORD n = 0; n <  size; n++ )
	{
		Dword4ccToString( _MPEG4_FOURCC[n], fcc2 );
		if( !_tcsicmp( fcc1, fcc2 ) )
			return TRUE;
	}
	return FALSE;
}

void CSeqHeaderSimpleParser::Dword4ccToString( DWORD fcc, LPTSTR buffer )
{
	buffer[0]=LOBYTE(LOWORD(fcc));
	buffer[1]=HIBYTE(LOWORD(fcc));
	buffer[2]=LOBYTE(HIWORD(fcc));
	buffer[3]=HIBYTE(HIWORD(fcc));
}

int	CSeqHeaderSimpleParser::FindStartCode	( DWORD stCode,	BYTE * pBuffer,  long lLength, SEARCHING_DIRECTION lDirection )
{
	int pos		= -1;
	BYTE *pBuf	= pBuffer;

	if( lDirection == SEARCH_FORWARD )
	{
		const BYTE * pBufBound	= pBuf + lLength;

		for( ; pBuf + 4 <= pBufBound ; pBuf++) 
		{
			if( BSWAP_DW(*(DWORD*)pBuf) == stCode ) {
				pos = int(pBuf - pBuffer);
				break;
			}
		}
	}
	else
		if(lDirection == SEARCH_BACKWARD )
		{
			const BYTE * pBufBound	= pBuf - lLength;
			pBuf -=sizeof(DWORD);
			for( ; pBuf >= pBufBound ; pBuf--) 
			{
				if( BSWAP_DW(*(DWORD*)pBuf) == stCode ){
					pos = int( pBuffer - pBuf );
					break;
				}
			}
		}

	return pos;
}
NAL_UNIT_TYPE	GetNALUnitType	(BYTE nalUnitTypeByte  )
{
	NAL_UNIT_TYPE tp = NAL_UNIT_TYPE(nalUnitTypeByte & 0x1F);

	return tp;
}
int IsStartCode(BYTE * pBuf)
{
	if (pBuf[0] == 0x00 && pBuf[1] == 0x00  && pBuf[2] == 0x00 && pBuf[3] == 0x01)
		return 4;
	else if (pBuf[0] == 0x00 && pBuf[1] == 0x00  && pBuf[2] == 0x01 )
		return 3;
	return 0;
}
int	CSeqHeaderSimpleParser::FindAVCStartCode( NAL_UNIT_TYPE nal_tp, BYTE *pBuffer,  long lLength,	SEARCHING_DIRECTION lDirection, int& scSize)
{
	int pos		= -1;
	BYTE *pBuf	= pBuffer;
	NAL_UNIT_TYPE tp = nal_unit_type_unspecified;

	if( lDirection == SEARCH_FORWARD )
	{
		const BYTE * pBufBound	= pBuf + lLength;

		for( ; pBuf + 4 < pBufBound ; pBuf++) 
		{
			int startCodeSize = IsStartCode(pBuf);
			if (startCodeSize != 0)
			{
				tp = GetNALUnitType ( *(pBuf+startCodeSize) );
				if( nal_tp == tp || 
					nal_tp == nal_unit_type_unspecified )
				{
					pos = int(pBuf - pBuffer);
					scSize = startCodeSize;
					break;
				}
				pBuf+=startCodeSize;
			}
		}
	}
	else
		if(lDirection == SEARCH_BACKWARD )
		{
			const BYTE * pBufBound	= pBuf - lLength;
			for( ; pBuf >= pBufBound ; pBuf--) 
			{
				int startCodeSize = IsStartCode(pBuf);
				if (startCodeSize != 0)
				{
					tp = GetNALUnitType ( *(pBuf+startCodeSize) );
					if( nal_tp == tp || 
						nal_tp == nal_unit_type_unspecified )
					{
						pos = int(pBuf - pBuffer);
						scSize = startCodeSize;
						break;
					}
					pBuf-=startCodeSize;
				}
			}
		}
	return pos;
}
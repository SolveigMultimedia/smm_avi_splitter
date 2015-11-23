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
enum SEARCHING_DIRECTION
{
	SEARCH_FORWARD = 0,
	SEARCH_BACKWARD = SEARCH_FORWARD + 1
};
enum NAL_UNIT_TYPE
{
	nal_unit_type_unspecified				= 0,
	slice_layer_without_partitioning_rbsp_NonIDR	= 1,
	slice_data_partition_a_layer_rbsp		= 2,
	slice_data_partition_b_layer_rbsp		= 3,
	slice_data_partition_c_layer_rbsp		= 4,
	slice_layer_without_partitioning_rbsp_IDR		= 5,
	sei_rbsp								= 6,
	seq_parameter_set_rbsp					= 7,
	pic_parameter_set_rbsp					= 8,
	access_unit_delimiter_rbsp				= 9,
	end_of_seq_rbsp							= 10,
	end_of_stream_rbsp						= 11,
	filler_data_rbsp						= 12
};

const 	DWORD	visual_object_sequence_start_code	= 0x000001B0;
	const 	DWORD 	visual_object_start_code			= 0x000001B5;
	const 	DWORD 	vop_start_code						= 0x000001B6; //video object plane
	const 	DWORD 	avop_start_code						= 0x000001B6; //video object plane
	const   DWORD	video_object_layer_start_code_first = 0x00000120;
    const   DWORD	video_object_layer_start_code_last  = 0x0000012F;
	
	class CSeqHeaderSimpleParser
	{
		enum FrameType
		{
			FrameType_Intra		= 0x00,
			FrameType_Predicted = 0x01,
			FrameType_Bidirect	= 0x02,
			FrameType_Sprite	= 0x03,
			FrameType_NonSet	= 0x04
		} ;
		

	public:
		struct MPG_OBJ
		{
			BYTE obj[400];
			LONG obj_len;
		};
		struct VisualObjectSequence
		{
			BYTE profile;
			BYTE level;
		};
		struct VideoObjectLayer
		{
			BYTE random_accessible_vol;
			BYTE video_object_type_indication;
			BYTE is_object_layer_identifier;
			BYTE video_object_layer_verid;
			BYTE video_object_layer_priority;
			BYTE aspect_ratio_info;
			BYTE par_width;
			BYTE par_height;
			// add other when needed
		};

		CSeqHeaderSimpleParser			( void );
		~CSeqHeaderSimpleParser			( void );
		bool ParseFrame				( BYTE *pb, long length);
        void Init(DWORD fourcc);        
        bool isInited();
        MPG_OBJ& VisualObjSeq			( void )
		{
			return m_visual_object_sequence;
		}
		
		MPG_OBJ& Seq_Parameter_Set		( void )
		{
			return m_visual_object_sequence;
		}
		
		MPG_OBJ& Pic_Parameter_Set		( void )
		{
			return m_pic_parameter_set_rbsp;
		}

 private:

		bool ParseAVCFrame			( BYTE *pb, long length);


		static BOOL IsFourCC_MPEG4	( DWORD );
		BOOL CheckIfFourccIsMPEG4	( DWORD fourcc );
		BOOL CheckIfFourccIsAVC		( DWORD fourcc );

	

		BOOL IsVideoMPEG4byFourcc	( void )
		{
			return m_bMPEG4ByFOurcc;
		}
		
		BOOL IsVideoAVCByFourcc		( void )
		{
			return m_bAVCByFOurcc;
		}
		
		


        private:
        int	FindStartCode	( DWORD stCode,	BYTE * pBuffer,  long lLength, SEARCHING_DIRECTION lDirection );
        int	FindAVCStartCode( NAL_UNIT_TYPE nal_tp, BYTE *pBuffer,  long lLength,	SEARCHING_DIRECTION lDirection, int& scSize);

		static void	Dword4ccToString(DWORD fcc, LPTSTR buffer);
		FrameType	m_FrameType;
		
		MPG_OBJ 	m_visual_object_sequence;
		MPG_OBJ 	m_pic_parameter_set_rbsp;
		
		BOOL		m_bMPEG4ByFOurcc;
		BOOL		m_bAVCByFOurcc;

	};


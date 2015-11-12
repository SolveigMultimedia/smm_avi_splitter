#include "stdafx.h"
#include "async_callback.h"

CAsyncCallback::CAsyncCallback()	:
	m_ardr(0),
	m_pos(0),
	m_length(0)
{
}

CAsyncCallback::~CAsyncCallback()
{
}


void CAsyncCallback::SetAsyncReader( IAsyncReader *ardr )
{
		m_ardr = ardr;
		calcLength();
}

void CAsyncCallback::BeginFlush()
{
	if (!m_ardr) throw "<CAsyncCallback::BeginFlush> invalid pointer to IAsyncReader interface";
	m_ardr->BeginFlush();
}

void CAsyncCallback::EndFlush()
{
	if (!m_ardr) throw "<CAsyncCallback::EndFlush> invalid pointer to IAsyncReader interface";
    m_ardr->EndFlush();
}

void CAsyncCallback::calcLength()
{
	if (!m_ardr) throw "<CAsyncCallback::read> invalid pointer to IAsyncReader interface";
	uint64	actual;
	m_ardr->Length((LONGLONG *)&m_length, (LONGLONG *)&actual);
}

uint32 CAsyncCallback::read(void*Buffer,size_t Size)
 {
	if (!m_ardr) throw "<CAsyncCallback::read> invalid pointer to IAsyncReader interface";

	if ( m_pos > m_length)
		return 0;

	HRESULT hr = m_ardr->SyncRead(m_pos, Size, (BYTE *)Buffer);
	if (FAILED(hr))
		throw "<CAsyncCallback::read> failed to read input data using IAsyncReader::SyncRead method";
	m_pos += Size;
	return Size;
}

void CAsyncCallback::validateSeekOffset( int64 offset)
{
		 if (offset > (int64)m_length) 	 throw after_end_of_stream;
			 //throw "<CAsyncCallback::setFilePointer> invalid seek offset. Resulting position is greater than stream length ";

		 if ( offset  < 0 ) 	throw before_begin_of_stream;
			 //throw "<CAsyncCallback::setFilePointer> invalid seek offset. Resulting position is smaller than 0 ";
}

 void CAsyncCallback::setFilePointer(int64 Offset,seek_mode Mode)
 {
	 if (Mode ==  seek_beginning )
	 {
		 validateSeekOffset(0 + Offset);
		 m_pos = 0 + Offset;
	 }
	 else if ( Mode == seek_current )
	 {
		 validateSeekOffset(m_pos + Offset);
		 m_pos = m_pos + Offset;
	 }
	 else if (Mode == seek_end )
	 {
		 validateSeekOffset(m_length + Offset);
		 m_pos = m_length + Offset;
	 }
	 else
		throw "<CAsyncCallback::setFilePointer> invalid seek mode";
 }

 size_t CAsyncCallback::write(const void*Buffer,size_t Size)
 {
	 // Not make sence
	 return 0;
 }

 uint64 CAsyncCallback::getFilePointer()
 {
	 return m_pos;
 }

 void CAsyncCallback::close()
 {
	 // Do nothing
 }

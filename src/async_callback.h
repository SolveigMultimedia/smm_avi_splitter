#pragma once

class CAsyncCallback : public IOCallback
{
public:
	// Pointer to async reader 
	IAsyncReader *m_ardr;
	// Current position
	uint64 m_pos;

	 // Calcualtes length of input stream
	 void calcLength();

	 // Stream length
	 uint64 m_length;

	 // Validation of seek position
	 void validateSeekOffset( int64 offset);
public:
	// Construction/destruction
	CAsyncCallback(void);
	virtual ~CAsyncCallback(void);
	
	// Set IAsyncReader pointer
	void SetAsyncReader( IAsyncReader *ardr );

	// Begin flush / End flush pair
	void BeginFlush();
	void EndFlush();

	// Exceptions
	enum ACallbackExc
	{
		after_end_of_stream = 2000,
		before_begin_of_stream
	};
public:
	// IOCallback implementation
	virtual uint32 read(void*Buffer,size_t Size);
	virtual void setFilePointer(int64 Offset,seek_mode Mode=seek_beginning);
	virtual size_t write(const void*Buffer,size_t Size);
	virtual uint64 getFilePointer();
	virtual void close();
};

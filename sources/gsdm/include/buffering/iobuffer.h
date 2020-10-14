#ifndef _IOBUFFER_H
#define _IOBUFFER_H

#include "platform/linuxplatform.h"
#include "utils/file.h"


#define GETAVAILABLEBYTESCOUNT(x)     ((x).published_ - (x).consumed_)
#define GETIBPOINTER(x)     ((uint8_t *)((x).buffer_ + (x).consumed_))

namespace gsdm {

struct PoolSocket;
class PoolWorkerEx;

/*!
	@class IOBuffer
	@brief Base class which contains all the necessary functions involving input/output buffering.
 */
class IOBuffer {
public:
	//constructor/destructor and initialization
	IOBuffer();
	virtual ~IOBuffer();

	/*!
		@brief initializes the buffer
		@param expected - The expected size of this buffer
	 */
	void Initialize(uint32_t expected);

	//Read from a source and put to this buffer
	/*!
		@brief Read from Pipe and saves it.
		@param fd
		@param expected - Expected size of the buffer
		@param recvAmount - Size of data read
	 */
	bool ReadFromFd(int32_t fd, uint32_t expected, int32_t &recv_amount);

	/*!
		@brief Read from TCP File Descriptor and saves it. This function is advisable for connection-oriented sockets.
		@param fd - Descriptor that contains the data
		@param expected - Expected size of the receiving buffer
		@param recvAmount - Size of data received
	 */
	bool ReadFromTCPFd(int32_t fd, uint32_t expected, int32_t &recv_amount);

	/*!
		@brief Read from TCP File Descriptor and saves it. This function is advisable for connection-oriented sockets.
		@param fd - Descriptor that contains the data
		@param expected - Expected size of the receiving buffer
		@param recvAmount - Size of data received
	 */
	bool ReadFromTCPPoolSocket(PoolSocket *fd, PoolWorkerEx *ex, uint32_t expected, int32_t &recv_amount, gsdm_msec_t timeout = (unsigned long long)(-1LL));

	/*!
		@brief Read from UDP File Descriptor and saves it. This function is advisable for connectionless-oriented sockets.
		@param fd - Descriptor that contains the data
		@param recvAmount - Size of data received
		@param peerAddress - The source's address
	 */
	bool ReadFromUDPFd(int32_t fd, int32_t &recv_amount, sockaddr_in &peer_address);

	/*!
		@brief Read from File Stream and saves it.
		@param fs - Descriptor that contains the data
		@param size - Size of the receiving buffer
	 */
	bool ReadFromFs(std::fstream &fs, uint32_t size);

	/*!
		@brief Read from File Stream and saves it
		@param fs - Descriptor that contains the data
		@param size - Size of the receiving buffer
	 */
	bool ReadFromFs(GSDMFile &file, uint32_t size);

	/*!
		@brief Read data from buffer and copy it.
		@param pBuffer - Buffer that contains the data to be read.
		@param size - Size of data to read.
	 */
	bool ReadFromBuffer(const uint8_t *buffer, const uint32_t size);

	/*!
		@brief Read data from an input buffer starting at a designated point
		@param pInputBuffer - Pointer to the input buffer
		@param start - The point where copying starts
		@param size - Size of data to read
	 */
	void ReadFromInputBuffer(IOBuffer *input_buffer, uint32_t start, uint32_t size);

	/*!
		@brief Read data from an input buffer
		@param buffer - Buffer that contains the data to be read.
		@param size - Size of data to read.
	 */
	bool ReadFromInputBuffer(const IOBuffer &buffer, uint32_t size);

	/*!
		@brief Read data from buffer from a string
		@param binary - The string that will be read
	 */
	bool ReadFromString(const std::string &binary);

	/*!
		@brief Read data from buffer byte data
		@param byte
	 */
	void ReadFromByte(uint8_t byte);

	/*!
		@brief Read data from a memory BIO
		@param pBIO
	 */
	// bool ReadFromBIO(BIO *pBIO);

	/*!
		@brief
	 */
	void ReadFromRepeat(uint8_t byte, uint32_t size);

	//Read from this buffer and put to a destination
	/*!
		@brief Read data from buffer and store it.
		@param fd
		@param size - Size of buffer
		@param sentAmount - Size of data sent
	 */
	bool WriteToTCPFd(int32_t fd, uint32_t size, int32_t &sent_amount);

	/*!
		@brief Read data from standard IO and store it.
		@param fd
		@param size
	 */
	bool WriteToFd(int32_t fd, uint32_t size);

	//Utility functions

	/*!
		@brief Returns the minimum chunk size. This value is initialized in the constructor.
	 */
	uint32_t GetMinChunkSize();
	/*!
		@brief Sets the value of the minimum chunk size
		@param minChunkSize
	 */
	void SetMinChunkSize(uint32_t min_chunk_size);

	/*!
		@brief Returns the size of the buffer that has already been written on.
	 */
	uint32_t GetCurrentWritePosition();

	/*!
		@brief Returns the pointer to the buffer
	 */
	uint8_t *GetPointer();

	//memory management

	/*!
		@brief Ignores a specified size of data in the buffer
		@param size
	 */
	bool Ignore(uint32_t size);

	/*!
		@brief Ignores all data in the buffer
	 */
	bool IgnoreAll();

	/*!
		@brief Moves the data in a buffer to optimize memory space
	 */
	bool MoveData();

	/*!
		@brief Makes sure that there is enough memory space in the buffer. If space is not enough, a new buffer is created.
		@param expected - This function makes sure that the size indicated by this parameter is accommodated.
		@discussion In the case that a new buffer is created and the current buffer has data, the data is copied to the new buffer.
	 */
	bool EnsureSize(uint32_t expected);

	/*!
		@brief
	 */

	static std::string DumpBuffer(uint8_t *buffer, uint32_t length);
  std::string ToString(uint32_t start_index = 0, uint32_t limit = 0);
	operator std::string();

protected:
	void Cleanup();
	void Recycle();

public:
	uint8_t *buffer_;
	uint64_t size_;
	uint64_t published_;
	uint64_t consumed_;
	uint64_t min_chunk_size_;
	socklen_t dummy_;

};

  
}




#endif // _IOBUFFER_H

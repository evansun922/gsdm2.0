/* 
 *  Copyright (c) 2012,
 *  sunlei
 *
 */


#include "buffering/iobuffer.h"
#include "logging/logger.h"
#include "pooldrive/poolworkerex.h"

#ifdef WITH_SANITY_INPUT_BUFFER
#define SANITY_INPUT_BUFFER \
assert(consumed_<=published_); \
assert(published_<=size_);
#define SANITY_INPUT_BUFFER_INDEX \
assert(index >= 0); \
assert((published_ - consumed_ - index) > 0);
#else
#define SANITY_INPUT_BUFFER
#define SANITY_INPUT_BUFFER_INDEX
#endif

namespace gsdm {

IOBuffer::IOBuffer() {
	buffer_ = NULL;
	size_ = 0;
	published_ = 0;
	consumed_ = 0;
	min_chunk_size_ = 4096;
	dummy_ = sizeof (sockaddr_in);
	SANITY_INPUT_BUFFER;
}

IOBuffer::~IOBuffer() {
	SANITY_INPUT_BUFFER;
	Cleanup();
	SANITY_INPUT_BUFFER;
}

void IOBuffer::Initialize(uint32_t expected) {
	if ((buffer_ != NULL) ||
			(size_ != 0) ||
			(published_ != 0) ||
			(consumed_ != 0))
		ASSERT("This buffer was used before. Please initialize it before using");
	EnsureSize(expected);
}

bool IOBuffer::ReadFromFd(int32_t fd, uint32_t expected, int32_t &recv_amount) {
	//TODO: This is an UGLY hack.
#ifndef WIN32
	SANITY_INPUT_BUFFER;
	if (published_ + expected > size_) {
		if (!EnsureSize(expected)) {
			SANITY_INPUT_BUFFER;
			return false;
		}
	}
	recv_amount = read(fd, (char *) (buffer_ + published_), expected);
	if (recv_amount > 0) {
		published_ += (uint32_t) recv_amount;
		SANITY_INPUT_BUFFER;
		return true;
	} else {
		if (errno != EINPROGRESS) {
			// FATAL("Unable to read from pipe: %d %s", err, strerror(err));
			SANITY_INPUT_BUFFER;
			return false;
		}
		SANITY_INPUT_BUFFER;
		return true;
	}
#else
	NYIA;
	return false;
#endif
}

bool IOBuffer::ReadFromTCPFd(int32_t fd, uint32_t expected, int32_t &recv_amount) {
	SANITY_INPUT_BUFFER;
	if (published_ + expected > size_) {
		if (!EnsureSize(expected)) {
			SANITY_INPUT_BUFFER;
      recv_amount = -1;
			return false;
		}
	}

	recv_amount = recv(fd, (char *) (buffer_ + published_), expected, MSG_NOSIGNAL);
	if (recv_amount > 0) {
		published_ += (uint32_t) recv_amount;
		SANITY_INPUT_BUFFER;
		return true;
	} else {
		SANITY_INPUT_BUFFER;
		return false;
	}
}

bool IOBuffer::ReadFromTCPPoolSocket(PoolSocket *fd, PoolWorkerEx *ex, uint32_t expected, int32_t &recv_amount, gsdm_msec_t timeout) {
	SANITY_INPUT_BUFFER;
	if (published_ + expected > size_) {
		if (!EnsureSize(expected)) {
			SANITY_INPUT_BUFFER;
      recv_amount = -1;
			return false;
		}
	}

	recv_amount = ex->RecvMsg(fd, (char *) (buffer_ + published_), expected, timeout);
	if (recv_amount > 0) {
		published_ += (uint32_t) recv_amount;
		SANITY_INPUT_BUFFER;
		return true;
	} else {
		SANITY_INPUT_BUFFER;
		return false;
	}
}

bool IOBuffer::ReadFromUDPFd(int32_t fd, int32_t &recv_amount, sockaddr_in &peer_address) {
	SANITY_INPUT_BUFFER;
	if (published_ + 65536 > size_) {
		if (!EnsureSize(65536)) {
			SANITY_INPUT_BUFFER;
      recv_amount = -1;
			return false;
		}
	}

	recv_amount = recvfrom(fd, (char *) (buffer_ + published_), 65536,
			MSG_NOSIGNAL, (sockaddr *) & peer_address, &dummy_);
	if (recv_amount > 0) {
		published_ += (uint32_t) recv_amount;
		SANITY_INPUT_BUFFER;
		return true;
	} else {
		SANITY_INPUT_BUFFER;
		return false;
	}
}

bool IOBuffer::ReadFromFs(std::fstream &fs, uint32_t size) {
	SANITY_INPUT_BUFFER;
	if (published_ + size > size_) {
		if (!EnsureSize(size)) {
			SANITY_INPUT_BUFFER;
			return false;
		}
	}
	fs.read((char *) buffer_ + published_, size);
	if (fs.fail()) {
		SANITY_INPUT_BUFFER;
		return false;
	}
	published_ += size;
	SANITY_INPUT_BUFFER;
	return true;
}

bool IOBuffer::ReadFromFs(GSDMFile &file, uint32_t size) {
	SANITY_INPUT_BUFFER;
	if (size == 0) {
		SANITY_INPUT_BUFFER;
		return true;
	}
	if (published_ + size > size_) {
		if (!EnsureSize(size)) {
			SANITY_INPUT_BUFFER;
			return false;
		}
	}
	if (!file.ReadBuffer(buffer_ + published_, size)) {
		SANITY_INPUT_BUFFER;
		return false;
	}
	published_ += size;
	SANITY_INPUT_BUFFER;
	return true;
}

bool IOBuffer::ReadFromBuffer(const uint8_t *buffer, const uint32_t size) {
	SANITY_INPUT_BUFFER;
	if (!EnsureSize(size)) {
		SANITY_INPUT_BUFFER;
		return false;
	}
	memcpy(buffer_ + published_, buffer, size);
	published_ += size;
	SANITY_INPUT_BUFFER;
	return true;
}

void IOBuffer::ReadFromInputBuffer(IOBuffer *input_buffer, uint32_t start, uint32_t size) {
	SANITY_INPUT_BUFFER;
	ReadFromBuffer(GETIBPOINTER(*input_buffer) + start, size);
	SANITY_INPUT_BUFFER;
}

bool IOBuffer::ReadFromInputBuffer(const IOBuffer &buffer, uint32_t size) {
	SANITY_INPUT_BUFFER;
	if (!ReadFromBuffer(buffer.buffer_ + buffer.consumed_, size)) {
		SANITY_INPUT_BUFFER;
		return false;
	}
	SANITY_INPUT_BUFFER;
	return true;
}

bool IOBuffer::ReadFromString(const std::string &binary) {
	SANITY_INPUT_BUFFER;
	if (!ReadFromBuffer((uint8_t *) binary.c_str(), (uint32_t) binary.length())) {
		SANITY_INPUT_BUFFER;
		return false;
	}
	SANITY_INPUT_BUFFER;
	return true;
}

void IOBuffer::ReadFromByte(uint8_t byte) {
	SANITY_INPUT_BUFFER;
	EnsureSize(1);
	buffer_[published_] = byte;
	published_++;
	SANITY_INPUT_BUFFER;
}

// bool IOBuffer::ReadFromBIO(BIO *pBIO) {
// 	SANITY_INPUT_BUFFER;
// 	if (pBIO == NULL) {
// 		SANITY_INPUT_BUFFER;
// 		return true;
// 	}
// 	int32_t bioAvailable = BIO_pending(pBIO);
// 	if (bioAvailable < 0) {
// 		FATAL("BIO_pending failed");
// 		SANITY_INPUT_BUFFER;
// 		return false;
// 	}
// 	if (bioAvailable == 0) {
// 		SANITY_INPUT_BUFFER;
// 		return true;
// 	}
// 	EnsureSize((uint32_t) bioAvailable);
// 	int32_t written = BIO_read(pBIO, buffer_ + published_, bioAvailable);
// 	published_ += written;
// 	SANITY_INPUT_BUFFER;
// 	return true;
// }

void IOBuffer::ReadFromRepeat(uint8_t byte, uint32_t size) {
	SANITY_INPUT_BUFFER;
	EnsureSize(size);
	memset(buffer_ + published_, byte, size);
	published_ += size;
	SANITY_INPUT_BUFFER;
}

bool IOBuffer::WriteToTCPFd(int32_t fd, uint32_t size, int32_t &sent_amount) {
	SANITY_INPUT_BUFFER;
	bool result = true;
	sent_amount = send(fd, (char *) (buffer_ + consumed_),
			//published_ - consumed_,
			size > published_ - consumed_ ? published_ - consumed_ : size,
			MSG_NOSIGNAL);

	if (sent_amount < 0) {
		if (errno != EAGAIN) {
			FATAL("Unable to send %u bytes of data data. Size advertised by network layer was %u [%d: %s]",
					published_ - consumed_, size, errno, strerror(errno));
			FATAL("Permanent error!");
			result = false;
		}
	} else {
		consumed_ += sent_amount;
	}
	if (result)
		Recycle();
	SANITY_INPUT_BUFFER;

	return result;
}

bool IOBuffer::WriteToFd(int32_t fd, uint32_t size) {
	SANITY_INPUT_BUFFER;
	bool result = true;
	int32_t sent = write(fd, (char *) (buffer_ + consumed_),
                          published_ - consumed_);
	//size > published_ - consumed_ ? published_ - consumed_ : size,

	if (sent < 0) {
		if (errno != EAGAIN) {
			FATAL("Unable to send %u bytes of data data. Size advertised by network layer was %u [%d: %s]",
					published_ - consumed_, size, errno, strerror(errno));
			FATAL("Permanent error!");
			result = false;
		}
	} else {
		consumed_ += sent;
	}
	if (result)
		Recycle();
	SANITY_INPUT_BUFFER;

	return result;
}

uint32_t IOBuffer::GetMinChunkSize() {
	return min_chunk_size_;
}

void IOBuffer::SetMinChunkSize(uint32_t min_chunk_size) {
	assert(min_chunk_size > 0 && min_chunk_size < 16 * 1024 * 1024);
	min_chunk_size_ = min_chunk_size;
}

uint32_t IOBuffer::GetCurrentWritePosition() {
	return published_;
}

uint8_t *IOBuffer::GetPointer() {
	return buffer_;
}

bool IOBuffer::Ignore(uint32_t size) {
	SANITY_INPUT_BUFFER;
	consumed_ += size;
	Recycle();
	SANITY_INPUT_BUFFER;

	return true;
}

bool IOBuffer::IgnoreAll() {
	SANITY_INPUT_BUFFER;
	consumed_ = published_;
	Recycle();
	SANITY_INPUT_BUFFER;

	return true;
}

bool IOBuffer::MoveData() {
	SANITY_INPUT_BUFFER;
	if (published_ - consumed_ <= consumed_) {
		memcpy(buffer_, buffer_ + consumed_, published_ - consumed_);
		published_ = published_ - consumed_;
		consumed_ = 0;
	}
	SANITY_INPUT_BUFFER;

	return true;
}

bool IOBuffer::EnsureSize(uint32_t expected) {
	SANITY_INPUT_BUFFER;
	//1. Do we have enough space?
	if (published_ + expected <= size_) {
		SANITY_INPUT_BUFFER;
		return true;
	}

	//2. Apparently we don't! Try to move some data
	MoveData();

	//3. Again, do we have enough space?
	if (published_ + expected <= size_) {
		SANITY_INPUT_BUFFER;
		return true;
	}

	//4. Nope! So, let's get busy with making a brand new buffer...
	//First, we allocate at least 1/3 of what we have and no less than min_chunk_size_
	if ((published_ + expected - size_)<(size_ / 3)) {
		expected = size_ + size_ / 3 - published_;
	}

	if (expected < min_chunk_size_) {
		expected = min_chunk_size_;
	}

	//5. Allocate
	uint8_t *pTempBuffer = new uint8_t[published_ + expected];

	//6. Copy existing data if necessary and switch buffers
	if (buffer_ != NULL) {
		memcpy(pTempBuffer, buffer_, published_);
		delete[] buffer_;
	}
	buffer_ = pTempBuffer;

	//7. Update the size
	size_ = published_ + expected;
	SANITY_INPUT_BUFFER;

	return true;
}

std::string IOBuffer::DumpBuffer(uint8_t *buffer, uint32_t length) {
	IOBuffer temp;
	temp.ReadFromBuffer(buffer, length);
	return temp.ToString();
}

std::string IOBuffer::ToString(uint32_t start_index, uint32_t limit) {
	SANITY_INPUT_BUFFER;
  std::string allowedCharacters = " 1234567890-=qwertyuiop[]asdfghjkl;'\\`zxcvbnm";
	allowedCharacters += ",./!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"|~ZXCVBNM<>?";
  std::stringstream ss;
	ss << "Size: " << size_ << std::endl;
	ss << "Published: " << published_ << std::endl;
	ss << "Consumed: " << consumed_ << std::endl;
	ss << format("Address: %p", buffer_) << std::endl;
	if (limit != 0) {
		ss << format("Limited to %u bytes", limit) << std::endl;
	}
  std::string address = "";
  std::string part1 = "";
  std::string part2 = "";
  std::string hr = "";
	limit = (limit == 0) ? published_ : limit;
	for (uint32_t i = start_index; i < limit; i++) {
		if (((i % 16) == 0) && (i > 0)) {
			ss << address << "  " << part1 << " " << part2 << " " << hr << std::endl;
			part1 = "";
			part2 = "";
			hr = "";
		}
		address = format("%08u", i - (i % 16));

		if ((i % 16) < 8) {
			part1 += format("%02hhx", buffer_[i]);
			part1 += " ";
		} else {
			part2 += format("%02hhx", buffer_[i]);
			part2 += " ";
		}

		if (allowedCharacters.find(buffer_[i], 0) != std::string::npos)
			hr += buffer_[i];
		else
			hr += '.';
	}

	if (part1 != "") {
		part1 += std::string(24 - part1.size(), ' ');
		part2 += std::string(24 - part2.size(), ' ');
		hr += std::string(16 - hr.size(), ' ');
		ss << address << "  " << part1 << " " << part2 << " " << hr << std::endl;
	}
	SANITY_INPUT_BUFFER;

	return ss.str();
}

IOBuffer::operator std::string() {

	return ToString(0, 0);
}

void IOBuffer::Cleanup() {
	if (buffer_ != NULL) {

		delete[] buffer_;
		buffer_ = NULL;
	}
	size_ = 0;
	published_ = 0;
	consumed_ = 0;
}

void IOBuffer::Recycle() {
	if (consumed_ != published_)
		return;
	SANITY_INPUT_BUFFER;
	consumed_ = 0;
	published_ = 0;
	SANITY_INPUT_BUFFER;
}



}

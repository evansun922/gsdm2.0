#ifndef _FILE_H
#define _FILE_H

#include "platform/linuxplatform.h"

namespace gsdm {

enum GSDM_FILE_OPEN_MODE {
	GSDM_FILE_OPEN_MODE_READ,
	GSDM_FILE_OPEN_MODE_TRUNCATE,
	GSDM_FILE_OPEN_MODE_APPEND
};

class GSDMFile {
public:
	GSDMFile();
	virtual ~GSDMFile();

	//Init
	bool Initialize(const std::string &path);
	bool Initialize(const std::string &path, GSDM_FILE_OPEN_MODE mode);
	void Close();

	//info
	uint64_t Size();
	uint64_t Cursor();
	bool IsEOF();
  std::string GetPath();
	bool Failed();
	bool IsOpen();

	//seeking
	bool SeekBegin();
	bool SeekEnd();
	bool SeekAhead(int64_t count);
	bool SeekBehind(int64_t count);
	bool SeekTo(uint64_t position);

	//read data
	bool ReadI8(int8_t *value);
	bool ReadI16(int16_t *value);
	bool ReadI24(int32_t *value);
	bool ReadI32(int32_t *value);
	bool ReadI64(int64_t *value);
	bool ReadUI8(uint8_t *value);
	bool ReadUI16(uint16_t *value);
	bool ReadUI24(uint32_t *value);
	bool ReadUI32(uint32_t *value);
	bool ReadUI64(uint64_t *value);
	bool ReadBuffer(uint8_t *buffer, uint64_t count);
	bool ReadLine(uint8_t *buffer, uint64_t &max_size);
	bool ReadAll(std::string &str);

	//peek data
	bool PeekI8(int8_t *value);
	bool PeekI16(int16_t *value);
	bool PeekI24(int32_t *value);
	bool PeekI32(int32_t *value);
	bool PeekI64(int64_t *value);
	bool PeekUI8(uint8_t *value);
	bool PeekUI16(uint16_t *value);
	bool PeekUI24(uint32_t *value);
	bool PeekUI32(uint32_t *value);
	bool PeekUI64(uint64_t *value);
	bool PeekBuffer(uint8_t *buffer, uint64_t count);

	//write data
	bool WriteI8(int8_t value);
	bool WriteI16(int16_t value);
	bool WriteI24(int32_t value);
	bool WriteI32(int32_t value);
	bool WriteI64(int64_t value);
	bool WriteUI8(uint8_t value);
	bool WriteUI16(uint16_t value);
	bool WriteUI24(uint32_t value);
	bool WriteUI32(uint32_t value);
	bool WriteUI64(uint64_t value);
	bool WriteString(const std::string &value);
	bool WriteBuffer(const uint8_t *buffer, uint64_t count);
	bool Flush();

private:
  std::fstream file_;
	uint64_t size_;
  std::string path_;
	// bool truncate_;
	// bool append_;
};





};


#endif // _FILE_H

/* 
 *  Copyright (c) 2012,
 *  sunlei
 *
 */

#include "utils/file.h"
#include "logging/logger.h"

namespace gsdm {

GSDMFile::GSDMFile():path_("")/*,truncate_(false),append_(false)*/ {

}

GSDMFile::~GSDMFile() {
	file_.flush();
	file_.close();
}

bool GSDMFile::Initialize(const std::string &path) {
	return Initialize(path, GSDM_FILE_OPEN_MODE_READ);
}

bool GSDMFile::Initialize(const std::string &path, GSDM_FILE_OPEN_MODE mode) {
	path_ = path;
  std::ios_base::openmode openMode = std::ios_base::binary;
	switch (mode) {
		case GSDM_FILE_OPEN_MODE_READ: {
			openMode |= std::ios_base::in;
			break;
		}
    case GSDM_FILE_OPEN_MODE_TRUNCATE: {
			openMode |= std::ios_base::in;
			openMode |= std::ios_base::out;
			openMode |= std::ios_base::trunc;
			break;
		}
		case GSDM_FILE_OPEN_MODE_APPEND: {
			openMode |= std::ios_base::in;
			openMode |= std::ios_base::out;
			if (fileExists(path_))
				openMode |= std::ios_base::app;
			else
				openMode |= std::ios_base::trunc;
			break;
		}
		default: {
			FATAL("Invalid open mode");
			return false;
		}
	}

	file_.open(STR(path_), openMode);
	if (file_.fail()) {
		FATAL("Unable to open file %s with mode 0x%x (%s)",
				STR(path_), (uint32_t) openMode, strerror(errno));
		return false;
	}

	if (!SeekEnd())
		return false;

	size_ = file_.tellg();

	if (!SeekBegin())
		return false;

	return true;
}

void GSDMFile::Close() {
	file_.flush();
	file_.close();
}

uint64_t GSDMFile::Size() {
	return size_;
}

uint64_t GSDMFile::Cursor() {
	return (uint64_t) file_.tellg();
}

bool GSDMFile::IsEOF() {
	return file_.eof();
}

std::string GSDMFile::GetPath() {
	return path_;
}

bool GSDMFile::Failed() {
	return file_.fail();
}

bool GSDMFile::IsOpen() {
	return file_.is_open();
}

bool GSDMFile::SeekBegin() {
	file_.seekg(0, std::ios_base::beg);
	if (file_.fail()) {
		FATAL("Unable to seek to the beginning of file");
		return false;
	}
	return true;
}

bool GSDMFile::SeekEnd() {
	file_.seekg(0, std::ios_base::end);
	if (file_.fail()) {
		FATAL("Unable to seek to the end of file");
		return false;
	}
	return true;
}

bool GSDMFile::SeekAhead(int64_t count) {
	if (count < 0) {
		FATAL("Invali count");
		return false;
	}

	if (count + Cursor() > size_) {
		FATAL("End of file will be reached");
		return false;
	}

	file_.seekg(count, std::ios_base::cur);
	if (file_.fail()) {
		FATAL("Unable to seek ahead %" PRId64 " bytes", count);
		return false;
	}
	return true;
}

bool GSDMFile::SeekBehind(int64_t count) {
	if (count < 0) {
		FATAL("Invali count");
		return false;
	}

	if (Cursor() < (uint64_t) count) {
		FATAL("End of file will be reached");
		return false;
	}

	file_.seekg((-1) * count, std::ios_base::cur);
	if (file_.fail()) {
		FATAL("Unable to seek behind %" PRId64 " bytes", count);
		return false;
	}
	return true;
}

bool GSDMFile::SeekTo(uint64_t position) {
	if (size_ < position) {
		FATAL("End of file will be reached");
		return false;
	}

	file_.seekg(position, std::ios_base::beg);
	if (file_.fail()) {
		FATAL("Unable to seek to position %" PRIu64, position);
		return false;
	}

	return true;
}

bool GSDMFile::ReadI8(int8_t *value) {
	return ReadBuffer((uint8_t *) value, 1);
}

bool GSDMFile::ReadI16(int16_t *value) {
	if (!ReadBuffer((uint8_t *) value, 2))
		return false;
	return true;
}

bool GSDMFile::ReadI24(int32_t *value) {
	*value = 0;
	if (!ReadBuffer((uint8_t *) value, 3))
		return false;
  *value = ((*value) << 8) >> 8;
	return true;
}

bool GSDMFile::ReadI32(int32_t *value) {
	if (!ReadBuffer((uint8_t *) value, 4))
		return false;
	return true;
}

bool GSDMFile::ReadI64(int64_t *value) {
	if (!ReadBuffer((uint8_t *) value, 8))
		return false;
	return true;
}

bool GSDMFile::ReadUI8(uint8_t *value) {
	return ReadI8((int8_t *) value);
}

bool GSDMFile::ReadUI16(uint16_t *value) {
	return ReadI16((int16_t *) value);
}

bool GSDMFile::ReadUI24(uint32_t *value) {
	return ReadI24((int32_t *) value);
}

bool GSDMFile::ReadUI32(uint32_t *value) {
	return ReadI32((int32_t *) value);
}

bool GSDMFile::ReadUI64(uint64_t *value) {
	return ReadI64((int64_t *) value);
}

bool GSDMFile::ReadBuffer(uint8_t *buffer, uint64_t count) {
	file_.read((char *) buffer, count);
	if (file_.fail()) {
		FATAL("Unable to read %" PRIu64 " bytes from the file. Cursor: %" PRIu64 " (0x%" PRIx64 "); %d (%s)",
				count, Cursor(), Cursor(), errno, strerror(errno));
		return false;
	}
	return true;
}

bool GSDMFile::ReadLine(uint8_t *buffer, uint64_t &max_size) {
	file_.getline((char *) buffer, max_size);
	if (file_.fail()) {
    if (!file_.eof())
      FATAL("Unable to read line from the file");
		return false;
	}
	return true;
}

bool GSDMFile::ReadAll(std::string &str) {
	str = "";
	if (Size() >= 0xffffffff) {
		FATAL("ReadAll can only be done on files smaller than 2^32 bytes (4GB)");
		return false;
	}
	if (Size() == 0) {
		return true;
	}
	if (!SeekBegin()) {
		FATAL("Unable to seek to begin");
		return false;
	}
	uint8_t *buffer = new uint8_t[(uint32_t) Size()];
	if (!ReadBuffer(buffer, Size())) {
		FATAL("Unable to read data");
		delete[] buffer;
		return false;
	}
	str = std::string((char *) buffer, (uint32_t) Size());
	delete[] buffer;
	return true;
}

bool GSDMFile::PeekI8(int8_t *value) {
	if (!ReadI8(value))
		return false;
	return SeekBehind(1);
}

bool GSDMFile::PeekI16(int16_t *value) {
	if (!ReadI16(value))
		return false;
	return SeekBehind(2);
}

bool GSDMFile::PeekI24(int32_t *value) {
	if (!ReadI24(value))
		return false;
	return SeekBehind(3);
}

bool GSDMFile::PeekI32(int32_t *value) {
	if (!ReadI32(value))
		return false;
	return SeekBehind(4);
}

bool GSDMFile::PeekI64(int64_t *value) {
	if (!ReadI64(value))
		return false;
	return SeekBehind(8);
}

bool GSDMFile::PeekUI8(uint8_t *value) {
	return PeekI8((int8_t *) value);
}

bool GSDMFile::PeekUI16(uint16_t *value) {
	return PeekI16((int16_t *) value);
}

bool GSDMFile::PeekUI24(uint32_t *value) {
	return PeekI24((int32_t *) value);
}

bool GSDMFile::PeekUI32(uint32_t *value) {
	return PeekI32((int32_t *) value);
}

bool GSDMFile::PeekUI64(uint64_t *value) {
	return PeekI64((int64_t *) value);
}

bool GSDMFile::PeekBuffer(uint8_t *buffer, uint64_t count) {
	if (!ReadBuffer(buffer, count))
		return false;
	return SeekBehind(count);
}

bool GSDMFile::WriteI8(int8_t value) {
	return WriteBuffer((uint8_t *) & value, 1);
}

bool GSDMFile::WriteI16(int16_t value) {
	return WriteBuffer((uint8_t *) & value, 2);
}

bool GSDMFile::WriteI24(int32_t value) {
	return WriteBuffer((uint8_t *) & value, 3);
}

bool GSDMFile::WriteI32(int32_t value) {
	return WriteBuffer((uint8_t *) & value, 4);
}

bool GSDMFile::WriteI64(int64_t value) {
	return WriteBuffer((uint8_t *) & value, 8);
}

bool GSDMFile::WriteUI8(uint8_t value) {
	return WriteI8((int8_t) value);
}

bool GSDMFile::WriteUI16(uint16_t value) {
	return WriteI16((int16_t) value);
}

bool GSDMFile::WriteUI24(uint32_t value) {
	return WriteI24((int32_t) value);
}

bool GSDMFile::WriteUI32(uint32_t value) {
	return WriteI32((int32_t) value);
}

bool GSDMFile::WriteUI64(uint64_t value) {
	return WriteI64((int64_t) value);
}

bool GSDMFile::WriteString(const std::string &value) {
	return WriteBuffer((uint8_t *) STR(value), value.length());
}

bool GSDMFile::WriteBuffer(const uint8_t *buffer, uint64_t count) {
	file_.write((char *) buffer, count);
	if (file_.fail()) {
		FATAL("Unable to write %" PRIu64 " bytes to file", count);
		return false;
	}
	return true;
}

bool GSDMFile::Flush() {
	file_.flush();
	if (file_.fail()) {
		FATAL("Unable to flush to file");
		return false;
	}
	return true;
}

}

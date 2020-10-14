/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _IOHANDLER_H
#define	_IOHANDLER_H

#include "platform/linuxplatform.h"

namespace gsdm {

class IOHandlerManager;
class BaseProcess;

typedef enum _IOHandlerType {
	IOHT_TCP_ACCEPTOR,
	IOHT_TCP_CONNECTOR,
	IOHT_TCP_CARRIER,
	IOHT_UDP_CARRIER,
	IOHT_UNIX_ACCEPTOR,
	IOHT_UNIX_CONNECTOR,
	IOHT_UNIX_CARRIER,
  IOHT_PIPE_CARRIER,
  IOHT_THREAD_CARRIER,
} IOHandlerType;

typedef enum _IOHandlerFlag {
  IOHT_FLAG_CLOSE_NONE = 0,
  IOHT_FLAG_CLOSE_FD = 1,
  IOHT_FLAG_CLOSE_PROCESS = 2,
  IOHT_FLAG_CLOSE_ALL = 3,
} IOHandlerFlag;

class IOHandler {

public:
	IOHandler(int32_t fd, IOHandlerType type, IOHandlerManager *pIOHandlerManager, BaseProcess *process);
	virtual ~IOHandler();

	int32_t GetFd();  
	IOHandlerType GetType();
  void SetFlag(IOHandlerFlag flag);
  BaseProcess *GetProcess();
  void SetProcess(BaseProcess *process);
  void SetManager(IOHandlerManager *pIOHandlerManager);
  IOHandlerManager *GetManager();
  void CloseFd();
	virtual bool OnEvent(struct epoll_event &event) = 0;
	static std::string IOHTToString(IOHandlerType type);
  
  
protected:
	int32_t fd_;  
  IOHandlerManager *pIOHandlerManager_;
  int32_t flag_;
  BaseProcess *process_;
private:
	IOHandlerType type_;

  
  
};

inline int32_t IOHandler::GetFd() {
	return fd_;
}

inline IOHandlerType IOHandler::GetType() {
	return type_;
}

inline void IOHandler::SetFlag(IOHandlerFlag flag) {
  flag_ = flag;
}

inline BaseProcess *IOHandler::GetProcess() {
  return process_;
}

inline void IOHandler::SetProcess(BaseProcess *process) {
  process_ = process;
}

inline void IOHandler::SetManager(IOHandlerManager *pIOHandlerManager) {
  pIOHandlerManager_ = pIOHandlerManager;
}

inline IOHandlerManager *IOHandler::GetManager() {
  return pIOHandlerManager_;
}

inline void IOHandler::CloseFd() {
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}


}

#endif	/* _IOHANDLER_H */



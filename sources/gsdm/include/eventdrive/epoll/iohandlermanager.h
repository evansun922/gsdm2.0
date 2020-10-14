/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _IOHANDLERMANAGER_H
#define	_IOHANDLERMANAGER_H

#include "platform/linuxplatform.h"

#define EPOLL_QUERY_SIZE 1024

namespace gsdm {

class IOHandler;
class NetWorker;

class IOHandlerManager {
public:
  IOHandlerManager(NetWorker *worker);
  virtual ~IOHandlerManager();

  NetWorker *GetManager(); 
  void Initialize();
  void SignalShutdown();
  void Shutdown();
  bool EnableReadData(IOHandler *pIOHandler);
  bool DisableReadData(IOHandler *pIOHandler, bool ignoreError = false);
  bool EnableWriteData(IOHandler *pIOHandler);
  bool DisableWriteData(IOHandler *pIOHandler, bool ignoreError = false);
  bool EnableAcceptConnections(IOHandler *pIOHandler);
  bool DisableAcceptConnections(IOHandler *pIOHandler, bool ignoreError = false);
  void EnqueueForDelete(IOHandler *pIOHandler);
  void Post(const uint8_t *v, int len);
  int GetFd();
  sockaddr_un *GetAddress();
  socklen_t GetLen();

  gsdm_msec_t Pulse(gsdm_msec_t millisecond);

private:
  int eq_;
  int unix_sock_post_;
  int unix_sock_wait_;
  char unix_buf_wait_[16];
  sockaddr_un address_wait_;
  socklen_t address_len_;
  epoll_event query_[EPOLL_QUERY_SIZE];
  struct epoll_event dummy_;
  NetWorker *worker_;
};

inline void IOHandlerManager::Post(const uint8_t *v, int len) {
  sendto(unix_sock_post_, v, len, 0, (const struct sockaddr *)&address_wait_, address_len_);
}

inline int IOHandlerManager::GetFd() {
  return unix_sock_wait_;
}

inline sockaddr_un *IOHandlerManager::GetAddress() {
  return &address_wait_;
}

inline socklen_t IOHandlerManager::GetLen() {
  return address_len_;
}

}

#endif	/* _IOHANDLERMANAGER_H */




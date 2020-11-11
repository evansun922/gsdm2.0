/*
 * Copyright (c) 2016. sswin0922@163.com (evan sun)
 */

#ifndef _POOLWORKER_H_
#define _POOLWORKER_H_

#include "platform/linuxplatform.h"
#include "3rdparty/include/st.h"

namespace gsdm { 

class PoolWorkerEx;
class PoolProcess;
class PoolAccept;

struct PoolSocket {
  st_netfd_t st_fd;
  int sys_fd;
  int sys_eq;

  PoolSocket();
  ~PoolSocket();

  bool (*pool_connect)(PoolSocket *, const struct sockaddr *, socklen_t, gsdm_msec_t);
  ssize_t (*pool_recv)(PoolSocket *, void *, uint32_t, gsdm_msec_t);
  ssize_t (*pool_send)(PoolSocket *, const void *, uint32_t, gsdm_msec_t);
  ssize_t (*pool_recvfrom)(PoolSocket *, void *, uint32_t, struct sockaddr *, socklen_t *, gsdm_msec_t);
  ssize_t (*pool_sendto)(PoolSocket *, const void *, uint32_t, const struct sockaddr *, socklen_t, gsdm_msec_t);
};

class PoolWorker {
public:
  PoolWorker(PoolWorkerEx *ex);
  virtual ~PoolWorker();
  virtual void Run() = 0;
  virtual pid_t StartWithProcess(const std::string &title, PoolProcess *process) = 0;
  virtual bool StartWithThread(PoolProcess *process) = 0;

  void CloseAllAccept();

  PoolWorkerEx *ex_;
  pid_t pool_pid_;
  sockaddr_un address_wait_;
  socklen_t address_len_;
  
  typedef std::unordered_map<int, PoolAccept *> PoolAcceptHash;
  PoolAcceptHash accept_hash_;
};


}


#endif // _POOLWORKER_H_

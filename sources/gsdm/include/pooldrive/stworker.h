/*
 * Copyright (c) 2016. sswin0922@163.com (evan sun)
 */

#ifndef _STPOOLWORKER_H_
#define _STPOOLWORKER_H_

#include "pooldrive/poolworker.h"

namespace gsdm { 


class StPoolWorker : public PoolWorker {
public:
  StPoolWorker(PoolWorkerEx *ex);
  virtual ~StPoolWorker();
  virtual void Run();
  virtual pid_t StartWithProcess(const std::string &title, PoolProcess *process);
  virtual bool StartWithThread(PoolProcess *process);

  void SendFd(int send_fd, char *ex_data, sockaddr_un *address, socklen_t address_len);
  void ProcessFd(st_netfd_t fd, char *ex_data);

private:

  static void *MainRun(void *arg);
  static void *DoRun(void *arg);
  static void *STRun(void *arg);

  st_netfd_t unix_sock_wait_;
  st_mutex_t lock_;
  struct pollfd *query_;
  
  struct PoolHandle {
    int handle_id;
    StPoolWorker *worker;    
    st_thread_t thread;
    st_cond_t signal;
    PoolProcess *process;

    PoolHandle(int id, StPoolWorker *w) {
      handle_id = id;
      worker = w;
      thread = NULL; 
      signal = NULL;      
      process = NULL; }
  };

  typedef std::unordered_map<int, PoolHandle *> PoolHandleHash;
  PoolHandleHash total_handles_;
  PoolHandleHash idle_handles_;
  PoolHandleHash use_handles_;
};


}


#endif // _POOLWORKER_H_

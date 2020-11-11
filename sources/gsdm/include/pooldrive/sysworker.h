/*
 * Copyright (c) 2016. sswin0922@163.com (evan sun)
 */

#ifndef _SYSPOOLWORKER_H_
#define _SYSPOOLWORKER_H_

#include "pooldrive/poolworker.h"

namespace gsdm { 


class SysPoolWorker : public PoolWorker {
public:
  SysPoolWorker(PoolWorkerEx *ex);
  virtual ~SysPoolWorker();
  virtual void Run();
  virtual pid_t StartWithProcess(const std::string &title, PoolProcess *process);
  virtual bool StartWithThread(PoolProcess *process);

  void SendFd(int send_fd, char *ex_data, sockaddr_un *address, socklen_t address_len);
  void ProcessFd(int fd, char *ex_data);

private:

  static void *MainRun(void *arg);
  static void *DoRun(void *arg);
  static void *STRun(void *arg);


  int eq_;
  int unix_sock_wait_;
  pthread_mutex_t *lock_;
  
  struct PoolHandle {
    int handle_id;
    SysPoolWorker *worker;
    pthread_t thread;
    sem_t signal;
    PoolProcess *process;

    PoolHandle(int id, SysPoolWorker *w) {
      handle_id = id; 
      worker = w; 
      thread = -1; 
      process = NULL; }
  };

  typedef std::unordered_map<int, PoolHandle *> PoolHandleHash;
  PoolHandleHash total_handles_;
  PoolHandleHash idle_handles_;
  PoolHandleHash use_handles_;

  pthread_mutex_t *child_lock_;
  typedef std::unordered_set<pid_t> ChildPidHash;
  ChildPidHash child_pid_hash_;
};


}


#endif // _POOLWORKER_H_

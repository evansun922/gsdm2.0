#ifndef _SYSPOOLACCEPT_H_
#define _SYSPOOLACCEPT_H_

#include "pooldrive/poolaccept.h"

namespace gsdm {

class SysPoolWorker;

class SysPoolAccept : public PoolAccept {
public:
  SysPoolAccept(int fd, SysPoolWorker *my_worker, const std::vector<PoolWorker *> workers);
  virtual ~SysPoolAccept();
  virtual void Process();
  virtual int GetFd();

  int fd_;
  SysPoolWorker *my_worker_;
  std::vector<PoolWorker *> workers_;
  uint32_t pos_;
};





}











#endif // _POOLACCEPT_H_

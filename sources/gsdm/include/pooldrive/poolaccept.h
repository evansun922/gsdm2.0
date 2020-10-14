#ifndef _POOLACCEPT_H_
#define _POOLACCEPT_H_

#include "platform/linuxplatform.h"

#define ACCEPT_EX_DATA_LENGTH 300

namespace gsdm {

class PoolWorker;

class PoolAccept {
public:
  PoolAccept();
  virtual ~PoolAccept();
  virtual void Process() = 0;
  virtual int GetFd() = 0;
  
  char ex_data_[ACCEPT_EX_DATA_LENGTH];
};





}











#endif // _POOLACCEPT_H_

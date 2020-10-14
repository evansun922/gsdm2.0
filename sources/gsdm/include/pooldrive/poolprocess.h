/*
 * Copyright (c) 2016. sswin0922@163.com (evan sun)
 */

#ifndef _POOLPROCESS_H_
#define _POOLPROCESS_H_

#include "platform/linuxplatform.h"

namespace gsdm {


class StPoolWorker;
class SysPoolWorker;
class PoolWorkerEx;
struct PoolSocket;
typedef struct PoolSocket* poolsocket_t;


class PoolProcess {
public:
friend class PoolWorkerEx;
friend class StPoolWorker;
friend class SysPoolWorker;

  PoolProcess();
  virtual ~PoolProcess();
  virtual void Process() = 0;

  bool IsClose();

protected:
  poolsocket_t pool_fd_;
  PoolWorkerEx *ex_;


};


}

#endif // _POOLPROCESS_H_

#ifndef _STPOOLACCEPT_H_
#define _STPOOLACCEPT_H_

#include "pooldrive/poolaccept.h"
#include "3rdparty/include/st.h"

namespace gsdm {

class StPoolWorker;

class StPoolAccept : public PoolAccept {
public:
  StPoolAccept(st_netfd_t fd, StPoolWorker *my_worker, const std::vector<PoolWorker *> workers);
  virtual ~StPoolAccept();
  virtual void Process();
  virtual int GetFd();

  st_netfd_t fd_;
  StPoolWorker *my_worker_;
  std::vector<PoolWorker *> workers_;
  uint32_t pos_;
};





}











#endif // _POOLACCEPT_H_

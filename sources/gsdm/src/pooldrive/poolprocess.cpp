#include "pooldrive/poolprocess.h"
#include "pooldrive/poolworkerex.h"

namespace gsdm {

PoolProcess::PoolProcess()
  : pool_fd_(NULL),ex_(NULL) {

}

PoolProcess::~PoolProcess() {
  if ( pool_fd_ ) {
    ex_->CloseSocket(pool_fd_);
    pool_fd_ = NULL;
  }
}

bool PoolProcess::IsClose() {
  return *ex_->is_close_;
}




}

#include "pooldrive/poolworker.h"
#include "pooldrive/poolworkerex.h"
#include "pooldrive/poolprocess.h"
#include "pooldrive/poolaccept.h"
#include "logging/logger.h"


namespace gsdm {


PoolSocket::PoolSocket() :st_fd(NULL),sys_fd(-1),sys_eq(-1)  {

}

PoolSocket::~PoolSocket() {
  if ( st_fd ) {
    st_netfd_close(st_fd);
    st_fd = NULL;
  }

  if ( -1 != sys_fd ) {
    close(sys_fd);
    sys_fd = -1;
  }

  if ( -1 != sys_eq ) {
    close(sys_eq);
    sys_eq = -1;
  }
}

PoolWorker::PoolWorker(PoolWorkerEx *ex) 
  : ex_(ex),pool_pid_(-1),address_len_(0) {
  memset(&address_wait_, 0, sizeof(sockaddr_un));
}

PoolWorker::~PoolWorker() {
  CloseAllAccept();
  if ( ex_ ) {
    delete ex_;
    ex_ = NULL;
  }
}

void PoolWorker::CloseAllAccept() {
  FOR_UNORDERED_MAP(accept_hash_,int,PoolAccept *,i) {
    delete MAP_VAL(i);
  }
  accept_hash_.clear();
}
  





}

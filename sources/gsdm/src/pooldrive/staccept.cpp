#include "pooldrive/staccept.h"
#include "pooldrive/stworker.h"
#include "logging/logger.h"

namespace gsdm {

StPoolAccept::StPoolAccept(st_netfd_t fd, StPoolWorker *my_worker, const std::vector<PoolWorker *> workers) 
  : PoolAccept(),fd_(fd),my_worker_(my_worker),workers_(workers),pos_(0) {

}

StPoolAccept::~StPoolAccept() {
  if ( fd_ ) {
    st_netfd_close(fd_);
    fd_ = NULL;
  }
} 

void StPoolAccept::Process() {
  struct sockaddr address;
  memset(&address, 0, sizeof(sockaddr));
  int len = sizeof(sockaddr);

  st_netfd_t fd = st_accept(fd_, &address, &len, ST_UTIME_NO_TIMEOUT);
  if ( NULL == fd ) {
    FATAL("Unable to accept client connection: %s (%d)", strerror(errno), errno);
    return;
  }

  PoolWorker *w = workers_[pos_++%workers_.size()];
  if ( w == my_worker_ ) {
    my_worker_->ProcessFd(fd, ex_data_);
  } else {
    my_worker_->SendFd(st_netfd_fileno(fd), ex_data_, &w->address_wait_, w->address_len_);
    st_netfd_close(fd);
  }
}

int StPoolAccept::GetFd() {
  return st_netfd_fileno(fd_);
}




};

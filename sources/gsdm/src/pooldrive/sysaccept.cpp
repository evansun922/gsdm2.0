#include "pooldrive/sysaccept.h"
#include "pooldrive/sysworker.h"
#include "logging/logger.h"

namespace gsdm {

SysPoolAccept::SysPoolAccept(int fd, SysPoolWorker *my_worker, const std::vector<PoolWorker *> workers) 
  :PoolAccept(),fd_(fd),my_worker_(my_worker),workers_(workers),pos_(0) {

}

SysPoolAccept::~SysPoolAccept() {
  if ( -1 != fd_ ) {
    close(fd_);
    fd_ = -1;
  }
} 

void SysPoolAccept::Process() {
  struct sockaddr address;
  memset(&address, 0, sizeof(sockaddr));
  socklen_t len = sizeof(sockaddr);

  int fd = accept(fd_, &address, &len);
  if ( fd < 0 ) {
    FATAL("Unable to accept client connection: %s (%d)", strerror(errno), errno);
    return;
  }

  PoolWorker *w = workers_[pos_++%workers_.size()];
  if ( w == my_worker_ ) {
    my_worker_->ProcessFd(fd, ex_data_);
  } else {
    my_worker_->SendFd(fd, ex_data_, &w->address_wait_, w->address_len_);
    close(fd);
  }
}

int SysPoolAccept::GetFd() {
  return fd_;
}




};

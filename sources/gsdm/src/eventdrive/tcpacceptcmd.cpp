#include "eventdrive/tcpacceptcmd.h"
#include "eventdrive/networker.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"

namespace gsdm {

TCPAcceptCmd::TCPAcceptCmd(int fd, uint16_t port)
  : BaseCmd(false),fd_(fd),port_(port) {
  
}

TCPAcceptCmd::~TCPAcceptCmd() {

}

bool TCPAcceptCmd::Process(BaseWorkerEx *ex) {
  BaseProcess *process = ex->CreateTCPProcess(port_);
  if ( process ) {
    process->ex_ = ex;
    new TCPCarrier(fd_, ex->worker_->GetIOHandlerManager(), process);
    return true;
  } else {
    WARN("BaseProcess is NULL");
    close(fd_);
  }
  return false;
}

}

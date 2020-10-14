#include "eventdrive/unixacceptcmd.h"
#include "eventdrive/networker.h"
#include "eventdrive/epoll/unixcarrier.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"

namespace gsdm {


UNIXAcceptCmd::UNIXAcceptCmd(int fd, const std::string &unix_path) 
  : BaseCmd(false),fd_(fd),unix_path_(unix_path) {
  
}

UNIXAcceptCmd::~UNIXAcceptCmd() {

}

bool UNIXAcceptCmd::Process(BaseWorkerEx *ex) {
  BaseProcess *process = ex->CreateUNIXProcess(unix_path_);
  if ( process ) {
    process->ex_ = ex;
    new UNIXCarrier(fd_, ex->worker_->GetIOHandlerManager(), process);
    return true;
  } else {
    WARN("BaseProcess is NULL");
    close(fd_);
  }
  return false;
}


}

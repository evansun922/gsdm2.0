#include "eventdrive/unixtransfercmd.h"
#include "eventdrive/networker.h"
#include "eventdrive/epoll/unixcarrier.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"

namespace gsdm {

UNIXTransferCmd::UNIXTransferCmd()
  : BaseCmd(false),fd_(-1),unix_path_(""),data_(NULL),data_len_(0) {
  
}

UNIXTransferCmd::~UNIXTransferCmd() {
  if ( data_ ) {
    delete [] data_;
    data_ = NULL;
  }
}

bool UNIXTransferCmd::Process(BaseWorkerEx *ex) {
  BaseProcess *process = ex->CreateUNIXProcess(unix_path_);
  if ( process ) {
    process->ex_ = ex;
    new UNIXCarrier(fd_, ex->worker_->GetIOHandlerManager(), process);
    process->input_buffer_.ReadFromBuffer(data_, data_len_);
    if ( false == process->Process((int)data_len_) ) {
      process->Free();
    }
  } else {
    WARN("BaseProcess is NULL");
    close(fd_);
  }

  if ( data_ ) {
    delete [] data_;
    data_ = NULL;
  }
  return true;
}

}

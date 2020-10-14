#include "eventdrive/tcptransfercmd.h"
#include "eventdrive/networker.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"

namespace gsdm {

TCPTransferCmd::TCPTransferCmd()
  : BaseCmd(false),fd_(-1),port_(0),data_(NULL),data_len_(0) {
  
}

TCPTransferCmd::~TCPTransferCmd() {
  if ( data_ ) {
    delete [] data_;
    data_ = NULL;
  }
}

bool TCPTransferCmd::Process(BaseWorkerEx *ex) {
  BaseProcess *process = ex->CreateTCPProcess(port_);
  if ( process ) {
    process->ex_ = ex;
    new TCPCarrier(fd_, ex->worker_->GetIOHandlerManager(), process);
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

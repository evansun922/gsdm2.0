/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/baseprocess.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "logging/logger.h"

namespace gsdm {

TCPCarrier::TCPCarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process)
  : IOHandler(fd, IOHT_TCP_CARRIER, pIOHandlerManager, process) {
  socklen_t len = sizeof (sockaddr);
  getpeername(fd, (sockaddr *)&peer_address_, &len);
  if ( process_ ) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_CONNECTED);
    process_->AttachNet();
  }
}

TCPCarrier::~TCPCarrier() {
  if ( process_ ) {
    process_->DetachNet();
    if ( !(flag_ & IOHT_FLAG_CLOSE_PROCESS) )
      process_->SetIOHandler(NULL);
  }
}

bool TCPCarrier::OnEvent(struct epoll_event &event) {
  if ( NULL == process_ || BaseProcess::PROCESS_NET_STATUS_CONNECTED != process_->process_net_status_ ) {
    WARN("status of process_ is invalid.");
    return false;
  }

  //1. Test error
  if ( 0 != (event.events & EPOLLERR) ) {
    if ( 0 != (event.events & EPOLLIN) ) {
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_PEER_RESET);
    } else {
      WARN("***Event handler ERR: %p, 0x%X, %s", this, event.events, STR(getHostIPv4(peer_address_.sin_addr.s_addr)));
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    }
    return false;
  }

  if ( 0 != (event.events & EPOLLHUP) ) {
    WARN("***Event handler HUP: %p, 0x%X, %s", this, event.events, STR(getHostIPv4(peer_address_.sin_addr.s_addr)));
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_PEER_RESET);
    return false;
  }

  //2. Read data
  if ( 0 != (event.events & EPOLLIN) ) {
    IOBuffer *input_buffer = process_->GetInputBuffer();
    int32_t read_amount = 0;
    if ( !input_buffer->ReadFromTCPFd(fd_, process_->recv_buffer_size_, read_amount) ) {
      if ( 0 == read_amount ) {
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_PEER_RESET);
        return false;
      }

      if ( EAGAIN != errno ) {
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
        return false;
      }
    } else if ( !process_->Process(read_amount) ) {
      // FATAL("Unable to signal data available");
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
      return false;
    }
  }

  //3. Write data
  if ( 0 != (event.events & EPOLLOUT) ) {
    int32_t write_amount = 0;
    IOBuffer *output_buffer = NULL;

    if ( NULL != (output_buffer = process_->GetOutputBuffer()) ) {
      if ( !output_buffer->WriteToTCPFd(fd_, 8388608, write_amount) ) {
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
        return false;
      }

      if ( 0 == GETAVAILABLEBYTESCOUNT(*output_buffer) ) {
        pIOHandlerManager_->DisableWriteData(this);
        process_->is_write_ = false;
      }
    } else {
      pIOHandlerManager_->DisableWriteData(this);
      process_->is_write_ = false;
    }
  }

  if ( process_->IsCloseNet() ) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
    return false;  
  }

  return true;
}


}





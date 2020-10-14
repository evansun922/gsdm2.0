/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/baseprocess.h"
#include "eventdrive/epoll/pipecarrier.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "logging/logger.h"

namespace gsdm {

PIPECarrier::PIPECarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process, const std::string &name)
  : IOHandler(fd, IOHT_PIPE_CARRIER, pIOHandlerManager, process),name_(name) {
  if ( process_ ) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_CONNECTED);
    process_->AttachNet();
  }
}

PIPECarrier::~PIPECarrier() {
  process_->DetachNet();
  if ( process_ && !(flag_ & IOHT_FLAG_CLOSE_PROCESS) ) {
    process_->SetIOHandler(NULL);
  }
}

bool PIPECarrier::OnEvent(struct epoll_event &event) {
	int32_t readAmount = 0;

  if ( NULL == process_ || BaseProcess::PROCESS_NET_STATUS_CONNECTED != process_->process_net_status_ ) {
    WARN("status of process_ is invalid.");
    return false;
  }

	//1. Read data
	if ((event.events & EPOLLIN) != 0) {
		IOBuffer *pInputBuffer = process_->GetInputBuffer();
		// assert(pInputBuffer != NULL);
		if (!pInputBuffer->ReadFromFd(fd_, process_->recv_buffer_size_, readAmount)) {
      // if (readAmount == 0) {
      //   process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_PEER_RESET);
      // }  else {
      //   process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
      // }
			// return false;
      if (readAmount != 0) {
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
        return false;
      }
		}

		if (!process_->Process(readAmount)) {
      // FATAL("Unable to signal data available");
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
			return false;
		}

	}

	//2. Write data
	if ((event.events & EPOLLOUT) != 0) {
		IOBuffer *pOutputBuffer = NULL;

    if (!process_->Process(readAmount)) {
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
      return false;
    }

		if ((pOutputBuffer = process_->GetOutputBuffer()) != NULL) {
			if (!pOutputBuffer->WriteToFd(fd_, 8388608)) {
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
				return false;
			}

			// if (GETAVAILABLEBYTESCOUNT(*pOutputBuffer) == 0) {
      //   pIOHandlerManager_->DisableWriteData(this);
      //   process_->is_write_ = false;
			// }
		} else {
      //  pIOHandlerManager_->DisableWriteData(this);
		}
	}

  //3. Test error
  if ((event.events & EPOLLERR) != 0) {
    WARN("***Event handler ERR: %p, 0x%X", this, event.events);
    process_->Process(0);
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    return false;
  }

  if ((event.events & EPOLLHUP) != 0) {
    // WARN("***Event handler HUP: %p, 0x%X", this, event.events);
    process_->Process(0);
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
  }

  if (process_->IsCloseNet()) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
    return false;  
  }

	return true;
}




}



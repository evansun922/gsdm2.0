/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/epoll/iohandler.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/baseprocess.h"

namespace gsdm {

IOHandler::IOHandler(int32_t fd, IOHandlerType type, IOHandlerManager *pIOHandlerManager, BaseProcess *process) 
  :fd_(fd),pIOHandlerManager_(pIOHandlerManager),flag_(IOHT_FLAG_CLOSE_ALL),process_(process),type_(type) {
  if ( pIOHandlerManager_ ) 
    pIOHandlerManager_->EnableReadData(this);
  if ( process_ ) {
    process_->SetIOHandler(this);
    if ( !process_->with_me_ )
      flag_ = IOHT_FLAG_CLOSE_FD;
    // if ( IOHT_TCP_CONNECTOR != type_ )
    //   process_->AttachNet();
  }
}

IOHandler::~IOHandler() {
  if ( process_ && (flag_ & IOHT_FLAG_CLOSE_PROCESS) ) {
    delete process_;
    process_ = NULL;
  }
 
  if ( flag_ & IOHT_FLAG_CLOSE_FD ) {
    CloseFd();
  }
}

std::string IOHandler::IOHTToString(IOHandlerType type) {
	switch (type) {
    case IOHT_TCP_ACCEPTOR:
			return "IOHT_ACCEPTOR";
		case IOHT_TCP_CARRIER:
			return "IOHT_TCP_CARRIER";
		case IOHT_UDP_CARRIER:
			return "IOHT_UDP_CARRIER";
		case IOHT_TCP_CONNECTOR:
			return "IOHT_TCP_CONNECTOR";
		default:
			return format("#unknown: %hhu#", type);
	}
}


}


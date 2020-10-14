/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/networker.h"
#include "eventdrive/epoll/iohandler.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "logging/logger.h"

namespace gsdm {

extern __thread int g_log_fd;

IOHandlerManager::IOHandlerManager(NetWorker *worker) {
  eq_ = epoll_create(EPOLL_QUERY_SIZE);
  unix_sock_post_ = -1;
  unix_sock_wait_ = -1;
  memset(&address_wait_, 0, sizeof(sockaddr_un));
  address_len_ = 0;
  worker_ = worker;
}

IOHandlerManager::~IOHandlerManager() {
  if ( -1 != unix_sock_wait_ ) {
    close(unix_sock_wait_);
    unix_sock_wait_ = -1;
  }

  if ( -1 != unix_sock_post_ ) {
    close(unix_sock_post_);
    unix_sock_post_ = -1;
  }

  remove(address_wait_.sun_path);
  Shutdown();
}

NetWorker *IOHandlerManager::GetManager() {
  return worker_;
}

void IOHandlerManager::Initialize() {
	memset(&dummy_, 0, sizeof (dummy_));

	// eq_ = epoll_create(EPOLL_QUERY_SIZE);
  // if ( -1 == eq_ ) {
	// 	FATAL("Unable to epoll_create: (%d) %s", errno, strerror(errno));
	// 	exit(0);
  // }

  unix_sock_wait_ = socket(PF_UNIX, SOCK_DGRAM, 0);
  if ( -1 == unix_sock_wait_ ) {
    FATAL("Unable to crate socket: (%d) %s", errno, strerror(errno));
    exit(0);
  }

  std::string unix_path = format("%s/gsdm_eventdrive_%lu", GSDM_BACK_ROOT_DIR, getTid());
  remove(STR(unix_path));
  address_wait_.sun_family = PF_UNIX;
  strncpy(address_wait_.sun_path, STR(unix_path), sizeof(address_wait_.sun_path) - 1);

  address_len_ = sizeof(address_wait_.sun_family) + strlen(address_wait_.sun_path);
  if ( -1 == bind(unix_sock_wait_, (sockaddr *)&address_wait_, address_len_) ) {
		FATAL("Unable to bind: (%d) %s", errno, strerror(errno));
		exit(0);
  }

  if (!setFdOptions(unix_sock_wait_, false)) {
    FATAL("Unable to set socket options");
    exit(0);
  }

	struct epoll_event evt = {0, {0}};
	evt.events = EPOLLIN;
	// evt.data.fd = unix_sock_wait_;
  evt.data.ptr = NULL;
	if (epoll_ctl(eq_, EPOLL_CTL_ADD, unix_sock_wait_, &evt) != 0) {
		FATAL("Unable to epoll_add: (%d) %s", errno, strerror(errno));
		exit(0);
	}

  g_log_fd = unix_sock_wait_;
  unix_sock_post_ = socket(PF_UNIX, SOCK_DGRAM, 0);
  if ( !setFdNonBlock(unix_sock_post_) ) {
    FATAL("Unable to setFdNonBlock: (%d) %s", errno, strerror(errno));
    exit(0);
  }
}

void IOHandlerManager::Shutdown() {
  if (eq_ >= 0) {
    close(eq_);
    eq_ = -1;
  }
}

bool IOHandlerManager::EnableReadData(IOHandler *pIOHandler) {
	struct epoll_event evt = {0, {0}};
	evt.events = EPOLLIN;
	evt.data.ptr = pIOHandler;
	if (epoll_ctl(eq_, EPOLL_CTL_ADD, pIOHandler->GetFd(), &evt) != 0) {
		FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
		return false;
	}

	return true;
}

bool IOHandlerManager::DisableReadData(IOHandler *pIOHandler, bool ignoreError) {
	struct epoll_event evt = {0, {0}};
	evt.events = EPOLLIN;
	evt.data.ptr = pIOHandler;
	if (epoll_ctl(eq_, EPOLL_CTL_DEL, pIOHandler->GetFd(), &evt) != 0) {
		if (!ignoreError) {
			FATAL("Unable to disable read data: (%d) %s", errno, strerror(errno));
			return false;
		}
	}

	return true;
}

bool IOHandlerManager::EnableWriteData(IOHandler *pIOHandler) {
	struct epoll_event evt = {0, {0}};
	evt.events = EPOLLIN | EPOLLOUT;
	evt.data.ptr = pIOHandler;

	if (epoll_ctl(eq_, EPOLL_CTL_MOD, pIOHandler->GetFd(), &evt) != 0) {
		FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
		return false;
	}

	return true;
}

bool IOHandlerManager::DisableWriteData(IOHandler *pIOHandler, bool ignoreError) {
	struct epoll_event evt = {0, {0}};
	evt.events = EPOLLIN;
	evt.data.ptr = pIOHandler;
	if (epoll_ctl(eq_, EPOLL_CTL_MOD, pIOHandler->GetFd(), &evt) != 0) {
		if (!ignoreError) {
			FATAL("Unable to disable write data: (%d) %s", errno, strerror(errno));
			return false;
		}
	}

	return true;
}

bool IOHandlerManager::EnableAcceptConnections(IOHandler *pIOHandler) {
	struct epoll_event evt = {0,
		{0}};
	evt.events = EPOLLIN;
	evt.data.ptr = pIOHandler;
	if (epoll_ctl(eq_, EPOLL_CTL_ADD, pIOHandler->GetFd(), &evt) != 0) {
		FATAL("Unable to enable accept connections: (%d) %s", errno, strerror(errno));
		return false;
	}
	return true;
}

bool IOHandlerManager::DisableAcceptConnections(IOHandler *pIOHandler, bool ignoreError) {
	struct epoll_event evt = {0,
		{0}};
	evt.events = EPOLLIN;
	evt.data.ptr = pIOHandler;
	if (epoll_ctl(eq_, EPOLL_CTL_DEL, pIOHandler->GetFd(), &evt) != 0) {
		if (!ignoreError) {
			FATAL("Unable to disable accept connections: (%d) %s", errno, strerror(errno));
			return false;
		}
	}
	return true;
}

void IOHandlerManager::EnqueueForDelete(IOHandler *pIOHandler) {
	DisableReadData(pIOHandler, true);
  delete pIOHandler;
}

gsdm_msec_t IOHandlerManager::Pulse(gsdm_msec_t millisecond) {
	int32_t events_count = 0;
	if ((events_count = epoll_wait(eq_, query_, EPOLL_QUERY_SIZE, millisecond)) < 0) {
    if (EINTR == errno) {
      millisecond = worker_->timer_manager_->TimeElapsed() + getCurrentGsdmMsec();
      return millisecond;
    }
		FATAL("Unable to execute epoll_wait: (%d) %s", errno, strerror(errno));
		return 0;
	}

  std::vector<GsdmCmdInfo> info_vec;
	for (int32_t i = 0; i < events_count; i++) {
    if ( NULL == query_[i].data.ptr ) {
      while ( sizeof(GsdmCmdInfo) == recvfrom(unix_sock_wait_, unix_buf_wait_, sizeof(unix_buf_wait_), 0, NULL, NULL) ) {
        info_vec.push_back(*((GsdmCmdInfo *)unix_buf_wait_));
      }
    } else {
      IOHandler *pHandler =	(IOHandler *) query_[i].data.ptr;
      if (!pHandler->OnEvent(query_[i])) {
        EnqueueForDelete(pHandler);
      }
    }
	}

  for ( int i = 0; i < (int)info_vec.size(); i++) {
    worker_->ProcessCmd(&info_vec[i]);
  }

  millisecond = worker_->timer_manager_->TimeElapsed();
	return millisecond > 1000 ? 1000 : millisecond;
}


}


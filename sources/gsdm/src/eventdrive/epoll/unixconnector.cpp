#include "eventdrive/baseprocess.h"
#include "eventdrive/epoll/unixcarrier.h"
#include "eventdrive/epoll/unixconnector.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "logging/logger.h"

namespace gsdm {

UNIXConnector::UNIXConnector(int32_t fd, const std::string &unix_path, IOHandlerManager *pIOHandlerManager, BaseProcess *process)
  : IOHandler(fd, IOHT_UNIX_CONNECTOR, pIOHandlerManager, process),unix_path_(unix_path) {
  if ( process_ )
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_CONNECTING);
}

UNIXConnector::~UNIXConnector() {	

} 

bool UNIXConnector::OnEvent(epoll_event &event) {
  if ((event.events & EPOLLERR) != 0) {
    DEBUG("***CONNECT ERROR: Unable to connect to: %s", STR(unix_path_));
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    process_->SetIOHandler(NULL);
    return false;
  }

  pIOHandlerManager_->DisableWriteData(this, true);
  pIOHandlerManager_->DisableReadData(this, true);

  if ( BaseProcess::PROCESS_NET_STATUS_CONNECTING == process_->process_net_status_ ) {
    new UNIXCarrier(fd_, pIOHandlerManager_, process_);
    SetFlag(IOHT_FLAG_CLOSE_NONE);
  } else {
    process_->SetIOHandler(NULL);
  }

  delete this;
  return true;
}

bool UNIXConnector::Connect(const std::string &unix_path, IOHandlerManager *pIOHandlerManager, BaseProcess *process) {

  int32_t fd = (int32_t) socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    int err = errno;
    FATAL("Unable to create fd: %s(%d)", strerror(err), err);
    return false;
  }

  if (!setFdOptions(fd, false)) {
    CLOSE_SOCKET(fd);
    FATAL("Unable to set socket options");
    return false;
  }

  UNIXConnector *pUNIXConnector = new UNIXConnector(fd, unix_path, pIOHandlerManager, process);
  //  pUNIXConnector->SetFlag(flag);
  if ( !pUNIXConnector->Connect() ) {
    process->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    pIOHandlerManager->DisableWriteData(pUNIXConnector, true);
    pIOHandlerManager->EnqueueForDelete(pUNIXConnector);
    FATAL("Unable to connect");
    return false;
  }

  return true;
}

bool UNIXConnector::Connect() {
  sockaddr_un address;
  address.sun_family = PF_UNIX;
  strncpy(address.sun_path, STR(unix_path_), sizeof(address.sun_path) - 1);

  if (!pIOHandlerManager_->EnableWriteData(this)) {
    FATAL("Unable to enable reading data");
    return false;
  }
  
  if (connect(fd_, (sockaddr *) & address, sizeof(address.sun_family)+strlen(address.sun_path)) != 0) {
    int err = errno;
    if (err != EINPROGRESS) {
      FATAL("Unable to connect to %s (%d) (%s)", STR(unix_path_),
						err, strerror(err));
      return false;
    }
  }

  return true;
}


}

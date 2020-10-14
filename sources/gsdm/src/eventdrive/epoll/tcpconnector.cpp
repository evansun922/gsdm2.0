
#include "eventdrive/baseprocess.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/epoll/tcpconnector.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "logging/logger.h"

namespace gsdm {

TCPConnector::TCPConnector(int32_t fd, const std::string &ip, uint16_t port, IOHandlerManager *pIOHandlerManager, BaseProcess *process)
  : IOHandler(fd, IOHT_TCP_CONNECTOR, pIOHandlerManager, process),ip_(ip),port_(port) {
  if ( process_ )
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_CONNECTING);
}

TCPConnector::~TCPConnector() {	

}

bool TCPConnector::OnEvent(epoll_event &event) {
  if ((event.events & EPOLLERR) != 0) {
    DEBUG("***CONNECT ERROR: Unable to connect to: %s:%hu", STR(ip_), port_);
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    process_->SetIOHandler(NULL);
    return false;
  }

  pIOHandlerManager_->DisableWriteData(this, true);
  pIOHandlerManager_->DisableReadData(this, true);

  if ( BaseProcess::PROCESS_NET_STATUS_CONNECTING == process_->process_net_status_ ) {
    new TCPCarrier(fd_, pIOHandlerManager_, process_);
    SetFlag(IOHT_FLAG_CLOSE_NONE);
  } else {
    process_->SetIOHandler(NULL);
  }

  delete this;
  return true;
}

bool TCPConnector::Connect(const std::string &ip, uint16_t port, IOHandlerManager *pIOHandlerManager, BaseProcess *process) {

  int32_t fd = (int32_t) socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    int err = errno;
    FATAL("Unable to create fd: %s(%d)", strerror(err), err);
    return false;
  }

  if (!setFdOptions(fd, true)) {
    CLOSE_SOCKET(fd);
    FATAL("Unable to set socket options");
    return false;
  }

  TCPConnector *pTCPConnector = new TCPConnector(fd, ip, port, pIOHandlerManager, process);
  //  pTCPConnector->SetFlag(flag);
  if ( !pTCPConnector->Connect() ) {
    process->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    pIOHandlerManager->DisableWriteData(pTCPConnector, true);
    pIOHandlerManager->EnqueueForDelete(pTCPConnector);
    FATAL("Unable to connect");
    return false;
  }

  return true;
}

bool TCPConnector::Connect() {
  sockaddr_in address;

  address.sin_family = PF_INET;
  address.sin_addr.s_addr = inet_addr(ip_.c_str());
  if (address.sin_addr.s_addr == INADDR_NONE) {
    FATAL("Unable to translate string %s to a valid IP address", STR(ip_));
    return false;
  }
  address.sin_port = htons(port_); //----MARKED-SHORT----

  if (!pIOHandlerManager_->EnableWriteData(this)) {
    FATAL("Unable to enable reading data");
    return false;
  }

  if (connect(fd_, (sockaddr *) & address, sizeof (address)) != 0) {
    if (errno != EINPROGRESS) {
      FATAL("Unable to connect to %s:%hu (%d) (%s)", STR(ip_), port_, errno, strerror(errno));
      return false;
    }
  }

  return true;
}


}

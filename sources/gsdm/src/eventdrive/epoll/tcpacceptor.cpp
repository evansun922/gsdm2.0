/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/networker.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/epoll/tcpacceptor.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/tcpacceptcmd.h"
#include "logging/logger.h"

namespace gsdm {

TCPAcceptor::TCPAcceptor(NetWorker *worker, std::vector<BaseWorkerEx*> &other, const std::string &ip_address, uint16_t port)
  : IOHandler(-1, IOHT_TCP_ACCEPTOR, NULL, NULL) {
	memset(&address_, 0, sizeof (sockaddr_in));
	address_.sin_family = PF_INET;
	address_.sin_addr.s_addr = inet_addr(ip_address.c_str());
	assert(address_.sin_addr.s_addr != INADDR_NONE);
	address_.sin_port = htons(port); //----MARKED-SHORT----


	accepted_count_ = 0;
	ip_address_ = ip_address;
	port_ = port;
  worker_ = worker;
  other_ = other;
  other_count_ = other_.size();
}

TCPAcceptor::~TCPAcceptor() {

}

bool TCPAcceptor::Bind() {
	fd_ = (int) socket(PF_INET, SOCK_STREAM, 0);
	if (fd_ < 0) {
		FATAL("Unable to create socket: %s(%d)", strerror(errno), errno);
		return false;
	}

	if (!setFdOptions(fd_, true)) {
		FATAL("Unable to set socket options");
		return false;
	}

	if (bind(fd_, (sockaddr *)&address_, sizeof (sockaddr)) != 0) {
		FATAL("Unable to bind on address: tcp://%s:%hu; Error was: %s (%d)",
				inet_ntoa(((sockaddr_in *) & address_)->sin_addr),
				ntohs(((sockaddr_in *) & address_)->sin_port),
				strerror(errno),
				errno);
		return false;
	}

	if (port_ == 0) {
		socklen_t tempSize = sizeof (sockaddr);
		if (getsockname(fd_, (sockaddr *) & address_, &tempSize) != 0) {
			FATAL("Unable to extract the random port");
			return false;
		}	
	}

	if (listen(fd_, 10240) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

	return true;
}

bool TCPAcceptor::StartAccept() {
	return pIOHandlerManager_->EnableAcceptConnections(this);
}

bool TCPAcceptor::OnEvent(struct epoll_event &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool TCPAcceptor::OnConnectionAvailable(struct epoll_event &event) {
  return Accept();
}

bool TCPAcceptor::Accept() {
	sockaddr address;
	memset(&address, 0, sizeof (sockaddr));
	socklen_t len = sizeof (sockaddr);
	int32_t fd;
	int32_t error;

	//1. Accept the connection
	fd = accept(fd_, &address, &len);
	error = errno;
	if (fd < 0) {
		FATAL("Unable to accept client connection: %s (%d)", strerror(error), error);
		return false;
	}

	if (!setFdOptions(fd, true)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(fd);
		return false;
	}

  if ( NetWorker::WORKER_STATUS_RUN == worker_->status_ ) {
    BaseWorkerEx *ex = other_[accepted_count_++%other_count_];
    BaseCmd *cmd = ex->GetCmd(worker_->GetEx(), TCP_ACCEPTCMD_TYPE);
    if ( !cmd ) {
      cmd = new TCPAcceptCmd(fd, port_);
    } else {
      ((TCPAcceptCmd*)cmd)->fd_ = fd;
      ((TCPAcceptCmd*)cmd)->port_ = port_;
      // ((AcceptCmd*)cmd)->worker_ = ex->worker_;
    }
    ex->SendCmd(worker_->GetEx(), TCP_ACCEPTCMD_TYPE, cmd);
  } else {
    close(fd);
  }

	//7. Done
	return true;
}

bool TCPAcceptor::IsAlive() {
	//TODO: Implement this. It must return true
	//if this acceptor is operational
	NYI;
	return true;
}

}




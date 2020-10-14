/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#include "eventdrive/networker.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/epoll/unixacceptor.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/epoll/unixcarrier.h"
#include "eventdrive/unixacceptcmd.h"
#include "logging/logger.h"

using namespace gsdm;

UNIXAcceptor::UNIXAcceptor(NetWorker *worker, std::vector<BaseWorkerEx*> &other, const std::string &unix_path)
  : IOHandler(-1, IOHT_UNIX_ACCEPTOR, NULL, NULL) {
	memset(&address_, 0, sizeof (sockaddr_un));
	address_.sun_family = PF_UNIX;
	strncpy(address_.sun_path, STR(unix_path), sizeof(address_.sun_path) - 1);

	accepted_count_ = 0;
  unix_path_ = unix_path;
  worker_ = worker;
  other_ = other;
  other_count_ = other_.size();
}

UNIXAcceptor::~UNIXAcceptor() {

}

bool UNIXAcceptor::Bind() {
	fd_ = (int) socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd_ < 0) {
		FATAL("Unable to create socket: %s(%d)", strerror(errno), errno);
		return false;
	}

	if (!setFdOptions(fd_, false)) {
		FATAL("Unable to set socket options");
		return false;
	}

  deleteFile(unix_path_);

  size_t socket_len = sizeof(address_.sun_family) + strlen(address_.sun_path);
	if (bind(fd_, (sockaddr *)&address_, socket_len) != 0) {
		FATAL("Unable to bind on address: unix://%s; Error was: %s (%d)",
          STR(unix_path_), strerror(errno), errno);
		return false;
	}

	if (listen(fd_, 10240) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

  if ( -1 == chmod(STR(unix_path_), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) ) {
    FATAL("Unable to chmod %s, errno is %d", STR(unix_path_), errno);
    return false;
  }

	return true;
}

bool UNIXAcceptor::StartAccept() {
	return pIOHandlerManager_->EnableAcceptConnections(this);
}

bool UNIXAcceptor::OnEvent(struct epoll_event &event) {
	//we should not return false here, because the acceptor will simply go down.
	//Instead, after the connection accepting routine failed, check to
	//see if the acceptor socket is stil in the business
	if (!OnConnectionAvailable(event))
		return IsAlive();
	else
		return true;
}

bool UNIXAcceptor::OnConnectionAvailable(struct epoll_event &event) {
  return Accept();
}

bool UNIXAcceptor::Accept() {
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

	if (!setFdOptions(fd, false)) {
		FATAL("Unable to set socket options");
		CLOSE_SOCKET(fd);
		return false;
	}

  if ( NetWorker::WORKER_STATUS_RUN == worker_->status_ ) {
    BaseWorkerEx *ex = other_[accepted_count_++%other_count_];
    BaseCmd *cmd = ex->GetCmd(worker_->GetEx(), UNIX_ACCEPTCMD_TYPE);
    if ( !cmd ) {
      cmd = new UNIXAcceptCmd(fd, unix_path_);
    } else {
      ((UNIXAcceptCmd*)cmd)->fd_ = fd;
      ((UNIXAcceptCmd*)cmd)->unix_path_ = unix_path_;
      // ((AcceptCmd*)cmd)->worker_ = ex->worker_;
    }
    ex->SendCmd(worker_->GetEx(), UNIX_ACCEPTCMD_TYPE, cmd);
  } else {
    close(fd);
  }

	//7. Done
	return true;
}

bool UNIXAcceptor::IsAlive() {
	//TODO: Implement this. It must return true
	//if this acceptor is operational
	NYI;
	return true;
}




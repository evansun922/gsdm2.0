#include "pooldrive/poolworker.h"
#include "pooldrive/poolworkerex.h"
#include "pooldrive/sysaccept.h"
#include "pooldrive/staccept.h"
#include "logging/logger.h"


namespace gsdm {

bool PoolEnableReadData(int eq, int fd);
bool PoolDisableReadData(int eq, int fd);
bool PoolEnableWriteData(int eq, int fd);
bool PoolDisableWriteData(int eq, int fd);
bool PoolSysConnect(poolsocket_t fd, const struct sockaddr *addr, socklen_t addrlen, gsdm_msec_t timeout);
bool PoolStConnect(poolsocket_t fd, const struct sockaddr *addr, socklen_t addrlen, gsdm_msec_t timeout);
ssize_t PoolSysRecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolStRecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolSysSendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolStSendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolSysRecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, gsdm_msec_t timeout);
ssize_t PoolStRecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, gsdm_msec_t timeout);
ssize_t PoolSysSendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                                gsdm_msec_t timeout);
ssize_t PoolStSendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                               gsdm_msec_t timeout);

class AutoEpollWrite {
public:
  AutoEpollWrite(int eq, int fd) :eq_(eq),fd_(fd) { PoolEnableWriteData(eq_, fd_); }
  ~AutoEpollWrite() { PoolDisableWriteData(eq_, fd_); }

  int eq_;
  int fd_;
};

PoolWorkerEx::PoolWorkerEx(const std::string &worker_title, PoolWorkerMode mode, int thread_count) 
  : pool_mode_(mode),thread_count_(thread_count),is_close_(NULL),worker_title_(worker_title),worker_(NULL) {

}

PoolWorkerEx::~PoolWorkerEx() {
  if ( is_close_ ) {
    shmdt(is_close_);
    is_close_ = NULL;
  }
}

void PoolWorkerEx::MainDo() {

}

PoolProcess *PoolWorkerEx::CreateTCPProcess(uint16_t port) {
  WARN("No carry out.");
  return NULL;
}

PoolProcess *PoolWorkerEx::CreateUNIXProcess(const std::string &unix_path) {
  WARN("No carry out.");
  return NULL;
}

bool PoolWorkerEx::NeedWaitProcess() {
  return true;
}

void PoolWorkerEx::Close() {

}

pid_t PoolWorkerEx::StartWithProcess(const std::string &title, PoolProcess *process) {
  return worker_->StartWithProcess(title, process);
}

bool PoolWorkerEx::StartWithThread(PoolProcess *process) {
  return worker_->StartWithThread(process);
}

poolsocket_t PoolWorkerEx::CreateSocket(int s) {
  PoolSocket *fd = new PoolSocket();
  if ( POOLWORKERMODE_SYS == pool_mode_ ) {
    fd->sys_fd = s;
    if (!setFdOptions(s, false)) {
      delete fd;
      FATAL("Unable to set socket options");
      return NULL;
    }

    fd->sys_eq = epoll_create(1);
    if ( -1 == fd->sys_eq ) {
      FATAL("Unable to epoll_create: (%d) %s", errno, strerror(errno));
      delete fd;
      return NULL;
    }

    if ( !PoolEnableReadData(fd->sys_eq, fd->sys_fd) ) {
      delete fd;
      return NULL;
    }
    
    fd->pool_connect = &PoolSysConnect;
    fd->pool_recv = &PoolSysRecvMsg;
    fd->pool_send = &PoolSysSendMsg;
    fd->pool_recvfrom = &PoolSysRecvFromMsg;
    fd->pool_sendto = &PoolSysSendToMsg;
    return fd;
  }

	if (!setFdNoSIGPIPE(s)) {
    FATAL("Unable to set no SIGPIPE");
    close(s);
		return NULL;
	}

  fd->st_fd = st_netfd_open_socket(s);
  if ( NULL == fd->st_fd ) {
    close(s);
    delete fd;
    return NULL;
  }

  fd->pool_connect = &PoolStConnect;
  fd->pool_recv = &PoolStRecvMsg;
  fd->pool_send = &PoolStSendMsg;
  fd->pool_recvfrom = &PoolStRecvFromMsg;
  fd->pool_sendto = &PoolStSendToMsg;
  return fd;
}

poolsocket_t PoolWorkerEx::CreateSocket(bool is_unix, bool is_dgram) {
  int s = socket(is_unix?AF_UNIX:AF_INET, is_dgram?SOCK_DGRAM:SOCK_STREAM, 0);
  if ( -1 == s ) {
    FATAL("Unable to create socket: (%d) %s", errno, strerror(errno));
    return NULL;
  }

  return CreateSocket(s);
}

void PoolWorkerEx::CloseSocket(poolsocket_t fd) {
  // if ( POOLWORKERMODE_SYS == ex_->pool_mode_ )
  //   PoolDisableReadData(fd->sys_eq, fd->sys_fd);
  delete fd;
}

bool PoolWorkerEx::TCPConnect(poolsocket_t fd, const std::string &ip, uint16_t port, gsdm_msec_t timeout) {
  sockaddr_in address;
  address.sin_family = PF_INET;
  address.sin_addr.s_addr = inet_addr(STR(ip));
  if (address.sin_addr.s_addr == INADDR_NONE) {
    FATAL("Unable to translate string %s to a valid IP address", STR(ip));
    return false;
  }
  address.sin_port = htons(port);

  if ( !(*fd->pool_connect)(fd, (const struct sockaddr *)&address, sizeof(address), timeout) ) {
    FATAL("Unable to connect to %s:%hu (%d) (%s)", STR(ip), port, errno, strerror(errno));
    return false;
  }
  
  return true;
}

bool PoolWorkerEx::TCPListen(const std::string &ip, uint16_t port, std::vector<PoolWorkerEx *> &other) {
  sockaddr_in address;
	memset(&address, 0, sizeof(sockaddr_in));
	address.sin_family = PF_INET;
	address.sin_addr.s_addr = inet_addr(STR(ip));
	assert(address.sin_addr.s_addr != INADDR_NONE);
	address.sin_port = htons(port);

	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		FATAL("Unable to create socket: %s(%d)", strerror(errno), errno);
		return false;
	}

	if (!setFdOptions(fd, true)) {
		FATAL("Unable to set socket options");
		return false;
	}

	if (bind(fd, (sockaddr *)&address, sizeof(sockaddr)) != 0) {
		FATAL("Unable to bind on address: tcp://%s:%hu; Error was: %s (%d)",
          inet_ntoa(((sockaddr_in *) & address)->sin_addr),
          ntohs(((sockaddr_in *) & address)->sin_port),
          strerror(errno),
          errno);
		return false;
	}

	if (listen(fd, 10240) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

  std::vector<PoolWorker *> vec;
  for(int i = 0; i < (int)other.size(); i++) {
    vec.push_back(other[i]->worker_);
  }

  PoolAccept *a;
  if ( POOLWORKERMODE_SYS == pool_mode_ ) {
    if (!setFdOptions(fd, true)) {
      FATAL("Unable to set socket options");
      return false;
    }
    a = new SysPoolAccept(fd, (SysPoolWorker *)worker_, vec);
  } else {
    st_netfd_t f = st_netfd_open_socket(fd);
    a = new StPoolAccept(f, (StPoolWorker *)worker_, vec);
  }
  a->ex_data_[1] = 0;
  memcpy(a->ex_data_+2, &port, sizeof(port));

  worker_->accept_hash_[fd] = a;
  return true;
}

bool PoolWorkerEx::UNIXConnect(poolsocket_t fd, const std::string &unix_path, gsdm_msec_t timeout) { 
  // remove(STR(unix_path));
  sockaddr_un address;
  memset(&address, 0, sizeof(sockaddr_un));
  address.sun_family = PF_UNIX;
  strncpy(address.sun_path, STR(unix_path), sizeof(address.sun_path) - 1);

  if ( !(*fd->pool_connect)(fd, (const struct sockaddr *)&address, 
                            sizeof(address.sun_family) + strlen(address.sun_path), timeout) ) {
    FATAL("Unable to connect to %s (%d) (%s)", STR(unix_path), errno, strerror(errno));
    return false;
  }
  
  return true;
}

bool PoolWorkerEx::UNIXListen(const std::string &sun_path, std::vector<PoolWorkerEx *> &other) {
  struct sockaddr_un address;
	memset(&address, 0, sizeof (struct sockaddr_un));
	address.sun_family = PF_UNIX;
	strncpy(address.sun_path, STR(sun_path), sizeof(address.sun_path) - 1);

  int fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		FATAL("Unable to create socket: %s(%d)", strerror(errno), errno);
		return false;
	}

	if (!setFdOptions(fd, false)) {
		FATAL("Unable to set socket options");
		return false;
	}

  deleteFile(sun_path);
  size_t socket_len = sizeof(address.sun_family) + strlen(address.sun_path);
	if (bind(fd, (sockaddr *)&address, socket_len) != 0) {
		FATAL("Unable to bind on address: unix://%s; Error was: %s (%d)",
          address.sun_path, strerror(errno), errno);
		return false;
	}

	if (listen(fd, 10240) != 0) {
		FATAL("Unable to put the socket in listening mode");
		return false;
	}

  if ( -1 == chmod(STR(sun_path), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) ) {
    FATAL("Unable to chmod %s, errno is %d", STR(sun_path), errno);
    return false;
  }

  std::vector<PoolWorker *> vec;
  for (int i = 0; i < (int)other.size(); i++) {
    vec.push_back(other[i]->worker_);
  }

  PoolAccept *a;
  if ( POOLWORKERMODE_SYS == pool_mode_ ) {
    if (!setFdOptions(fd, false)) {
      FATAL("Unable to set socket options");
      return false;
    }
    a = new SysPoolAccept(fd, (SysPoolWorker *)worker_, vec);
  } else {
    st_netfd_t f = st_netfd_open_socket(fd);
    a = new StPoolAccept(f, (StPoolWorker *)worker_, vec);
  }
  a->ex_data_[1] = 1;
  strncpy(a->ex_data_+2, STR(sun_path), ACCEPT_EX_DATA_LENGTH-2);

  worker_->accept_hash_[fd] = a;  
  return true;
}

ssize_t PoolWorkerEx::RecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout) {
  return (*fd->pool_recv)(fd, buf, len, timeout);
}

ssize_t PoolWorkerEx::SendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout) {
  return (*fd->pool_send)(fd, buf, len, timeout);
}

ssize_t PoolWorkerEx::RecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen,
                                  gsdm_msec_t timeout) {
  return (*fd->pool_recvfrom)(fd, buf, len, src_addr, addrlen, timeout);
}

ssize_t PoolWorkerEx::SendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                                gsdm_msec_t timeout) {
  return (*fd->pool_sendto)(fd, buf, len, dest_addr, addrlen, timeout);
}



bool PoolEnableReadData(int eq, int fd) {
  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN;
  evt.data.fd = fd;
  if ( 0 != epoll_ctl(eq, EPOLL_CTL_ADD, fd, &evt) ) {
    FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
    return false;
  }

  return true;
}

bool PoolDisableReadData(int eq, int fd) {
  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN;
  evt.data.fd = fd;
  if (epoll_ctl(eq, EPOLL_CTL_DEL, fd, &evt) != 0) {
    FATAL("Unable to disable read data: (%d) %s", errno, strerror(errno));
    return false;    
  }

  return true;
}

bool PoolEnableWriteData(int eq, int fd) {
  struct epoll_event evt = {0, {0}};
  // evt.events = EPOLLIN | EPOLLOUT;
  evt.events = EPOLLOUT;
  evt.data.fd = fd;

  if (epoll_ctl(eq, EPOLL_CTL_MOD, fd, &evt) != 0) {
    FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
    return false;
  }

  return true;
}

bool PoolDisableWriteData(int eq, int fd) {
  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN;
  evt.data.fd = fd;
  if (epoll_ctl(eq, EPOLL_CTL_MOD, fd, &evt) != 0) {
    FATAL("Unable to disable write data: (%d) %s", errno, strerror(errno));
    return false;
  }

  return true;
}

bool PoolSysConnect(poolsocket_t fd, const struct sockaddr *addr, socklen_t addrlen, gsdm_msec_t timeout) {
  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN | EPOLLOUT;
  evt.data.fd = fd->sys_fd;

  if (epoll_ctl(fd->sys_eq, EPOLL_CTL_MOD, fd->sys_fd, &evt) != 0) {
    FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
    return false;
  }

  if ( 0 != connect(fd->sys_fd, addr, addrlen) ) {
    if ( EINTR != errno && EINPROGRESS != errno ) {
      return false;
    }
  }

  epoll_event query;
  gsdm_msec_t t1, t2;
  while ( true ) {
    t1 = getCurrentGsdmMsec();
    int count = epoll_wait(fd->sys_eq, &query, 1, timeout);
    if ( 0 < count ) {
      if ((query.events & EPOLLERR) != 0) { 
        return false;
      }
    
      if ( !PoolDisableWriteData(fd->sys_eq, fd->sys_fd) )
        return false;

      return true;
    }
  
    if ( 0 == count ) {
      errno = ETIMEDOUT;
      return false;
    }

    if ( EINTR == errno ) {
      t2 = getCurrentGsdmMsec() - t1;
      if ( t2 >= timeout ) {
        errno = ETIMEDOUT;
        return false;
      }
      timeout -= t2;
      continue;
    }
    return false;
  }
   
  return false;
}

bool PoolStConnect(poolsocket_t fd, const struct sockaddr *addr, socklen_t addrlen, gsdm_msec_t timeout) {
  if ( POOL_UTIME_NO_TIMEOUT == timeout ) {
    if ( -1 == st_connect(fd->st_fd, addr, addrlen, ST_UTIME_NO_TIMEOUT) )
      return false;
    return true;
  }
  if ( -1 == st_connect(fd->st_fd, addr, addrlen, timeout*1000) )
    return false;
  return true;
}

ssize_t PoolSysRecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout) {
  ssize_t rs = recv(fd->sys_fd, buf, len, 0);
  if ( 0 <= rs )
    return rs;
  if ( EINTR != errno && EAGAIN != errno )
    return -1;

  epoll_event query;
  gsdm_msec_t t1, t2;
  while ( true ) {
    t1 = getCurrentGsdmMsec();
    int count = epoll_wait(fd->sys_eq, &query, 1, timeout);
    if ( 0 < count ) {
      if ( 0 != (query.events & EPOLLERR) ) { 
        return -1;
      }

      if ( 0 != (query.events & EPOLLHUP) ) {
        return -1;
      }
    
      if ( 0 != (query.events & EPOLLIN) ) {
        rs = recv(fd->sys_fd, buf, len, 0);
        if ( 0 <= rs )
          return rs;
        if ( EINTR != errno && EAGAIN != errno )
          return -1;
      }
    } else if ( 0 == count ) {
      errno = ETIME;
      return -1;
    } else if ( EINTR != errno ) {
      return -1;
    }
     
    t2 = getCurrentGsdmMsec() - t1;
    if ( t2 >= timeout ) {
      errno = ETIME;
      return -1;
    }
    timeout -= t2;
  }
   
  return -1;  
}

ssize_t PoolStRecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout) {
  if ( POOL_UTIME_NO_TIMEOUT == timeout ) {
    return st_read(fd->st_fd, buf, len, ST_UTIME_NO_TIMEOUT);
  } 
  return st_read(fd->st_fd, buf, len, timeout*1000);
}

ssize_t PoolSysSendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout) {
  ssize_t send_len = 0;
  ssize_t rs = send(fd->sys_fd, (uint8_t *)buf+send_len, len - send_len, 0);
  if ( 0 > rs ) {
    if ( EINTR != errno && EAGAIN != errno )
      return -1;
  } else {
    send_len += rs;
    if ( (ssize_t)len == send_len )
      return send_len;
  }

  AutoEpollWrite a(fd->sys_eq, fd->sys_fd);
  epoll_event query;
  gsdm_msec_t t1, t2;
  while ( true ) {
    t1 = getCurrentGsdmMsec();
    int count = epoll_wait(fd->sys_eq, &query, 1, timeout);
    if ( 0 < count ) {
      if ( 0 != (query.events & EPOLLERR) ) { 
        return -1;
      }

      if ( 0 != (query.events & EPOLLHUP) ) {
        return -1;
      }
    
      if ( 0 != (query.events & EPOLLOUT) ) {
        rs = send(fd->sys_fd, (uint8_t *)buf+send_len, len - send_len, 0);
        if ( 0 > rs ) {
          if ( EINTR != errno && EAGAIN != errno )
            return -1;
        } else {
          send_len += rs;
          if ( (ssize_t)len == send_len )
            return send_len;
        }
      }
    } else if ( 0 == count ) {
      errno = ETIME;
      return -1;
    } else if ( EINTR != errno ) {
      return -1;
    }

    t2 = getCurrentGsdmMsec() - t1;
    if ( t2 >= timeout ) {
      errno = ETIME;
      return -1;
    }
    timeout -= t2;
  }
   
  return -1;  
}

ssize_t PoolStSendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout) {
  if ( POOL_UTIME_NO_TIMEOUT == timeout ) {
    return st_write(fd->st_fd, buf, len, ST_UTIME_NO_TIMEOUT);
  }
  return st_write(fd->st_fd, buf, len, timeout*1000);
}

ssize_t PoolSysRecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, gsdm_msec_t timeout) {
  ssize_t rs = recvfrom(fd->sys_fd, buf, len, 0, src_addr, addrlen);
  if ( 0 < rs )
    return rs;
  if ( EINTR != errno && EAGAIN != errno ) { 
    return -1;
  }

  epoll_event query;
  gsdm_msec_t t1, t2;
  while ( true ) {
    t1 = getCurrentGsdmMsec();
    int count = epoll_wait(fd->sys_eq, &query, 1, timeout);
    if ( 0 < count ) {
      if ( 0 != (query.events & EPOLLERR) ) { 
        return -1;
      }

      if ( 0 != (query.events & EPOLLHUP) ) {
        return -1;
      }
    
      if ( 0 != (query.events & EPOLLIN) ) {
        rs = recvfrom(fd->sys_fd, buf, len, 0, src_addr, addrlen);
        if ( 0 < rs )
          return rs;
        if ( EINTR != errno && EAGAIN != errno ) {
          return -1;
        }
      }
    } else if ( 0 == count ) {
      errno = ETIME;
      return -1;
    } else if ( EINTR != errno ) {
      return -1;
    }

    t2 = getCurrentGsdmMsec() - t1;
    if ( t2 >= timeout ) {
      errno = ETIME;
      return -1;
    }
    timeout -= t2;
  }
   
  return -1;  
}

ssize_t PoolStRecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, gsdm_msec_t timeout) {
  if ( POOL_UTIME_NO_TIMEOUT == timeout ) {
    return st_recvfrom(fd->st_fd, buf, (int)len, src_addr, (int *)addrlen, ST_UTIME_NO_TIMEOUT);
  }
  return st_recvfrom(fd->st_fd, buf, (int)len, src_addr, (int *)addrlen, timeout*1000);
}

ssize_t PoolSysSendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                                gsdm_msec_t timeout) {
  ssize_t rs = sendto(fd->sys_fd, buf, len, 0, dest_addr, addrlen);
  if ( 0 > rs ) {
    if ( EINTR != errno && EAGAIN != errno )
      return -1;
  } else {
    return rs;
  }

  AutoEpollWrite a(fd->sys_eq, fd->sys_fd);
  epoll_event query;
  gsdm_msec_t t1, t2;
  while ( true ) {
    t1 = getCurrentGsdmMsec();
    int count = epoll_wait(fd->sys_eq, &query, 1, timeout);
    if ( 0 < count ) {
      if ( 0 != (query.events & EPOLLERR) ) { 
        return -1;
      }

      if ( 0 != (query.events & EPOLLHUP) ) {
        return -1;
      }
    
      if ( 0 != (query.events & EPOLLOUT) ) {
        rs = sendto(fd->sys_fd, buf, len, 0, dest_addr, addrlen);
        if ( 0 > rs ) {
          if ( EINTR != errno && EAGAIN != errno )
            return -1;
        } else {
          return rs;
        }
      }
    } else if ( 0 == count ) {
      errno = ETIME;
      return -1;
    } else if ( EINTR != errno ) {
      return -1;
    }

    t2 = getCurrentGsdmMsec() - t1;
    if ( t2 >= timeout ) {
      errno = ETIME;
      return -1;
    }
    timeout -= t2;
  }
   
  return -1;  
}

ssize_t PoolStSendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                                 gsdm_msec_t timeout) {
  if ( POOL_UTIME_NO_TIMEOUT == timeout ) {
    return st_sendto(fd->st_fd, buf, (int)len, dest_addr, (int)addrlen, ST_UTIME_NO_TIMEOUT);
  }
  return st_sendto(fd->st_fd, buf, (int)len, dest_addr, (int)addrlen, timeout*1000);
}




}

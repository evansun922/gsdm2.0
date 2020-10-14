#include "pooldrive/stworker.h"
#include "pooldrive/poolworkerex.h"
#include "pooldrive/poolprocess.h"
#include "pooldrive/staccept.h"
#include "logging/logger.h"

namespace gsdm {

extern __thread int g_log_fd;

StPoolWorker::StPoolWorker(PoolWorkerEx *ex) 
  : PoolWorker(ex),unix_sock_wait_(NULL),lock_(NULL),query_(NULL) {

}

StPoolWorker::~StPoolWorker() {
  if ( unix_sock_wait_ ) {
    st_netfd_close(unix_sock_wait_);
    unix_sock_wait_ = NULL;
    g_log_fd = -1;    
  }

  if ( lock_ ) {
    st_mutex_destroy(lock_);
    lock_ = NULL;
  }

  if ( query_ ) {
    delete [] query_;
    query_ = NULL;
  }

  FOR_UNORDERED_MAP(total_handles_,int,PoolHandle *,i) {
    st_cond_destroy(MAP_VAL(i)->signal);
    if ( MAP_VAL(i)->process ) {
      delete MAP_VAL(i)->process;
      MAP_VAL(i)->process = NULL;
    }
    delete MAP_VAL(i);
  }
  total_handles_.clear();
  idle_handles_.clear();
  use_handles_.clear();
}

void StPoolWorker::Run() {
  st_init();
  pool_pid_ = getpid();

  int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  if ( -1 == fd ) {
    FATAL("Unable to crate socket: (%d) %s", errno, strerror(errno));
    return;
  }
  
  remove(address_wait_.sun_path);
  if ( -1 == bind(fd, (sockaddr *)&address_wait_, address_len_) ) {
    FATAL("Unable to bind: (%d) %s", errno, strerror(errno));
    exit(0);
  }
  unix_sock_wait_ = st_netfd_open_socket(fd);
  if ( NULL == unix_sock_wait_ ) {
    FATAL("Unable to st_netfd_open_socket: (%d) %s", errno, strerror(errno));
    exit(0);
  }

  int count = accept_hash_.size()+1;
  query_ = new pollfd[count];
  query_[0].fd = st_netfd_fileno(unix_sock_wait_);
  query_[0].events = POLLIN;
  query_[0].revents = 0;

  int query_i = 1;
  FOR_UNORDERED_MAP(accept_hash_,int,PoolAccept *,i) {
    PoolAccept *a = MAP_VAL(i);
    query_[query_i].fd = a->GetFd();
    query_[query_i].events = POLLIN;
    query_[query_i++].revents = 0;
  }
  
  lock_ = st_mutex_new();  
  g_log_fd = st_netfd_fileno(unix_sock_wait_);
  
  st_thread_t main_thread = st_thread_create(StPoolWorker::MainRun, this, 1, 1024*512);
  if ( NULL == main_thread ) {
    FATAL("Unable to pthread_create main: (%d) %s", errno, strerror(errno));
    exit(0);
  }

  do {
    st_mutex_lock(lock_);
    for (int i = 0; i < ex_->thread_count_; i++) {
      PoolHandle *h = new PoolHandle(i, this);
      total_handles_[i] = h;
      h->signal = st_cond_new();
      h->thread = st_thread_create(StPoolWorker::DoRun, h, 1, 1024*512);
      if ( NULL == h->thread ) {
        FATAL("Unable to pthread_create DoRun: (%d) %s", errno, strerror(errno));
        exit(0);
      }
    }
    idle_handles_ = total_handles_;
    st_mutex_unlock(lock_);
  } while (0);

  int rs;
  while ( false == *ex_->is_close_ ) {
    rs = st_poll(query_, count, 5000000);
    if ( -1 == rs ) {
      if ( EINTR == errno )
        continue;
      FATAL("Unable to execute epoll_wait: (%d) %s", errno, strerror(errno));
      exit(0);
    }

    if ( query_[0].revents & POLLIN ) {
      rs--;
      query_[0].revents = 0;

      struct msghdr msg;
      char recv_char[ACCEPT_EX_DATA_LENGTH];
      struct iovec vec;
      int recv_fd;
      char cmsg_buf[CMSG_SPACE(sizeof(recv_fd))];
      struct cmsghdr *p_cmsg;
      int *p_fd;

      msg.msg_name = NULL;
      msg.msg_namelen = 0;
      msg.msg_iov = &vec;
      msg.msg_iovlen = 1;
      msg.msg_control = cmsg_buf;
      msg.msg_controllen = sizeof(cmsg_buf);
      msg.msg_flags = 0;

      vec.iov_base = recv_char; // &recv_char;
      vec.iov_len = ACCEPT_EX_DATA_LENGTH; // sizeof(recv_char);
      p_fd = (int *)CMSG_DATA(CMSG_FIRSTHDR(&msg));
      *p_fd = -1;
      if ( -1 == st_recvmsg(unix_sock_wait_, &msg, 0, ST_UTIME_NO_TIMEOUT) )
        continue;

      if ( 'A' == recv_char[0] ) {
        p_cmsg = CMSG_FIRSTHDR(&msg);
        if ( p_cmsg ) {
          p_fd = (int *)CMSG_DATA(p_cmsg);
          recv_fd = *p_fd;
          if ( -1 != recv_fd ) {
            st_netfd_t st_fd = st_netfd_open_socket(recv_fd);
            ProcessFd(st_fd, recv_char);
          }
        }
      } else if ( 'C' == recv_char[0] ) {
        *ex_->is_close_ = true;
        ex_->Close();
        st_mutex_lock(lock_);
        FOR_UNORDERED_MAP(idle_handles_,int,PoolHandle *,i) {
          st_cond_signal(MAP_VAL(i)->signal);
        }
        st_mutex_unlock(lock_);
        break;
      }
    }

    for (int i = 1; i < count; i++) {
      if ( query_[i].revents & POLLIN ) {
        accept_hash_[query_[i].fd]->Process();
        query_[i].revents = 0;
        if ( 0 == --rs )
          break;
      }
    }
  }
  
  st_thread_join(main_thread, NULL);
  FOR_UNORDERED_MAP(total_handles_,int,PoolHandle *,i) {
    st_thread_join(MAP_VAL(i)->thread, NULL);
    st_cond_destroy(MAP_VAL(i)->signal);
    if ( MAP_VAL(i)->process ) {
      delete MAP_VAL(i)->process;
      MAP_VAL(i)->process = NULL;
    }
    delete MAP_VAL(i);
  }
  total_handles_.clear();
  idle_handles_.clear();
  use_handles_.clear();
}

pid_t StPoolWorker::StartWithProcess(const std::string &title, PoolProcess *process) {
  FATAL("st mode is noe support function StartWithProcess.");
  usleep(100);
  abort();
}

bool StPoolWorker::StartWithThread(PoolProcess *process) {
  process->ex_ = ex_;
  st_mutex_lock(lock_);
  if ( idle_handles_.empty() ) {
    WARN("there is no any  handle for pool process");
    delete process;
    st_mutex_unlock(lock_);
    return false;
  } 

  PoolHandleHash::iterator iter = idle_handles_.begin();
  PoolHandle *h = MAP_VAL(iter);
  h->process = process;
  idle_handles_.erase(iter);
  use_handles_[h->handle_id] = h;    
  st_mutex_unlock(lock_);
  st_cond_signal(h->signal);  
  return true;
}

void StPoolWorker::SendFd(int send_fd, char *ex_data, sockaddr_un *address, socklen_t address_len) {
  struct msghdr msg;
  struct iovec vec;
  struct cmsghdr *p_cmsg;
  char cmsg_buf[CMSG_SPACE(sizeof(send_fd))];
  int *p_fds;
  // char send_char = 'A';
  ex_data[0] = 'A';

  msg.msg_name = address;
  msg.msg_namelen = address_len;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof(cmsg_buf);
  msg.msg_flags = 0;

  vec.iov_base = ex_data; // &send_char;
  vec.iov_len = ACCEPT_EX_DATA_LENGTH; // sizeof(send_char);

  p_cmsg = CMSG_FIRSTHDR(&msg);
  p_cmsg->cmsg_level = SOL_SOCKET;
  p_cmsg->cmsg_type = SCM_RIGHTS;
  p_cmsg->cmsg_len = CMSG_LEN(sizeof(send_fd));
  p_fds = (int *)CMSG_DATA(p_cmsg);
  *p_fds = send_fd;

  if ( -1 == st_sendmsg(unix_sock_wait_, &msg, 0, ST_UTIME_NO_TIMEOUT) ) {
    FATAL("Unable to sendmsg: (%d) %s", errno, strerror(errno));
  }
}

bool PoolStConnect(poolsocket_t fd, const struct sockaddr *addr, socklen_t addrlen, gsdm_msec_t timeout);
ssize_t PoolStRecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolStSendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout);
ssize_t PoolStRecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, gsdm_msec_t timeout);
ssize_t PoolStSendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen,
                               gsdm_msec_t timeout);

void StPoolWorker::ProcessFd(st_netfd_t fd, char *ex_data) {
  poolsocket_t pool_fd = new PoolSocket();
  pool_fd->st_fd = fd;
  pool_fd->pool_connect = &PoolStConnect;
  pool_fd->pool_recv = &PoolStRecvMsg;
  pool_fd->pool_send = &PoolStSendMsg;
  pool_fd->pool_recvfrom = &PoolStRecvFromMsg;
  pool_fd->pool_sendto = &PoolStSendToMsg;

  PoolProcess *p;
  if ( 0 == ex_data[1] ) {
    p = ex_->CreateTCPProcess(*((uint16_t *)(ex_data+2)));
  } else {
    p = ex_->CreateUNIXProcess(ex_data+2);
  }

  if ( NULL == p ) {
    WARN("call CreateProcess, but ret is NULL");
    delete pool_fd;
  }

  p->pool_fd_ = pool_fd;
  p->ex_ = ex_;
  StartWithThread(p);
}

void *StPoolWorker::MainRun(void *arg) {
  StPoolWorker *w = (StPoolWorker *)arg;
  g_log_fd = st_netfd_fileno(w->unix_sock_wait_);
  w->ex_->MainDo();
  return NULL;
}

void *StPoolWorker::DoRun(void *arg) {
  PoolHandle *h = (PoolHandle *)arg;  
  g_log_fd = st_netfd_fileno(h->worker->unix_sock_wait_);

  while ( false == *h->worker->ex_->is_close_ ) {
    st_cond_wait(h->signal);

    if ( h->process ) {
      h->process->Process();
      delete h->process;
      h->process = NULL;
    }
    st_mutex_lock(h->worker->lock_);
    h->worker->use_handles_.erase(h->handle_id);
    h->worker->idle_handles_[h->handle_id] = h;
    st_mutex_unlock(h->worker->lock_);
  }
  return NULL;
}



}

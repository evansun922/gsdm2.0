#include <sys/wait.h>
#include "pooldrive/manager.h"
#include "pooldrive/sysworker.h"
#include "pooldrive/poolworkerex.h"
#include "pooldrive/poolprocess.h"
#include "pooldrive/sysaccept.h"
#include "logging/logger.h"


namespace gsdm {

extern __thread int g_log_fd;


SysPoolWorker::SysPoolWorker(PoolWorkerEx *ex) 
  : PoolWorker(ex),eq_(-1),unix_sock_wait_(-1),lock_(NULL),child_lock_(NULL) {

}

SysPoolWorker::~SysPoolWorker() {
  if ( -1 != eq_ ) {
    close(eq_);
    eq_ = -1;
  }

  if ( -1 != unix_sock_wait_ ) {
    close(unix_sock_wait_);
    unix_sock_wait_ = -1;
    g_log_fd = -1;    
  }

  if ( lock_ ) {
    pthread_mutex_destroy(lock_);
    delete lock_;
    lock_ = NULL;
  }

  FOR_UNORDERED_MAP(total_handles_,int,PoolHandle *,i) {
    sem_destroy(&MAP_VAL(i)->signal);
    if ( MAP_VAL(i)->process ) {
      delete MAP_VAL(i)->process;
      MAP_VAL(i)->process = NULL;
    }
    delete MAP_VAL(i);
  }
  total_handles_.clear();
  idle_handles_.clear();
  use_handles_.clear();

  if ( child_lock_ ) {
    pthread_mutex_destroy(child_lock_);
    delete child_lock_;
    child_lock_ = NULL;
  }
}

void SysPoolWorker::Run() {
  pool_pid_ = getpid();
  INFO("start sys mode worker %s[%d]", STR(ex_->worker_title_),pool_pid_);

  eq_ = epoll_create(32);
  if ( -1 == eq_ ) {
    FATAL("Unable to epoll_create: (%d) %s", errno, strerror(errno));
    return;
  }

  unix_sock_wait_ = (int)socket(PF_UNIX, SOCK_DGRAM, 0);
  if ( -1 == unix_sock_wait_ ) {
    FATAL("Unable to crate socket: (%d) %s", errno, strerror(errno));
    return;
  }

  remove(address_wait_.sun_path);
  if ( -1 == bind(unix_sock_wait_, (sockaddr *)&address_wait_, address_len_) ) {
    FATAL("Unable to bind: (%d) %s", errno, strerror(errno));
    return;
  }

  if (!setFdOptions(unix_sock_wait_, false)) {
    FATAL("Unable to set socket options");
    return;
  }

  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN;
  evt.data.fd = unix_sock_wait_;
  if (epoll_ctl(eq_, EPOLL_CTL_ADD, unix_sock_wait_, &evt) != 0) {
    FATAL("Unable to enable epoll_add: (%d) %s", errno, strerror(errno));
    return;
  }

  FOR_UNORDERED_MAP(accept_hash_,int,PoolAccept *,i) {
    PoolAccept *a = MAP_VAL(i);
    struct epoll_event e = {0, {0}};
    e.events = EPOLLIN;
    e.data.ptr = a;
    if (epoll_ctl(eq_, EPOLL_CTL_ADD, a->GetFd(), &e) != 0) {
      FATAL("Unable to enable epoll_add: (%d) %s", errno, strerror(errno));
      exit(0);
    }
  }

  lock_ = new pthread_mutex_t();
  if ( -1 == pthread_mutex_init(lock_, NULL) ) {
    FATAL("Unable to enable pthread_mutex_init: (%d) %s", errno, strerror(errno));
    delete lock_;
    lock_ = NULL;
    return;
  }

  child_lock_ = new pthread_mutex_t();
  if ( -1 == pthread_mutex_init(child_lock_, NULL) ) {
    FATAL("Unable to enable pthread_mutex_init: (%d) %s", errno, strerror(errno));
    delete child_lock_;
    child_lock_ = NULL;
    return;
  }

  g_log_fd = unix_sock_wait_;

  do {
    AutoMutexLock lock(lock_);
    for (int i = 0; i < ex_->thread_count_; i++) {
      PoolHandle *h = new PoolHandle(i, this);
      total_handles_[i] = h;
      sem_init(&h->signal, 0, 0);
      if ( 0 != pthread_create(&h->thread, NULL, SysPoolWorker::DoRun, h) ) {
        FATAL("Unable to pthread_create DoRun: (%d) %s", errno, strerror(errno));
        exit(0);
      }
    }
    idle_handles_ = total_handles_;
  } while (0);

  pthread_t main_thread;
  if ( 0 != pthread_create(&main_thread, NULL, SysPoolWorker::MainRun, this) ) {
    FATAL("Unable to pthread_create main: (%d) %s", errno, strerror(errno));
    return;
  }

  int count;
  epoll_event query[32];
  while ( false == *ex_->is_close_ ) {
    count = epoll_wait(eq_, query, 32, 5000);
    if ( -1 == count ) {
      if ( EINTR == errno ) {
        continue;
      }
      FATAL("Unable to execute epoll_wait: (%d) %s", errno, strerror(errno));
      exit(0);
    }

    if (0 == count && ex_->NeedWaitProcess()) {
      while ( true ) {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if ( 0 < pid ) {
          INFO("StartWithProcess with %d exit", pid);
          AutoMutexLock lock(child_lock_);
          child_pid_hash_.erase(pid);
          continue;
        }
        break;
      }
    }

    for (int i = 0; i < count; i++) {
      if ( unix_sock_wait_ == query[i].data.fd ) {
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
        if ( -1 == recvmsg(unix_sock_wait_, &msg, 0) )
          continue;

        if ( 'A' == recv_char[0] ) {
          p_cmsg = CMSG_FIRSTHDR(&msg);
          if ( p_cmsg ) {
            p_fd = (int *)CMSG_DATA(p_cmsg);
            recv_fd = *p_fd;
            if ( -1 != recv_fd ) {
              ProcessFd(recv_fd, recv_char);
            }
          }
        } else if ( 'C' == recv_char[0] ) {
          *ex_->is_close_ = true;
          ex_->Close();
          AutoMutexLock lock(lock_);
          FOR_UNORDERED_MAP(idle_handles_,int,PoolHandle *,i) {
            int sval = 0;
            sem_getvalue(&MAP_VAL(i)->signal, &sval);
            sem_post(&MAP_VAL(i)->signal);
          }
          break;
        }
      } else {
        PoolAccept *a = (PoolAccept *)query[i].data.ptr;
        a->Process();
      }
    }
  }
  
  pthread_join(main_thread, NULL);

  FOR_UNORDERED_MAP(total_handles_,int,PoolHandle *,i) {
    pthread_join(MAP_VAL(i)->thread, NULL);
    sem_destroy(&MAP_VAL(i)->signal);
    if ( MAP_VAL(i)->process ) {
      delete MAP_VAL(i)->process;
      MAP_VAL(i)->process = NULL;
    }
    delete MAP_VAL(i);
  }
  total_handles_.clear();
  idle_handles_.clear();
  use_handles_.clear();

  if (ex_->NeedWaitProcess()) {
    while ( false == child_pid_hash_.empty() ) {
      pid_t pid = wait(NULL);
      child_pid_hash_.erase(pid);
    }
  }
}

pid_t SysPoolWorker::StartWithProcess(const std::string &title, PoolProcess *process) {
  process->ex_ = ex_;
  pid_t pid = fork();
  if ( 0 == pid ) {
    g_log_fd = unix_sock_wait_;
    PoolManager::GetManager()->SetTitle(title, false);
    process->Process();
    delete process;
    PoolManager::GetManager()->Free();
    exit(0);  
  }
  delete process;
  if (ex_->NeedWaitProcess()) {
    AutoMutexLock lock(child_lock_);
    child_pid_hash_.insert(pid);
  }
  return pid;
}

bool SysPoolWorker::StartWithThread(PoolProcess *process) {
  process->ex_ = ex_;
  pthread_mutex_lock(lock_);
  if ( idle_handles_.empty() ) {
    WARN("there is no any  handle for pool process");
    delete process;
    pthread_mutex_unlock(lock_);
    return false;
  }

  PoolHandleHash::iterator iter = idle_handles_.begin();
  PoolHandle *h = MAP_VAL(iter);
  h->process = process;
  idle_handles_.erase(iter);
  use_handles_[h->handle_id] = h;    
  pthread_mutex_unlock(lock_);
  sem_post(&h->signal);
  return true;
}

void SysPoolWorker::SendFd(int send_fd, char *ex_data, sockaddr_un *address, socklen_t address_len) {
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

  if ( -1 == sendmsg(unix_sock_wait_, &msg, 0) ) {
    FATAL("Unable to sendmsg: (%d) %s", errno, strerror(errno));
    close(send_fd);
  }
}

void SysPoolWorker::ProcessFd(int fd, char *ex_data) {
  poolsocket_t pool_fd = ex_->CreateSocket(fd);
  if ( NULL == pool_fd ) {
    FATAL("Unable to CreateSocket: (%d) %s", errno, strerror(errno));
    return;
  }

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

void *SysPoolWorker::MainRun(void *arg) {
  SysPoolWorker *w = (SysPoolWorker *)arg;
  g_log_fd = w->unix_sock_wait_;
  w->ex_->MainDo();
  return NULL;
}

void *SysPoolWorker::DoRun(void *arg) {
  PoolHandle *h = (PoolHandle *)arg;  
  g_log_fd = h->worker->unix_sock_wait_;

  while ( false == *h->worker->ex_->is_close_ ) {
    sem_wait(&h->signal);

    if ( h->process ) {
      h->process->Process();
      delete h->process;
      h->process = NULL;
    }
    AutoMutexLock lock(h->worker->lock_);
    h->worker->use_handles_.erase(h->handle_id);
    h->worker->idle_handles_[h->handle_id] = h;
  }
  return NULL;
}





}

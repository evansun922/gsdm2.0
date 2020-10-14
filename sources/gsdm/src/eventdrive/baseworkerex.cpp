#include <unistd.h>
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/epoll/tcpconnector.h"
#include "eventdrive/epoll/tcpacceptor.h"
#include "eventdrive/epoll/unixacceptor.h"
#include "eventdrive/epoll/unixconnector.h"
#include "eventdrive/epoll/udpcarrier.h"
#include "eventdrive/epoll/pipecarrier.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/networker.h"
#include "eventdrive/manager.h"
#include "eventdrive/tcpacceptcmd.h"
#include "eventdrive/unixacceptcmd.h"
#include "eventdrive/tcptransfercmd.h"
#include "eventdrive/unixtransfercmd.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"


namespace gsdm {

extern pthread_key_t gsdm_log_cmd_key;

BaseWorkerEx::BaseWorkerEx(uint32_t timeout):worker_(NULL),timeout_(timeout) {

}

BaseWorkerEx::~BaseWorkerEx() {

}

BaseProcess *BaseWorkerEx::CreateTCPProcess(uint16_t port) {
  WARN("No carry out.");
  return NULL;
}

BaseProcess *BaseWorkerEx::CreateUNIXProcess(const std::string &unix_path) {
  WARN("No carry out.");
  return NULL;
}

BaseCmd *BaseWorkerEx::GetCmd(BaseWorkerEx *from, int cmd_type) {
  return worker_->GetCmd(from->worker_->GetID(), cmd_type);
}

void BaseWorkerEx::SendCmd(BaseWorkerEx *from, int cmd_type, BaseCmd *cmd) {
  worker_->SendCmd(from->worker_->GetID(), cmd_type, cmd);
}

bool BaseWorkerEx::AddTimeoutManager(TimeoutManager *manager) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }
  worker_->AddTimeoutManager(manager);
  return true;
}

bool BaseWorkerEx::RemoveTimeoutManager(TimeoutManager *manager) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }
  worker_->RemoveTimeoutManager(manager);
  return true;
}

bool BaseWorkerEx::AddTimerEvent(TimerEvent *timer) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }
  worker_->AddTimerEvent(timer);
  return true;
}

bool BaseWorkerEx::RemoveTimerEvent(TimerEvent *timer) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }

  worker_->RemoveTimerEvent(timer);
  return true;
}

void BaseWorkerEx::CreateTCPTransfer(std::vector<BaseWorkerEx *> &other) {
  EventManager::GetManager()->SetupQueue(this, other, TCP_TRANSFERCMD_TYPE); 
}

void BaseWorkerEx::TCPTransfer(BaseWorkerEx *dst, uint16_t port, BaseProcess *process) {
  TCPTransferCmd *cmd = static_cast<TCPTransferCmd *>(dst->GetCmd(this, TCP_TRANSFERCMD_TYPE));
  if ( !cmd ) {
    cmd = new TCPTransferCmd();
  }

  uint8_t *buffer = GETIBPOINTER(process->input_buffer_);
  uint32_t size = GETAVAILABLEBYTESCOUNT(process->input_buffer_);

  cmd->fd_         = dup(process->io_handler_->GetFd());
  cmd->port_       = port;
  if ( size ) {
    cmd->data_     = new uint8_t[size];
    memcpy(cmd->data_, buffer, size);
    cmd->data_len_ = size;
  }

  dst->SendCmd(this, TCP_TRANSFERCMD_TYPE, cmd);
}

bool BaseWorkerEx::TCPConnect(const std::string &ip, uint16_t port, BaseProcess *process) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }

  process->ex_ = this;
  return TCPConnector::Connect(ip, port, worker_->GetIOHandlerManager(), process);
}

bool BaseWorkerEx::TCPListen(const std::string &ip, uint16_t port, std::vector<BaseWorkerEx *> &other) {
  TCPAcceptor *acceptor = new TCPAcceptor(worker_, other, ip, port);
  if (!acceptor->Bind()/* || !acceptor->StartAccept()*/) {
    delete acceptor;
    FATAL("Listen %s %d falied", STR(ip), port);
    return false;
  }
  
  worker_->AddTCPAcceptor(acceptor);
  EventManager::GetManager()->SetupQueue(this, other, TCP_ACCEPTCMD_TYPE);
  return true;
}

void BaseWorkerEx::CreateUNIXTransfer(std::vector<BaseWorkerEx *> &other) {
  EventManager::GetManager()->SetupQueue(this, other, UNIX_TRANSFERCMD_TYPE); 
}

void BaseWorkerEx::UNIXTransfer(BaseWorkerEx *dst, const std::string &sun_path, BaseProcess *process) {
  UNIXTransferCmd *cmd = static_cast<UNIXTransferCmd *>(dst->GetCmd(this, UNIX_TRANSFERCMD_TYPE));
  if ( !cmd ) {
    cmd = new UNIXTransferCmd();
  }

  uint8_t *buffer = GETIBPOINTER(process->input_buffer_);
  uint32_t size = GETAVAILABLEBYTESCOUNT(process->input_buffer_);

  cmd->fd_         = dup(process->io_handler_->GetFd());
  cmd->unix_path_   = sun_path;
  if ( size ) {
    cmd->data_     = new uint8_t[size];
    memcpy(cmd->data_, buffer, size);
    cmd->data_len_ = size;
  }

  dst->SendCmd(this, UNIX_TRANSFERCMD_TYPE, cmd);
}

bool BaseWorkerEx::UNIXConnect(const std::string &unix_path, BaseProcess *process) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }

  process->ex_ = this;
  return UNIXConnector::Connect(unix_path, worker_->GetIOHandlerManager(), process);
}

bool BaseWorkerEx::UNIXListen(const std::string &unix_path, std::vector<BaseWorkerEx *> &other) {
  UNIXAcceptor *acceptor = new UNIXAcceptor(worker_, other, unix_path);
  if (!acceptor->Bind()/* || !acceptor->StartAccept()*/) {
    delete acceptor;
    FATAL("Listen %s falied", STR(unix_path));
    return false;
  }
  
  worker_->AddUNIXAcceptor(acceptor);
  EventManager::GetManager()->SetupQueue(this, other, UNIX_ACCEPTCMD_TYPE);
  return true;
}

bool BaseWorkerEx::UDPListenOrConnect(const std::string &ip, uint16_t port, BaseProcess *process, int32_t rcv_buf, int32_t snd_buf) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }

  process->ex_ = this;
  UDPCarrier *tmp = UDPCarrier::Create(ip, port, worker_->GetIOHandlerManager(), process, 256, 256, rcv_buf, snd_buf);
  if ( NULL == tmp ) {
    return false;
  }

  // worker_->AddUDPCarrier(tmp);
  return true;
}

bool BaseWorkerEx::UDPListenOrConnect(const BaseProcess *copy_process, BaseProcess *process) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }

  process->ex_ = this;
  UDPCarrier *tmp = UDPCarrier::Create((UDPCarrier *)copy_process->io_handler_, worker_->GetIOHandlerManager(), process);
  if ( NULL == tmp ) {
    return false;
  }

  // worker_->AddUDPCarrier(tmp);
  return true;
}

bool BaseWorkerEx::CreatePipe(const std::string &pipe_path) {  
  if ( fileExists(pipe_path) )
    deleteFile(pipe_path);

  if ( -1 == mkfifo(STR(pipe_path), S_IRUSR | S_IWUSR) ) {
    FATAL("mkfifo %s falied, errno is %d", STR(pipe_path), errno);
    return false;
  }
  return true;
}

bool BaseWorkerEx::OpenPipeForRead(const std::string &pipe_path, BaseProcess *process) {
  int fd = open(STR(pipe_path), O_RDONLY);
  if ( -1 == fd ) {
    FATAL("open pipe %s falied, errno is %d", STR(pipe_path), errno);
    return false;
  }

  if ( !setFdNonBlock(fd) ) {
    FATAL("setFdNonBlock pipe %s falied, errno is %d", STR(pipe_path), errno);
    close(fd);
    return false;
  }

  process->ex_ = this;
  /*PIPECarrier *p  = */new PIPECarrier(fd, worker_->GetIOHandlerManager(), process, pipe_path);
  //worker_->GetIOHandlerManager()->EnableWriteData(p);
  return true;
}

bool BaseWorkerEx::OpenPipeForWrite(const std::string &pipe_path, BaseProcess *process) {
  int fd = open(STR(pipe_path), O_WRONLY);
  if ( -1 == fd ) {
    FATAL("open pipe %s falied, errno is %d", STR(pipe_path), errno);
    return false;
  }
  
  if ( !setFdNonBlock(fd) ) {
    FATAL("setFdNonBlock pipe %s falied, errno is %d", STR(pipe_path), errno);
    close(fd);
    return false;
  }

  process->ex_ = this;
  PIPECarrier *p = new PIPECarrier(fd, worker_->GetIOHandlerManager(), process, pipe_path);
  worker_->GetIOHandlerManager()->EnableWriteData(p);
  return true;
}

BaseProcess *BaseWorkerEx::GetNextProcess(BaseProcess *process) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return NULL;
  }

  return worker_->GetNextProcess(process);
}

bool BaseWorkerEx::AddNetTimeoutEvent(TimeoutEvent *timer) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }
  worker_->AddNetTimeoutEvent(timer);
  return true;
}

bool BaseWorkerEx::RemoveNetTimeoutEvent(TimeoutEvent *timer) {
  if ( !worker_ ) {
    WARN("WorkerEx should be attached with a networker in GManager::AddWorker()");
    return false;
  }
  worker_->RemoveNetTimeoutEvent(timer);
  return true;
}



}

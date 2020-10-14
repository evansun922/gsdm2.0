#ifndef _BASEWORKEREX_H
#define _BASEWORKEREX_H

#include "platform/linuxplatform.h"
#include "utils/timersmanager.h"

namespace gsdm {

class NetWorker;
class EventManager;
class TCPAcceptor;
class UNIXAcceptor;
class BaseCmd;
class TCPAcceptCmd;
class UNIXAcceptCmd;
class BaseProcess;
class TCPTransferCmd;
class UNIXTransferCmd;

class BaseWorkerEx {
public:
friend class NetWorker;
friend class EventManager;
friend class TCPAcceptor;
friend class UNIXAcceptor;
friend class TCPAcceptCmd;
friend class UNIXAcceptCmd;
friend class BaseProcess;
friend class TCPTransferCmd;
friend class UNIXTransferCmd;

  BaseWorkerEx(uint32_t timeout = 60000);
  virtual ~BaseWorkerEx();

  virtual void Close() = 0;
  virtual bool Clear() = 0;

  virtual BaseProcess *CreateTCPProcess(uint16_t port);
  virtual BaseProcess *CreateUNIXProcess(const std::string &unix_path);
  
  /*
   * to->GetCmd(from, cmd_type)
   */
  BaseCmd *GetCmd(BaseWorkerEx *from, int cmd_type);
  void SendCmd(BaseWorkerEx *from, int cmd_type, BaseCmd *cmd);

  bool AddTimeoutManager(TimeoutManager *manager);
  bool RemoveTimeoutManager(TimeoutManager *manager);

  bool AddTimerEvent(TimerEvent *timer);
  bool RemoveTimerEvent(TimerEvent *timer);

  void CreateTCPTransfer(std::vector<BaseWorkerEx *> &other);
  void TCPTransfer(BaseWorkerEx *dst, uint16_t port, BaseProcess *process);

  bool TCPConnect(const std::string &ip, uint16_t port, BaseProcess *process);
  bool TCPListen(const std::string &ip, uint16_t port, std::vector<BaseWorkerEx *> &other);

  void CreateUNIXTransfer(std::vector<BaseWorkerEx *> &other);
  void UNIXTransfer(BaseWorkerEx *dst, const std::string &sun_path, BaseProcess *process);

  bool UNIXConnect(const std::string &unix_path, BaseProcess *process);
  bool UNIXListen(const std::string &sun_path, std::vector<BaseWorkerEx *> &other);

  /*
    if not set rcv_buf or snd_buf, they are zero
   */
  bool UDPListenOrConnect(const std::string &ip, uint16_t port, BaseProcess *process, int32_t rcv_buf/*byte*/, int32_t snd_buf);
  bool UDPListenOrConnect(const BaseProcess *copy_process, BaseProcess *process);

  bool CreatePipe(const std::string &pipe_path);
  bool OpenPipeForRead(const std::string &pipe_path, BaseProcess *process);
  bool OpenPipeForWrite(const std::string &pipe_path, BaseProcess *process);

  BaseProcess *GetNextProcess(BaseProcess *process);

private:
  bool AddNetTimeoutEvent(TimeoutEvent *timer);
  bool RemoveNetTimeoutEvent(TimeoutEvent *timer);
  
  NetWorker *worker_;
  uint32_t timeout_;
};

}

#endif // _BASEWORKEREX_H

#ifndef _NETWORKER_H
#define _NETWORKER_H

#include "platform/linuxplatform.h"
#include "utils/timersmanager.h"
#include "utils/container.h"

namespace gsdm {

class BaseProcess;
class IOHandlerManager;
class EventManager;
class BaseWorkerEx;
class BaseCmd;
class TCPAcceptor;
class UNIXAcceptor;
class UDPCarrier;

struct GsdmCmdInfo {
  uint32_t worker_id;
  int cmd_type;
};

class NetWorker {
public:
friend class EventManager;
friend class TCPAcceptor;
friend class UNIXAcceptor;
friend class IOHandlerManager;
  NetWorker(EventManager *manager, uint32_t id, int cpu, BaseWorkerEx *ex);
  ~NetWorker();

  static void *Loop(void *arg);
  void Run();
  void Clear();
  void Close();

  void SetupQueue(uint32_t src_worker_id, int cmd_type);
  BaseCmd *GetCmd(uint32_t worker_id, int cmd_type);
  void SendCmd(uint32_t worker_id, int cmd_type, BaseCmd *cmd);
  void ProcessCmd(GsdmCmdInfo *info);
  
  IOHandlerManager *GetIOHandlerManager();
  BaseWorkerEx *GetEx();
  uint32_t GetID();
  void AddTCPAcceptor(TCPAcceptor *acceptor);
  void AddUNIXAcceptor(UNIXAcceptor *acceptor);
  void AddUDPCarrier(UDPCarrier *carrier);
  void RemoveUDPCarrier(UDPCarrier *carrier);

  void AddTimeoutManager(TimeoutManager *manager);
  void RemoveTimeoutManager(TimeoutManager *manager);

  void AddTimerEvent(TimerEvent *timer);
  void RemoveTimerEvent(TimerEvent *timer);

  void AddNetTimeoutEvent(TimeoutEvent *timer);
  void RemoveNetTimeoutEvent(TimeoutEvent *timer);
  BaseProcess *GetNextProcess(BaseProcess *process);

private:

  enum WorkerStatus {
    WORKER_STATUS_RUN = 0,
    WORKER_STATUS_CLOSE,
    WORKER_STATUS_CLEAR,
    WORKER_STATUS_STOP,
  };
  
  WorkerStatus status_;
  EventManager *manager_;
  uint32_t id_;
  int cpu_;
  BaseWorkerEx *ex_;
  pthread_t thread_;
  IOHandlerManager *pIOHandlerManager_;
  TimersManager *timer_manager_;
  TimeoutManager *net_timeout_manager_;
  std::list<TCPAcceptor *> tcp_acceptor_list_;
  std::list<UNIXAcceptor *> unix_acceptor_list_;
  std::set<UDPCarrier *> udp_carrier_set_;

  typedef std::tr1::unordered_map<int, CycleList<BaseCmd>*> ListHash;
  typedef std::tr1::unordered_map<uint32_t, ListHash> WorkerQueue;
  WorkerQueue worker_queue_;
};

inline IOHandlerManager *NetWorker::GetIOHandlerManager() {
  return pIOHandlerManager_;
}

inline BaseWorkerEx *NetWorker::GetEx() {
  return ex_;
}

inline uint32_t NetWorker::GetID() {
  return id_;
}

inline void NetWorker::AddTCPAcceptor(TCPAcceptor *acceptor) {
  tcp_acceptor_list_.push_back(acceptor);
}

inline void NetWorker::AddUNIXAcceptor(UNIXAcceptor *acceptor) {
  unix_acceptor_list_.push_back(acceptor);
}

inline void NetWorker::AddUDPCarrier(UDPCarrier *carrier) {
  udp_carrier_set_.insert(carrier);
}

inline void NetWorker::RemoveUDPCarrier(UDPCarrier *carrier) {
  udp_carrier_set_.erase(carrier);
}

inline void NetWorker::AddTimeoutManager(TimeoutManager *manager) {
  timer_manager_->AddTimeoutManager(manager);
}

inline void NetWorker::RemoveTimeoutManager(TimeoutManager *manager) {
  timer_manager_->RemoveTimeoutManager(manager);
}

inline void NetWorker::AddTimerEvent(TimerEvent *timer) {
  timer_manager_->AddTimerEvent(timer);
}

inline void NetWorker::RemoveTimerEvent(TimerEvent *timer) {
  timer_manager_->RemoveTimerEvent(timer);
}

inline void NetWorker::AddNetTimeoutEvent(TimeoutEvent *timer) {
  net_timeout_manager_->AddTimeoutEvent(timer);
}

inline void NetWorker::RemoveNetTimeoutEvent(TimeoutEvent *timer) {
  net_timeout_manager_->RemoveTimeoutEvent(timer);
}

inline BaseProcess *NetWorker::GetNextProcess(BaseProcess *process) {
  return (BaseProcess *)net_timeout_manager_->GetNextTimeoutEvent((TimeoutEvent *)process);
};

}

#endif // _NETWORKER_H

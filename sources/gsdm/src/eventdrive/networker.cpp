#include "eventdrive/baseprocess.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/basecmd.h"
#include "eventdrive/manager.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/epoll/tcpacceptor.h"
#include "eventdrive/epoll/unixacceptor.h"
#include "eventdrive/epoll/udpcarrier.h"
#include "eventdrive/networker.h"
#include "logging/logger.h"

namespace gsdm {

extern __thread int g_log_fd;

NetWorker::NetWorker(EventManager *manager, uint32_t id, int cpu, BaseWorkerEx *ex)
  :status_(WORKER_STATUS_RUN),manager_(manager),id_(id),cpu_(cpu),ex_(ex),thread_((pthread_t)-1),pIOHandlerManager_(NULL) {
  timer_manager_ = new TimersManager();
  net_timeout_manager_ = new TimeoutManager(ex->timeout_);
  timer_manager_->AddTimeoutManager(net_timeout_manager_);
  pIOHandlerManager_ = new IOHandlerManager(this);
}

NetWorker::~NetWorker() {
  for ( std::list<TCPAcceptor *>::iterator i = tcp_acceptor_list_.begin(); i != tcp_acceptor_list_.end(); ++i ) {
    pIOHandlerManager_->DisableReadData(*i);
    delete *i;
  }
  tcp_acceptor_list_.clear();
  
  for ( std::list<UNIXAcceptor *>::iterator i = unix_acceptor_list_.begin(); i != unix_acceptor_list_.end(); ++i ) {
    pIOHandlerManager_->DisableReadData(*i);
    delete *i;
  }
  unix_acceptor_list_.clear();

  std::set<UDPCarrier *> udp_carrier_set = udp_carrier_set_;
  for ( std::set<UDPCarrier *>::iterator i = udp_carrier_set.begin(); i != udp_carrier_set.end(); ++i ) {
    pIOHandlerManager_->DisableReadData(*i);
    delete *i;
  }

  if ( timer_manager_ ) {
    delete timer_manager_;
    timer_manager_ = NULL;
  }

  WorkerQueue::iterator wq;
  ListHash::iterator lh;
  FOR_UNORDERED_MAP(worker_queue_, uint32_t, ListHash, wq) {
    FOR_UNORDERED_MAP(wq->second, int, CycleList<BaseCmd>*, lh) {
      delete MAP_VAL(lh);
    }
  }
  worker_queue_.clear();

  if ( ex_ ) {
    delete ex_;
    ex_ = NULL;
  }

  if ( pIOHandlerManager_ ) {
    pIOHandlerManager_->Shutdown();
    delete pIOHandlerManager_;
    pIOHandlerManager_ = NULL;
  }

}

void *NetWorker::Loop(void *args) {
  ((NetWorker *)args)->Run();
  return NULL;
}

void NetWorker::Run() {
  pIOHandlerManager_->Initialize();
  thread_ = pthread_self();
  FATAL("networker %u is run-net in cpu %d", id_, cpu_);

  if (cpu_ >= 0) {
    std::vector<int> cpus;
    cpus.push_back(cpu_);
    if ( !setaffinityNp(cpus, thread_) ) {
      FATAL("ThreadID:%d cpuID:%d setaffinityNp failed.", id_, cpu_);
      exit(0);
    }
  }

  for (std::list<TCPAcceptor *>::iterator i = tcp_acceptor_list_.begin(); i != tcp_acceptor_list_.end(); ++i) {
    (*i)->SetManager(pIOHandlerManager_);
    (*i)->StartAccept();
  }

  for (std::list<UNIXAcceptor *>::iterator i = unix_acceptor_list_.begin(); i != unix_acceptor_list_.end(); ++i) {
    (*i)->SetManager(pIOHandlerManager_);
    (*i)->StartAccept();
  }

  // for (std::set<UDPCarrier *>::iterator i = udp_carrier_set_.begin(); i != udp_carrier_set_.end(); ++i) {
  //   (*i)->SetManager(pIOHandlerManager_);
  //   pIOHandlerManager_->EnableReadData(*i);
  // }

  // WorkerQueue::iterator wq;
  // ListHash::iterator lh;
  // BaseCmd *tmp;
  // gsdm_msec_t sleep_time = getCurrentGsdmMsec();
  gsdm_msec_t sleep_time = 1000;
  while( WORKER_STATUS_STOP != status_ && (sleep_time = pIOHandlerManager_->Pulse(sleep_time)) ) {

    // FOR_UNORDERED_MAP(worker_queue_, uint32_t, ListHash, wq) {
    //   FOR_UNORDERED_MAP(wq->second, int, CycleList<BaseCmd>*, lh) {
    //     while ( (tmp = lh->second->GetData()) ) {
    //       tmp->Process(ex_);
    //     }
    //   }
    // }

    if ( WORKER_STATUS_CLOSE == status_ ) {
      // sleep_time = getCurrentGsdmMsec();
      sleep_time = 5;
      ex_->Close();
      timer_manager_->Close();
      status_ = WORKER_STATUS_CLEAR;
    } else if ( WORKER_STATUS_CLEAR == status_ ) {
      // sleep_time = getCurrentGsdmMsec();
      sleep_time = 5;
      if ( ex_->Clear() ) {
        FATAL("networker %u is run-net in cpu %d stop", id_, cpu_);
        status_ = WORKER_STATUS_STOP;
        break;
      }
    }
  }

  // ex_->Close();
}

void NetWorker::Clear() {
  if ((pthread_t)-1 != thread_)
    pthread_join(thread_, NULL);
}

void NetWorker::Close() {
  if ( pIOHandlerManager_ ) {
    status_ = WORKER_STATUS_CLOSE;
    GsdmCmdInfo info = { (uint32_t)-1, -1 };
    pIOHandlerManager_->Post((uint8_t *)&info, sizeof(GsdmCmdInfo));
  }
}

void NetWorker::SetupQueue(uint32_t src_worker_id, int cmd_type) {
  if ( !MAP_HAS1(worker_queue_[src_worker_id], cmd_type) ) {
    BaseCmd *head = new BaseCmd(true);
    BaseCmd *tail = new BaseCmd(true);
    CycleList<BaseCmd> *cl = new CycleList<BaseCmd>(head, tail);
    worker_queue_[src_worker_id][cmd_type] = cl;
  }
}

BaseCmd *NetWorker::GetCmd(uint32_t worker_id, int cmd_type) {
  ListHash &lh = worker_queue_[worker_id];
  ListHash::iterator i = lh.find(cmd_type);
  if ( lh.end() == i )
    return NULL;
  
  return i->second->GetCycle();
}

void NetWorker::SendCmd(uint32_t worker_id, int cmd_type, BaseCmd *cmd) {
  ListHash &lh = worker_queue_[worker_id];
  ListHash::iterator i = lh.find(cmd_type);
  if ( lh.end() == i )
    return;

  i->second->Insert(cmd);
  GsdmCmdInfo info = { worker_id, cmd_type };
  pIOHandlerManager_->Post((uint8_t *)&info, sizeof(GsdmCmdInfo));
}

void NetWorker::ProcessCmd(GsdmCmdInfo *info) {
  if ( (uint32_t)-1 == info->worker_id ) {
    return;
  }

  ListHash &lh = worker_queue_[info->worker_id];
  ListHash::iterator i = lh.find(info->cmd_type);
  if ( lh.end() == i )
    return;

  BaseCmd *tmp;
  while ( (tmp = i->second->GetData()) ) {
    tmp->Process(ex_);
  }
}

}

#include "common.h"
#include "baseworkerex.h"
#include "netmanager.h"
#include "baseprocess.h"



class EchoProcess : public BaseProcess {
public:
  EchoProcess(BaseWorkerEx *ex):BaseProcess(ex)  { }
  virtual ~EchoProcess() { DetachNet(); STAT("Free EchoProcess (%s)", STR(GetNetStatusStr())); }
  
  // virtual void FreeTimeout() { FreeProcess(); STAT("EchoProcess FreeTimeout"); }
  virtual void DoTimeout() { BaseProcess::DoTimeout(); STAT("EchoProcess timeout %ld", heart_); }


  virtual bool SignalInputData(int32_t recv_amount) {
    std::string v((char*)GETIBPOINTER(input_buffer_), GETAVAILABLEBYTESCOUNT(input_buffer_));
    trim(v);
    STAT("%s",STR(v));

    if ( "AAA" == v ) {
      return false;
    }

    if ( "BBB" == v ) {
      ex_->TCPConnect("127.0.0.1", 5555, new EchoProcess(ex_));
    }

    if ( "CCC" == v ) {
      ex_->UNIXConnect("/tmp/ccc.unix", new EchoProcess(ex_));
    }

    SendMsg(GETIBPOINTER(input_buffer_), GETAVAILABLEBYTESCOUNT(input_buffer_));
    input_buffer_.IgnoreAll();

    UpdateTimeout();
    return true;
  }

  // virtual void AttachNet() {
  //   ex_->AddTimeoutEvent(this);
  // }

  static BaseProcess *CreateEchoProcess(BaseWorkerEx *ex, uint16_t port) {
    WARN("prot %u", port);
    EchoProcess *ep = new EchoProcess(ex);   
    return ep;
  }

  static BaseProcess *CreateEchoProcess2(BaseWorkerEx *ex, const std::string &unix_path) {
    WARN("unix_path %s", STR(unix_path));
    EchoProcess *ep = new EchoProcess(ex);   
    return ep;
  }

};


class Connect5555;
class MyEx : public BaseWorkerEx {
public:
  MyEx();
  virtual ~MyEx();
  
  virtual void Do();
  virtual void Close();
  virtual bool Clear();


  Connect5555 * c5;
  int connect_count;
};


class ConnectEchoProcess : public BaseProcess {
public:
  ConnectEchoProcess(BaseWorkerEx *ex):BaseProcess(ex)  { }
  virtual ~ConnectEchoProcess() { 
    STAT("Free ConnectEchoProcess (%s)", STR(GetNetStatusStr())); 
    ((MyEx*)ex_)->connect_count++;
  }
  

  // virtual void DoTimeout() { BaseProcess::DoTimeout(); STAT("EchoProcess timeout %ld", heart_); }


  virtual bool SignalInputData(int32_t recv_amount) {
    std::string v((char*)GETIBPOINTER(input_buffer_), GETAVAILABLEBYTESCOUNT(input_buffer_));
    trim(v);
    STAT("%s",STR(v));

    if ( "AAA" == v ) {
      return false;
    }

    // if ( "BBB" == v ) {
    //   ex_->TCPConnect("127.0.0.1", 5555, new EchoProcess(ex_));
    // }

    SendMsg(GETIBPOINTER(input_buffer_), GETAVAILABLEBYTESCOUNT(input_buffer_));
    input_buffer_.IgnoreAll();

    //UpdateTimeout();
    return true;
  }

  virtual void AttachNet() {
    STAT("I AttachNet 5555");
  }

  virtual void DetachNet() {
    STAT("I DetachNet 5555");
  }

  virtual void ConnectSuccess() {
    STAT("I ConnectSuccess 5555");
  }

  // static BaseProcess *CreateEchoProcess(BaseWorkerEx *ex) {
  //   EchoProcess *ep = new EchoProcess(ex);   
  //   return ep;
  // }

};

class Connect5555 : public TimerEvent {
public:
  Connect5555(BaseWorkerEx *ex) : TimerEvent(3){ process_ = new ConnectEchoProcess(ex); process_->SetWithMe(false); }
  virtual ~Connect5555(){ process_->Free(); }
  
  virtual void Process() {
    if ( BaseProcess::PROCESS_NET_STATUS_CONNECTING != process_->GetNetStatus() &&
         BaseProcess::PROCESS_NET_STATUS_CONNECTED != process_->GetNetStatus() ) {
      process_->GetEx()->UNIXConnect("/tmp/ccc.unix", process_);
    }
  }

  
  ConnectEchoProcess *process_;
};



MyEx::MyEx() : BaseWorkerEx(WORKER_RUN_MODE_NET,30),connect_count(0) { c5 = NULL; }
MyEx::~MyEx() { }

void MyEx::Do() { }
void MyEx::Close(){ if (c5) { RemoveTimerEvent(c5); delete c5; c5=NULL; } } 
bool MyEx::Clear() { STAT("MyEx Clear"); return 1 == connect_count; }

int main(int argc, char *argv[]) {
  if ( !NetManager::GetManager()->Initialize(argc, argv, NULL,"a") ) {
    return 0;
  }



  MyEx *ex1 = new MyEx();
  NetManager::GetManager()->AddWorker(NULL, NULL, ex1);
  MyEx *ex2 = new MyEx();
  NetManager::GetManager()->AddWorker(EchoProcess::CreateEchoProcess, EchoProcess::CreateEchoProcess2, ex2);

  std::vector<BaseWorkerEx *> other;
  other.push_back(ex2);
  ex1->UNIXListen("/tmp/abc.unix", other);
  ex1->TCPListen("127.0.0.1", 5555, other);

  // ex2->c5 = new Connect5555(ex2);
  // ex2->AddTimerEvent(ex2->c5);
  ex1->connect_count = 1;
  ex2->connect_count = 1;  

  NetManager::GetManager()->Loop();
  NetManager::GetManager()->Close();
  return 0;
}

#include <gsdm.h>


using namespace gsdm;

class EchoProcess : public BaseProcess {
public:
  EchoProcess(BaseWorkerEx *ex):BaseProcess(ex, 4096, 4096)  { }
  virtual ~EchoProcess() { DetachNet(); STAT("Free EchoProcess (%s)", STR(GetNetStatusStr())); }
  
  // virtual void FreeTimeout() { FreeProcess(); STAT("EchoProcess FreeTimeout"); }
  virtual void DoTimeout() { BaseProcess::DoTimeout(); STAT("EchoProcess timeout %ld", heart_); }


  virtual bool SignalInputData(int32_t recv_amount) {
    std::string v((char*)GETIBPOINTER(input_buffer_), GETAVAILABLEBYTESCOUNT(input_buffer_));
    trim(v);
    //    STAT("%s",STR(v));

    gsdm_msec_t tv = gsdm_current_msec;
    v = format("%s\n%s  %lu\n", STR(getPrivateIP()), STR(v), tv);

    // MAIL("Test", v);

    SendMsg((uint8_t *)STR(v), v.length());

    input_buffer_.IgnoreAll();

    UpdateTimeout();
    return true;
  }

  // virtual void AttachNet() {
  //   ex_->AddTimeoutEvent(this);
  // }



};



class MyEx : public BaseWorkerEx {
public:
  MyEx();
  virtual ~MyEx();
  
  virtual void Close();
  virtual bool Clear();
  virtual BaseProcess *CreateTCPProcess(uint16_t port);


};






MyEx::MyEx() : BaseWorkerEx(30000) { }
MyEx::~MyEx() { }

void MyEx::Close(){ } 
bool MyEx::Clear() { return true; }

BaseProcess *MyEx::CreateTCPProcess(uint16_t port) {
  EchoProcess *ep = new EchoProcess(this);
  return ep;
}




int main(int argc, char *argv[]) {

  if ( !GManager::GetManager()->Initialize(argc, argv, NULL,"a") ) {
    return 0;
  }



  // MyEx *ex1 = new MyEx();
  // NetManager::GetManager()->AddWorker(NULL, ex1);
  MyEx *ex2 = new MyEx();
  GManager::GetManager()->AddWorker(ex2);

  std::vector<BaseWorkerEx *> other;
  other.push_back(ex2);
  ex2->TCPListen("0.0.0.0", 3333, other);

  GSDMFile file;
  file.Initialize("./unix.cpp");
  std::string str;
  file.ReadAll(str);
  file.Close();
  std::string v = md5(str, true);
  
  DEBUG("md5: %s", STR(v));


  GManager::GetManager()->Loop();
  GManager::GetManager()->Close();
  return 0;
}

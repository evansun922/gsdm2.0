#include "gsdm/common.h"


class TimerEventA : public TimerEvent {
public:
  TimerEventA() : TimerEvent(1000){ }
  virtual ~TimerEventA(){ }

  virtual void Process() { 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t msec = ((uint64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    int s = rand()%20;
    STAT("TimerEventA  %lu  sleep %d", msec, s);
    WARN("TimerEventA  %lu  sleep %d", msec, s); 
    //gsdm_sleep(s);
  }
};

class TimerEventB : public TimerEvent {
public:
  TimerEventB() : TimerEvent(3000){ }
  virtual ~TimerEventB(){ }

  virtual void Process() { 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t msec = ((uint64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    int s = rand()%20;
    STAT("TimerEventB  %lu  sleep %d", msec, s); 
    WARN("TimerEventB  %lu  sleep %d", msec, s); 
    //gsdm_sleep(s);
  }
};


class MyEx : public BaseWorkerEx {
public:
  MyEx() : BaseWorkerEx(15000) { }
  virtual ~MyEx() { }
  
  virtual void Close(){ } 
  virtual bool Clear() { return true; }

};


class TimeoutEventA : public TimeoutEvent {
public:
  TimeoutEventA(int id):id_(id)  { }
  virtual ~TimeoutEventA() { }
  
  virtual void Free() { delete this; STAT("%d free", id_); }
  virtual void DoTimeout() { STAT("%d timeout %ld", id_, heart_/1000); }

  int id_;
};

TimeoutEventA *tea1,*tea2, *tea3;
class TimerEventD;
TimerEventD *tpd;
MyEx *ex;

class TimerEventD : public TimerEvent {
public:
  TimerEventD() : TimerEvent(5000){ }
  virtual ~TimerEventD(){ }

  virtual void Process() { 
    tea1->UpdateTimeout();
    tea2->UpdateTimeout();
    tea3->UpdateTimeout();
    ex->RemoveTimerEvent(tpd);
  }
};



int main(int argc, char *argv[]) {
  srand(time(0));
  
  if ( !NetManager::GetManager()->Initialize(argc, argv, NULL, "a") ) {
    return 0;
  }

  ex = new MyEx();
  NetManager::GetManager()->AddWorker(NULL, NULL, ex);

  TimerEventA *tpa = new TimerEventA();
  ex->AddTimerEvent(tpa);

  MyEx *ex1 = new MyEx();
  NetManager::GetManager()->AddWorker(NULL, NULL, ex1);

  TimerEventB *tpb = new TimerEventB();
  ex1->AddTimerEvent(tpb);
  // TimerEventC *tpc = new TimerEventC();
  // ex->AddTimerEvent(tpc);


  // tpd = new TimerEventD();
  // ex->AddTimerEvent(tpd);


  // for (int i = 0; i < 10; i++) {
  //   TimeoutEventA *tea = new TimeoutEventA(i);
  //   ex->AddTimeoutEvent(tea);
  //   printf("A%d\n",i);
  //   if (3 == i)
  //     tea1 = tea;
  //   else if (4 == i)
  //     tea2 = tea;
  //   else if(5 == i)
  //     tea3 = tea;
  //   //sleep(2);
  // }
  
  NetManager::GetManager()->Loop();
  NetManager::GetManager()->Close();
  return 0;
}

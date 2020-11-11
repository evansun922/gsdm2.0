#ifndef _TIMERSMANAGER_H
#define _TIMERSMANAGER_H


#include "platform/linuxplatform.h"


namespace gsdm {

class TimersManager;
class TimeoutManager;

class TimeoutEvent {
public:
friend class TimeoutManager;
  TimeoutEvent();
  virtual ~TimeoutEvent();

  /*  when it is timeout, what it should do */
  virtual bool DoTimeout();
  /* free myslef */
  virtual void Free() = 0;
  bool Check(gsdm_msec_t now, gsdm_msec_t timeout);
  void UpdateTimeout();
  uint64_t GetLastHeart() { return heart_; }
  TimeoutManager *GetTimeoutManager() { return manager_; }

protected:
  gsdm_msec_t heart_;

private:
  TimeoutEvent *prev_;
  TimeoutEvent *next_;
  bool is_add_;
  TimeoutManager *manager_;
};

class TimeoutManager {
public:
friend class TimersManager;
  TimeoutManager(gsdm_msec_t timeout);
  virtual ~TimeoutManager();
  
  void AddTimeoutEvent(TimeoutEvent *timer);
  void RemoveTimeoutEvent(TimeoutEvent *timer);
  void ReaddTimeoutEvent(TimeoutEvent *timer);
  TimeoutEvent *GetTimeoutHead();
  uint32_t GetTimeoutCount();
  TimeoutEvent *GetNextTimeoutEvent(TimeoutEvent *timer);

private:
  void TimeElapsedTimeout();

  TimeoutEvent *head_;
  TimeoutEvent *tail_;
  uint32_t count_;
  // gsdm_msec_t timeout_last_;
  gsdm_msec_t timeout_;
  uint32_t myself_id_;
  TimersManager *manager_;
};

class TimerEvent {
public:
friend class TimersManager;

  TimerEvent(uint32_t period);
  virtual ~TimerEvent();
  virtual void Process() = 0;

private:
  uint32_t id_;
  uint32_t period_;
};

class TimersManager {
public:
  TimersManager();
  ~TimersManager();
 
  void AddTimeoutManager(TimeoutManager *timeout_manager);
  void RemoveTimeoutManager(TimeoutManager *timeout_manager);
  void AddTimerEvent(TimerEvent *timer);
  void RemoveTimerEvent(TimerEvent *timer);

  // return millisecond of min
  uint32_t TimeElapsed();
  void Close();

private:
  void TimeElapsedTimeout();
  uint32_t TimeElapsedProcess();

  uint32_t process_id_;
  typedef std::multimap<gsdm_msec_t, TimerEvent *> Slot;
  Slot slots_;  

  typedef std::unordered_map<uint32_t, TimeoutManager *> TimeoutManagerHash;
  TimeoutManagerHash timeout_manager_hash_;
};



}


#endif // _TIMERSMANAGER_H

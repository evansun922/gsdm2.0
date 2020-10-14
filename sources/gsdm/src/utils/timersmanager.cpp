#include "utils/timersmanager.h"
#include "logging/logger.h"

namespace gsdm {

TimeoutEvent::TimeoutEvent() 
  : heart_(getCurrentGsdmMsec()),prev_(NULL),next_(NULL),is_add_(false),manager_(NULL) {

}

TimeoutEvent::~TimeoutEvent() {
  prev_ = next_ = NULL;
  is_add_ = false;
  manager_ = NULL;
}

bool TimeoutEvent::DoTimeout() {
  WARN("Time out, default to do");
  return true;
}

bool TimeoutEvent::Check(gsdm_msec_t now, gsdm_msec_t timeout) {
  if (timeout + heart_ >= now) {
    return true;
  }

  return false;

  // return timeout >= (now - heart_);
}

void TimeoutEvent::UpdateTimeout() {
  // gsdm_msec_t now = getCurrentGsdmMsec();
  // if ( now == heart_ )
  //   return;
  // heart_ = now;
  if (manager_) {
    heart_ = getCurrentGsdmMsec();
    manager_->ReaddTimeoutEvent(this);
  } else {
    WARN("TimeoutEvent::UpdateTimeout, but manager_ is NULL.");
  }
}

TimeoutManager::TimeoutManager(gsdm_msec_t timeout) :head_(NULL),tail_(NULL),count_(0),timeout_(timeout),myself_id_(0),manager_(NULL) {
  
}

TimeoutManager::~TimeoutManager() {
  while( head_ ) {
    TimeoutEvent *tmp = head_->next_;
    head_->Free();
    head_ = tmp;
  }
  tail_ = NULL;
}

void TimeoutManager::AddTimeoutEvent(TimeoutEvent *timer) {
  if ( true == timer->is_add_ )
    return;

  count_++;
  timer->is_add_ = true;
  timer->manager_ = this;
  if (!head_)   {
    head_ = timer;
    tail_ = timer;
    return;
  }

  timer->prev_ = tail_;
  tail_->next_ = timer;
  tail_ = timer;
}

void TimeoutManager::RemoveTimeoutEvent(TimeoutEvent *timer) {
  if ( false == timer->is_add_ )
    return;

  count_--;
  timer->is_add_ = false;
  timer->manager_ = NULL;
  if (timer->prev_ && timer->next_) {
    timer->prev_->next_ = timer->next_;
    timer->next_->prev_ = timer->prev_;
    timer->next_ = NULL;
    timer->prev_ = NULL;
    return;
  }

  if (!timer->prev_ && !timer->next_) {
    head_ = tail_ = NULL;
    return;
  }

  if (!timer->prev_) {
    timer->next_->prev_ = NULL;
    head_ = timer->next_;
    timer->next_ = NULL;
    return;
  }

  // if (!timer->_next)
  timer->prev_->next_ = NULL;
  tail_ = timer->prev_;
  timer->prev_ = NULL;
  return;  
}

void TimeoutManager::ReaddTimeoutEvent(TimeoutEvent *timer) {
  RemoveTimeoutEvent(timer);
  AddTimeoutEvent(timer);
}

TimeoutEvent *TimeoutManager::GetTimeoutHead() {
  return head_;
}

uint32_t TimeoutManager::GetTimeoutCount() {
  return count_;
}

TimeoutEvent *TimeoutManager::GetNextTimeoutEvent(TimeoutEvent *timer) {
  if ( timer )
    return timer->next_;
  return head_;
}

void TimeoutManager::TimeElapsedTimeout() {
  TimeoutEvent *tmp = NULL;
  gsdm_msec_t now = getCurrentGsdmMsec();
  while( head_ ) {
    if ( !head_->Check(now, timeout_) ) {
      tmp = head_;
      head_ = head_->next_;
      tmp->is_add_ = false;
      tmp->prev_ = NULL;
      tmp->next_ = NULL;
      count_--;
      
      if ( tmp->DoTimeout() ) {
        tmp->Free();
      } else {
        tmp->UpdateTimeout();
      }
      continue;
    }
    break;
  }
  
  if (!head_)
    tail_ = NULL;
  else
    head_->prev_ = NULL;
}

TimerEvent::TimerEvent(uint32_t period) : id_(0),period_(period) {

}

TimerEvent::~TimerEvent() {

}

TimersManager::TimersManager() {
  process_id_ = 0;
}

TimersManager::~TimersManager() {
  Close();
}

void TimersManager::AddTimeoutManager(TimeoutManager *timeout_manager) {
  timeout_manager->manager_ = this;
  timeout_manager->myself_id_ = ++process_id_;
  timeout_manager_hash_[timeout_manager->myself_id_] = timeout_manager;
}

void TimersManager::RemoveTimeoutManager(TimeoutManager *timeout_manager) {
  timeout_manager->manager_ = NULL;
  timeout_manager_hash_.erase(timeout_manager->myself_id_);
}

void TimersManager::AddTimerEvent(TimerEvent *timer) {
  timer->id_ = ++process_id_;
  slots_.insert(std::pair<uint64_t, TimerEvent *>(getCurrentGsdmMsec(), timer));
}

void TimersManager::RemoveTimerEvent(TimerEvent *timer) {
  for(Slot::iterator i = slots_.begin(); i != slots_.end(); ++i) {
    if ( timer->id_ == MAP_VAL(i)->id_) {
      slots_.erase(i);
      break;
    }
  }
}

uint32_t TimersManager::TimeElapsed() {
  TimeElapsedTimeout();
  return TimeElapsedProcess();
}

void TimersManager::TimeElapsedTimeout() {
  FOR_UNORDERED_MAP(timeout_manager_hash_, uint32_t, TimeoutManager *, i) {
    MAP_VAL(i)->TimeElapsedTimeout();
  }
}

uint32_t TimersManager::TimeElapsedProcess() {
  uint64_t now = getCurrentGsdmMsec();
  for(Slot::iterator i = slots_.begin(); i != slots_.end(); ) {    
    if ( now >= MAP_KEY(i) ) {
      MAP_VAL(i)->Process();
      slots_.insert(std::pair<uint64_t, TimerEvent *>(now + MAP_VAL(i)->period_, MAP_VAL(i)));
      slots_.erase(i++);
      continue;
    }

    return (uint32_t)(MAP_KEY(i) - now);
  }

  return 1000;
}

void TimersManager::Close() {
  FOR_UNORDERED_MAP(timeout_manager_hash_, uint32_t, TimeoutManager *, i) {
    delete MAP_VAL(i);
  }
  timeout_manager_hash_.clear();

  for(Slot::iterator i = slots_.begin(); i != slots_.end(); ++i) {
    delete MAP_VAL(i);
  }
  slots_.clear();
}




}

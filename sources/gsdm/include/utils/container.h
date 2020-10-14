#ifndef _GSDMCONTAINER_H
#define _GSDMCONTAINER_H

namespace gsdm {

template<class T>
class CycleList {
public:
  CycleList(T *head, T *cycle_head) {
    head_ = head;
    tail_ = NULL;
    cycle_head_ = cycle_head;
    cycle_tail_ = NULL;
  }
  
  ~CycleList() {
    T *tmp;
    while( head_ ) {
      tmp = head_->next;
      delete head_;
      head_ = tmp;
    }
    tail_ = NULL;

    while( cycle_head_ ) {
      tmp = cycle_head_->next;
      delete cycle_head_;
      cycle_head_ = tmp;
    }
    cycle_tail_ = NULL;
  }
  
  void Insert(T *data) {
    if ( tail_ ) {
      tail_->next = data;
      tail_ = data;
    } else {
      // first
      tail_ = data;
      head_->next = tail_;
    }
  }

  T *GetData() {
    if ( head_->next ) {
      T *tmp = head_->next;

      head_->next = NULL;
      if ( head_->is_free ) {
        delete head_;
      } else {
        if ( cycle_tail_ ) {
          cycle_tail_->next = head_;
          cycle_tail_ = head_;
        } else {
          // first
          cycle_tail_ = head_;
          cycle_head_->next = cycle_tail_;
        }
      }

      head_ = tmp;
      return tmp;
    }
    
    return NULL;
  }

  T *GetCycle() {
    while ( cycle_head_->next ) {
      T *tmp = cycle_head_;
      cycle_head_ = cycle_head_->next;
      tmp->next = NULL;
      if ( tmp->is_free ) {
        delete tmp;
        continue;
      }
      return tmp;
    }

    return NULL;
  }

private:
  
  T *head_, *tail_;
  T *cycle_head_, *cycle_tail_;
};




template<class T>
class DoublyList {
public:
  DoublyList() {
    head_ = NULL;
    count_ = 0;
  }
  
  ~DoublyList() {
    while( head_ ) {
      T *tmp = head_;
      Remove(tmp);
      delete tmp;
    }
  }
  
  bool Insert(T *data) {
    count_++;
    if ( !head_ ) {
      head_ = data;
      head_->prev = data;
      head_->next = data;
      return true;
    }
    
    head_->prev->next = data;
    data->next = head_;

    data->prev = head_->prev;
    head_->prev = data;    
    return true;
  }
  
  bool Remove(T *data) {
    if ( !data->next || !data->prev ) {
      return false;
    }

    count_--;
    if ( data == data->next ) {
      // only one
      data->next = NULL;
      data->prev = NULL;
      head_ = NULL;
      return true;
    }
    
    if ( data == head_ )
      head_ = head_->next;

    data->prev->next = data->next;
    data->next->prev = data->prev;

    data->next = NULL;
    data->prev = NULL;
    return true;
  }
  
  int GetCount() {
    return count_;
  }

  T *PrevShift(int32_t count) {
    if ( 0 < count && head_ ) {
      while( count-- ) {
        head_ = head_->prev;
      }
    }

    return head_;
  }

  T *NextShift(int32_t count) {
    if ( 0 < count && head_ ) {
      while( count-- ) {
        head_ = head_->next;
      }
    }
    
    return head_;
  }

  T *head_;
  int count_;
};

template<class T>
class BidirectionalList {
public:
  BidirectionalList() {
    head_ = NULL;
    tail_ = NULL;
  }
  
  ~BidirectionalList() {
    
  }
  
  void Add(T *data) {
    if ( data->is_add )
      return;
    data->is_add = true;

    if ( !head_ )   {
      head_ = data;
      tail_ = data;
      return;
    }
    
    data->prev = tail_;
    tail_->next = data;
    tail_ = data;
  }
 
  void Remove(T *data) {
    if ( false == data->is_add )
      return;
    data->is_add = false;

    if ( data->prev && data->next ) {
      data->prev->next = data->next;
      data->next->prev = data->prev;
      data->next = NULL;
      data->prev = NULL;
      return;
    }

    if ( !data->prev && !data->next ) {
      head_ = tail_ = NULL;
      return;
    }

    if ( !data->prev ) {
      data->next->prev = NULL;
      head_ = data->next;
      data->next = NULL;
      return;
    }


    data->prev->next = NULL;
    tail_ = data->prev;
    data->prev = NULL;
  }

  void Readd(T *data) {
    Remove(data);
    Add(data);
  }

  T *head_, *tail_; 
};




}


#endif

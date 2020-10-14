/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */

#ifndef _BASEPROCESS_H
#define	_BASEPROCESS_H

#include "platform/linuxplatform.h"
#include "utils/timersmanager.h"
#include "buffering/iobuffer.h"

namespace gsdm {

class IOHandler;
class TCPCarrier;
class UNIXCarrier;
class UDPCarrier;
class TCPConnector;
class UNIXConnector;
class PIPECarrier;
class THREADCarrier;
class BaseWorkerEx;
class TCPAcceptCmd;
class UNIXAcceptCmd;
class TCPTransferCmd;
class UNIXTransferCmd;

class BaseProcess : public TimeoutEvent {

public:
friend class IOHandler;
friend class TCPCarrier;
friend class UNIXCarrier;
friend class UDPCarrier;
friend class TCPConnector;
friend class UNIXConnector;
friend class PIPECarrier;
friend class THREADCarrier;
friend class BaseWorkerEx;
friend class TCPAcceptCmd;
friend class UNIXAcceptCmd;
friend class TCPTransferCmd;
friend class UNIXTransferCmd;

	BaseProcess(int32_t recv_buffer_size, int32_t send_buffer_size);
	virtual ~BaseProcess();

  enum ProcessNetStatus {
    PROCESS_NET_STATUS_DISCONNECT = 0,
    PROCESS_NET_STATUS_CONNECTING,
    PROCESS_NET_STATUS_CONNECTED,
    PROCESS_NET_STATUS_PEER_RESET,
    PROCESS_NET_STATUS_SYS_ERROR,
    PROCESS_NET_STATUS_TIMEOUT,
    
    PROCESS_NET_STATUS_COUNT,
  };
  
  void Free();

  /* TimeoutEvent end */

  /*!
    @brief Gets the input buffers
  */
  IOBuffer *GetInputBuffer();
  
  /*!
    @brief Gets the output buffers
  */
  IOBuffer *GetOutputBuffer();

  /*!
    @brief This is called by the framework when data is available for processing, directly from the network i/o layer
    @param recvAmount
    @discussion This function must be implemented by the class that inherits this class
  */
  virtual bool Process(int32_t recv_amount);

  /*
    default to add that checking timeout
   */
  virtual void AttachNet();
  /*
    default to add that checking timeout
  */  
  virtual void DetachNet();
    
  virtual bool IsCloseNet();
  
  /* TimeoutEvent */
  virtual bool DoTimeout();

  virtual bool SendMsg(const void *data, uint32_t len);

  virtual bool SendToMsg(const void *data, uint32_t len, sockaddr_in *peer_address);
  
  virtual bool SendToMsg(const void *data, uint32_t len);

  /*!
    @brief if with_me is true, when net is closed, we will be closed
   */
  void SetWithMe(bool with_me);
  bool GetWithMe();

  void SetEnableWrite();

  /*
    Get ip and port of tcp and udp
   */
  uint32_t GetIP();
  uint16_t GetPort();
  std::string GetAddress();
  sockaddr_in *GetAddressIn();
  BaseWorkerEx *GetEx();
  
  std::string GetNetStatusStr();
  BaseProcess::ProcessNetStatus GetNetStatus();

protected:
  IOBuffer output_buffer_;
  IOBuffer input_buffer_;
  BaseWorkerEx *ex_;

private:

  struct UDPDataCache {
    uint16_t len;
    sockaddr_in address;
    uint8_t data[0];
  };
  
  void SetIOHandler(IOHandler *io);
  void SetNetStatus(ProcessNetStatus status);

  static const char *ProcessNetStatusString[PROCESS_NET_STATUS_COUNT];
  IOHandler *io_handler_;
  int32_t recv_buffer_size_;
  uint32_t send_buffer_size_;
  int32_t process_errno_;
  ProcessNetStatus process_net_status_;
  bool is_write_;
  bool with_me_;
};


inline IOBuffer *BaseProcess::GetInputBuffer() {
  return &input_buffer_;
}

inline IOBuffer *BaseProcess::GetOutputBuffer() {
  if ( 0 == GETAVAILABLEBYTESCOUNT(output_buffer_) )
    return NULL;
  return &output_buffer_;
}

// inline int32_t BaseProcess::GetRecvBufferSize() {
//   return recv_buffer_size_/3;
// }

inline std::string BaseProcess::GetNetStatusStr() {
  if ( PROCESS_NET_STATUS_SYS_ERROR == process_net_status_ ) {
    return format("%s: %s(%d)", ProcessNetStatusString[PROCESS_NET_STATUS_SYS_ERROR], 
                  STR(getStrerror(process_errno_)), process_errno_);
  }

  return ProcessNetStatusString[process_net_status_];
}

inline BaseProcess::ProcessNetStatus BaseProcess::GetNetStatus() {
  return process_net_status_;
}

inline void BaseProcess::SetIOHandler(IOHandler *io) {
  io_handler_ = io;
}

inline void BaseProcess::SetNetStatus(ProcessNetStatus status) {
  process_net_status_ = status;
  if ( PROCESS_NET_STATUS_SYS_ERROR == status )
    process_errno_ = errno;    
}

inline bool BaseProcess::GetWithMe() {
  return with_me_;
}

inline BaseWorkerEx *BaseProcess::GetEx() {
  return ex_;
}


}
#endif	/* _BASEPROCESS_H */



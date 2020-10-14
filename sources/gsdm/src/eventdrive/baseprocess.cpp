#include "eventdrive/epoll/iohandler.h"
#include "eventdrive/epoll/udpcarrier.h"
#include "eventdrive/epoll/tcpcarrier.h"
#include "eventdrive/epoll/unixcarrier.h"
#include "eventdrive/epoll/pipecarrier.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/baseprocess.h"
#include "logging/logger.h"

namespace gsdm {

const char *BaseProcess::ProcessNetStatusString[PROCESS_NET_STATUS_COUNT] = { "disconnect", "connecting", "connected", "peer reset", "error of system", "peer timeout" };

BaseProcess::BaseProcess(int32_t recv_buffer_size, int32_t send_buffer_size)
  :TimeoutEvent(),ex_(NULL), io_handler_(NULL), recv_buffer_size_((uint32_t)recv_buffer_size),send_buffer_size_((uint32_t)send_buffer_size),
   process_errno_(0), process_net_status_(PROCESS_NET_STATUS_DISCONNECT),is_write_(false),with_me_(true){
  input_buffer_.Initialize(recv_buffer_size);
}

BaseProcess::~BaseProcess() { 
  io_handler_ = NULL;
  output_buffer_.IgnoreAll();
  input_buffer_.IgnoreAll();
}

bool BaseProcess::DoTimeout() {
  SetNetStatus(PROCESS_NET_STATUS_TIMEOUT);
  return true;
}

void BaseProcess::Free() {
  switch(process_net_status_) {
    case PROCESS_NET_STATUS_CONNECTED: {
      process_net_status_ = PROCESS_NET_STATUS_DISCONNECT;
      // io_handler_->SetFlag(IOHT_FLAG_CLOSE_ALL);
      SetWithMe(true);
      io_handler_->GetManager()->EnqueueForDelete(io_handler_);
      break;
    }
    case PROCESS_NET_STATUS_CONNECTING: {
      process_net_status_ = PROCESS_NET_STATUS_DISCONNECT;
      // io_handler_->SetFlag(IOHT_FLAG_CLOSE_ALL);
      SetWithMe(true);
      io_handler_->GetManager()->EnqueueForDelete(io_handler_);
      break;
    }
    //case PROCESS_NET_STATUS_PEER_RESET:
    //case PROCESS_NET_STATUS_SYS_ERROR:
    case PROCESS_NET_STATUS_TIMEOUT: {
      // io_handler_->SetFlag(IOHT_FLAG_CLOSE_ALL);
      SetWithMe(true);
      io_handler_->GetManager()->EnqueueForDelete(io_handler_);
      break;
    }
    default:
      delete this;
  }
}

bool BaseProcess::Process(int32_t recv_amount) {
  return false;
}

void BaseProcess::AttachNet() {
  // heart_ = Logger::GetCurrentGsdmMsec();
  ex_->AddNetTimeoutEvent(this);
}

void BaseProcess::DetachNet() {
  ex_->RemoveNetTimeoutEvent(this);
}

bool BaseProcess::IsCloseNet() {
  return false;
}

bool BaseProcess::SendMsg(const void *data, uint32_t len) {
  if ( GetNetStatus() != PROCESS_NET_STATUS_CONNECTED ) {
    DEBUG("SendMsg in status != PROCESS_NET_STATUS_CONNECTED");
    return true;
  }

  uint32_t cache_send_len = GETAVAILABLEBYTESCOUNT(output_buffer_);
  int rs = 0;
  if ( 0 < cache_send_len ) {
    int32_t write_amount = 0;
    if ( io_handler_->GetType() == IOHT_TCP_CARRIER || io_handler_->GetType() == IOHT_UNIX_CARRIER ) {
      if ( !output_buffer_.WriteToTCPFd(io_handler_->GetFd(), 0x800000/* 8M */, write_amount) ) {
        WARN("WriteToTCPFd failed, errno is %d", errno);
        return false;
      }
    } else if ( io_handler_->GetType() == IOHT_PIPE_CARRIER ) {
      if ( !output_buffer_.WriteToFd(io_handler_->GetFd(), 0x800000/* 8M */) ) {
        WARN("WriteToFd failed, errno is %d", errno);
        return false;
      }
    } else {
      WARN("handler of type is unkown, 0x%x", io_handler_->GetType()); 
      return false;
    }

    cache_send_len = GETAVAILABLEBYTESCOUNT(output_buffer_);
    if ( 0 < cache_send_len ) {
      goto SENDMSG_SAVE_CACHE;
    }
  }

  if ( io_handler_->GetType() == IOHT_TCP_CARRIER || io_handler_->GetType() == IOHT_UNIX_CARRIER )  {
    rs = send(io_handler_->GetFd(), data, len, MSG_NOSIGNAL);
  } else if ( io_handler_->GetType() == IOHT_PIPE_CARRIER ) {
    rs = write(io_handler_->GetFd(), data, len);
  } else {
    WARN("handler of type is unkown, 0x%x", io_handler_->GetType());
    return false;
  }

  if ( 0 > rs ) {
    if ( errno != EAGAIN ) {
      FATAL("Unable to send %u bytes of data. [%d: %s]", len, errno, strerror(errno));
      return false;
    }
    rs = 0;
  }

  len -= (uint32_t)rs;
  if ( 0 < len ) {
    goto SENDMSG_SAVE_CACHE;
  }

  if ( is_write_ ) {
    io_handler_->GetManager()->DisableWriteData(io_handler_);
    is_write_ = false;
  }

  return true;

 SENDMSG_SAVE_CACHE:
  if ( send_buffer_size_ < cache_send_len + len ) {
    WARN("The out-buffer and date-len( %u+%u ) greater than %u", cache_send_len, len, send_buffer_size_);
    return false;
  }

  if ( !output_buffer_.ReadFromBuffer(((const uint8_t *)data) + rs, len) ) {
    WARN("The ReadFromBuffer failed, outbuffer-len:%u, data-len:%u", cache_send_len, len);
    return false;
  }

  if ( !is_write_ ) {
    io_handler_->GetManager()->EnableWriteData(io_handler_);
    is_write_ = true;
  }

  return true;
}

bool BaseProcess::SendToMsg(const void *data, uint32_t len, sockaddr_in *peer_address) {
  if ( io_handler_->GetType() != IOHT_UDP_CARRIER ) {
    WARN("handler is not udp carrier, 0x%x", io_handler_->GetType());
    return false;
  }

  uint32_t cache_send_len = GETAVAILABLEBYTESCOUNT(output_buffer_);
  uint8_t *buffer = GETIBPOINTER(output_buffer_);
  UDPDataCache *cache, save;
  while ( cache_send_len ) {
    cache = (UDPDataCache *)buffer;
    if ( 0 > ((UDPCarrier *)io_handler_)->SendTo(cache->data, cache->len - sizeof(UDPDataCache), &cache->address) ) {
      if ( EAGAIN != errno ) {
        FATAL("Unable to sendto %u bytes of data. [%d: %s]", cache->len - sizeof(UDPDataCache), errno, strerror(errno));
        return false;
      }
      goto SENDTOMSG_SAVE_EAGAIN;
    }
    output_buffer_.Ignore(cache->len);
    buffer += cache->len;
    cache_send_len -= cache->len;
  }

  if ( 0 > ((UDPCarrier *)io_handler_)->SendTo((const uint8_t *)data, len, peer_address) ) {
    if ( EAGAIN != errno ) {
      FATAL("Unable to sendto %u bytes of data. [%d: %s]", len, errno, strerror(errno));
      return false;
    }
    goto SENDTOMSG_SAVE_EAGAIN;
  }

  if ( is_write_ ) {
    io_handler_->GetManager()->DisableWriteData(io_handler_);
    is_write_ = false;
  }
  return true;

 SENDTOMSG_SAVE_EAGAIN:
  if ( send_buffer_size_ < cache_send_len + len + sizeof(UDPDataCache) ) {
    WARN("The out-buffer and date-len( %u+%u ) greater than %u", cache_send_len, len, send_buffer_size_);
    return false;
  }

  save.len = len + sizeof(UDPDataCache);
  save.address = *peer_address; 
  if ( !output_buffer_.ReadFromBuffer((const uint8_t *)&save, sizeof(UDPDataCache)) ) {
    WARN("The ReadFromBuffer failed, outbuffer-len:%u, data-len:%u", cache_send_len, len);
    return false;
  }

  if ( !output_buffer_.ReadFromBuffer((const uint8_t *)data, len) ) {
    WARN("The ReadFromBuffer failed, outbuffer-len:%u, data-len:%u", cache_send_len, len);
    return false;
  }
  
  if ( !is_write_ ) {
    io_handler_->GetManager()->EnableWriteData(io_handler_);
    is_write_ = true;
  }
  return true;
}

bool BaseProcess::SendToMsg(const void *data, uint32_t len) {
  if ( io_handler_->GetType() != IOHT_UDP_CARRIER ) {
    WARN("handler is not udp carrier, 0x%x", io_handler_->GetType());
    return false;
  }
  return SendToMsg(data, len, ((UDPCarrier *)io_handler_)->GetPeerAddress());
}

void BaseProcess::SetWithMe(bool with_me) {
  with_me_ = with_me;
  if ( io_handler_ ) {    
    io_handler_->SetFlag(with_me_ ? IOHT_FLAG_CLOSE_ALL : IOHT_FLAG_CLOSE_FD);
  }
}

uint32_t BaseProcess::GetIP() {
  if ( io_handler_->GetType() == IOHT_TCP_CARRIER ) {
    return ((TCPCarrier *)io_handler_)->GetPeerAddress()->sin_addr.s_addr;
  }

  if ( io_handler_->GetType() == IOHT_UDP_CARRIER ) {
    return ((UDPCarrier *)io_handler_)->GetPeerAddress()->sin_addr.s_addr;
  }

  return INADDR_NONE;
}

uint16_t BaseProcess::GetPort() {
  if ( io_handler_->GetType() == IOHT_TCP_CARRIER ) {
    return ntohs(((TCPCarrier *)io_handler_)->GetPeerAddress()->sin_port);
  }

  if ( io_handler_->GetType() == IOHT_UDP_CARRIER ) {
    return ntohs(((UDPCarrier *)io_handler_)->GetPeerAddress()->sin_port);
  }

  return 0;
}

std::string BaseProcess::GetAddress() {
  if ( io_handler_->GetType() == IOHT_TCP_CARRIER ) {
    return getHostIPv4(((TCPCarrier *)io_handler_)->GetPeerAddress()->sin_addr.s_addr);
  }

  if ( io_handler_->GetType() == IOHT_UDP_CARRIER ) {
    return getHostIPv4(((UDPCarrier *)io_handler_)->GetPeerAddress()->sin_addr.s_addr);
  }

  if ( io_handler_->GetType() == IOHT_PIPE_CARRIER ) {
    return ((PIPECarrier *)io_handler_)->GetPeerAddress();
  }

  return "";
}

sockaddr_in *BaseProcess::GetAddressIn() {
  if ( io_handler_->GetType() == IOHT_TCP_CARRIER ) {
    return ((TCPCarrier *)io_handler_)->GetPeerAddress();
  }

  if ( io_handler_->GetType() == IOHT_UDP_CARRIER ) {
    return ((UDPCarrier *)io_handler_)->GetPeerAddress();
  }
  
  return NULL;
}

void BaseProcess::SetEnableWrite() {
  if ( !io_handler_ )
    return;

  if ( !is_write_ ) {
    io_handler_->GetManager()->EnableWriteData(io_handler_);
    is_write_ = true;
  }
}


}


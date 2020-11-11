/* 
 *  Copyright (c) 2010,
 *  sunlei (sswin0922@163.com) 
 */
#include "eventdrive/baseprocess.h"
#include "eventdrive/epoll/udpcarrier.h"
#include "eventdrive/epoll/iohandlermanager.h"
#include "eventdrive/networker.h"
#include "logging/logger.h"

#define SOCKET_READ_CHUNK 65536
#define SOCKET_WRITE_CHUNK SOCKET_READ_CHUNK

namespace gsdm {

UDPCarrier::UDPCarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process)
  : IOHandler(fd, IOHT_UDP_CARRIER, pIOHandlerManager, process) {
	memset(&peer_address_, 0, sizeof (sockaddr_in));
	memset(&near_address_, 0, sizeof (sockaddr_in));
	near_ip_ = "";
	near_port_ = 0;
  if ( process_ ) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_CONNECTED);
    process_->AttachNet();
  }
}

UDPCarrier::~UDPCarrier() {
  process_->DetachNet();
  if ( process_ && !(flag_ & IOHT_FLAG_CLOSE_PROCESS) ) {
    process_->SetIOHandler(NULL);
  }

  if ( pIOHandlerManager_ ) {
    NetWorker *worker = pIOHandlerManager_->GetManager();
    worker->RemoveUDPCarrier(this);
  }
}

bool UDPCarrier::OnEvent(struct epoll_event &event) {
  if ( NULL == process_ || BaseProcess::PROCESS_NET_STATUS_CONNECTED != process_->process_net_status_ ) {
    WARN("status of process_ is invalid.");
    return false;
  }

  //1. Test error
  if ( 0 != (event.events & EPOLLERR) ) {
    if ( 0 != (event.events & EPOLLHUP) ) {
      WARN("***Event handler HUP: %p, 0x%X", this, event.events);
    } else {
      WARN("***Event handler ERR: %p, 0x%X", this, event.events);
    }

    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
    return false;
  }

	//2. Read data
	if ( 0 != (event.events & EPOLLIN) ) {
		IOBuffer *input_buffer = process_->GetInputBuffer();
		int32_t recv_bytes = 0;
		if ( !input_buffer->ReadFromUDPFd(fd_, recv_bytes, peer_address_) ) {
      if ( EAGAIN != errno ) {
        FATAL("Unable to read data");
        process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
        return false;
      }
		} else if ( 0 == recv_bytes ) {
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_PEER_RESET);
			FATAL("Connection closed");
			return false;
		} else if ( !process_->Process(recv_bytes) ) {
      process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
			// FATAL("Unable to signal data available");
			return false;
		}
	}

	//3. Write data
	if ( 0 != (event.events & EPOLLOUT) ) {
    IOBuffer *output_buffer = NULL;
    if ( NULL != (output_buffer = process_->GetOutputBuffer()) ) {
      uint32_t cache_send_len = GETAVAILABLEBYTESCOUNT(*output_buffer);
      uint8_t *buffer = GETIBPOINTER(*output_buffer);
      BaseProcess::UDPDataCache *cache;
      while ( cache_send_len ) {
        cache = (BaseProcess::UDPDataCache *)buffer;
        if ( 0 > SendTo(cache->data, cache->len - sizeof(BaseProcess::UDPDataCache), &cache->address) ) {
          if ( EAGAIN != errno ) {
            process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_SYS_ERROR);
            return false;
          }
          break;
        }
        output_buffer->Ignore(cache->len);
        buffer += cache->len;
        cache_send_len -= cache->len;
      }

      if ( !cache_send_len ) {
        pIOHandlerManager_->DisableWriteData(this);
        process_->is_write_ = false;
      }
    }
	}

  if ( process_->IsCloseNet() ) {
    process_->SetNetStatus(BaseProcess::PROCESS_NET_STATUS_DISCONNECT);
    return false;  
  }

	return true;
}

UDPCarrier *UDPCarrier::Create(const std::string &bind_ip, uint16_t bind_port, IOHandlerManager *pIOHandlerManager,
                               BaseProcess *process, uint16_t ttl, uint16_t tos, int32_t rcv_buf, int32_t snd_buf) {
	if (process == NULL) {
		FATAL("Process can't be null");
		return NULL;
	}
  
	//1. Create the socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		FATAL("Unable to create socket: %s(%d)", strerror(errno), errno);
		return NULL;
	}

	//2. fd options
	if (!setFdOptions(sock, false)) {
		FATAL("Unable to set fd options");
		CLOSE_SOCKET(sock);
		return NULL;
	}

	if (tos <= 255) {
		if (!setFdTOS(sock, (uint8_t) tos)) {
			FATAL("Unable to set tos");
			CLOSE_SOCKET(sock);
			return NULL;
		}
	}

  if (0 < rcv_buf) {
    if (!setFdRcvBuf(sock, rcv_buf)) {
			FATAL("Unable to set rcvbuf %d", rcv_buf);
			CLOSE_SOCKET(sock);
			return NULL;
		}
  }

  if (0 < snd_buf) {
    if (!setFdSndBuf(sock, snd_buf)) {
			FATAL("Unable to set sndbuf %d", snd_buf);
			CLOSE_SOCKET(sock);
			return NULL;
		}
  }

	//3. bind if necessary
	sockaddr_in bind_address;
	memset(&bind_address, 0, sizeof (bind_address));
	if (bind_ip != "") {
		bind_address.sin_family = PF_INET;
		bind_address.sin_addr.s_addr = inet_addr(bind_ip.c_str());
		bind_address.sin_port = htons(bind_port); //----MARKED-SHORT----
		if (bind_address.sin_addr.s_addr == INADDR_NONE) {
			FATAL("Unable to bind on address %s:%hu", STR(bind_ip), bind_port);
			CLOSE_SOCKET(sock);
			return NULL;
		}
		uint32_t testVal = htonl(bind_address.sin_addr.s_addr);
		if ((testVal > 0xe0000000) && (testVal < 0xefffffff)) {
			INFO("Subscribe to multicast %s:%" PRIu16, STR(bind_ip), bind_port);
			if (ttl <= 255) {
				if (!setFdMulticastTTL(sock, (uint8_t) ttl)) {
					FATAL("Unable to set ttl");
					CLOSE_SOCKET(sock);
					return NULL;
				}
			}
		} else {
			if (ttl <= 255) {
				if (!setFdTTL(sock, (uint8_t) ttl)) {
					FATAL("Unable to set ttl");
					CLOSE_SOCKET(sock);
					return NULL;
				}
			}
		}

		if (bind(sock, (sockaddr *)&bind_address, sizeof (sockaddr)) != 0) {
			int error = errno;
			FATAL("Unable to bind on address: udp://%s:%" PRIu16 "; Error was: %s (%" PRId32 ")",
					STR(bind_ip), bind_port, strerror(error), error);
			CLOSE_SOCKET(sock);
			return NULL;
		}
		if ((testVal > 0xe0000000) && (testVal < 0xefffffff)) {
			struct ip_mreq group;
			group.imr_multiaddr.s_addr = inet_addr(bind_ip.c_str());
			group.imr_interface.s_addr = INADDR_ANY;
			if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &group, sizeof (group)) < 0) {
				FATAL("Adding multicast group error");
				CLOSE_SOCKET(sock);
				return NULL;
			}
		}
	}

	//4. Create the carrier
	UDPCarrier *result = new UDPCarrier(sock, pIOHandlerManager, process);
	if ( NULL == result ) {
		FATAL("Unable to create UDP carrier");
		return NULL;
	}

  if ( bind_ip == "" ) {
    socklen_t len = sizeof (bind_address);
    getsockname(sock, (sockaddr *)&bind_address, &len);
  }
  result->near_address_ = bind_address;

	return result;
}

UDPCarrier* UDPCarrier::Create(UDPCarrier *copy_udp, IOHandlerManager *pIOHandlerManager, BaseProcess *process) {
  if ( NULL == process ) {
		FATAL("Process can't be null");
		return NULL;
	}
  
	UDPCarrier *result = new UDPCarrier(dup(copy_udp->GetFd()), pIOHandlerManager, process);
	if ( NULL == result ) {
		FATAL("Unable to create UDP carrier");
		return NULL;
	}

  result->near_address_ = copy_udp->near_address_;
  result->near_ip_      = copy_udp->near_ip_;
  result->near_port_    = copy_udp->near_port_;

	return result;
}

}




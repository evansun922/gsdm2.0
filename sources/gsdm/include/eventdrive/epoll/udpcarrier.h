/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 */



#ifndef _UDPCARRIER_H
#define	_UDPCARRIER_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class UDPCarrier
: public IOHandler {
public:
	virtual ~UDPCarrier();

  int32_t SendTo(const uint8_t *data, uint32_t len, sockaddr_in *peerAddress);
  int32_t SendTo(const uint8_t *data, uint32_t len);
  sockaddr_in *GetPeerAddress();
	static UDPCarrier* Create(const std::string &bind_ip, uint16_t bind_port, IOHandlerManager *pIOHandlerManager,
                            BaseProcess *process, uint16_t ttl = 256,
                            uint16_t tos = 256, int32_t rcv_buf = -1, int32_t snd_buf = -1);
	static UDPCarrier* Create(UDPCarrier *copy_udp, IOHandlerManager *pIOHandlerManager, BaseProcess *process);

	virtual bool OnEvent(struct epoll_event &event);

private:
	UDPCarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process);

  
private:
	sockaddr_in peer_address_;
	sockaddr_in near_address_;
  std::string near_ip_;
	uint16_t near_port_;

};

inline int32_t UDPCarrier::SendTo(const uint8_t *data, uint32_t len, sockaddr_in *peerAddress) {
  return sendto(fd_, data, len, 0, (const sockaddr*)peerAddress, sizeof (sockaddr));
}

inline int32_t UDPCarrier::SendTo(const uint8_t *data, uint32_t len) {
  return sendto(fd_, data, len, 0, (const sockaddr*)&peer_address_, sizeof (sockaddr));
}

inline sockaddr_in *UDPCarrier::GetPeerAddress() {
  return &peer_address_;
}

}

#endif	/* _UDPCARRIER_H */



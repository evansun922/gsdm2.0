/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */

#ifndef _TCPCARRIER_H
#define	_TCPCARRIER_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class TCPCarrier: public IOHandler {

public:
	TCPCarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process);
	virtual ~TCPCarrier();
	virtual bool OnEvent(struct epoll_event &event);
  sockaddr_in *GetPeerAddress(); 

private:
  sockaddr_in peer_address_;
};

inline sockaddr_in *TCPCarrier::GetPeerAddress() {
  return &peer_address_;
}


}

#endif	/* _TCPCARRIER_H */




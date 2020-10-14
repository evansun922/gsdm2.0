/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _TCPCONNECTOR_H
#define	_TCPCONNECTOR_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class IOHandlerManager;
class BaseProcess;

class TCPConnector: public IOHandler {
public:
  ~TCPConnector();

	virtual bool OnEvent(epoll_event &event);
	static bool Connect(const std::string &ip, uint16_t port, IOHandlerManager *pIOHandlerManager, BaseProcess *process);

private:
	TCPConnector(int32_t fd, const std::string &ip, uint16_t port, IOHandlerManager *pIOHandlerManager, BaseProcess *process);
	bool Connect();

  std::string ip_;
	uint16_t port_;
	
};

}

#endif	/* _TCPCONNECTOR_H */




/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _TCPACCEPTOR_H
#define	_TCPACCEPTOR_H


#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class NetWorker;
class BaseWorkerEx;

class TCPAcceptor
: public IOHandler {
public:
	TCPAcceptor(NetWorker *worker, std::vector<BaseWorkerEx*> &other, const std::string &ip_address, uint16_t port);
	virtual ~TCPAcceptor();

	bool Bind();
	bool StartAccept();
	virtual bool OnEvent(struct epoll_event &event);
	virtual bool OnConnectionAvailable(struct epoll_event &event);
	bool Accept();


protected:
	bool IsAlive();
  
protected:
	sockaddr_in address_;
	uint32_t accepted_count_;
  std::string ip_address_;
	uint16_t port_;
  NetWorker *worker_;
  std::vector<BaseWorkerEx*> other_;
  uint32_t other_count_;
};

}

#endif	/* _TCPACCEPTOR_H */




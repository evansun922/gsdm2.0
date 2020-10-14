/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _UNIXACCEPTOR_H
#define	_UNIXACCEPTOR_H


#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class NetWorker;
class BaseWorkerEx;

class UNIXAcceptor
: public IOHandler {
public:
	UNIXAcceptor(NetWorker *worker, std::vector<BaseWorkerEx*> &other, const std::string &unix_path);
	virtual ~UNIXAcceptor();

	bool Bind();
	bool StartAccept();
	virtual bool OnEvent(struct epoll_event &event);
	virtual bool OnConnectionAvailable(struct epoll_event &event);
	bool Accept();


protected:
	bool IsAlive();
  
protected:
	sockaddr_un address_;
	uint32_t accepted_count_;
  std::string unix_path_;
  NetWorker *worker_;
  std::vector<BaseWorkerEx*> other_;
  uint32_t other_count_;
};

}

#endif	/* _UNIXACCEPTOR_H */




/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */

#ifndef _PIPECARRIER_H
#define	_PIPECARRIER_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class PIPECarrier: public IOHandler {

public:
	PIPECarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process, const std::string &name);
	virtual ~PIPECarrier();
	virtual bool OnEvent(struct epoll_event &event);
  std::string GetPeerAddress(); 

private:
  std::string name_;
};

inline std::string PIPECarrier::GetPeerAddress() {
  return name_;
}


}


#endif	/* _PIPECARRIER_H */




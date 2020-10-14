/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */

#ifndef _UNIXCARRIER_H
#define	_UNIXCARRIER_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class UNIXCarrier: public IOHandler {

public:
	UNIXCarrier(int32_t fd, IOHandlerManager *pIOHandlerManager, BaseProcess *process);
	virtual ~UNIXCarrier();
	virtual bool OnEvent(struct epoll_event &event);


private:


};

}

#endif	/* _UNIXCARRIER_H */




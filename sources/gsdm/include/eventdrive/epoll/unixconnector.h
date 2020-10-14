/* 
 *  Copyright (c) 2012,
 *  sunlei (sswin0922@163.com)
 *
 */


#ifndef _UNIXCONNECTOR_H
#define	_UNIXCONNECTOR_H

#include "eventdrive/epoll/iohandler.h"

namespace gsdm {

class IOHandlerManager;
class BaseProcess;

class UNIXConnector: public IOHandler {
public:
  ~UNIXConnector();

	virtual bool OnEvent(epoll_event &event);
	static bool Connect(const std::string &unix_path, IOHandlerManager *pIOHandlerManager, BaseProcess *process);

private:
	UNIXConnector(int32_t fd, const std::string &unix_path, IOHandlerManager *pIOHandlerManager, BaseProcess *process);
	bool Connect();

  std::string unix_path_;
	
};

}

#endif	/* _UNIXCONNECTOR_H */




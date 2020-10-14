#ifndef _TCPACCEPTCMD_H
#define _TCPACCEPTCMD_H

#include "basecmd.h"

#define TCP_ACCEPTCMD_TYPE (BASECMD_TYPE-1)

class NetWorker;

namespace gsdm {

class TCPAcceptCmd : public BaseCmd {
public:
  TCPAcceptCmd(int fd, uint16_t prot);
  virtual ~TCPAcceptCmd();

  virtual bool Process(BaseWorkerEx *ex);

  int fd_;
  uint16_t port_;
};


}

#endif // _TCPACCEPTCMD_H

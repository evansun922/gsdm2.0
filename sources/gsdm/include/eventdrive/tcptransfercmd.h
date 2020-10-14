#ifndef _TCPTRANSFERCMD_H
#define _TCPTRANSFERCMD_H

#include "basecmd.h"

#define TCP_TRANSFERCMD_TYPE (BASECMD_TYPE-3)

class NetWorker;

namespace gsdm {

class TCPTransferCmd : public BaseCmd {
public:
  TCPTransferCmd();
  virtual ~TCPTransferCmd();

  virtual bool Process(BaseWorkerEx *ex);

  int fd_;
  uint16_t port_;
  uint8_t *data_;
  uint32_t data_len_;
};


}

#endif // _TCPTRANSFERCMD_H

#ifndef _UNIXTRANSFERCMD_H
#define _UNIXTRANSFERCMD_H

#include "basecmd.h"

#define UNIX_TRANSFERCMD_TYPE (BASECMD_TYPE-4)

class NetWorker;

namespace gsdm {

class UNIXTransferCmd : public BaseCmd {
public:
  UNIXTransferCmd();
  virtual ~UNIXTransferCmd();

  virtual bool Process(BaseWorkerEx *ex);

  int fd_;
  std::string unix_path_;
  uint8_t *data_;
  uint32_t data_len_;
};


}

#endif // _UNIXTRANSFERCMD_H

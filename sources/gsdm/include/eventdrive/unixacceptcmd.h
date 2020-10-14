#ifndef _UNIXACCEPTCMD_H
#define _UNIXACCEPTCMD_H

#include "basecmd.h"

#define UNIX_ACCEPTCMD_TYPE (BASECMD_TYPE-2)

namespace gsdm {

class NetWorker;

class UNIXAcceptCmd : public BaseCmd {
public:
  UNIXAcceptCmd(int fd, const std::string &unix_path);
  virtual ~UNIXAcceptCmd();

  virtual bool Process(BaseWorkerEx *ex);

  int fd_;
  std::string unix_path_;
};


}

#endif // _ACCEPTCMD_H

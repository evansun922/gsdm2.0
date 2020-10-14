#ifndef _BASECMD_H
#define _BASECMD_H

#define BASECMD_TYPE INT32_MAX

#include "platform/linuxplatform.h"

namespace gsdm {

class BaseWorkerEx;

class BaseCmd {
public:
  BaseCmd(bool free);
  virtual ~BaseCmd();

  virtual bool Process(BaseWorkerEx *ex);

  BaseCmd *next;
  bool is_free;
};

}

#endif //_BASECMD_H

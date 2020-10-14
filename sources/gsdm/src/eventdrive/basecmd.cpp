#include "eventdrive/basecmd.h"

namespace gsdm {

BaseCmd::BaseCmd(bool free) :next(NULL),is_free(free) {
  
}

BaseCmd::~BaseCmd() {

}

bool BaseCmd::Process(BaseWorkerEx *ex) {
  return false;
}

}

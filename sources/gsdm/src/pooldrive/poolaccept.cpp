#include "pooldrive/poolaccept.h"
#include "pooldrive/poolworker.h"
#include "logging/logger.h"

namespace gsdm {

PoolAccept::PoolAccept() {
  memset(ex_data_, 0, ACCEPT_EX_DATA_LENGTH);
}

PoolAccept::~PoolAccept() {
 
} 


};

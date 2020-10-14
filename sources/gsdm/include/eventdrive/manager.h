#ifndef _GSDMMANAGER_H
#define _GSDMMANAGER_H

#include "platform/linuxplatform.h"

namespace gsdm {

class NetWorker;
class BaseWorkerEx;
class BaseCmd;

class EventManager {
public:

  ~EventManager();

  bool Initialize(int argc, char *argv[], void (*usage)(char *prog), const std::string &version);
  void AddWorker(BaseWorkerEx *ex);

  void SetupQueue(BaseWorkerEx *from, std::vector<BaseWorkerEx *> &to, int cmd_tpye);
  void SetupQueue(std::vector<BaseWorkerEx *> &from, BaseWorkerEx *to, int cmd_tpye);
  void SetupQueue(std::vector<BaseWorkerEx *> &from, std::vector<BaseWorkerEx *> &to, int cmd_tpye);

  bool Loop();
  void Stop();
  void Close();
  
  static EventManager *GetManager() { return &myself_; }

private:
  EventManager();

  std::vector< NetWorker * > worker_;
  uint32_t worker_id_;
  std::vector<int> cpu_;
  pthread_t log_pthread_;
  static EventManager myself_;
  
};


}

#endif // _GMANAGER_H

#ifndef _GSDMPOOLMANAGER_H
#define _GSDMPOOLMANAGER_H

#include "platform/linuxplatform.h"


namespace gsdm {

class PoolWorker;
class PoolWorkerEx;
class SysPoolWorker;

class PoolManager {
public:
friend class SysPoolWorker;

  ~PoolManager();

  bool Initialize(int argc, char *argv[], void (*usage)(char *prog), const std::string &version);
  void AddWorker(PoolWorkerEx *ex);

  bool Loop();
  void Stop();
  void Close();
  
  static PoolManager *GetManager() { return &myself_; }

  void SetTitle(const std::string &title, bool cmd);

private:
  PoolManager();
  void Free();

  std::vector<PoolWorker *> worker_;
  std::vector<int> cpu_;
  pid_t log_pid_;
  bool *is_close_;
  std::string process_name_;
  static PoolManager myself_;
};


}

#endif // _GMANAGER_H

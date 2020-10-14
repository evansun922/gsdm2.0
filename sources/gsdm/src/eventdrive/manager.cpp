#include "logging/logger.h"
#include "utils/gsdmconfig.h"
#include "eventdrive/basecmd.h"
#include "eventdrive/baseworkerex.h"
#include "eventdrive/networker.h"
#include "eventdrive/manager.h"

extern char *optarg;

namespace gsdm {

EventManager EventManager::myself_;

static void eventQuitSignalHandler(int sig) {
  EventManager::GetManager()->Stop();
}

EventManager::EventManager() :worker_id_(0),log_pthread_((pthread_t)-1) {
  int32_t total = getCPUCount();
  for (int i = 0; i < total; i++)
    cpu_.push_back(0);
}

EventManager::~EventManager() {

}

bool EventManager::Initialize(int argc, char *argv[], void (*usage)(char *prog), const std::string &version) {

  const char *config_txt = "";
  int opt;
  struct option longopts[] = {
    {"help", 0, NULL, 'h'},
    {"version",0, NULL, 'v'},
    {"config",1, NULL, 'f'},
    {"reset",1, NULL, 'r'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "hvf:r:#",
                            longopts, NULL)) != -1) {
    switch (opt) {
      case 'h': {
        usage(argv[0]);
      }
      return false;
      case 'v': {
        printf("The version is \"%s\".\n", STR(version));
      }
      return false;
      case 'r': {
        std::string app_name = getFileName(argv[0]);
        std::vector<pid_t> pids;
        getPidByName(pids, app_name);

        for (int i = 0; i < (int)pids.size(); i++) {
          if ( pids[i] == getpid() )
            continue;

          std::string v = ::optarg;
          trim(v);
          std::string log_unix_path = gsdm::format("%s/gsdm_log_%u", GSDM_BACK_ROOT_DIR, pids[i]);
          sockaddr_un log_address;
          memset(&log_address, 0, sizeof(sockaddr_un));
          log_address.sun_family = PF_UNIX;
          strncpy(log_address.sun_path, STR(log_unix_path), sizeof(log_address.sun_path) - 1);
          socklen_t log_address_len = sizeof(log_address.sun_family) + strlen(log_address.sun_path);
          int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
          char cmd[1024] = { 0 };
          int len = snprintf(cmd, sizeof(cmd), "u%s", STR(v));
          sendto(fd, cmd, len + 1, 0, (const struct sockaddr *)&log_address, log_address_len);
          close(fd);
          break;
        }

      }
      return false;
      case 'f': {
        config_txt = ::optarg;
      }
      break;
    }
  }

  if ( !GsdmConfig::GetConfig()->Initialize(config_txt) ) {
    WARN("This path of configuratoin %s is invalid", config_txt);
    usage(argv[0]);
    return false;
  }

  std::string detach = GsdmConfig::GetConfig()->GetString("detach", "");
  if ( "" != detach ) {
    trim(detach);
    std::vector<std::string> res;
    split(detach, ",", res);
    for (uint32_t i = 0; i < res.size(); i++) {
      char *endptr;
      std::string v = res[i];
      trim(v);
      int rs = strtol(STR(v), &endptr, 10);
      if (errno != ERANGE && *endptr == '\0' && STR(v) != endptr && rs >= 0 && rs < (int)cpu_.size()) {
        cpu_[rs] = INT32_MAX;
      }
    }
  }

  bool bind_cpu = GsdmConfig::GetConfig()->GetBool("bind_cpu", true);
  if (!bind_cpu) {
    cpu_.clear();
  }

  // log  
  if ( !fileExists(GSDM_BACK_ROOT_DIR) ) {
    createFolder(GSDM_BACK_ROOT_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  // set core of limit
  int64_t limit_core = GsdmConfig::GetConfig()->GetLongInt("limit_core", -1);
  if ( 0 == limit_core ) {
    if ( !setLimitCore(RLIM_INFINITY) ) {
      FATAL("Unable to setLimitCore %ld", limit_core);
      return false;
    }
  } else if ( 0 < limit_core ) {
    if ( !setLimitCore((uint64_t)limit_core) ) {
      FATAL("Unable to setLimitCore %ld", limit_core);
      return false;
    }
  }

  // set file of limit
  int64_t limit_file = GsdmConfig::GetConfig()->GetLongInt("limit_file", -1);
  if ( 0 < limit_file ) {
    if ( !setLimitNoFile((uint64_t)limit_file) ) {
      FATAL("Unable to setLimitNoFile %ld", limit_file);
      return false;
    }
  }

  return true;
}

void EventManager::AddWorker(BaseWorkerEx *ex) {
  int min = INT32_MAX;
  int index = 0;
  if (cpu_.empty()) {
    index = -1;
  } else {
    for (int i = 0; i < (int)cpu_.size(); i++) {
      if ( min > cpu_[i] ) {
        min = cpu_[i];
        index = i;
      }
    }
    cpu_[index]++;
  }

  // NetWorker *w = new NetWorker(this, worker_id_++, index, CreateTCPProcess, CreateUNIXProcess, ex);
  NetWorker *w = new NetWorker(this, worker_id_++, index, ex);
  worker_.push_back(w);
  ex->worker_ = w;
}

void EventManager::SetupQueue(BaseWorkerEx *from, std::vector<BaseWorkerEx *> &to, int cmd_type) {
  for(uint32_t i = 0; i < to.size(); i++) {
    to[i]->worker_->SetupQueue(from->worker_->GetID(), cmd_type);
  }
}

void EventManager::SetupQueue(std::vector<BaseWorkerEx *> &from, BaseWorkerEx *to, int cmd_tpye) {
  std::vector<BaseWorkerEx *> vec;
  vec.push_back(to);
  SetupQueue(from, vec, cmd_tpye);
}

void EventManager::SetupQueue(std::vector<BaseWorkerEx *> &from, std::vector<BaseWorkerEx *> &to, int cmd_tpye) {
  for (int i = 0; i < (int)from.size(); i++) {
    SetupQueue(from[i], to, cmd_tpye);
  }
}

bool EventManager::Loop() {
  do {
    if ( !installQuitSignal(eventQuitSignalHandler) ) {
      ASSERT("Unable to install the quit signal");
      return false;
    }

    if ( !installSignal(SIGPIPE, SIG_IGN) ) {
      ASSERT("Unable to install the SIGPIPE signal");
      return false;
    }

    bool daemon = GsdmConfig::GetConfig()->GetBool("daemon", false);
    if ( daemon && !doDaemon() ) {
      FATAL("Unable to start daemonize. doDaemon() failed");
      return false;
    }

    std::string log_dir = GsdmConfig::GetConfig()->GetString("log_dir", "");
    int min = INT32_MAX;
    int index = 0;
    if (cpu_.empty()) {
      index = -1;
    } else {
      for (int i = 0; i < (int)cpu_.size(); i++) {
        if ( min > cpu_[i] ) {
          min = cpu_[i];
          index = i;
        }
      }
      cpu_[index] += 1;
    }
    std::string log_unix_path = format("%s/gsdm_log_%u", GSDM_BACK_ROOT_DIR, getpid());
    bool log_color = GsdmConfig::GetConfig()->GetBool("log_color", false);
    int log_level = _INFO_;
    std::string log_level_str = GsdmConfig::GetConfig()->GetString("log_level", "INFO");
    trim(log_level_str);
    if ( "FATAL" == log_level_str )
      log_level = _FATAL_;
    else if ( "ERROR" == log_level_str )
      log_level = _ERROR_;
    else if ( "WARNING" == log_level_str )
      log_level = _WARNING_;
    else if ( "INFO" == log_level_str )
      log_level = _INFO_;
    else if ( "DEBUG" == log_level_str )
      log_level = _DEBUG_;
    else if ( "FINE" == log_level_str )
      log_level = _FINE_;
    else if ( "FINEST" == log_level_str )
      log_level = _FINEST_; 

    if ( !GLogger::Initialize(log_unix_path, log_dir, log_level, log_color, index) ) {
      FATAL("Logger::Initialize(%s, %d) failed", STR(log_unix_path), index);
      return false;
    }
   
    // change user
    std::string user = GsdmConfig::GetConfig()->GetString("user", "");
    if ( "" != user && !changeUser(user, GLogger::GetLogDir()) ) {
      FATAL("changeUser() failed");
      return false;
    }

    // start log thrad
    if ( 0 != pthread_create(&log_pthread_, NULL, GLogger::LogRun, NULL) ) {
      FATAL("pthread_create GLogger::LogRun is failed, errno is %d", errno);
      return false;
    }

    // start thread
    for (int i = 1; i < (int)worker_.size(); i++) {
      pthread_t thread;
      int rs = pthread_create(&thread, NULL, NetWorker::Loop, worker_[i]);
      if ( rs ) {
        FATAL("pthread_create failed, errno is %d", rs);
        return false;
      }
      usleep(30);
    }
  } while(0);

  worker_[0]->Run();
  
  for (int i = 1; i < (int)worker_.size(); i++) {
    worker_[i]->Clear();
  }

  usleep(10);
  return true;
}

void EventManager::Stop() {
  for (int i = 0; i < (int)worker_.size(); i++) {
    worker_[i]->Close();
  }
}

void EventManager::Close() {
  WARN("gsdm stop now");
  for (int i = 0; i < (int)worker_.size(); i++) {
    delete worker_[i];
  }
  worker_.clear();

  int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  GLogger::Close(fd);
  close(fd);
  if ( (pthread_t)-1 != log_pthread_ )
    pthread_join(log_pthread_, NULL);
}




}





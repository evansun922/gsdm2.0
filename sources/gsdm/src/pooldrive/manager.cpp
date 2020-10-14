#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <libgen.h>
#include "logging/logger.h"
#include "utils/gsdmconfig.h"
#include "pooldrive/poolworkerex.h"
#include "pooldrive/stworker.h"
#include "pooldrive/sysworker.h"
#include "pooldrive/manager.h"

extern char *optarg;
extern char **environ;

namespace gsdm {

PoolManager PoolManager::myself_;
static char **pool_argv;
static char *pool_argv_end;
char *pool_env = NULL;

static void poolQuitSignalHandler(int sig) {
  PoolManager::GetManager()->Stop();
}

PoolManager::PoolManager() :log_pid_((pid_t)-1),is_close_(NULL),process_name_("") {
  int32_t total = getCPUCount();
  for (int i = 0; i < total; i++)
    cpu_.push_back(0);
}

PoolManager::~PoolManager() {

}

bool PoolManager::Initialize(int argc, char *argv[], void (*usage)(char *prog), const std::string &version) {
  process_name_ = basename(argv[0]);

  size_t size = 0;
  for (int i = 0; environ[i]; i++) {
    size += strlen(environ[i]) + 1;
  }
  pool_env = new char[size];
  
  pool_argv = (char **)argv;
  pool_argv_end = (char *)argv[0];
  for (int i = 0; argv[i]; i++) {
    if ( pool_argv_end == argv[i] ) {
      pool_argv_end = argv[i] + strlen(argv[i]) + 1;
    }
  }

  char *p = pool_env;
  for (int i = 0; environ[i]; i++) {
    if ( pool_argv_end == environ[i] ) {
      size = strlen(environ[i]) + 1;
      pool_argv_end = environ[i] + size;

      strncpy(p, environ[i], size);
      environ[i] = (char *)p;
      p += size;
    }
  }

  pool_argv_end--;

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
        return false;
      }
      case 'v': {
        printf("The version is \"%s\".\n", STR(version));
        return false;
      }
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
        return false;
      }
      case 'f': {
        config_txt = ::optarg;
        break;
      }
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

void PoolManager::AddWorker(PoolWorkerEx *ex) {
  // int min = INT32_MAX;
  // uint32_t index = 0;
  // for (uint32_t i = 0; i < cpu_.size(); i++) {
  //   if ( min > cpu_[i] ) {
  //     min = cpu_[i];
  //     index = i;
  //   }
  // }
  // cpu_[index]++;

  PoolWorker *w;
  if ( POOLWORKERMODE_SYS == ex->pool_mode_ ) {
    w = new SysPoolWorker(ex);
  } else {
    w = new StPoolWorker(ex);
  }
  worker_.push_back(w);
  ex->worker_ = w;
}

bool PoolManager::Loop() {
  if ( !installQuitSignal(poolQuitSignalHandler) ) {
    ASSERT("Unable to install the quit signal");
    return false;
  }

  // std::string name;
  std::tr1::unordered_map<pid_t, PoolWorker *> w_hash;
  do{
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
    for (int i = 0; i < (int)cpu_.size(); i++) {
      if ( min > cpu_[i] ) {
        min = cpu_[i];
        index = i;
      }
    }
    cpu_[index] += 1;
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

    // char n[256] = { 0 };
    // prctl(PR_GET_NAME, n, NULL, NULL, NULL);
    // process_name_ = n;

    prctl(PR_SET_NAME, STR(process_name_));


    // start log 
    log_pid_ = fork();
    if ( -1 == log_pid_ ) {
      FATAL("fork GLogger::LogRun is failed, errno is %d", errno);
      return false;
    } else if ( 0 == log_pid_ ) {
      installQuitSignal(SIG_IGN);
      SetTitle("gsdm-log", false);
      GLogger::LogRun(NULL);
      Free();
      exit(0);
    }

    int shm_id = shmget((key_t)getpid(), sizeof(bool), 0666| IPC_CREAT);
    if ( -1 == shm_id ) {
      FATAL("shmget is failed, errno is %d", errno);
      exit(0);
    }

    is_close_ = (bool *)shmat(shm_id, 0, 0);
    if ( (void *)-1 == is_close_ ) {
      shmctl(shm_id, IPC_RMID, 0);
      FATAL("shmat is failed, errno is %d", errno);
      exit(0);
    }
    shmctl(shm_id, IPC_RMID, 0);
    *is_close_ = false;

    // start thread
    for (int i = 0; i < (int)worker_.size(); i++) {
      std::string unix_path = format("%s/gsdm_pooldrive_%d_%d", GSDM_BACK_ROOT_DIR, getpid(), i);
      worker_[i]->address_wait_.sun_family = PF_UNIX;
      strncpy(worker_[i]->address_wait_.sun_path, STR(unix_path), sizeof(worker_[i]->address_wait_.sun_path) - 1);
      worker_[i]->address_len_ = sizeof(worker_[i]->address_wait_.sun_family) + strlen(worker_[i]->address_wait_.sun_path);
    }

    for (int i = 0; i < (int)worker_.size(); i++) {
      worker_[i]->ex_->is_close_ = is_close_;
      worker_[i]->pool_pid_ = fork();
      if ( -1 == worker_[i]->pool_pid_ ) {
        FATAL("fork worker is failed, errno is %d", errno);
        return false;
      } else if ( 0 == worker_[i]->pool_pid_ ) {
        installQuitSignal(SIG_IGN);
        SetTitle(worker_[i]->ex_->worker_title_, false);
        worker_[i]->Run();
        Free();
        exit(0);
      }
      worker_[i]->ex_->is_close_ = NULL;
      w_hash[worker_[i]->pool_pid_] = worker_[i];
    }

    SetTitle("gsdm-master", true);
  } while(0);

  while ( true ) {
    int status;
    pid_t pid = ::wait(&status);
    std::tr1::unordered_map<pid_t, PoolWorker *>::iterator iter = w_hash.find(pid);
    if ( w_hash.end() == iter ) {
      if ( true == *is_close_ )
        break;
      continue;
    }
    
    PoolWorker *w = MAP_VAL(iter);
    w_hash.erase(iter);
    if ( true == *is_close_ ) {
      w->pool_pid_ = -1;
      break;
    }
    
    w->ex_->is_close_ = is_close_;
    w->pool_pid_ = fork();
    if ( -1 == w->pool_pid_ ) {
      FATAL("fork worker is failed, errno is %d", errno);
      return false;
    } else if ( 0 == w->pool_pid_ ) {
      installQuitSignal(SIG_IGN);
      SetTitle(w->ex_->worker_title_, false);
      w->Run();
      Free();
      exit(0);
    }
    w->ex_->is_close_ = NULL;
    w_hash[w->pool_pid_] = w;
  }
  return true;
}

void PoolManager::Stop() {
  *is_close_ = true;
  int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  for (int i = 0; i < (int)worker_.size(); i++) {
    sendto(fd, "C", 1, 0, (const struct sockaddr *)&worker_[i]->address_wait_, 
           sizeof(worker_[i]->address_wait_.sun_family) + strlen(worker_[i]->address_wait_.sun_path));
  }
  close(fd);
}

void PoolManager::Close() {
  for (int i = 0; i < (int)worker_.size(); i++) {
    if ( -1 != worker_[i]->pool_pid_ ) {
      int status;
      waitpid(worker_[i]->pool_pid_, &status, 0);
    }
    delete worker_[i];
  }
  worker_.clear();

  int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  GLogger::Close(fd);
  close(fd);

  int status;
  waitpid(log_pid_, &status, 0);

  if ( is_close_ ) {
    shmdt(is_close_);
    is_close_ = NULL;
  }

  if ( pool_env ) {
    delete [] pool_env;
    pool_env = NULL;
  }
}

void PoolManager::Free() {
  for (int i = 0; i < (int)worker_.size(); i++) {
    delete worker_[i];
  }
  worker_.clear();

  if ( is_close_ ) {  
    shmdt(is_close_);  
    is_close_ = NULL;  
  }

  if ( pool_env ) {
    delete [] pool_env;
    pool_env = NULL;
  }
}

void PoolManager::SetTitle(const std::string &title, bool cmd) {
  int end = pool_argv_end - pool_argv[0];
  char buf[1024] = { 0 };
  int rs = snprintf(buf, 1024, "%s: %s process", STR(process_name_), STR(title));
  if ( cmd ) {
    for(int i = 0; pool_argv[i] && 1024 > rs; i++) {
      rs += snprintf(buf+rs, 1024-rs, " %s", pool_argv[i]);
    }
  }
  
  pool_argv[1] = NULL;  
  strncpy(pool_argv[0], buf, end);
  if ( end > rs ) {
    memset(pool_argv[0]+rs, 0 , end - rs); 
  }
}


}





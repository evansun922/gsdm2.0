/*
 *  Copyright (C) 2012, 
 *  sunlei
 *
 */
#include "logging/logger.h"
#include "utils/gsdmconfig.h"

namespace gsdm {

static const char *g_logger_level_str[] = { "FATAL", "ERROR", "WARNING", "INFO", "DEBUG", "FINE", "FINEST" };
static const char *g_logger_colors[] = { FATAL_COLOR, ERROR_COLOR, WARNING_COLOR, INFO_COLOR, DEBUG_COLOR, 
                                         FINE_COLOR, FINEST_COLOR, NORMAL_COLOR };

//static const char hexdig[] = "0123456789ABCDEF";

__thread int g_log_fd = -1;

int GLogger::debug_level_ = _INFO_;
FILE *GLogger::log_ = stdout;
FILE *GLogger::stat_ = stdout;
bool GLogger::allow_colors_ = true;
std::string GLogger::dir_ = "";
time_t GLogger::check_ = 0;
bool GLogger::log_close_ = true;
sockaddr_un GLogger::log_address_;
socklen_t GLogger::log_address_len_ = 0;
int GLogger::cpu_ = 0;
bool GLogger::no_cache_ = false;

bool GLogger::Initialize(const std::string &log_unix_path, const std::string &log_dir, int log_level, bool log_colors, int cpu) {
  remove(STR(log_unix_path));
  memset(&log_address_, 0, sizeof (sockaddr_un));
  log_address_.sun_family = PF_UNIX;
  strncpy(log_address_.sun_path, STR(log_unix_path), sizeof(log_address_.sun_path) - 1);
  log_address_len_ = sizeof(log_address_.sun_family) + strlen(log_address_.sun_path);

  dir_ = log_dir;
  debug_level_ = log_level;
  allow_colors_ = log_colors;
  cpu_ = cpu;
  return true;
}
  
void GLogger::Log(int level, const char *file_name, uint32_t line_number, const char *format_string, ...) {
  if ( level > debug_level_ )
    return;

  char str[MAX_PRINT_LEN] = { 0 };
  int len = 0;
  if ( allow_colors_ ) {
    len = snprintf(str, MAX_PRINT_LEN - sizeof(NORMAL_COLOR"\n"), 
                   "l%s%s:%u(%u) [%s] [%s] ", g_logger_colors[level], file_name, line_number, getTid(), g_logger_level_str[level],
                   STR(getCurrentGsdmMsecStr()));
    va_list args;
    va_start(args, format_string);
    int a = MAX_PRINT_LEN - len - sizeof(NORMAL_COLOR"\n");
    int b = vsnprintf(str+len, a, format_string, args);
    len += (b > a ? a : b);
    va_end(args);
    memcpy(str+len, NORMAL_COLOR"\n", sizeof(NORMAL_COLOR"\n")-1);
    len += (sizeof(NORMAL_COLOR"\n")-1);
  } else {
    len = snprintf(str, MAX_PRINT_LEN - sizeof("\n"), "l%s:%u(%u) [%s] [%s] ", file_name, line_number, getTid(), g_logger_level_str[level],
                   STR(getCurrentGsdmMsecStr()));
    va_list args;
    va_start(args, format_string);
    int a = MAX_PRINT_LEN - len - sizeof("\n");
    int b = vsnprintf(str+len, a, format_string, args);
    len += (b > a ? a : b);
    va_end(args);
    str[len++] = '\n';
  }

  if ( /*log_close_ ||*/ -1 == g_log_fd ) {
    fprintf(log_, "%s", str + 1);
  } else {
    sendto(g_log_fd, str, len, 0, (const struct sockaddr *)&log_address_, log_address_len_);
  }
}

void GLogger::Statistics(const char *format_string, ...) {  
  char str[MAX_PRINT_LEN] = { 0 };
  int len = snprintf(str, MAX_PRINT_LEN - sizeof("\n"), "s[%s] ", STR(getCurrentGsdmMsecStr()));
	va_list args;
	va_start(args, format_string);
  int a = MAX_PRINT_LEN - len - sizeof("\n");
  int b = vsnprintf(str + len, a, format_string, args);
  len += (b > a ? a : b);
	va_end(args);
  str[len++] = '\n';

  if ( /*log_close_ ||*/ -1 == g_log_fd ) {
    fprintf(stat_, "%s", str + 1);
  } else {
    sendto(g_log_fd, str, len, 0, (const struct sockaddr *)&log_address_, log_address_len_);
  }
}

void *GLogger::LogRun(void *arg) {
  if (cpu_ >= 0) {
    std::vector<int> cpus;
    cpus.push_back(cpu_);
    if ( !setaffinityNp(cpus, pthread_self()) ) {
      FATAL("setaffinityNp failed.");
      abort();
    }
  }

  int log_listen = socket(PF_UNIX, SOCK_DGRAM, 0);
  if ( -1 == log_listen ) {
    FATAL("Create log_listen failed errno %d", errno);
    abort();
  }

  if ( !setFdNonBlock(log_listen) ) {
    FATAL("setFdNonBlock failed");
    abort();
  }

  if ( !setFdRcvBuf(log_listen, 1024*1024*50) ) {
    FATAL("setFdRcvBuf failed");
    abort();
  }

  if ( 0 != bind(log_listen, (struct sockaddr *)&log_address_, log_address_len_) ) {
    FATAL("Bind log_listen failed errno %d", errno);
    abort();
  }

  int log_ep = epoll_create(1024);
  if ( -1 == log_ep ) {
    FATAL("epoll_create failed %d", errno);
    abort();
  }

  struct epoll_event evt = {0, {0}};
  evt.events = EPOLLIN;
  evt.data.fd = log_listen;
  if ( 0 != epoll_ctl(log_ep, EPOLL_CTL_ADD, log_listen, &evt) ) {
    FATAL("Unable to enable read data: (%d) %s", errno, strerror(errno));
    abort();
  }

  if ( false == SetupLogFile() ) {
    INFO("SetupLogFile failed");
    abort();
  }
  
  int count;
  epoll_event query[1024];
  char log_buffer[MAX_PRINT_LEN];
  log_close_ = false;
  while ( !log_close_ ) {
    if ( (count = epoll_wait(log_ep, query, 1024, 5000)) < 0 ) {
      if (EINTR == errno) {
        continue;
      }
      abort();
    }

    ChangeLogFile(time(0));

    if ( 0  < count ) {
      int rs = recvfrom(log_listen, log_buffer, MAX_PRINT_LEN, 0, NULL, NULL);
      if ( 0 < rs ) {
        log_buffer[rs] = 0;
        if ( 'l' == log_buffer[0] ) {
          fprintf(log_, "%s", log_buffer + 1);
          if ( no_cache_ )
            fflush(log_);
        } else if ( 's' == log_buffer[0] ) {
          fprintf(stat_, "%s", log_buffer + 1);
          if ( no_cache_ )
            fflush(stat_);
        } else if ( 'u' == log_buffer[0] ) {
          std::string v = log_buffer + 1;
          std::vector<std::string> vec;
          split(v, ",", vec);
          if ( 3 == vec.size() ) {
            v = vec[0];
            trim(v);
            if ( "FATAL" == v )
              debug_level_ = _FATAL_;
            else if ( "ERROR" == v )
              debug_level_ = _ERROR_;
            else if ( "WARNING" == v )
              debug_level_ = _WARNING_;
            else if ( "INFO" == v )
              debug_level_ = _INFO_;
            else if ( "DEBUG" == v )
              debug_level_ = _DEBUG_;
            else if ( "FINE" == v )
              debug_level_ = _FINE_;
            else if ( "FINEST" == v )
              debug_level_ = _FINEST_;

            v = vec[1];
            trim(v);
            if ( "yes" == v ) {
              allow_colors_ = true;
            } else if ( "no" == v ) {
              allow_colors_ = false;
            }

            v = vec[2];
            trim(v);
            if ( "yes" == v ) {
              no_cache_ = false;
            } else if ( "no" == v ) {
              no_cache_ = true;
            }
          }
        } else if ( 'c' == log_buffer[0] ) {
          // close log
          log_close_ = true;
          break;
        }
      }
    }
  }
  
  close(log_listen);
  close(log_ep);

  fflush(stat_);
  fflush(log_);
  fclose(stat_);
  fclose(log_);
  return NULL;
}

bool GLogger::SetupLogFile() {
  if ( "" == dir_ ) {
    return true;
  }

  if ( !fileExists(dir_) ) {
    FATAL("The \"%s\" path of log is invalid\n", STR(dir_));
    return false;
  }
  
  char dummy1[PATH_MAX];
  if ( NULL == realpath(STR(dir_), dummy1)) {
    FATAL("The \"%s\" realpath() failed %d", STR(dir_), errno);
    return false;
  }

  dir_ = dummy1;
  time_t now = time(0);

  check_ = now;
  struct tm tm;
  gsdm_localtime_r(&check_, &tm, 8);
  tm.tm_sec = 0;
  tm.tm_min = 0;
  //tm.tm_hour = 0;    
  time_t tmp = mktime(&tm);
  if (tmp <= check_) {
    for (int i = 0; true; i++) {
      if (tmp+i*CHANGE_FILE_TIME >= check_) {
        check_ = tmp+i*CHANGE_FILE_TIME;
        break;
      }
    }
  } else {
    check_ = tmp;
  }
  
  std::string d = format("%s/%s", STR(dir_), STR(getLocalTimeString("%Y%m%d", now)));
  if ( !fileExists(d) ) {
    createFolder(d, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  std::string log_dir = d + "/log";
  if ( !fileExists(log_dir) ) {
    createFolder(log_dir, S_IRWXU | S_IRWXG | S_IRWXO);
  }
  
  std::string stat_dir = d + "/stat";
  if ( !fileExists(stat_dir) ) {
    createFolder(stat_dir, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  char log_path[256], stat_path[256];
  std::string name = getLocalTimeString("%H_%M", now);
  snprintf(log_path, 255, "%s/%d_%s.txt", STR(log_dir), getpid(), STR(name));
  snprintf(stat_path, 255, "%s/%d_%s.txt", STR(stat_dir), getpid(), STR(name));
  
  FILE *new_file = fopen(log_path, "w+");
  if (!new_file) {
    FATAL("The \"%s\" path of log is invalid\n", log_path);
    return false;
  }
  SetLogFile(new_file);
  
  new_file = fopen(stat_path, "w+");
  if (!new_file) {
    FATAL("The \"%s\" path of stat is invalid\n", stat_path);
    return false;
  }
  SetLogStat(new_file);
  
  std::string check_str = getLocalTimeString("%Y-%m-%d %H:%M:%S", check_);
  INFO("Log check time is %s", STR(check_str));

  return true;
}

FILE *GLogger::SetLogFile(FILE *file) {
  FILE *tmp = log_;
	log_ = file;
  return tmp;
}

FILE *GLogger::GetLogFile() {
  return log_;
}

int32_t GLogger::GetLogLevel() {
	return debug_level_;
}

FILE *GLogger::SetLogStat(FILE *file) {
  FILE *tmp = stat_;
	stat_ = file;
  return tmp;
}

FILE *GLogger::GetLogStat() {
  return stat_;
}

bool GLogger::ChangeLogFile(time_t now) {
  if (dir_ == "" || check_ >= now)
    return true;
  
  check_ += CHANGE_FILE_TIME;

  std::string dir = format("%s/%s", STR(dir_), STR(getLocalTimeString("%Y%m%d", now)));
  std::string log_dir = dir + "/log";
  std::string stat_dir = dir + "/stat";
  if ( !fileExists(dir) ) {
    createFolder(dir, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  if ( !fileExists(log_dir) ) {
    createFolder(log_dir, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  if ( !fileExists(stat_dir) ) {
    createFolder(stat_dir, S_IRWXU | S_IRWXG | S_IRWXO);
  }

  char log_path[256], stat_path[256];
  std::string hour = getLocalTimeString("%H_%M", now);
  snprintf(log_path, 255, "%s/%d_%s.txt", STR(log_dir), getpid(), STR(hour));
  snprintf(stat_path, 255, "%s/%d_%s.txt", STR(stat_dir), getpid(), STR(hour));

  FILE *nowLog, *newStat, *oldLog, *oldStat;
  nowLog = fopen(log_path, "w+");
  if (!nowLog) {
    FATAL("Can open \"%s\" errno %d", log_path, errno);
    return false;
  }
  oldLog = SetLogFile(nowLog);

  newStat = fopen(stat_path, "w+");
  if (!newStat) {
    FATAL("Can open \"%s\" errno %d", stat_path, errno);
    fclose(oldLog);
    return false;
  }
  oldStat = SetLogStat(newStat);
      
  fclose(oldLog);
  fclose(oldStat);

  if ( "04_00" == hour ) {
    std::string day = getLocalTimeString("%Y%m%d", now - (86400/*day*/ * GsdmConfig::GetConfig()->GetInt("clear_day", 5)));
    int num = atoi(STR(day));
    
    DIR *pDir = opendir(STR(dir_));
    if (pDir == NULL) {
      FATAL("Unable to open folder: %s %d %s", STR(dir_), errno, strerror(errno));
      return true;
    }

    struct dirent *pDirent = NULL;
    while ((pDirent = readdir(pDir)) != NULL) {
      std::string entry = pDirent->d_name;
      if ((entry == ".") || (entry == "..")) {
        continue;
      }

      if ( entry.empty() )
        continue;

      std::string full_entry = dir_ + "/" + entry;

      if ( isDirectory(full_entry) ) {
        int tmp_num = atoi(STR(entry));
        if ( tmp_num < num ) {
          deleteFolder(full_entry);
        }
      }
    }

    closedir(pDir);
  }

  return true;
}

void GLogger::Close(int fd) {
  log_close_ = true;
  sendto(fd, "c", 1, 0, (const struct sockaddr *)&log_address_, log_address_len_);
  deleteFile(log_address_.sun_path);
}

int32_t GLogger::GetLogThreadLogFd() {
  return g_log_fd;
}

void GLogger::SetLogThreadLogFd(int fd) {
  g_log_fd = fd;
}


// void Logger::LogHex(int level, const uint8_t *data, uint32_t len) {  
//   if ( len >= (MAX_PRINT_LEN/3)-3 )
//     len = (MAX_PRINT_LEN/3)-3;

//   // char str[MAX_PRINT_LEN] = { 0 };
//   LogCmdList *cmd_list = (LogCmdList *)pthread_getspecific(gsdm_log_cmd_key);
//   LogCmd *cmd = cmd_list->GetCycle();
//   if( !cmd )
//     cmd = new LogCmd();

//   uint32_t i;
//   cmd->buffer_[0] = 'l';
//   char *ptr;
//   ptr = cmd->buffer_ + 1;

//   for(i = 0; i < len; i++) {
//     *ptr++ = hexdig[0x0f & (data[i] >> 4)];
//     *ptr++ = hexdig[0x0f & data[i]];
//     if ((i & 0x0f) == 0x0f) {
//       *ptr++ = '\n';
//     } else {
//       *ptr++ = ' ';
//     }
//   }
//   *ptr++ = '\n';

//   cmd_list->Insert(cmd);
//   *ptr = 0;
//   if ( 0 < logger_.log_ep_ ) {
//     // send(*((int32_t*)pthread_getspecific(gsdm_log_socket_key)), str, ptr - str, 0);
//     send(logger_.log_sock_, "A", 1, 0);
//     return;
//   }

//   fprintf(logger_.log_, "%s", cmd->buffer_ + 1);
//   cmd_list->GetData();
// }

// bool Logger::Initialize(const std::string &log_unix_path, int bind_cpu) {
//   // SetLogLevel(level);
//   // SetLogColors(allow_colors);

//   if ( "" != log_unix_path ) {

    
//     log_sock_ = (int) socket(PF_UNIX, SOCK_DGRAM, 0);
//     if ( -1 == log_sock_ ) {
//       int32_t err = errno;
//       FATAL("Unable to crate socket: (%d) %s", err, strerror(err));
//       return false;
//     }

//     if ( -1 == connect(log_sock_, (struct sockaddr *)&address, socket_len) ) {
//       int32_t err = errno;
//       FATAL("Unable to bind: (%d) %s", err, strerror(err));
//       return false;
//     }

//     if (!setFdOptions(log_sock_, false)) {
//       FATAL("Unable to set socket options");
//       return false;
//     }

//     int rs = pthread_create(&log_thread_, NULL, Logger::LogRun, NULL);
//     if ( rs  ) {
//       FATAL("pthread_create failed, errno is %d", rs);
//       return false;
//     }
    
//     std::vector<int> cpu_ids;
//     cpu_ids.push_back(bind_cpu);
//     if ( !setaffinityNp(cpu_ids, log_thread_) ) {
//       FATAL("cpuID:%d setaffinityNp failed.", bind_cpu);
//       return false;
//     }

//     usleep(5000);
//   }

//   return true;
// }





// bool Logger::Chown(const std::string &dir, uid_t owner, gid_t group) {
//   if (0 != chown(STR(dir), owner, group)) {
//     FATAL("Unable to start daemonize. %s fchown() failed %d", STR(dir), errno);
//     return false;
//   }

//   std::vector<std::string> result;
//   if ( !listFolder(dir, result, true, true, true) )
//     return false;

//   for (uint32_t i = 0; i < result.size(); i++) {
//     if (0 != chown(STR(result[i]), owner, group)) {
//       FATAL("Unable to start daemonize. %s fchown() failed %d", STR(result[i]), errno);
//       return false;
//     }
//   }

//   return true;
// }







}

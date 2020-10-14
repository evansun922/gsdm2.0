#ifndef _LOGGER_H
#define _LOGGER_H

#include "platform/linuxplatform.h"

#define GSDM_BACK_ROOT_DIR "/tmp/.gsdm1"

#define _DEBUG_ 4
#define _ERROR_ 1
#define _FATAL_ 0
#define _FINEST_ 6
#define _FINE_ 5
#define _INFO_ 3
#define _WARNING_ 2

#define COLOR_TYPE std::string
#define FATAL_COLOR "\033[01;31m"
#define ERROR_COLOR "\033[22;31m"
#define WARNING_COLOR "\033[01;33m"
#define INFO_COLOR "\033[22;36m"
#define DEBUG_COLOR "\033[01;37m"
#define FINE_COLOR "\033[22;37m"
#define FINEST_COLOR "\033[22;37m"
#define NORMAL_COLOR "\033[0m"
#define SET_CONSOLE_TEXT_COLOR(color) std::cout<<color

#define MAX_PRINT_LEN      1500
#define CHANGE_FILE_TIME   3600

namespace gsdm {


class GLogger {
public:
  
  static bool Initialize(const std::string &log_unix_path, const std::string &log_dir, int log_level, bool log_colors, int cpu);
  static void Log(int level, const char *file_name, uint32_t line_number, const char *format_string, ...); 
  static void Statistics(const char *format_string, ...);
  // static void LogHex(int level, const uint8_t *data, uint32_t len);
  static void *LogRun(void *arg);
  static std::string GetLogDir() { return dir_; }
  static void Close(int fd);  
  static int32_t GetLogLevel(); 
  static int32_t GetLogThreadLogFd(); 
  static void SetLogThreadLogFd(int fd); 

private:

  static bool SetupLogFile();
  static FILE *SetLogFile(FILE *file);
  static FILE *GetLogFile();
  static FILE *SetLogStat(FILE *file);
  static FILE *GetLogStat();
  static bool ChangeLogFile(time_t now);


  static int debug_level_;
  static FILE *log_, *stat_;
  static bool allow_colors_;
  static std::string dir_;
  static time_t check_;
  static bool log_close_;
  static sockaddr_un log_address_;
  static socklen_t log_address_len_;
  static int cpu_;
  static bool no_cache_;
};

#define LOG(level,...) do{ GLogger::Log(level, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__); }while(0)
#define FATAL(...) do{ GLogger::Log(_FATAL_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)

#define WARN(...) do{GLogger::Log(_WARNING_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)
#define INFO(...) do{GLogger::Log(_INFO_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)
#define DEBUG(...) do{GLogger::Log(_DEBUG_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)
#define FINE(...) do{GLogger::Log(_FINE_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)
#define FINEST(...) do{GLogger::Log(_FINEST_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);}while(0)
#define ASSERT(...) do{GLogger::Log(_FATAL_, __FILE__, __LINE__, /*__func__,*/ __VA_ARGS__);assert(false);abort();}while(0) 
//#define LOGHEX(data,len) do{ Logger::LogHex(_DEBUG_, data, len); }while(0)
#define STAT(...) do{ GLogger::Statistics(__VA_ARGS__); }while(0)



#define NYI WARN("%s not yet implemented",__func__);
#define NYIR do{NYI;return false;}while(0)
#define NYIA do{NYI;assert(false);abort();}while(0)

}


#endif // _LOGGER_H

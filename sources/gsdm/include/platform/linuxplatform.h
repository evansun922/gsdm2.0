#ifndef _LINUXPLATFORM_H
#define _LINUXPLATFORM_H

#define __STDC_FORMAT_MACROS
#include <unistd.h>
#include <inttypes.h>
#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <cctype>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <glob.h>
#include <iostream>
#include <list>
#include <map>
#include <bitset>
//#include <ext/hash_map>
#include <unordered_map>
#include <unordered_set>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <memory>
#include <set>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif
#include <pwd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <semaphore.h>
#include <iomanip>
#include <iconv.h>

#define timespecsub(a, b, result)                           \
  do {                                                      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;           \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;			  \
    if ((result)->tv_nsec < 0) {                            \
      --(result)->tv_sec;                                   \
      (result)->tv_nsec += 1000000000;                      \
    }                                                       \
  } while (0)

#define timespecadd(a, b, result)                           \
  do {                                                      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;        \
    if ((result)->tv_nsec >= 1000000000) {                  \
      ++(result)->tv_sec;                                   \
      (result)->tv_nsec -= 1000000000;                      \
    }                                                       \
  } while (0)

#define STR(x) ((x).c_str())//(((string)(x)).c_str())
#define MAP_HAS1(m,k) ((bool)((m).find((k))!=(m).end()))
#define MAP_HAS2(m,k1,k2) ((MAP_HAS1((m),(k1))==true)?MAP_HAS1((m)[(k1)],(k2)):false)
#define MAP_HAS3(m,k1,k2,k3) ((MAP_HAS1((m),(k1)))?MAP_HAS2((m)[(k1)],(k2),(k3)):false)
#define FOR_MAP(m,k,v,i) for(std::map< k , v >::iterator i=(m).begin();i!=(m).end();++i)
#define FOR_UNORDERED_MAP(m,k,v,i) for(std::unordered_map< k , v >::iterator i=(m).begin();i!=(m).end();++i)
#define MAP_KEY(i) ((i)->first)
#define MAP_VAL(i) ((i)->second)
#define MAP_ERASE1(m,k) if(MAP_HAS1((m),(k))) (m).erase((k));
#define MAP_ERASE2(m,k1,k2) \
if(MAP_HAS1((m),(k1))){ \
    MAP_ERASE1((m)[(k1)],(k2)); \
    if((m)[(k1)].size()==0) \
        MAP_ERASE1((m),(k1)); \
}
#define MAP_ERASE3(m,k1,k2,k3) \
if(MAP_HAS1((m),(k1))){ \
    MAP_ERASE2((m)[(k1)],(k2),(k3)); \
    if((m)[(k1)].size()==0) \
        MAP_ERASE1((m),(k1)); \
}

#define FOR_VECTOR(v,i) for(uint32_t i=0;i<(v).size();i++)
#define FOR_VECTOR_ITERATOR(e,v,i) for(std::vector<e>::iterator i=(v).begin();i!=(v).end();i++)
#define FOR_VECTOR_WITH_START(v,i,s) for(uint32_t i=s;i<(v).size();i++)
#define ADD_VECTOR_END(v,i) (v).push_back((i))
#define ADD_VECTOR_BEGIN(v,i) (v).insert((v).begin(),(i))
#define VECTOR_VAL(i) (*(i))

#define CLOSE_SOCKET(fd) if((fd)>=0) close((fd))



namespace gsdm {

typedef uint64_t gsdm_msec_t;

class AutoRWLock {
public:
  AutoRWLock(pthread_rwlock_t *lock, bool isWrite);
  ~AutoRWLock();

private:
  pthread_rwlock_t *_lock;
};

class AutoSpinLock {
public:
  AutoSpinLock(pthread_spinlock_t *lock);
  ~AutoSpinLock();

private:
  pthread_spinlock_t *_lock;
};

class AutoMutexLock {
public:
  AutoMutexLock(pthread_mutex_t *lock);
  ~AutoMutexLock();

private:
  pthread_mutex_t *_lock;
};

typedef void (*SignalFnc)(int);



std::string format(const std::string &fmt, ...);
std::string vFormat(const std::string &fmt, va_list args);
void replace(std::string &target, const std::string &search, const std::string &replacement);
bool fileExists(const std::string &path);
bool isDirectory(const std::string &path);
bool isFile(const std::string &path);
std::string lowerCase(const std::string &value);
std::string upperCase(const std::string &value);
std::string changeCase(const std::string &value, bool lowerCase);
std::string tagToString(uint64_t tag);
bool setFdNonBlock(int32_t fd);
bool setFdNoSIGPIPE(int32_t fd);
bool setFdKeepAlive(int32_t fd);
bool setFdNoNagle(int32_t fd);
bool setFdReuseAddress(int32_t fd);
bool setFdTTL(int32_t fd, uint8_t ttl);
bool setFdRcvBuf(int32_t fd, int32_t rcvbuf);
bool setFdSndBuf(int32_t fd, int32_t sndbuf);
bool setFdMulticastTTL(int32_t fd, uint8_t ttl);
bool setFdTOS(int32_t fd, uint8_t tos);
bool setFdOptions(int32_t fd, bool set_nagle);
bool deleteFile(const std::string &path);
bool deleteFolder(const std::string &path);
bool createFolder(const std::string &path, mode_t mode);
std::string getHostByName(const std::string &name);
bool isNumeric(const std::string &value, int base);

void split(const std::string &str, const std::string &separator, std::vector<std::string> &result);
uint64_t getTagMask(uint64_t tag);
std::string generateRandomString(uint32_t length);
void lTrim(std::string &value);
void rTrim(std::string &value);
void trim(std::string &value);
int8_t getCPUCount();
std::map<std::string, std::string> mapping(const std::string &str, const std::string &separator1, const std::string &separator2, bool trimStrings);
void splitFileName(const std::string &fileName, std::string &name, std::string &extension, char separator = '.');
std::string getFileName(const std::string &path);
double getFileModificationDate(const std::string &path);
// std::string normalizePath(const std::string &base, const std::string &file);
std::string normalizePath(const std::string &file);
bool listFolder(const std::string &path, std::vector<std::string> &result,
                bool normalizeAllPaths = true, bool includeFolders = false,
                bool recursive = true);
bool moveFile(const std::string &src, const std::string &dst);
bool installSignal(int sig, SignalFnc pSignalFnc);
bool installQuitSignal(SignalFnc pQuitSignalFnc);
bool installConfRereadSignal(SignalFnc pConfRereadSignalFnc);
void getAllHostIP(std::map<std::string , std::string> &ips);
std::string getPublicIP();
std::string getPrivateIP();
std::string getHostIPv4(uint32_t addr);
std::string getHostIPv6(const struct in6_addr *sin6_addr);
uint32_t getHostIPv4(const std::string &ip);
std::string getLocalTimeString(const char *format, time_t t);
bool doDaemon();
bool changeUser(const std::string &user, const std::string &log_path);
bool setLimitNoFile(uint64_t limit);
bool setLimitCore(uint64_t limit);
pid_t getTid(void);
bool setaffinityNp(std::vector<int> &cpuID, pthread_t thread);
std::string getStrerror(int errnum);
size_t gsdmIconv(const std::string &fromcodee, const std::string &tocode, char *inbuf, size_t inlen, char *outbuf, size_t outlen);
gsdm_msec_t getCurrentGsdmMsec();
uint64_t getCurrentGsdmMicro();
struct tm* gsdm_localtime_r(const time_t *unix_sec, struct tm* tm, int time_zone);
std::string getCurrentGsdmMsecStr();
void getPidByName(std::vector<pid_t> &pids, const std::string &task_name);
std::string getNameByPid(pid_t pid);
char *rstrstr(const char *str, int str_len, const char *needle, int needle_len);

}



#endif // _LINUXPLATFORM_H

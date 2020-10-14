/*
 *  Copyright (c) 2012,
 *  sunlei 
 */



#include "platform/linuxplatform.h"
#include "logging/logger.h"

namespace gsdm {

static std::string alowedCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";


AutoRWLock::AutoRWLock(pthread_rwlock_t *lock, bool isWrite) :_lock(lock) {
  if ( isWrite )
    pthread_rwlock_wrlock(_lock);
  else
    pthread_rwlock_rdlock(_lock);
}

AutoRWLock::~AutoRWLock() {
  if (_lock) {
    pthread_rwlock_unlock( _lock );
    _lock = NULL;
  }
}

AutoSpinLock::AutoSpinLock(pthread_spinlock_t *lock):_lock(lock) {
  pthread_spin_lock(_lock);
}

AutoSpinLock::~AutoSpinLock() {
  pthread_spin_unlock(_lock);
}

AutoMutexLock::AutoMutexLock(pthread_mutex_t *lock):_lock(lock) {
  pthread_mutex_lock(_lock);
}

AutoMutexLock::~AutoMutexLock() {
  pthread_mutex_unlock(_lock);
}

std::string format(const std::string &fmt, ...) {
  std::string result = "";
	va_list arguments;
	va_start(arguments, fmt);
	result = vFormat(fmt, arguments);
	va_end(arguments);
	return result;
}

std::string vFormat(const std::string &fmt, va_list args) {
	char *pBuffer = NULL;
	if (vasprintf(&pBuffer, STR(fmt), args) == -1) {
		assert(false);
		return "";
	}
  std::string result = pBuffer;
	free(pBuffer);
	return result;
}

void replace(std::string &target, const std::string &search, const std::string &replacement) {
	if (search == replacement)
		return;
	if (search == "")
		return;
  std::string::size_type i = std::string::npos;
  std::string::size_type lastPos = 0;
	while ((i = target.find(search, lastPos)) != std::string::npos) {
		target.replace(i, search.length(), replacement);
		lastPos = i + replacement.length();
	}
}

bool fileExists(const std::string &path) {
  return 0 == access(STR(path), F_OK);
}

bool isDirectory(const std::string &path) {
	struct stat s;
	if (stat(STR(path), &s) != 0) {
    FATAL("Unable to stat file %s", STR(path));
		return false;
	}
  return S_ISDIR(s.st_mode);
}

bool isFile(const std::string &path) {
	struct stat s;
	if (stat(STR(path), &s) != 0) {
    FATAL("Unable to stat file %s", STR(path));
		return false;
	}
  return S_ISREG(s.st_mode);
}

std::string lowerCase(const std::string &value) {
	return changeCase(value, true);
}

std::string upperCase(const std::string &value) {
	return changeCase(value, false);
}

std::string changeCase(const std::string &value, bool lowerCase) {
  std::string result = "";
	for (std::string::size_type i = 0; i < value.length(); i++) {
		if (lowerCase)
			result += tolower(value[i]);
		else
			result += toupper(value[i]);
	}
	return result;
}

std::string tagToString(uint64_t tag) {
  std::stringstream result;
	for (uint32_t i = 0; i < 8; i++) {
		uint8_t v = (tag >> ((7 - i)*8)&0xff);
		if (v == 0)
			break;
		result << (char) v;
	}
	return result.str();
}

bool setFdNonBlock(int32_t fd) {
	int32_t arg;
	if ((arg = fcntl(fd, F_GETFL, NULL)) < 0) {
		int32_t err = errno;
    FATAL("Unable to get fd flags: %d,%s", err, strerror(err));
		return false;
	}
	arg |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, arg) < 0) {
		int32_t err = errno;
    FATAL("Unable to set fd flags: %d,%s", err, strerror(err));
		return false;
	}

	return true;
}

bool setFdNoSIGPIPE(int32_t fd) {
	//This is not needed because we use MSG_NOSIGNAL when using
	//send/write functions
	return true;
}

bool setFdKeepAlive(int32_t fd) {
	int32_t one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
			(const char*) & one, sizeof (one)) != 0) {
    FATAL("Unable to set SO_NOSIGPIPE");
		return false;
	}
	return true;
}

bool setFdNoNagle(int32_t fd) {
	int32_t one = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) & one, sizeof (one)) != 0) {
		return false;
	}
	return true;
}

bool setFdReuseAddress(int32_t fd) {
	int32_t one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) & one, sizeof (one)) != 0) {
    FATAL("Unable to reuse address");
		return false;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) & one, sizeof (one)) != 0) {
    FATAL("Unable to reuse port");
		return false;
	}
#endif /* SO_REUSEPORT */
	return true;
}

bool setFdTTL(int32_t fd, uint8_t ttl) {
	int temp = ttl;
	if (setsockopt(fd, IPPROTO_IP, IP_TTL, &temp, sizeof (temp)) != 0) {
		int err = errno;
    WARN("Unable to set IP_TTL: %"PRIu8"; error was %"PRId32" %s", ttl, err, strerror(err));
	}
	return true;
}

bool setFdRcvBuf(int32_t fd, int32_t rcvbuf) {
	int temp = rcvbuf;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &temp, sizeof (temp)) != 0) {
		int err = errno;
    WARN("Unable to set SO_RCVBUF: %"PRIu8"; error was %"PRId32" %s", rcvbuf, err, strerror(err));
	}
	return true;
}

bool setFdSndBuf(int32_t fd, int32_t sndbuf) {
	int temp = sndbuf;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &temp, sizeof (temp)) != 0) {
		int err = errno;
    WARN("Unable to set SO_SNDBUF: %"PRIu8"; error was %"PRId32" %s", sndbuf, err, strerror(err));
	}
	return true;
}

bool setFdMulticastTTL(int32_t fd, uint8_t ttl) {
	int temp = ttl;
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &temp, sizeof (temp)) != 0) {
		int err = errno;
    WARN("Unable to set IP_MULTICAST_TTL: %"PRIu8"; error was %"PRId32" %s", ttl, err, strerror(err));
	}
	return true;
}

bool setFdTOS(int32_t fd, uint8_t tos) {
	int temp = tos;
	if (setsockopt(fd, IPPROTO_IP, IP_TOS, &temp, sizeof (temp)) != 0) {
		int err = errno;
    WARN("Unable to set IP_TOS: %"PRIu8"; error was %"PRId32" %s", tos, err, strerror(err));
	}
	return true;
}

bool setFdOptions(int32_t fd, bool set_nagle) {
	if (!setFdNonBlock(fd)) {
    FATAL("Unable to set non block");
		return false;
	}

	if (!setFdNoSIGPIPE(fd)) {
    FATAL("Unable to set no SIGPIPE");
		return false;
	}

	if (!setFdKeepAlive(fd)) {
    FATAL("Unable to set keep alive");
		return false;
	}

	if (set_nagle && !setFdNoNagle(fd)) {
    WARN("Unable to disable Nagle algorithm");
	}

	if (!setFdReuseAddress(fd)) {
    FATAL("Unable to enable reuse address");
		return false;
	}

	return true;
}

bool deleteFile(const std::string &path) {
	if (remove(STR(path)) != 0) {
    FATAL("Unable to delete file `%s`", STR(path));
		return false;
	}
	return true;
}

bool deleteFolder(const std::string &path) {
  std::string full_path = normalizePath(path);
  if ( full_path.empty() ) {
    FATAL("Unable to get full path for %s", STR(path));
    return false;
  }

  if ( !isDirectory(full_path) ) {
    return deleteFile(full_path);
  }

  DIR *pDir = opendir(STR(full_path));
  if (pDir == NULL) {
    FATAL("Unable to open folder: %s %d %s", STR(path), errno, strerror(errno));
    return false;
  }

  struct dirent *pDirent = NULL;
  while ((pDirent = readdir(pDir)) != NULL) {
    std::string entry = pDirent->d_name;
    if ((entry == ".") || (entry == "..")) {
      continue;
    }

    if ( !deleteFolder(full_path + "/" + entry) ) {
      closedir(pDir);
      return false; 
    }
  }

  closedir(pDir);
  deleteFile(full_path);
  return true;
}

bool createFolder(const std::string &path, mode_t mode) {
  if ( 0 != mkdir(STR(path), mode) ) {
    FATAL("Unable to create folder %s, mode %d, %d", STR(path), (int)mode, errno); 
    return false;
  }

  if ( 0 != chmod(STR(path), mode) ) {
    FATAL("Unable to chmod %s, mode %d, %d", STR(path), (int)mode, errno);
    return false;
  }
  return true;
}

std::string getHostByName(const std::string &name) {
  char buf[1024] = {0};
  struct hostent host_ent, *phost_ent;
  int h_errnop;

  if (gethostbyname_r(STR(name),
                      &host_ent, buf, sizeof(buf),
                      &phost_ent, &h_errnop) < 0) {
    return "";
  }
  
	if (phost_ent->h_length <= 0)
		return "";
  
  if ( !phost_ent->h_addr_list )
    return "";
  
  if ( !phost_ent->h_addr_list[0] )
    return "";
  
  std::string result = format("%hhu.%hhu.%hhu.%hhu",
                              (uint8_t) phost_ent->h_addr_list[0][0],
                              (uint8_t) phost_ent->h_addr_list[0][1],
                              (uint8_t) phost_ent->h_addr_list[0][2],
                              (uint8_t) phost_ent->h_addr_list[0][3]);
	return result;
}

bool isNumeric(const std::string &value, int base) {
  if ( "" == value )
    return false;

  char *endptr = NULL;
  strtoll(STR(value), &endptr, base);
	return '\0' == (*endptr);
}

void split(const std::string &str, const std::string &separator, std::vector<std::string> &result) {
	result.clear();
  std::string::size_type position = str.find(separator);
  std::string::size_type lastPosition = 0;
	uint32_t separatorLength = separator.length();

	while (position != str.npos) {
		ADD_VECTOR_END(result, str.substr(lastPosition, position - lastPosition));
		lastPosition = position + separatorLength;
		position = str.find(separator, lastPosition);
	}
	ADD_VECTOR_END(result, str.substr(lastPosition, std::string::npos));
}

uint64_t getTagMask(uint64_t tag) {
	uint64_t result = 0xffffffffffffffffLL;
	for (int8_t i = 56; i >= 0; i -= 8) {
		if (((tag >> i)&0xff) == 0)
			break;
		result = result >> 8;
	}
	return ~result;
}

std::string generateRandomString(uint32_t length) {
  std::string result = "";
	for (uint32_t i = 0; i < length; i++)
		result += alowedCharacters[rand() % alowedCharacters.length()];
	return result;
}

void lTrim(std::string &value) {
  std::string::size_type i = 0;
	for (i = 0; i < value.length(); i++) {
		if (value[i] != ' ' &&
				value[i] != '\t' &&
				value[i] != '\n' &&
				value[i] != '\r')
			break;
	}
	value = value.substr(i);
}

void rTrim(std::string &value) {
	int32_t i = 0;
	for (i = (int32_t) value.length() - 1; i >= 0; i--) {
		if (value[i] != ' ' &&
				value[i] != '\t' &&
				value[i] != '\n' &&
				value[i] != '\r')
			break;
	}
	value = value.substr(0, i + 1);
}

void trim(std::string &value) {
  lTrim(value);
  rTrim(value);
}

int8_t getCPUCount() {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

std::map<std::string, std::string> mapping(const std::string &str, const std::string &separator1, const std::string &separator2, bool trimStrings) {
  std::map<std::string, std::string> result;

  std::vector<std::string> pairs;
  split(str, separator1, pairs);

	FOR_VECTOR_ITERATOR(std::string, pairs, i) {
		if (VECTOR_VAL(i) != "") {
			if (VECTOR_VAL(i).find(separator2) != std::string::npos) {
        std::string key = VECTOR_VAL(i).substr(0, VECTOR_VAL(i).find(separator2));
        std::string value = VECTOR_VAL(i).substr(VECTOR_VAL(i).find(separator2) + 1);
				if (trimStrings) {
          trim(key);
          trim(value);
				}
				result[key] = value;
			} else {
				if (trimStrings) {
          trim(VECTOR_VAL(i));
				}
				result[VECTOR_VAL(i)] = "";
			}
		}
	}
	return result;
}

void splitFileName(const std::string &fileName, std::string &name, std::string & extension, char separator) {
	size_t dotPosition = fileName.find_last_of(separator);
	if (dotPosition == std::string::npos) {
		name = fileName;
		extension = "";
		return;
	}
	name = fileName.substr(0, dotPosition);
	extension = fileName.substr(dotPosition + 1);
}

std::string getFileName(const std::string &path) {
	size_t dotPosition = path.find_last_of('/');
	if (dotPosition == std::string::npos) {
		return path;
	}
	return path.substr(dotPosition+1);
}

double getFileModificationDate(const std::string &path) {
	struct stat s;
	if (stat(STR(path), &s) != 0) {
    FATAL("Unable to stat file %s", STR(path));
		return 0;
	}
	return (double) s.st_mtime;
}

std::string normalizePath(const std::string &file) {
  char dummy1[PATH_MAX];
  char *rs = realpath(STR(file), dummy1);
  if ( NULL == rs )
    return "";
  return rs;
}

// std::string normalizePath(const std::string &base, const std::string &file) {
// 	char dummy1[PATH_MAX];
// 	char dummy2[PATH_MAX];
// 	char *pBase = realpath(STR(base), dummy1);
// 	char *pFile = realpath(STR(base + file), dummy2);

// 	if (pBase != NULL) {
// 		base = pBase;
// 	} else {
// 		base = "";
// 	}

// 	if (pFile != NULL) {
// 		file = pFile;
// 	} else {
// 		file = "";
// 	}

// 	if (file == "" || base == "") {
// 		return "";
// 	}

// 	if (file.find(base) != 0) {
// 		return "";
// 	} else {
// 		if (!fileExists(file))   {
// 			return "";
// 		} else {
// 			return file;
// 		}
// 	}
// }

bool listFolder(const std::string &path, std::vector<std::string> &result, bool normalizeAllPaths, bool includeFolders, bool recursive) {
  std::string p = normalizePath(path);

	DIR *pDir = opendir(STR(p));
	if (pDir == NULL) {
    FATAL("Unable to open folder: %s %d %s", STR(path), errno, strerror(errno));
		return false;
	}

	struct dirent *pDirent = NULL;
	while ((pDirent = readdir(pDir)) != NULL) {
    std::string entry = pDirent->d_name;
		if ((entry == ".") || (entry == "..")) {
			continue;
		}

		if ( entry.empty() )
			continue;

    std::string full_entry = p + "/" + entry;
    if ( isDirectory(full_entry) ) {
      if ( includeFolders ) {
        if ( normalizeAllPaths ) {
          ADD_VECTOR_END(result, full_entry);
        } else {
          ADD_VECTOR_END(result, entry);
        } 
      }
      
      if ( recursive ) {
        if ( !listFolder(full_entry, result, normalizeAllPaths, includeFolders, recursive) ) {
          FATAL("Unable to list folder");
					closedir(pDir);
					return false;
				}
      }
    } else {
      if ( normalizeAllPaths ) {
        ADD_VECTOR_END(result, full_entry);
      } else {
        ADD_VECTOR_END(result, entry);
      }
    }
  }

	closedir(pDir);
	return true;
}

bool moveFile(const std::string &src, const std::string &dst) {
	if (rename(STR(src), STR(dst)) != 0) {
    FATAL("Unable to move file from `%s` to `%s`",
				STR(src), STR(dst));
		return false;
	}
	return true;
}

bool installSignal(int sig, SignalFnc pSignalFnc) {
	struct sigaction action;
	action.sa_handler = pSignalFnc;
	action.sa_flags = 0;
	if (sigemptyset(&action.sa_mask) != 0) {
    ASSERT("Unable to install the %d signal", sig);
		return false;
	}
	if (sigaction(sig, &action, NULL) != 0) {
    ASSERT("Unable to install the %d signal", sig);
		return false;
	}

  return true;
}

bool installQuitSignal(SignalFnc pQuitSignalFnc) {
	return installSignal(SIGINT, pQuitSignalFnc);
}

bool installConfRereadSignal(SignalFnc pConfRereadSignalFnc) {
	return installSignal(SIGHUP, pConfRereadSignalFnc);
}

// for example: key=eth0 value=192.168.1.1
void getAllHostIP(std::map<std::string , std::string> &ips) {
  ips.clear();
  int s;
  struct ifconf conf;
  struct ifreq *ifr;
  char buff[BUFSIZ];
  int num;
  int i;

  s = socket(PF_INET, SOCK_DGRAM, 0);
  conf.ifc_len = BUFSIZ;
  conf.ifc_buf = buff;

  ioctl(s, SIOCGIFCONF, &conf);
  num = conf.ifc_len / sizeof(struct ifreq);
  ifr = conf.ifc_req;

  for(i=0;i < num;i++) {
    struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

    ioctl(s, SIOCGIFFLAGS, ifr);
    if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)) {
      char dst[128] = { 0 };
      inet_ntop(AF_INET, &sin->sin_addr, dst, 128);
      ips[ifr->ifr_name] = dst;
    }
    ifr++;
  }

  close(s);
}

std::string getPublicIP() {
  std::map<std::string , std::string> ips;
  getAllHostIP(ips);
  std::string ip = "";
  FOR_MAP(ips, std::string , std::string, i) {
    if ( 0 == strncmp("172.", STR(MAP_VAL(i)), 4) ||
         0 == strncmp("10.", STR(MAP_VAL(i)), 3) ||
         0 == strncmp("192.168.", STR(MAP_VAL(i)), 8) ) {
      continue;
    }
    ip = MAP_VAL(i);
    break;    
  }
  return ip;
}

std::string getPrivateIP() {
  std::map<std::string , std::string> ips;
  getAllHostIP(ips);
  std::string ip = "";
  FOR_MAP(ips, std::string , std::string, i) {
    if ( 0 == strncmp("172.", STR(MAP_VAL(i)), 4) ||
         0 == strncmp("10.", STR(MAP_VAL(i)), 3) ||
         0 == strncmp("192.168.", STR(MAP_VAL(i)), 8) ) {
      ip = MAP_VAL(i);
      break;
    }
  }
  return ip;
}

std::string getHostIPv4(uint32_t addr) {
  struct in_addr in = { addr };
  char dst[128] = { 0 };
  inet_ntop(AF_INET, &in, dst, 128);
  // return string(dst);
  return dst;
}

std::string getHostIPv6(const struct in6_addr *sin6_addr) {
  char dst[128] = { 0 };
  inet_ntop(AF_INET6, sin6_addr, dst, 128);
  // return string(dst);
  return dst;
}

uint32_t getHostIPv4(const std::string &ip) {
  struct in_addr in;
  memset(&in, 0, sizeof(in));
  inet_pton(AF_INET, STR(ip), &in);
  return (uint32_t)in.s_addr;
}

std::string getLocalTimeString(const char *format, time_t t) {
  char str[128] = { 0 };
  struct tm tm;
  gsdm_localtime_r(&t, &tm, 8);
  strftime(str, 128, format, &tm);
  // return string(str);
  return str;
}

bool doDaemon() {
  pid_t pid = fork();
  if (pid < 0) {
    FATAL("Unable to start daemonize. fork() failed %d", errno);
    return false;
  }

  if (pid > 0) {
    _exit(0);
  }

  pid_t sid = setsid();
  if (sid < 0) {
    FATAL("Unable to start daemonize. setsid() failed %d", errno);
    return false;
  }

  close(0);
  close(1);
  close(2);
  open("/dev/null", O_RDONLY);
  open("/dev/null", O_WRONLY);
  open("/dev/null", O_WRONLY);
  return true;
}

bool changeUser(const std::string &user, const std::string &log_path) {
  if ( "" == log_path )
    return true;

  struct passwd *pw = getpwnam(STR(user));
  if (!pw) {
    FATAL("Unable to start daemonize. getpwnam(%s) failed %d", STR(user), errno);
    return false;
  }

  if (!log_path.empty() && 0 != chown(STR(log_path), pw->pw_uid, pw->pw_gid)) {
    FATAL("Unable to chown %s to %s chown() failed %d", STR(log_path), STR(user), errno);
    return false;
  }

  std::vector<std::string> result;
  if ( !listFolder(log_path, result, true, true, true) )
    return false;

  for (uint32_t i = 0; i < result.size(); i++) {
    if (0 != chown(STR(result[i]), pw->pw_uid, pw->pw_gid)) {
      FATAL("Unable to start daemonize. %s fchown() failed %d", STR(result[i]), errno);
      return false;
    }
  }

  if (setgid(pw->pw_gid) < 0) {
    FATAL("Unable to start daemonize. setgid() failed %d", errno);
    return false;
  }
  
  if (setuid(pw->pw_uid) < 0) {
    FATAL("Unable to start daemonize. setuid() failed %d", errno);
    return false;
  }
  
  return true;
}

bool setLimitNoFile(uint64_t limit) {
  struct rlimit rlim = { limit, limit };
  if ( -1 == setrlimit(RLIMIT_NOFILE, &rlim) ) {
    FATAL("setrlimit(RLIMIT_NOFILE) failed %d", errno);
    return false;
  }
  return true;
}

bool setLimitCore(uint64_t limit) {
  struct rlimit rlim = { limit, limit };
  if ( -1 == setrlimit(RLIMIT_CORE, &rlim) ) {
    FATAL("setrlimit(RLIMIT_CORE) failed %d", errno);
    return false;
  }
  return true;
}

pid_t getTid(void) {
  return syscall(SYS_gettid);
}

bool setaffinityNp(std::vector<int> &cpuID, pthread_t thread) {
  int s;
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  for (uint32_t j = 0; j < cpuID.size(); j++)
    CPU_SET(cpuID[j], &cpuset);

  s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  if (s != 0) {
    FATAL("pthread_setaffinity_np failed, errno is %d", errno);
    return false;
  }

  return true;
}

std::string getStrerror(int errnum) {
  char buf[512] = { 0 };
  char *rs = strerror_r(errnum, buf, 512);
  return rs;
}

// find fromcode or tocode , use to `iconv -l`
size_t gsdmIconv(const std::string &fromcode, const std::string &tocode, char *inbuf, size_t inlen, char *outbuf, size_t outlen) {
  iconv_t cd = iconv_open(STR(tocode), STR(fromcode));
  if ( (iconv_t)-1 == cd ) {
    WARN("iconv_open falied, errno %d\n", errno);
    return 0;
  }
  
  size_t bak = outlen;
  size_t rs = iconv(cd, &inbuf, &inlen, &outbuf, &outlen);
  iconv_close(cd);
  if ( 0 != rs ) {
    WARN("iconv_open falied, errno %d\n", errno);
    return 0;
  }

  return bak - outlen;
}

gsdm_msec_t getCurrentGsdmMsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((gsdm_msec_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

uint64_t getCurrentGsdmMicro() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((gsdm_msec_t)tv.tv_sec) * 1000000 + tv.tv_usec;
}

struct tm* gsdm_localtime_r(const time_t *unix_sec, struct tm* tm, int time_zone) {
  memset(tm, 0, sizeof(struct tm));
  static const int khours_in_day = 24;
  static const int kminutes_in_hour = 60;
  static const int kdays_from_unixtime = 2472632;
  static const int kdays_from_year = 153;
  static const int kmagic_unkonwn_first = 146097;
  static const int kmagic_unkonwn_sec = 1461;
  
  tm->tm_sec  =  (*unix_sec) % kminutes_in_hour;
  int i      = ((*unix_sec)/kminutes_in_hour);
  tm->tm_min  = i % kminutes_in_hour; //nn
  i /= kminutes_in_hour;
  tm->tm_hour = (i + time_zone) % khours_in_day; // hh
  tm->tm_mday = (i + time_zone) / khours_in_day;
  int a = tm->tm_mday + kdays_from_unixtime;
  int b = (a*4  + 3)/kmagic_unkonwn_first;
  int c = (-b*kmagic_unkonwn_first)/4 + a;
  int d =((c*4 + 3) / kmagic_unkonwn_sec);
  int e = -d * kmagic_unkonwn_sec;
  e = e/4 + c;
  int m = (5*e + 2)/kdays_from_year;
  tm->tm_mday = -(kdays_from_year * m + 2)/5 + e + 1;
  tm->tm_mon = (-m/10)*12 + m + 2;
  tm->tm_year = b*100 + d  - 6700 + (m/10);
  return tm;
}

std::string getCurrentGsdmMsecStr() {
  uint64_t now = getCurrentGsdmMsec();
  time_t t = (time_t)(now / 1000);
  char str[128] = { 0 };
  struct tm tm;
  gsdm_localtime_r(&t, &tm, 8);
  size_t size = strftime(str, 128, "%Y-%m-%d %H:%M:%S", &tm);
  snprintf(str + size, 128 - size, ".%lu", now%1000);
  return str;
}

void getPidByName(std::vector<pid_t> &pids, const std::string &task_name) {
  pids.clear();

  DIR *dir;
  struct dirent *ptr;
  FILE *fp;
  char file_path[256] = { 0 }, cur_task_name[256] = { 0 }, buf[1024] = { 0 };

  dir = opendir("/proc");
  if (NULL != dir) {
    while ( (ptr = readdir(dir) ) != NULL) {
      if ( 0 == (strcmp(ptr->d_name, ".") ) || ( 0 == strcmp(ptr->d_name, "..") ))
        continue;

      sprintf(file_path, "/proc/%s/status", ptr->d_name);
      fp = fopen(file_path, "r");
      if ( NULL != fp ) {
        if ( NULL == fgets(buf, sizeof(buf)-1, fp) ) {
          fclose(fp);
          continue;
        }
        sscanf(buf, "%*s %s", cur_task_name);

        if ( 0 == strcmp(STR(task_name), cur_task_name) ) {
          pid_t pid;
          sscanf(ptr->d_name, "%d", &pid);
          pids.push_back(pid);
        }
        fclose(fp);
      }
    }
    closedir(dir);
    std::sort(pids.begin(), pids.end());
  }
}

std::string getNameByPid(pid_t pid) {
  char tmp[256] = { 0 }, buf[1024] = { 0 };

  sprintf(tmp, "/proc/%d/status", pid);
  FILE* fp = fopen(tmp, "r");
  if ( NULL == fp ) {
    return "";
  }

  if ( NULL == fgets(buf, sizeof(buf)-1, fp) ) {
    fclose(fp);
    return "";
  }

  fclose(fp);
  memset(tmp, 0, sizeof(tmp));
  sscanf(buf, "%*s %s", tmp);
  return tmp;
}


char *rstrstr(const char *str, int str_len, const char *needle, int needle_len) {
  if (!str_len || !needle_len) {
    return NULL;
  }

  if (str_len < needle_len) {
    return NULL;
  }

  const char *rstr = str + str_len - needle_len;
  const char *bp, *sp;

  while (rstr >= str) {
    bp = rstr;
    sp = needle;
    
    do {
      if (*sp == '\0') {
        return (char*)rstr;
      }
    } while (*(bp--) == *(sp--));
    
    rstr--;
  }

  return NULL;
}


}



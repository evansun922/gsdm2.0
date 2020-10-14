#ifndef _POOLWORKEREX_H_
#define _POOLWORKEREX_H_


#include "pooldrive/poolprocess.h"
#include "3rdparty/include/st.h"

namespace gsdm {

#define POOL_UTIME_NO_TIMEOUT ST_UTIME_NO_TIMEOUT

enum PoolWorkerMode {
  POOLWORKERMODE_SYS = 0,
  POOLWORKERMODE_ST
};

class PoolWorker;
class SysPoolWorker;
class StPoolWorker;
class PoolManager;
class PoolProcess;

class PoolWorkerEx {
public:
friend class PoolWorker;
friend class SysPoolWorker;
friend class StPoolWorker;
friend class PoolManager;
friend class PoolProcess;

  /*!
   * @brief : Create a process object.
   *
   * @param : worker_title    [IN] process of title. 
   * @param : mode            [IN] system mode or sy mode.
   * @param : thread_count    [IN] thread of count in this process.
   *
   * @return: void.
   */
  PoolWorkerEx(const std::string &worker_title, PoolWorkerMode mode, int thread_count);
  virtual ~PoolWorkerEx();
  /*!
   * @brief : Main thread, you can overload this function.
   *
   * @param : none.
   *
   * @return: void.
   */  
  virtual void MainDo();
  /*!
   * @brief : When accept a new socket, gsdm will call this function to create PoolProcess and execute PoolProcess::Process().
   *
   * @param : port     [IN] port of listen.
   *
   * @return: caller of PoolProcess.
   */  
  virtual PoolProcess *CreateTCPProcess(uint16_t port);
  /*!
   * @brief : When accept a new socket, gsdm will call this function to create PoolProcess and execute PoolProcess::Process().
   *
   * @param : unix_path    [IN] unix path of listen.
   *
   * @return: caller of PoolProcess.
   */  
  virtual PoolProcess *CreateUNIXProcess(const std::string &unix_path);
  /*!
   * @brief : Whether gsdm is required to call waitpid.
   *
   * @param : none.
   *
   * @return: Call waitpid by gsdm, true is returned.
   */
  virtual bool NeedWaitProcess();
  /*!
   * @brief : When this process is Closing, gsdm will call this function.
   *
   * @param : none.
   *
   * @return: none. 
   */
  virtual void Close();
  /*!
   * @brief : Create a new process to execute PoolProcess::Process.
   *
   * @param : title     [IN] process of title.
   * @param : process   [IN] caller of PoolProcess, executing PoolProcess::Process(). gsdm will free this process
   *
   * @return: on success, child of pid is returned. on error , -1 is returned.
   */
  pid_t StartWithProcess(const std::string &title, PoolProcess *process);
  /*!
   * @brief : Create a new process to execute PoolProcess::Process with pool of threads.
   *
   * @param : process   [IN] caller of PoolProcess, executing PoolProcess::Process(). gsdm will free this process
   *
   * @return: on success, true is returned. on error, false is returned.
   */
  bool StartWithThread(PoolProcess *process);
  /*!
   * @brief : Create a new poolsocket_t by system of socket.
   *
   * @param : s     [IN] system of socket.
   *
   * @return: pooldrive of socket.
   */
  poolsocket_t CreateSocket(int s);
  /*!
   * @brief : Create a new poolsocket_t.
   *
   * @param : is_unix     [IN] true is unix socket, false is IPv4.
   * @param : is_dgram    [IN] true is udp, false is tcp.
   *
   * @return: pooldrive of socket.
   */
  poolsocket_t CreateSocket(bool is_unix, bool is_dgram);
  /*!
   * @brief : Close socket.
   *
   * @param : fd     [IN] pooldrive of socket.
   *
   * @return: none.
   */
  void CloseSocket(poolsocket_t fd);
  /*!
   * @brief : Tcp connect.
   *
   * @param : fd          [IN] pooldrive of socket.
   * @param : ip          [IN] ip.
   * @param : port        [IN] port.
   * @param : timeout     [IN] timeout.
   *
   * @return: on success, true is returned. on error, false is returned.
   */
  bool TCPConnect(poolsocket_t fd, const std::string &ip, uint16_t port, gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);
  /*!
   * @brief : Tcp listen.
   *
   * @param : ip          [IN] ip of listen.
   * @param : port        [IN] port.
   * @param : other       [IN] new socket will be operated on these processes.
   *
   * @return: on success, true is returned. on error, false is returned.
   */
  bool TCPListen(const std::string &ip, uint16_t port, std::vector<PoolWorkerEx *> &other);
  /*!
   * @brief : Unix connect.
   *
   * @param : fd          [IN] pooldrive of socket.
   * @param : port        [IN] unix of path.
   * @param : timeout     [IN] timeout.
   *
   * @return: on success, true is returned. on error, false is returned.
   */
  bool UNIXConnect(poolsocket_t fd, const std::string &unix_path, gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);
  /*!
   * @brief : Unix listen. 
   *
   * @param : fd          [IN] pooldrive of socket.
   * @param : port        [IN] unix of path.
   * @param : other       [IN] new socket will be operated on these processes.
   *
   * @return: on success, true is returned. on error, false is returned.
   */
  bool UNIXListen(const std::string &sun_path, std::vector<PoolWorkerEx *> &other);
  /*!
   * @brief : recv data.
   *
   * @param : fd          [IN]  pooldrive of socket.
   * @param : buf         [OUT] buffer.
   * @param : len         [IN]  buf of length.
   * @param : timeout     [IN]  timeout.
   *
   * @return: these calls return the number of bytes received, or -1 if an error occurred.  
   *          The return value will be 0 when the peer has performed an orderly shutdown.
   */
  ssize_t RecvMsg(poolsocket_t fd, void *buf, uint32_t len, gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);
  /*!
   * @brief : send data.
   *
   * @param : fd          [IN]  pooldrive of socket.
   * @param : buf         [IN]  buffer.
   * @param : len         [IN]  buf of length.
   * @param : timeout     [IN]  timeout.
   *
   * @return: on success, these calls return the number of characters sent.  
   *          on error, -1 is returned, and errno is set appropriately.
   */
  ssize_t SendMsg(poolsocket_t fd, const void *buf, uint32_t len, gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);
  /*!
   * @brief : recvfrom data.
   *
   * @param : fd          [IN]  pooldrive of socket.
   * @param : buf         [OUT] buffer.
   * @param : len         [IN]  buf of length.
   * @param : src_addr    [IN]  address.
   * @param : addrlen     [IN]  address of length.
   * @param : timeout     [IN]  timeout.
   *
   * @return: these calls return the number of bytes received, or -1 if an error occurred.  
   */
  ssize_t RecvFromMsg(poolsocket_t fd, void *buf, uint32_t len, struct sockaddr *src_addr, socklen_t *addrlen, 
                      gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);
  /*!
   * @brief : sendfrom data.
   *
   * @param : fd          [IN] pooldrive of socket.
   * @param : buf         [IN] buffer.
   * @param : len         [IN] buf of length.
   * @param : dest_addr   [IN] address.
   * @param : addrlen     [IN] address of length.
   * @param : timeout     [IN] timeout.
   *
   * @return: on success, these calls return the number of characters sent.  
   *          on error, -1 is returned, and errno is set appropriately.
   */
  ssize_t SendToMsg(poolsocket_t fd, const void *buf, uint32_t len, const struct sockaddr *dest_addr, socklen_t addrlen, 
                 gsdm_msec_t timeout = POOL_UTIME_NO_TIMEOUT);

private:
  PoolWorkerMode pool_mode_;
  int thread_count_;
  bool *is_close_;
  std::string worker_title_;
  PoolWorker *worker_;
};

}


#endif // _POOLWORKEREX_H_

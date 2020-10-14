#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

#include "baseprocess.h"

#define HTTP_CONTENT_LENGTH "Content-Length"

namespace gsdm {

typedef std::tr1::unordered_map<std::string, std::string> HttpHeader;

class HttpClient : public BaseProcess {
public:
  HttpClient();
  virtual ~HttpClient();

  /*!
   * @brief : Connect webserver. (TODO connecting timeout)
   *
   * @param : url.         [IN] url of http.
   *
   * @return: return true if success,return false if failed.
   */
  bool Connect(const std::string &url);
  /*!
   * @brief : Process respond of http.
   *
   * @param : code.              [IN] code of http(for example: "200 OK").
   * @param : header.            [IN] header of http(for example: "Content-Length: 168", key is "Content-Length", value is "168").
   * @param : body.              [IN] content of http.
   * @param : body_length.       [IN] content of length.
   *
   * @return: none.
   */
  virtual void ProcessRespond(int code, const HttpHeader &header, const uint8_t *body, uint32_t body_length) = 0;


private:

  bool Process(int32_t recv_amount);
  void AttachNet();
  void DetachNet();
  bool ParseHeader(char *header);

  std::string server_ip_;
  uint16_t server_port_;
  HttpHeader http_header_;
  int http_code_;
  uint32_t content_length_;

  std::string domain_;
  std::string app_;
};

}

#endif // _HTTPCLIENT_H

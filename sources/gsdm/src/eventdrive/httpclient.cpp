#include "eventdrive/httpclient.h"
#include "eventdrive/baseworkerex.h"
#include "logging/logger.h"

namespace gsdm {

HttpClient::HttpClient() 
  : BaseProcess(1024, 4096),server_ip_(""),server_port_(80),http_code_(-1),content_length_(0),domain_(""),app_("") {

}

HttpClient::~HttpClient() {

}

bool HttpClient::Connect(const std::string &url) {
  if ( PROCESS_NET_STATUS_CONNECTING == GetNetStatus() || PROCESS_NET_STATUS_CONNECTED == GetNetStatus() ) {
    return false;
  }

  SetWithMe(false);
  server_ip_      = "";
  server_port_    = 80;
  http_header_.clear();
  http_code_      = -1;
  content_length_ = 0;

  domain_ = "";
  app_    = "";

  input_buffer_.IgnoreAll();
  output_buffer_.IgnoreAll();

  // http://10.16.14.183:8080/abc
  std::vector<std::string> vec;
  split(url, "/", vec);
  if ( 3 > vec.size() ) {
    WARN("url of format is error, %s", STR(url));
    return false;
  }

  std::vector<std::string> vec1;
  split(vec[2], ":", vec1);
  server_ip_ = getHostByName(vec1[0]);
  if ( server_ip_.empty() ) {
    WARN("getHostByName failed, %s", STR(url));
    return false;
  }

  if ( 2 == vec1.size() ) {
    server_port_ = (uint16_t)atoi(STR(vec1[1]));
  } else {
    server_port_ = 80;
  }

  domain_ = vec[2];
  const char *p = STR(url) + sizeof("http://");
  const char *tmp = strstr(p, "/");
  if ( NULL == tmp ) {
    app_ = "/";
  } else {
    app_ = tmp;
  }

  ex_->TCPConnect(server_ip_, server_port_, this);
  return true;
}

bool HttpClient::Process(int32_t recv_amount) {
  uint8_t *buffer = GETIBPOINTER(input_buffer_);
  uint32_t size = GETAVAILABLEBYTESCOUNT(input_buffer_);
  
  if ( 0 == content_length_ ) {
    if ( 4 > size )
      return true;

    uint8_t bak = buffer[size - 1];
    buffer[size - 1] = 0;

    char *find = strstr((char *)buffer, "\r\n\r");
    if ( NULL == find )
      return true;

    *find = 0;
    buffer[size - 1] = bak;

    if ( !ParseHeader((char *)buffer) ) {
      WARN("ParseHeader failed, %s", STR(app_));
      return false;
    }

    if ( http_header_.end() == http_header_.find(HTTP_CONTENT_LENGTH) ) {
      WARN("Only support Content-Length, %s", STR(app_));
      return false;
    }

    content_length_ = (uint32_t)atoi(STR(http_header_[HTTP_CONTENT_LENGTH]));

    uint32_t len = (uint8_t *)find - buffer + 4;
    input_buffer_.Ignore(len);
    buffer += len;
    size -= len;
  }

  if ( 0 != content_length_ && content_length_ == size ) {
    ProcessRespond(http_code_, http_header_, buffer, content_length_);
    server_ip_ = "";
    return false;
  }
  
  return true;
}

void HttpClient::AttachNet() {
  std::string v = format("GET %s HTTP/1.1\r\nUser-Agent: libgsdm\r\nHost: %s\r\nAccept: */*\r\n\r\n", STR(app_), STR(domain_));
  SendMsg(STR(v), v.length());
}

void HttpClient::DetachNet() {
  if ( !server_ip_.empty() ) {
    ProcessRespond(-1, http_header_, NULL, 0);
  }
}

bool HttpClient::ParseHeader(char *header) {
  /*
    HTTP/1.1 404 Not Found
    Server: nginx/1.5.3
    Date: Fri, 25 Dec 2015 08:59:30 GMT
    Content-Type: text/html
    Content-Length: 168
    Connection: keep-alive
  */
  
  char *tmp2, *tmp1 = strstr(header, "\r\n");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  tmp2 = tmp1 + 2;

  // app
  header += sizeof("HTTP/1.1");
  tmp1 = strstr(header, " ");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  http_code_ = atoi(header);

  header = tmp2;
  while ( NULL != (tmp1 = strstr(header, "\r\n")) ) {
    *tmp1 = 0;
    tmp2 = tmp1 + 2;
    tmp1 = strstr(header, ":");
    if ( NULL == tmp1 )
      return false;
    *tmp1 = 0;
    http_header_[header] = tmp1 + 2;
    header = tmp2;
  }

  tmp1 = strstr(header, ":");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  http_header_[header] = tmp1 + 2;

  return true;
}


}



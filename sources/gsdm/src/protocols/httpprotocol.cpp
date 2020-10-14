#include "protocols/httpprotocol.h"
#include "protocols/urlcode.h"
#include <utils/crypto.h>
#include "logging/logger.h"

namespace gsdm {

HTTPProtocol::HTTPProtocol() 
  :content_length_(0),code_(0) {
  
}

HTTPProtocol::~HTTPProtocol() {
  
}

bool HTTPProtocol::Handler() {
  return false;
}

bool HTTPProtocol::HandlerRespond(int code, const std::string &data) {
  return false;
}

bool HTTPProtocol::SendHead(const std::string &code, const std::string &content_type) {
  char date[128] = {0};
  time_t t = time(0);
  struct tm tm;
  gmtime_r(&t, &tm);
  strftime(date, 127, "%a, %d %b %Y %T %Z", &tm);

  std::string http_version = GetRequestHeader(HTTP_HEADER_VERSION);
  if ( http_version.empty() ) {
    http_version = "HTTP/1.1";
  }

  std::stringstream respond;
  respond << http_version << " " << code << "\r\n";
  respond << "Server: libgsdm\r\n";
  respond << "Date: " << date << "\r\n";
  respond << "Content-Type: " << content_type << "\r\n";
  // if ( (uint32_t)-1 != len ) {
  //   respond << "Content-Length: " << len << "\r\n";
  // } else {
  //   respond << "Transfer-Encoding: chunked\r\n";
  // }
  // respond << "Connection: keep-alive\r\n";

  FOR_UNORDERED_MAP(http_respond_header_hash_,std::string,std::string,i) {
    respond << MAP_KEY(i) << ": " << MAP_VAL(i) << "\r\n";
  }
  respond << "\r\n";

  std::string r = respond.str();
  return SendHTTPData(STR(r), r.length());
}

bool HTTPProtocol::SendContent(const void *data, uint32_t len) {
  return SendHTTPData(data, len);
}

bool HTTPProtocol::SendChunked(const void *data, uint32_t len) {
  char h[32] = { 0 };
  int l = snprintf(h, 32, "%X\r\n", len);
  if ( false == SendHTTPData(h, l) )
    return false;
  if ( len ) {
    if ( false == SendHTTPData(data, len) )
      return false;
  }
  return SendHTTPData("\r\n", 2);
}

bool HTTPProtocol::ParseUrl(const std::string &url) {
  http_request_header_hash_.clear();  
  content_length_ = 0;
  code_ = 0;
  char *find, str[4096] = { 0 };
  strncpy(str, STR(url)+sizeof("http:/"), 4095);
  find = strstr(str, "/");
  if ( NULL == find ) {
    WARN("url of format is error, %s", STR(url));
    return false;
  }
  *find = 0;
  // find++;

  char *v = strstr(str, ":");
  if ( NULL == v ) {
    http_request_header_hash_[HTTP_HEADER_HOST] = str;
    http_request_header_hash_[HTTP_HEADER_PORT] = "80";
  } else {
    *v = 0;
    v++;
    http_request_header_hash_[HTTP_HEADER_HOST] = str;
    http_request_header_hash_[HTTP_HEADER_PORT] = v;
  }
  
  *find = '/';
  http_request_header_hash_[HTTP_HEADER_URI] = find;
  return true;
}

bool HTTPProtocol::HTTPGet() {
  std::stringstream request;
  request << "GET " << http_request_header_hash_[HTTP_HEADER_URI] << " HTTP/1.1\r\n";
  request << "User-Agent: libgsdm\r\n";
  request << "Host: " << http_request_header_hash_[HTTP_HEADER_HOST] << "\r\n";
  request << "Accept: */*\r\n";
  request << "Connection: keep-alive\r\n\r\n";
  
  std::string v = request.str();
  return SendHTTPData(STR(v), v.length());
}

std::string HTTPProtocol::GetRequestArgs(const std::string &key) {
  return http_args_hash_[key];
}

std::string HTTPProtocol::GetRequestHeader(const std::string &key) {
  return http_request_header_hash_[key];
}

void HTTPProtocol::SetRespondHeader(const std::string &key, const std::string &value) {
  http_respond_header_hash_[key] = value;
}

void HTTPProtocol::SetRespondHeaderForContentLength(uint64_t length) {
  std::string value = gsdm::format("%lu", length);
  SetRespondHeader("Content-Length", value);
}

void HTTPProtocol::SetRespondHeaderForTransferEncoding() {
  SetRespondHeader("Transfer-Encoding", "chunked");
}

void HTTPProtocol::SetRespondHeaderForConnectionKeepalive() {
  SetRespondHeader("Connection", "keep-alive");
}

void HTTPProtocol::SetRespondHeaderForConnectionClose() {
  SetRespondHeader("Connection", "close");
}

int HTTPProtocol::HandlerProtocol(uint8_t *data, uint32_t len) {  
  char *find = rstrstr((const char *)data, (int)len, "\r\n\r\n", 4);
  if (find == NULL) {
    return 0;
  }
  
  *find = 0;
  
  if ( !ParseHeader((char *)data) ) { 
    return -1;
  }

  if ( !CheckHeader() ) {
    SendHead(HTTP_RESPOND_CODE_400, HTTP_CONTENT_TYPE_TEXT);
    return -1;
  }
    
  // for caller
  if ( !Handler() ) {
    return -1;
  }

  return ((uint8_t*)find - data) + 4;
}

int HTTPProtocol::HandlerProtocolRespond(uint8_t *data, uint32_t len) {
  uint8_t *tmp = data;
  if ( 0 == content_length_ ) {
    if ( 4 > len )
      return 0;

    uint8_t bak = data[len-1];
    data[len-1] = 0;
    char *find = strstr((char *)data, "\r\n\r");
    if ( NULL == find ) {
      data[len-1] = bak;
      return 0;
    }

    *find = 0;
    data[len-1] = bak; 
    if ( !ParseRespondHeader((char *)data) ) { 
      return -1;
    }
    data = (uint8_t *)find+4;
    len -= (data-tmp);

    if ( http_respond_header_hash_.end() == http_respond_header_hash_.find(HTTP_CONTENT_LENGTH) ) {
      WARN("Only support Content-Length");
      return -1;
    }
    content_length_ = atoi(STR(http_respond_header_hash_[HTTP_CONTENT_LENGTH]));
  }

  if ( content_length_ != (int)len ) {
    return data - tmp;
  }
    
  // for caller
  std::string v((char *)data, len);
  if ( !HandlerRespond(code_, v) ) {
    return -1;
  }

  data += len;
  return data - tmp;
}

bool HTTPProtocol::ParseHeader(char *header) {
  http_request_header_hash_.clear();

  // GET /chat?args=abc HTTP/1.1
  char *tmp2, *tmp1 = strstr(header, "\r\n");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  tmp2 = tmp1 + 2;

  // app
  header += sizeof("GET");
  tmp1 = strstr(header, " ");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;

  // char decode[8192] = { 0 };
  // decodeURIComponent(header, decode, sizeof(decode));
  // header = decode;
  
  char *find = strstr(header, "?");
  if ( NULL == find ) {  
    http_request_header_hash_[HTTP_HEADER_URI] = header;
  } else {
    *find = 0;
    http_request_header_hash_[HTTP_HEADER_URI] = header;
    ++find;
    http_request_header_hash_[HTTP_HEADER_ARGS] = find;
    char *arg1, *arg2;
    while ( NULL != (arg1 = strstr(find, "&")) ) {
      *arg1 = 0;
      arg2 = strstr(find, "=");
      if ( NULL == arg2 ) {
        http_args_hash_[find] = "";
      } else {
        *arg2 = 0;
        ++arg2;
        http_args_hash_[find] = arg2;
      }
      find = ++arg1;
    }

    arg2 = strstr(find, "=");
    if ( NULL == arg2 ) {
      http_args_hash_[find] = "";
    } else {
      *arg2 = 0;
      ++arg2;
      http_args_hash_[find] = arg2;
    }
  }

  // http version
  header = ++tmp1;
  http_request_header_hash_[HTTP_HEADER_VERSION] = header;

  // Host: 10.16.14.183:9999
  header = tmp2;
  while ( NULL != (tmp1 = strstr(header, "\r\n")) ) {
    *tmp1 = 0;
    tmp2 = tmp1 + 2;
    tmp1 = strstr(header, ":");
    if ( NULL == tmp1 )
      return false;
    *tmp1 = 0;
    http_request_header_hash_[header] = tmp1 + 2;
    header = tmp2;
  }

  tmp1 = strstr(header, ":");
  if ( NULL == tmp1 )
    return true;
  *tmp1 = 0;
  http_request_header_hash_[header] = tmp1 + 2;

  return true;
}

bool HTTPProtocol::ParseRespondHeader(char *header) {

  /*
    HTTP/1.1 404 Not Found
    Server: nginx/1.5.3
    Date: Fri, 25 Dec 2015 08:59:30 GMT
    Content-Type: text/html
    Content-Length: 168
    Connection: keep-alive
  */
  
  http_respond_header_hash_.clear();
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
  code_ = atoi(header);

  header = tmp2;
  while ( NULL != (tmp1 = strstr(header, "\r\n")) ) {
    *tmp1 = 0;
    tmp2 = tmp1 + 2;
    tmp1 = strstr(header, ":");
    if ( NULL == tmp1 )
      return false;
    *tmp1 = 0;
    http_respond_header_hash_[header] = tmp1 + 2;
    header = tmp2;
  }

  tmp1 = strstr(header, ":");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  http_respond_header_hash_[header] = tmp1 + 2;

  return true;
}

bool HTTPProtocol::CheckHeader() {
  HTTPHeaderHash::iterator iter = http_request_header_hash_.find(HTTP_HEADER_VERSION);
  if ( http_request_header_hash_.end() == iter ) {
    FATAL("The request version is empty.");
    return false;
  }
  
  return true;
}



}


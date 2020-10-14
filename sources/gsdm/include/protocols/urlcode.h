/*
 *  Copyright (c) 2016,
 *  It complete websocket protocol on server from [RFC6455]
 *  sunlei (sswin0922@163.com)
 */

#ifndef _GSDMURLCODE_H_
#define _GSDMURLCODE_H_

#include "platform/linuxplatform.h"


namespace gsdm {

  std::string encodeURIComponent(const std::string &value);
  std::string decodeURIComponent(const std::string &value);

  void decodeURIComponent(const std::string &value, char *rs, int rs_len);

}

#endif 

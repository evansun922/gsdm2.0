#include "protocols/urlcode.h"

namespace gsdm {

std::string encodeURIComponent(const std::string &value) {
  char rs[4096] = { 0 };
  static const char *s_en_uri_char[256] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    "%20","%21","%22","%23","%24",NULL, "%26","%27","%28","%29","%2A","%2B","%2C",NULL, NULL, "%2F", 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "%3A","%3B",NULL, "%3D",NULL, "%3F", 
    "%40",NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "%5B",NULL,"%5D", NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "%7B",NULL, "%7D", NULL,NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  };

  int len = (int)value.length();
  for (int i = 0, j = 0; i < len && j < 4094; i++) {
    if ( NULL == s_en_uri_char[(int)value[i]] ) {
      rs[j++] = value[i];      
    } else {
      rs[j++] = s_en_uri_char[(int)value[i]][0];
      rs[j++] = s_en_uri_char[(int)value[i]][1];
      rs[j++] = s_en_uri_char[(int)value[i]][2];
    }
  }
  return rs;
}

std::string decodeURIComponent(const std::string &value) {
  char rs[4096] = { 0 };
  char tmp_v[3] = { 0 };
  char *end;
  int len = (int)value.length();
  for (int i = 0, j = 0; i < len && j < 4096; i++) {
    if ( '%' == value[i] && len > (i + 2) ) {
      tmp_v[0] = value[i+1];
      tmp_v[1] = value[i+2];
      char v = (char)strtol(tmp_v, &end, 16);
      if ( tmp_v + 2 == end ) {
        rs[j++] = v;
        i += 2;
      } else {
        rs[j++] = value[i];
      }
    } else {
      rs[j++] = value[i];
    }
  }

  return rs;
}

void decodeURIComponent(const std::string &value, char *rs, int rs_len) {
  memset(rs, 0, rs_len);
  char tmp_v[3] = { 0 };
  char *end;
  int len = (int)value.length();
  for (int i = 0, j = 0; i < len && j < rs_len; i++) {
    if ( '%' == value[i] && len > (i + 2) ) {
      tmp_v[0] = value[i+1];
      tmp_v[1] = value[i+2];
      char v = (char)strtol(tmp_v, &end, 16);
      if ( tmp_v + 2 == end ) {
        rs[j++] = v;
        i += 2;
      } else {
        rs[j++] = value[i];
      }
    } else {
      rs[j++] = value[i];
    }
  }
}


}


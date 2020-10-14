#include "utils/gsdmconfig.h"
#include "logging/logger.h"
#include "utils/file.h"

namespace gsdm {


GsdmConfig GsdmConfig::config_;

GsdmConfig::GsdmConfig() {
  
}


GsdmConfig::~GsdmConfig() {
  
}

bool GsdmConfig::Initialize(const std::string &path) {
  config_map_.clear();

  if ( "" == path )
    return true;

  GSDMFile file;
  if (!file.Initialize(path)) {
    WARN("Open file %s failed errno: %d", STR(path), errno);
    return false;
  }
  
  char buffer[4096];
  uint64_t maxSize = 4096;
  while(file.ReadLine((uint8_t*)buffer, maxSize)) {
    char *find = strstr(buffer, "=");
    if ( NULL == find )
      continue;

    *find = 0;
    std::string key = buffer;
    trim(key);
    if ( key.empty() || '#' == key[0] || '\n' == key[0] || '\r' == key[0] )
      continue;

    char *h = find + 1;
    find = strstr(h, "\"");
    if ( find ) {
      h = find + 1;
      find = strstr(h, "\"");
      if ( NULL == find )
        continue;
      *find = 0;
      config_map_[key] = h;
    } else {
      find = strstr(h, "#");
      if ( find )
        *find = 0;
      std::string v = h;
      trim(v);
      config_map_[key] = v;
    }

    maxSize = 4096;
  }
  file.Close();
  return true;
}

int32_t GsdmConfig::GetInt(const std::string &key, int32_t def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  return atoi(STR(MAP_VAL(i)));
}

int64_t GsdmConfig::GetLongInt(const std::string &key, int64_t def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  return atoll(STR(MAP_VAL(i)));
}

std::string GsdmConfig::GetString(const std::string &key, const std::string &def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  return MAP_VAL(i);
}

bool GsdmConfig::GetBool(const std::string &key, bool def) {
  ConfigMap::iterator i = config_map_.find(key);
  if ( config_map_.end() == i ) {
    return def;
  }

  if ( "true" == lowerCase(MAP_VAL(i)) )
    return true;
  return false;
}



}

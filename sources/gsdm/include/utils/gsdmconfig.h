#ifndef _GSDMCONFIG_H
#define _GSDMCONFIG_H

#include "platform/linuxplatform.h"

namespace gsdm {

class GsdmConfig {
public:
  ~GsdmConfig();
  static GsdmConfig *GetConfig();

  bool Initialize(const std::string &path);
  int32_t GetInt(const std::string &key, int32_t def);
  int64_t GetLongInt(const std::string &key, int64_t def);
  std::string GetString(const std::string &key, const std::string &def);
  bool GetBool(const std::string &key, bool def);

private:
  GsdmConfig();

  static GsdmConfig config_;
  typedef std::tr1::unordered_map<std::string, std::string> ConfigMap;
  ConfigMap config_map_;
};

inline GsdmConfig *GsdmConfig::GetConfig() {
  return &config_;
}



}

#endif // _GSDMCONFIG_H

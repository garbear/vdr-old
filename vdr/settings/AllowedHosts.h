#pragma once

#include "Types.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>

class CAllowedHost
{
public:
  virtual ~CAllowedHost(void) {}

  bool IsLocalhost(void) const;
  bool Accepts(in_addr_t Address) const;

  static CAllowedHost* FromString(const std::string& value);

private:
  CAllowedHost(void) {}

  struct in_addr addr;
  in_addr_t mask;
};

class CAllowedHosts
{
public:
  static CAllowedHosts& Get(void);
  virtual ~CAllowedHosts(void);

  bool LocalhostOnly(void) const;
  bool Acceptable(in_addr_t Address) const;

  bool Load(void);
  bool Load(const std::string& strFilename);
  bool Save(const std::string& strFilename);

private:
  CAllowedHosts(void);

  std::string                m_strFilename;
  std::vector<CAllowedHost*> m_hosts;
};

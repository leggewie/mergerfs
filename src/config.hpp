/*
  Copyright (c) 2016, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include "branch.hpp"
#include "config_cachefiles.hpp"
#include "config_inodecalc.hpp"
#include "config_moveonenospc.hpp"
#include "config_nfsopenhack.hpp"
#include "config_readdir.hpp"
#include "config_statfs.hpp"
#include "config_statfsignore.hpp"
#include "config_xattr.hpp"
#include "category.hpp"
#include "enum.hpp"
#include "errno.hpp"
#include "funcs.hpp"
#include "policy.hpp"
#include "policy_cache.hpp"
#include "tofrom_wrapper.hpp"

#include "fuse.h"

#include <string>
#include <vector>

#include <stdint.h>
#include <sys/stat.h>

typedef ToFromWrapper<bool>                 ConfigBOOL;
typedef ToFromWrapper<uint64_t>             ConfigUINT64;
typedef ToFromWrapper<int>                  ConfigINT;
typedef ToFromWrapper<std::string>          ConfigSTR;
typedef std::map<std::string,ToFromString*> Str2TFStrMap;

extern const std::string CONTROLFILE;

class Config
{
public:
  Config();

public:
  class Read
  {
  public:
    Read(const Config &config_)
      : _config(config_),
        _guard(config_._rwlock)
    {}
  public:
    inline const Config* operator->() const { return &_config; };
    inline const Config& raw() const { return _config; };
  private:
    const Config              &_config;
    mutable rwlock::ReadGuard  _guard;
  };

public:
  class Write
  {
  public:
    Write(Config &config_)
      : _config(config_),
        _guard(config_._rwlock)
    {}
  public:
    inline Config* operator->() { return &_config; };
    inline Config& raw() { return _config; };
  private:
    Config                     &_config;
    mutable rwlock::WriteGuard  _guard;
  };

public:
  mutable PolicyCache open_cache;

public:
  ConfigBOOL     async_read;
  ConfigBOOL     auto_cache;
  Branches       branches;
  ConfigUINT64   cache_attr;
  ConfigUINT64   cache_entry;
  CacheFiles     cache_files;
  ConfigUINT64   cache_negative_entry;
  ConfigBOOL     cache_readdir;
  ConfigUINT64   cache_statfs;
  ConfigBOOL     cache_symlinks;
  Categories     category;
  ConfigBOOL     direct_io;
  ConfigBOOL     dropcacheonclose;
  ConfigSTR      fsname;
  Funcs          func;
  ConfigUINT64   fuse_msg_size;
  ConfigBOOL     ignorepponrename;
  InodeCalc      inodecalc;
  ConfigBOOL     kernel_cache;
  ConfigBOOL     link_cow;
  ConfigUINT64   minfreespace;
  ConfigSTR      mount;
  MoveOnENOSPC   moveonenospc;
  NFSOpenHack    nfsopenhack;
  ConfigBOOL     nullrw;
  ConfigUINT64   pid;
  ConfigBOOL     posix_acl;
  ReadDir        readdir;
  ConfigBOOL     readdirplus;
  ConfigBOOL     security_capability;
  SrcMounts      srcmounts;
  StatFS         statfs;
  StatFSIgnore   statfs_ignore;
  ConfigBOOL     symlinkify;
  ConfigUINT64   symlinkify_timeout;
  ConfigINT      threads;
  ConfigSTR      version;
  ConfigBOOL     writeback_cache;
  XAttr          xattr;

public:
  friend std::ostream& operator<<(std::ostream &s,
                                  const Config &c);

public:
  bool has_key(const std::string &key) const;
  void keys(std::string &s) const;
  void keys_xattr(std::string &s) const;

public:
  int get(const std::string &key, std::string *val) const;
  int set_raw(const std::string &key, const std::string &val);
  int set(const std::string &key, const std::string &val);

private:
  Str2TFStrMap _map;

private:
  mutable pthread_rwlock_t _rwlock;

private:
  static Config _singleton;

public:
  static inline Config&       instance(void);
  static inline Config::Read  ro(void);
  static inline Config::Write rw(void);
};

inline
Config&
Config::instance(void)
{
  return Config::_singleton;
}

inline
Config::Read
Config::ro(void)
{
  return Config::Read(Config::instance());
}

inline
Config::Write
Config::rw(void)
{
  return Config::Write(Config::instance());
}

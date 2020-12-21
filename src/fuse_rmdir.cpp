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

#include "config.hpp"
#include "errno.hpp"
#include "fs_rmdir.hpp"
#include "fs_path.hpp"
#include "ugid.hpp"

#include <fuse.h>

#include <string>

#include <unistd.h>

using std::string;
using std::vector;


namespace error
{
  static
  int
  calc(const int rv_,
       const int prev_,
       const int cur_)
  {
    if(prev_ != 0)
      return prev_;
    if(rv_ == -1)
      return cur_;
    return 0;
  }
}

namespace l
{
  static
  int
  rmdir_loop_core(const string &basepath_,
                  const char   *fusepath_,
                  const int     error_)
  {
    int rv;
    string fullpath;

    fullpath = fs::path::make(basepath_,fusepath_);

    rv = fs::rmdir(fullpath);

    return error::calc(rv,error_,errno);
  }

  static
  int
  rmdir_loop(const StrVec &basepaths_,
             const char   *fusepath_)
  {
    int error;

    error = 0;
    for(size_t i = 0, ei = basepaths_.size(); i != ei; i++)
      {
        error = l::rmdir_loop_core(basepaths_[i],fusepath_,error);
      }

    return -error;
  }

  static
  int
  rmdir(const Policy::Action &actionFunc_,
        const Branches       &branches_,
        const char           *fusepath_)
  {
    int rv;
    vector<string> basepaths;

    rv = actionFunc_(branches_,fusepath_,&basepaths);
    if(rv == -1)
      return -errno;

    return l::rmdir_loop(basepaths,fusepath_);
  }
}

namespace FUSE
{
  int
  rmdir(const char *fusepath_)
  {
    const fuse_context *fc  = fuse_get_context();
    Config::Read        cfg = Config::ro();
    const ugid::Set     ugid(fc->uid,fc->gid);

    return l::rmdir(cfg->func.rmdir.policy,
                    cfg->branches,
                    fusepath_);
  }
}

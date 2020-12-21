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
#include "fs_clonepath.hpp"
#include "fs_link.hpp"
#include "fs_path.hpp"
#include "ugid.hpp"

#include <fuse.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace error
{
  static
  inline
  int
  calc(const int rv_,
       const int prev_,
       const int cur_)
  {
    if(rv_ == -1)
      {
        if(prev_ == 0)
          return 0;
        return cur_;
      }

    return 0;
  }
}

namespace l
{
  static
  int
  link_create_path_core(const string &oldbasepath_,
                        const string &newbasepath_,
                        const char   *oldfusepath_,
                        const char   *newfusepath_,
                        const int     error_)
  {
    int rv;
    string oldfullpath;
    string newfullpath;

    oldfullpath = fs::path::make(oldbasepath_,oldfusepath_);
    newfullpath = fs::path::make(oldbasepath_,newfusepath_);

    rv = fs::link(oldfullpath,newfullpath);

    return error::calc(rv,error_,errno);
  }

  static
  int
  link_create_path_loop(const StrVec &oldbasepaths_,
                        const string &newbasepath_,
                        const char   *oldfusepath_,
                        const char   *newfusepath_,
                        const string &newfusedirpath_)
  {
    int rv;
    int error;

    error = -1;
    for(size_t i = 0, ei = oldbasepaths_.size(); i != ei; i++)
      {
        rv = fs::clonepath_as_root(newbasepath_,oldbasepaths_[i],newfusedirpath_);
        if(rv == -1)
          error = error::calc(rv,error,errno);
        else
          error = l::link_create_path_core(oldbasepaths_[i],newbasepath_,
                                           oldfusepath_,newfusepath_,
                                           error);
      }

    return -error;
  }

  static
  int
  link_create_path(const Policy::Search &searchFunc_,
                   const Policy::Action &actionFunc_,
                   const Branches       &branches_,
                   const char           *oldfusepath_,
                   const char           *newfusepath_)
  {
    int rv;
    string newfusedirpath;
    StrVec oldbasepaths;
    StrVec newbasepaths;

    rv = actionFunc_(branches_,oldfusepath_,&oldbasepaths);
    if(rv == -1)
      return -errno;

    newfusedirpath = fs::path::dirname(newfusepath_);

    rv = searchFunc_(branches_,newfusedirpath,&newbasepaths);
    if(rv == -1)
      return -errno;

    return l::link_create_path_loop(oldbasepaths,newbasepaths[0],
                                    oldfusepath_,newfusepath_,
                                    newfusedirpath);
  }

  static
  int
  clonepath_if_would_create(const Policy::Search &searchFunc_,
                            const Policy::Create &createFunc_,
                            const Branches       &branches_,
                            const string         &oldbasepath_,
                            const char           *oldfusepath_,
                            const char           *newfusepath_)
  {
    int rv;
    string newfusedirpath;
    StrVec newbasepath;

    newfusedirpath = fs::path::dirname(newfusepath_);

    rv = createFunc_(branches_,newfusedirpath,&newbasepath);
    if(rv == -1)
      return -1;

    if(oldbasepath_ != newbasepath[0])
      return (errno=EXDEV,-1);

    rv = searchFunc_(branches_,newfusedirpath,&newbasepath);
    if(rv == -1)
      return -1;

    return fs::clonepath_as_root(newbasepath[0],oldbasepath_,newfusedirpath);
  }

  static
  int
  link_preserve_path_core(const Policy::Search &searchFunc_,
                          const Policy::Create &createFunc_,
                          const Branches       &branches_,
                          const string         &oldbasepath_,
                          const char           *oldfusepath_,
                          const char           *newfusepath_,
                          const int             error_)
  {
    int rv;
    string oldfullpath;
    string newfullpath;

    oldfullpath = fs::path::make(oldbasepath_,oldfusepath_);
    newfullpath = fs::path::make(oldbasepath_,newfusepath_);

    rv = fs::link(oldfullpath,newfullpath);
    if((rv == -1) && (errno == ENOENT))
      {
        rv = l::clonepath_if_would_create(searchFunc_,createFunc_,
                                          branches_,
                                          oldbasepath_,
                                          oldfusepath_,newfusepath_);
        if(rv != -1)
          rv = fs::link(oldfullpath,newfullpath);
      }

    return error::calc(rv,error_,errno);
  }

  static
  int
  link_preserve_path_loop(const Policy::Search &searchFunc_,
                          const Policy::Create &createFunc_,
                          const Branches       &branches_,
                          const char           *oldfusepath_,
                          const char           *newfusepath_,
                          const StrVec         &oldbasepaths_)
  {
    int error;

    error = -1;
    for(size_t i = 0, ei = oldbasepaths_.size(); i != ei; i++)
      {
        error = l::link_preserve_path_core(searchFunc_,createFunc_,
                                           branches_,
                                           oldbasepaths_[i],
                                           oldfusepath_,newfusepath_,
                                           error);
      }

    return -error;
  }

  static
  int
  link_preserve_path(const Policy::Search &searchFunc_,
                     const Policy::Action &actionFunc_,
                     const Policy::Create &createFunc_,
                     const Branches       &branches_,
                     const char           *oldfusepath_,
                     const char           *newfusepath_)
  {
    int rv;
    StrVec oldbasepaths;

    rv = actionFunc_(branches_,oldfusepath_,&oldbasepaths);
    if(rv == -1)
      return -errno;

    return l::link_preserve_path_loop(searchFunc_,createFunc_,
                                      branches_,
                                      oldfusepath_,newfusepath_,
                                      oldbasepaths);
  }
}

namespace FUSE
{
  int
  link(const char *from_,
       const char *to_)
  {
    const fuse_context *fc  = fuse_get_context();
    Config::Read        cfg = Config::ro();
    const ugid::Set     ugid(fc->uid,fc->gid);

    //    if(cfg->func.create.policy->path_preserving() && !cfg->ignorepponrename)
    return l::link_preserve_path(cfg->func.getattr.policy,
                                 cfg->func.link.policy,
                                 cfg->func.create.policy,
                                 cfg->branches,
                                 from_,
                                 to_);

    return l::link_create_path(cfg->func.getattr.policy,
                               cfg->func.link.policy,
                               cfg->branches,
                               from_,
                               to_);
  }
}

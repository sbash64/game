#ifndef SBASH64_GAME_SNDFILE_WRAPPERS_HPP_
#define SBASH64_GAME_SNDFILE_WRAPPERS_HPP_

#include <sndfile.h>

#include <string>

namespace sbash64::game::sndfile_wrappers {
struct File {
  File(const std::string &path);
  ~File();

  SF_INFO info{};
  SNDFILE *file;
};
} // namespace sbash64::game::sndfile_wrappers

#endif

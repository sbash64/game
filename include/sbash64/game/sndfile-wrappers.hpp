#ifndef SBASH64_GAME_SNDFILE_WRAPPERS_HPP_
#define SBASH64_GAME_SNDFILE_WRAPPERS_HPP_

#include <sndfile.h>

#include <string>

namespace sbash64::game::sndfile_wrappers {
struct File {
  explicit File(const std::string &path);
  ~File();

  File(File &&) = delete;
  auto operator=(File &&) -> File & = delete;
  File(const File &) = delete;
  auto operator=(const File &) -> File & = delete;

  SF_INFO info{};
  SNDFILE *file;
};
} // namespace sbash64::game::sndfile_wrappers

#endif

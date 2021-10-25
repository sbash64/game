#include <sbash64/game/sndfile-wrappers.hpp>

#include <sndfile.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

namespace sbash64::game {
[[noreturn]] static void throwSndfileRuntimeError(std::string_view message,
                                                  SNDFILE *file = {}) {
  std::stringstream stream;
  stream << message << ": " << sf_strerror(file);
  throw std::runtime_error{stream.str()};
}

namespace sndfile_wrappers {
File::File(const std::string &path)
    : file{sf_open(path.c_str(), SFM_READ, &info)} {
  if (file == nullptr)
    throwSndfileRuntimeError("Not able to open input file");
}

File::~File() { sf_close(file); }
} // namespace sndfile_wrappers
} // namespace sbash64::game

#ifndef SBASH64_GAME_ALSA_WRAPPERS_HPP_
#define SBASH64_GAME_ALSA_WRAPPERS_HPP_

#include <asoundlib.h>

#include <string_view>

namespace sbash64::game {
[[noreturn]] void throwAlsaRuntimeError(std::string_view message, int error);

namespace alsa_wrappers {
struct PCM {
  PCM();
  PCM(PCM &&) noexcept;
  ~PCM();

  snd_pcm_t *pcm{};
};
} // namespace alsa_wrappers
} // namespace sbash64::game

#endif

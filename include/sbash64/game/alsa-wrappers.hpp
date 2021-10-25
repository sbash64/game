#ifndef SBASH64_GAME_ALSA_WRAPPERS_HPP_
#define SBASH64_GAME_ALSA_WRAPPERS_HPP_

#include <asoundlib.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

namespace sbash64::game {
[[noreturn]] static void throwAlsaRuntimeError(std::string_view message,
                                               int error) {
  std::stringstream stream;
  stream << message << ": " << snd_strerror(error);
  throw std::runtime_error{stream.str()};
}

namespace alsa_wrappers {
struct PCM {
  PCM() {
    if (const auto error{
            snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)};
        error < 0)
      throwAlsaRuntimeError("playback open error", error);
  }

  ~PCM() {
    // snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
  }

  snd_pcm_t *pcm;
};
} // namespace alsa_wrappers
} // namespace sbash64::game

#endif

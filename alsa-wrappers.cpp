#include <sbash64/game/alsa-wrappers.hpp>

#include <alsa/pcm.h>

#include <sstream>
#include <stdexcept>
#include <string_view>

namespace sbash64::game {
[[noreturn]] void throwAlsaRuntimeError(std::string_view message, int error) {
  std::stringstream stream;
  stream << message << ": " << snd_strerror(error);
  throw std::runtime_error{stream.str()};
}

namespace alsa_wrappers {
PCM::PCM() {
  if (const auto error{
          snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)};
      error < 0)
    throwAlsaRuntimeError("playback open error", error);
}

PCM::~PCM() {
  if (pcm != nullptr) {
    // snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
  }
}

PCM::PCM(PCM &&other) noexcept : pcm{other.pcm} { other.pcm = nullptr; }
} // namespace alsa_wrappers
} // namespace sbash64::game

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sbash64/game/alsa-wrappers.hpp>
#include <sbash64/game/game.hpp>
#include <sbash64/game/sdl-wrappers.hpp>
#include <sbash64/game/sndfile-wrappers.hpp>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_image.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <pthread.h>

// https://stackoverflow.com/a/53067795
static auto getpixel(SDL_Surface *surface, int x, int y) -> Uint32 {
  const auto bpp{surface->format->BytesPerPixel};
  const auto *p = static_cast<const Uint8 *>(surface->pixels) +
                  static_cast<std::ptrdiff_t>(y) * surface->pitch +
                  static_cast<std::ptrdiff_t>(x) * bpp;
  switch (bpp) {
  case 1:
    return *p;
  case 2:
    return *reinterpret_cast<const Uint16 *>(p);
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;
  case 4:
    return *reinterpret_cast<const Uint32 *>(p);
  default:
    return 0;
  }
}

namespace sbash64::game {
constexpr auto toSDLRect(Rectangle a) -> SDL_Rect {
  SDL_Rect converted;
  converted.x = a.origin.x;
  converted.y = a.origin.y;
  converted.w = a.width;
  converted.h = a.height;
  return converted;
}

static auto pressing(const Uint8 *keyStates, SDL_Scancode code) -> bool {
  return keyStates[code] != 0U;
}

static auto applyHorizontalForces(PlayerState playerState,
                                  distance_type groundFriction,
                                  distance_type playerMaxHorizontalSpeed,
                                  distance_type playerRunAcceleration)
    -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_LEFT)) {
    playerState.object.velocity.horizontal -= playerRunAcceleration;
    playerState.directionFacing = DirectionFacing::left;
  }
  if (pressing(keyStates, SDL_SCANCODE_RIGHT)) {
    playerState.object.velocity.horizontal += playerRunAcceleration;
    playerState.directionFacing = DirectionFacing::right;
  }
  playerState.object.velocity.horizontal = withFriction(
      clamp(playerState.object.velocity.horizontal, playerMaxHorizontalSpeed),
      groundFriction);
  return playerState;
}

static auto applyVerticalForces(PlayerState playerState,
                                distance_type playerJumpAcceleration,
                                RationalDistance gravity,
                                std::atomic<bool> &playJumpSound)
    -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::grounded) {
    playerState.jumpState = JumpState::started;
    playJumpSound = true;
    playerState.object.velocity.vertical += playerJumpAcceleration;
  }
  playerState.object.velocity.vertical += gravity;
  if (!pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::started) {
    playerState.jumpState = JumpState::released;
    if (playerState.object.velocity.vertical < 0)
      playerState.object.velocity.vertical = {0, 1};
  }
  return playerState;
}

static void present(const sdl_wrappers::Renderer &rendererWrapper,
                    const sdl_wrappers::Texture &textureWrapper,
                    const Rectangle &sourceRectangle, int pixelScale,
                    const Rectangle &destinationRectangle,
                    SDL_RendererFlip flip = SDL_FLIP_NONE) {
  const auto projection{toSDLRect(destinationRectangle * pixelScale)};
  const auto sourceSDLRect{toSDLRect(sourceRectangle)};
  SDL_RenderCopyEx(rendererWrapper.renderer, textureWrapper.texture,
                   &sourceSDLRect, &projection, 0, nullptr, flip);
}

static auto pollSdlEvents() -> bool {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
    if (event.type == SDL_QUIT)
      return false;
  return true;
}

static auto applyVelocity(PlayerState playerState) -> PlayerState {
  playerState.object = applyVelocity(playerState.object);
  return playerState;
}

static void throwAlsaRuntimeErrorOnFailure(const std::function<int()> &f,
                                           std::string_view message) {
  if (const auto error{f()}; error < 0)
    throwAlsaRuntimeError(message, error);
}

static void loopAudio(std::atomic<bool> &quitAudioThread,
                      std::atomic<bool> &playJumpSound,
                      const std::vector<short> &backgroundMusicData,
                      const std::vector<short> &jumpSoundData,
                      const alsa_wrappers::PCM &pcm,
                      std::vector<short> buffer) {
  const auto periodSize{buffer.size() / 2};
  std::ptrdiff_t backgroundMusicDataOffset{0};
  std::ptrdiff_t jumpSoundDataOffset{0};
  auto playingJumpSound{false};

  while (!quitAudioThread) {
    {
      auto expected{true};
      if (!playingJumpSound &&
          playJumpSound.compare_exchange_strong(expected, false)) {
        playingJumpSound = true;
        jumpSoundDataOffset = 0;
      }
    }

    if (backgroundMusicDataOffset + buffer.size() > backgroundMusicData.size())
      backgroundMusicDataOffset = 0;

    std::copy(backgroundMusicData.begin() + backgroundMusicDataOffset,
              backgroundMusicData.begin() + backgroundMusicDataOffset +
                  buffer.size(),
              buffer.begin());

    if (playingJumpSound) {
      if (jumpSoundDataOffset + periodSize > jumpSoundData.size()) {
        playingJumpSound = false;
      } else {
        auto counter{0};
        for (auto &x : buffer)
          x += jumpSoundData[jumpSoundDataOffset + counter++ / 2];
      }
    }

    throwAlsaRuntimeErrorOnFailure(
        [&pcm]() { return snd_pcm_wait(pcm.pcm, -1); }, "wait failed");

    if (const auto framesWritten{
            snd_pcm_writei(pcm.pcm, buffer.data(), periodSize)};
        framesWritten < 0)
      throwAlsaRuntimeErrorOnFailure(
          [&pcm, framesWritten]() {
            return snd_pcm_recover(pcm.pcm, static_cast<int>(framesWritten), 0);
          },
          "recover failed");
    else {
      backgroundMusicDataOffset += 2 * periodSize;
      if (playingJumpSound)
        jumpSoundDataOffset += periodSize;
    }
  }
}

static auto readShortAudio(const std::string &path) -> std::vector<short> {
  std::vector<short> audio;
  sndfile_wrappers::File file{path};
  audio.resize(static_cast<std::vector<short>::size_type>(file.info.frames *
                                                          file.info.channels));
  sf_readf_short(file.file, audio.data(), file.info.frames);
  return audio;
}

static auto initializeAlsaPcm(snd_pcm_uframes_t periodSize)
    -> alsa_wrappers::PCM {
  alsa_wrappers::PCM pcm;
  snd_pcm_hw_params_t *hw_params = nullptr;
  throwAlsaRuntimeErrorOnFailure(
      [&hw_params]() { return snd_pcm_hw_params_malloc(&hw_params); },
      "cannot allocate hardware parameter structure");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() { return snd_pcm_hw_params_any(pcm.pcm, hw_params); },
      "cannot initialize hardware parameter structure");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() {
        return snd_pcm_hw_params_set_access(pcm.pcm, hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED);
      },
      "cannot set access type");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() {
        return snd_pcm_hw_params_set_format(pcm.pcm, hw_params,
                                            SND_PCM_FORMAT_S16);
      },
      "cannot set sample format");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() {
        auto desiredSampleRate{44100U};
        auto direction{0};
        return snd_pcm_hw_params_set_rate_near(pcm.pcm, hw_params,
                                               &desiredSampleRate, &direction);
      },
      "cannot set sample rate");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params, periodSize]() {
        snd_pcm_uframes_t desiredPeriodSize{periodSize};
        auto direction{0};
        return snd_pcm_hw_params_set_period_size(pcm.pcm, hw_params,
                                                 desiredPeriodSize, direction);
      },
      "cannot set period size");
  unsigned int periods{2};
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params, &periods]() {
        auto direction{0};
        return snd_pcm_hw_params_set_periods_near(pcm.pcm, hw_params, &periods,
                                                  &direction);
      },
      "cannot set periods");
  snd_pcm_uframes_t bufferSize{periods * periodSize};
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params, &bufferSize]() {
        return snd_pcm_hw_params_set_buffer_size_near(pcm.pcm, hw_params,
                                                      &bufferSize);
      },
      "cannot set buffer size");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() {
        return snd_pcm_hw_params_set_channels(pcm.pcm, hw_params, 2);
      },
      "cannot set channel count");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, hw_params]() { return snd_pcm_hw_params(pcm.pcm, hw_params); },
      "cannot set parameters");
  snd_pcm_hw_params_free(hw_params);

  snd_pcm_sw_params_t *sw_params = nullptr;
  throwAlsaRuntimeErrorOnFailure(
      [&sw_params]() { return snd_pcm_sw_params_malloc(&sw_params); },
      "cannot allocate software parameters structure");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, sw_params]() {
        return snd_pcm_sw_params_current(pcm.pcm, sw_params);
      },
      "cannot initialize software parameters structure");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, sw_params, periodSize]() {
        return snd_pcm_sw_params_set_avail_min(pcm.pcm, sw_params, periodSize);
      },
      "cannot set minimum available count");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, sw_params, bufferSize, periodSize]() {
        return snd_pcm_sw_params_set_start_threshold(
            pcm.pcm, sw_params, (bufferSize / periodSize) * periodSize);
      },
      "cannot set start threshold");
  throwAlsaRuntimeErrorOnFailure(
      [&pcm, sw_params]() { return snd_pcm_sw_params(pcm.pcm, sw_params); },
      "cannot set software parameters");

  snd_output_t *debugOutput{};
  throwAlsaRuntimeErrorOnFailure(
      [&debugOutput]() {
        return snd_output_stdio_attach(&debugOutput, stdout, 0);
      },
      "cannot initialize stdio output");
  snd_pcm_dump(pcm.pcm, debugOutput);
  snd_output_close(debugOutput);

  return pcm;
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath,
                const std::string &enemyImagePath,
                const std::string &backgroundMusicPath,
                const std::string &jumpSoundPath) -> int {
  sdl_wrappers::Init sdlInitialization;
  constexpr auto pixelScale{4};
  const auto cameraWidth{256};
  const auto cameraHeight{240};
  constexpr auto screenWidth{cameraWidth * pixelScale};
  constexpr auto screenHeight{cameraHeight * pixelScale};
  sdl_wrappers::Window windowWrapper{screenWidth, screenHeight};
  sdl_wrappers::Renderer rendererWrapper{windowWrapper.window};
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  sdl_wrappers::ImageInit sdlImageInitialization;
  sdl_wrappers::ImageSurface playerImageSurfaceWrapper{playerImagePath};
  SDL_SetColorKey(playerImageSurfaceWrapper.surface, SDL_TRUE,
                  getpixel(playerImageSurfaceWrapper.surface, 1, 9));
  const auto playerWidth{16};
  const auto playerHeight{16};
  const Rectangle playerSourceRect{Point{1, 9}, playerWidth, playerHeight};
  sdl_wrappers::ImageSurface backgroundImageSurfaceWrapper{backgroundImagePath};
  const auto backgroundSourceWidth{backgroundImageSurfaceWrapper.surface->w};
  sdl_wrappers::ImageSurface enemyImageSurfaceWrapper{enemyImagePath};
  SDL_SetColorKey(enemyImageSurfaceWrapper.surface, SDL_TRUE,
                  getpixel(enemyImageSurfaceWrapper.surface, 1, 28));
  const auto enemyWidth{16};
  const auto enemyHeight{16};
  const Rectangle enemySourceRect{Point{1, 28}, enemyWidth, enemyHeight};
  sdl_wrappers::Texture playerTextureWrapper{rendererWrapper.renderer,
                                             playerImageSurfaceWrapper.surface};
  sdl_wrappers::Texture backgroundTextureWrapper{
      rendererWrapper.renderer, backgroundImageSurfaceWrapper.surface};
  sdl_wrappers::Texture enemyTextureWrapper{rendererWrapper.renderer,
                                            enemyImageSurfaceWrapper.surface};
  const Rectangle floorRectangle{Point{0, cameraHeight - 32},
                                 backgroundSourceWidth, 32};
  const Rectangle levelRectangle{Point{-1, -1}, backgroundSourceWidth + 1,
                                 cameraHeight + 1};
  const RationalDistance gravity{1, 4};
  const auto groundFriction{1};
  const auto playerMaxHorizontalSpeed{4};
  const auto playerJumpAcceleration{-6};
  const auto playerRunAcceleration{2};
  PlayerState playerState{
      {Rectangle{Point{0, topEdge(floorRectangle) - playerHeight}, playerWidth,
                 playerHeight},
       Velocity{{0, 1}, 0}},
      JumpState::grounded,
      DirectionFacing::right};
  MovingObject enemy{{Point{140, topEdge(floorRectangle) - enemyHeight},
                      enemyWidth, enemyHeight},
                     Velocity{{0, 1}, 0}};
  const Rectangle blockRectangle{Point{256, 144}, 15, 15};
  const auto pipeHeight{40};
  const Rectangle pipeRectangle{
      Point{448, topEdge(floorRectangle) - pipeHeight}, 30, pipeHeight};
  Rectangle backgroundSourceRectangle{Point{0, 0}, cameraWidth, cameraHeight};

  std::atomic<bool> quitAudioThread;
  std::atomic<bool> playJumpSound;
  const auto alsaPeriodSize{512};
  std::thread audioThread{loopAudio,
                          std::ref(quitAudioThread),
                          std::ref(playJumpSound),
                          readShortAudio(backgroundMusicPath),
                          readShortAudio(jumpSoundPath),
                          initializeAlsaPcm(alsaPeriodSize),
                          std::vector<short>(2 * alsaPeriodSize)};
  {
    sched_param param{sched_get_priority_max(SCHED_RR)};
    pthread_setschedparam(audioThread.native_handle(), SCHED_RR, &param);
  }
  while (pollSdlEvents()) {
    playerState = handleVerticalCollisions(
        applyVerticalForces(applyHorizontalForces(playerState, groundFriction,
                                                  playerMaxHorizontalSpeed,
                                                  playerRunAcceleration),
                            playerJumpAcceleration, gravity, playJumpSound),
        {blockRectangle, pipeRectangle}, {blockRectangle}, floorRectangle);
    playerState.object = handleHorizontalCollisions(
        {playerState.object.rectangle, playerState.object.velocity},
        {blockRectangle, pipeRectangle}, {blockRectangle, pipeRectangle},
        levelRectangle);
    playerState = applyVelocity(playerState);
    if (leftEdge(playerState.object.rectangle) < leftEdge(enemy.rectangle))
      enemy.velocity.horizontal = -1;
    else if (leftEdge(playerState.object.rectangle) > leftEdge(enemy.rectangle))
      enemy.velocity.horizontal = 1;
    else
      enemy.velocity.horizontal = 0;
    enemy = handleHorizontalCollisions(enemy, {pipeRectangle}, {pipeRectangle},
                                       levelRectangle);
    enemy.rectangle = applyHorizontalVelocity(enemy);
    backgroundSourceRectangle =
        shiftBackground(backgroundSourceRectangle, backgroundSourceWidth,
                        playerState.object.rectangle, cameraWidth);
    present(rendererWrapper, backgroundTextureWrapper,
            backgroundSourceRectangle, pixelScale,
            {Point{0, 0}, cameraWidth, cameraHeight});
    present(rendererWrapper, enemyTextureWrapper, enemySourceRect, pixelScale,
            shiftHorizontally(enemy.rectangle,
                              -leftEdge(backgroundSourceRectangle)),
            enemy.velocity.horizontal < 0 ? SDL_FLIP_HORIZONTAL
                                          : SDL_FLIP_NONE);
    present(rendererWrapper, playerTextureWrapper, playerSourceRect, pixelScale,
            shiftHorizontally(playerState.object.rectangle,
                              -leftEdge(backgroundSourceRectangle)),
            playerState.directionFacing == DirectionFacing::right
                ? SDL_FLIP_NONE
                : SDL_FLIP_HORIZONTAL);
    SDL_RenderPresent(rendererWrapper.renderer);
  }
  quitAudioThread = true;
  audioThread.join();
  return EXIT_SUCCESS;
}
} // namespace sbash64::game

int main(int argc, char *argv[]) {
  std::span<char *> arguments{argv,
                              static_cast<std::span<char *>::size_type>(argc)};
  if (arguments.size() < 6)
    return EXIT_FAILURE;
  try {
    return sbash64::game::run(arguments[1], arguments[2], arguments[3],
                              arguments[4], arguments[5]);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <limits>
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

static void initializeSDLImage() {
  constexpr auto imageFlags{IMG_INIT_PNG};
  if (!(IMG_Init(imageFlags) & imageFlags)) {
    std::stringstream stream;
    stream << "SDL_image could not initialize! SDL_image Error: "
           << IMG_GetError();
    throw std::runtime_error{stream.str()};
  }
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
                                RationalDistance gravity) -> PlayerState {
  const auto *keyStates{SDL_GetKeyboardState(nullptr)};
  if (pressing(keyStates, SDL_SCANCODE_UP) &&
      playerState.jumpState == JumpState::grounded) {
    playerState.jumpState = JumpState::started;
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
                    const Rectangle &destinationRectangle) {
  const auto projection{toSDLRect(destinationRectangle * pixelScale)};
  const auto sourceSDLRect{toSDLRect(sourceRectangle)};
  SDL_RenderCopyEx(rendererWrapper.renderer, textureWrapper.texture,
                   &sourceSDLRect, &projection, 0, nullptr, SDL_FLIP_NONE);
}

static void
presentFlippedHorizontally(const sdl_wrappers::Renderer &rendererWrapper,
                           const sdl_wrappers::Texture &textureWrapper,
                           const Rectangle &sourceRectangle, int pixelScale,
                           const Rectangle &destinationRectangle) {
  const auto projection{toSDLRect(destinationRectangle * pixelScale)};
  const auto sourceSDLRect{toSDLRect(sourceRectangle)};
  SDL_RenderCopyEx(rendererWrapper.renderer, textureWrapper.texture,
                   &sourceSDLRect, &projection, 0, nullptr,
                   SDL_FLIP_HORIZONTAL);
}

static void flushEvents(bool &playing) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0)
    playing = event.type != SDL_QUIT;
}

static auto applyVelocity(PlayerState playerState) -> PlayerState {
  playerState.object = applyVelocity(playerState.object);
  return playerState;
}

static auto run(const std::string &playerImagePath,
                const std::string &backgroundImagePath,
                const std::string &enemyImagePath,
                const std::string &backgroundMusicPath) -> int {

  std::atomic<bool> quitAudioThread;
  std::vector<short> backgroundMusicData;
  {
    sndfile_wrappers::File backgroundMusicFile{backgroundMusicPath};
    backgroundMusicData.resize(backgroundMusicFile.info.frames *
                               backgroundMusicFile.info.channels);
    sf_readf_short(backgroundMusicFile.file, backgroundMusicData.data(),
                   backgroundMusicFile.info.frames);
  }
  alsa_wrappers::PCM pcm;
  std::thread audioThread{[&quitAudioThread, &backgroundMusicData, &pcm]() {
    const auto framesPerUpdate{4096};
    snd_pcm_hw_params_t *hw_params = nullptr;
    if (const auto error{snd_pcm_hw_params_malloc(&hw_params)}; error < 0)
      throwAlsaRuntimeError("cannot allocate hardware parameter structure",
                            error);

    if (const auto error{snd_pcm_hw_params_any(pcm.pcm, hw_params)}; error < 0)
      throwAlsaRuntimeError("cannot initialize hardware parameter structure",
                            error);

    if (const auto error{snd_pcm_hw_params_set_access(
            pcm.pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)};
        error < 0)
      throwAlsaRuntimeError("cannot set access type", error);

    if (const auto error{snd_pcm_hw_params_set_format(pcm.pcm, hw_params,
                                                      SND_PCM_FORMAT_S16)};
        error < 0)
      throwAlsaRuntimeError("cannot set sample format", error);

    auto desiredSampleRate{44100U};
    auto direction{0};
    if (const auto error{snd_pcm_hw_params_set_rate_near(
            pcm.pcm, hw_params, &desiredSampleRate, &direction)};
        error < 0)
      throwAlsaRuntimeError("cannot set sample rate", error);

    if (const auto error{snd_pcm_hw_params_set_channels(pcm.pcm, hw_params, 2)};
        error < 0)
      throwAlsaRuntimeError("cannot set channel count", error);

    if (const auto error{snd_pcm_hw_params(pcm.pcm, hw_params)}; error < 0)
      throwAlsaRuntimeError("cannot set parameters", error);

    snd_pcm_hw_params_free(hw_params);

    /* tell ALSA to wake us up whenever 4096 or more frames
       of playback data can be delivered. Also, tell
       ALSA that we'll start the device ourselves.
    */

    snd_pcm_sw_params_t *sw_params = nullptr;
    if (const auto error{snd_pcm_sw_params_malloc(&sw_params)}; error < 0)
      throwAlsaRuntimeError("cannot allocate software parameters structure",
                            error);

    if (const auto error{snd_pcm_sw_params_current(pcm.pcm, sw_params)};
        error < 0)
      throwAlsaRuntimeError("cannot initialize software parameters structure",
                            error);

    if (const auto error{snd_pcm_sw_params_set_avail_min(pcm.pcm, sw_params,
                                                         framesPerUpdate)};
        error < 0)
      throwAlsaRuntimeError("cannot set minimum available count", error);

    if (const auto error{
            snd_pcm_sw_params_set_start_threshold(pcm.pcm, sw_params, 0U)};
        error < 0)
      throwAlsaRuntimeError("cannot set start mode", error);

    if (const auto error{snd_pcm_sw_params(pcm.pcm, sw_params)}; error < 0)
      throwAlsaRuntimeError("cannot set software parameters", error);

    /* the interface will interrupt the kernel every 4096 frames, and ALSA
       will wake up this program very soon after that.
    */

    if (const auto error{snd_pcm_prepare(pcm.pcm)}; error < 0)
      throwAlsaRuntimeError("cannot prepare audio interface for use", error);

    std::array<std::int16_t, 2 * framesPerUpdate> buffer{};
    auto fileOffset{0};

    while (!quitAudioThread) {
      std::copy(backgroundMusicData.begin() + fileOffset,
                backgroundMusicData.begin() + fileOffset + buffer.size(),
                buffer.begin());

      if (const auto error{snd_pcm_wait(pcm.pcm, 1000)}; error < 0)
        throwAlsaRuntimeError("poll failed", error);

      auto frames_to_deliver = snd_pcm_avail_update(pcm.pcm);

      if (frames_to_deliver < 0) {
        if (frames_to_deliver == -EPIPE)
          throw std::runtime_error{"an xrun occured"};
        throw std::runtime_error{"unknown ALSA avail update"};
      }

      frames_to_deliver = frames_to_deliver > framesPerUpdate
                              ? framesPerUpdate
                              : frames_to_deliver;

      if (const auto error{
              snd_pcm_writei(pcm.pcm, buffer.data(), frames_to_deliver)};
          error != frames_to_deliver)
        throw std::runtime_error{"write error"};

      fileOffset += 2 * frames_to_deliver;

      if (fileOffset + buffer.size() > backgroundMusicData.size())
        fileOffset = 0;
    }
  }};

  sdl_wrappers::Init scopedInitialization;
  constexpr auto pixelScale{4};
  const auto cameraWidth{256};
  const auto cameraHeight{240};
  constexpr auto screenWidth{cameraWidth * pixelScale};
  constexpr auto screenHeight{cameraHeight * pixelScale};
  sdl_wrappers::Window windowWrapper{screenWidth, screenHeight};
  sdl_wrappers::Renderer rendererWrapper{windowWrapper.window};
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  initializeSDLImage();
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
  auto playing{true};
  while (playing) {
    flushEvents(playing);
    playerState = handleVerticalCollisions(
        applyVerticalForces(applyHorizontalForces(playerState, groundFriction,
                                                  playerMaxHorizontalSpeed,
                                                  playerRunAcceleration),
                            playerJumpAcceleration, gravity),
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
                              -leftEdge(backgroundSourceRectangle)));
    if (playerState.directionFacing == DirectionFacing::right)
      present(rendererWrapper, playerTextureWrapper, playerSourceRect,
              pixelScale,
              shiftHorizontally(playerState.object.rectangle,
                                -leftEdge(backgroundSourceRectangle)));
    else
      presentFlippedHorizontally(
          rendererWrapper, playerTextureWrapper, playerSourceRect, pixelScale,
          shiftHorizontally(playerState.object.rectangle,
                            -leftEdge(backgroundSourceRectangle)));
    SDL_RenderPresent(rendererWrapper.renderer);
  }
  quitAudioThread = true;
  audioThread.join();
  return EXIT_SUCCESS;
}
} // namespace sbash64::game

int main(int argc, char *argv[]) {
  if (argc < 5)
    return EXIT_FAILURE;
  try {
    return sbash64::game::run(argv[1], argv[2], argv[3], argv[4]);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}

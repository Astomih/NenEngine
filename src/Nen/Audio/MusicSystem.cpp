#include <memory>
#include <string>
#include <unordered_map>

#include <SDL_mixer.h>

#include <Audio/MusicSystem.hpp>
#include <IO/AssetReader.hpp>

namespace nen {
music::music() {}

music::~music() {}

void music::set_volume(int value) { ::Mix_VolumeMusic(value); }

void music::PlayMusic() { ::Mix_PlayMusic((::Mix_Music *)buffer, -1); }

void music::LoadMusicFromFile(std::string_view fileName) {
  auto rwops = (SDL_RWops *)data_io::LoadAsRWops(asset_type::Music, fileName);
  ::Mix_Music *m = ::Mix_LoadMUS_RW(rwops, 1);
  if (m) {
    buffer = (void *)m;
  }
}

void music::UnloadMusic() {
  if (buffer) {
    ::Mix_FreeMusic((::Mix_Music *)buffer);
    buffer = nullptr;
  }
}

} // namespace nen
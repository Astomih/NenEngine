﻿#include "../Texture/SurfaceHandle.hpp"
#include <Nen.hpp>
#include <SDL.h>
#include <SDL_ttf.h>
#include <cassert>

namespace nen {
bool font::LoadFromFile(std::string_view fontName, int pointSize) {
  this->fontName = fontName;
  this->pointSize = pointSize;
  m_font = (void *)::TTF_OpenFontRW(
      (SDL_RWops *)asset_reader::LoadAsRWops(asset_type::Font, this->fontName),
      1, this->pointSize);
  if (!m_font) {
    logger::Error("%s", TTF_GetError());
    return false;
  }
  return (isLoad = true);
}

std::shared_ptr<texture> font::RenderText(std::string_view text,
                                          const color &_color, quality quality,
                                          const color &backgroundColor) {
  auto tex = std::make_shared<texture>();
  // My Color to SDL_Color
  SDL_Color sdlColor;
  sdlColor.r = static_cast<Uint8>(_color.r * 255);
  sdlColor.g = static_cast<Uint8>(_color.g * 255);
  sdlColor.b = static_cast<Uint8>(_color.b * 255);
  sdlColor.a = 255;
  tex->id = std::string(text);
  ::SDL_Surface *surf = nullptr;
  switch (quality) {
  case nen::font::quality::Solid:
    surf = ::TTF_RenderUTF8_Solid((::TTF_Font *)m_font,
                                  std::string(text).c_str(), sdlColor);
    break;
  case nen::font::quality::Shaded: {
    SDL_Color bg;
    bg.r = static_cast<Uint8>(_color.r * 255);
    bg.g = static_cast<Uint8>(_color.g * 255);
    bg.b = static_cast<Uint8>(_color.b * 255);
    bg.a = 255;
    surf = ::TTF_RenderUTF8_Shaded((::TTF_Font*)m_font, std::string(text).c_str(), sdlColor,
                                   bg);
  } break;
  case nen::font::quality::Blended:
    surf =
        ::TTF_RenderUTF8_Blended((::TTF_Font*)m_font, std::string(text).c_str(), sdlColor);
    break;
  default:
    break;
  }
  tex->SetWidth(surf->w);
  tex->SetHeight(surf->h);
  surface_handler::Register(
      text, std::move(std::unique_ptr<::SDL_Surface, SDLObjectCloser>(surf)));
  return tex;
}
} // namespace nen

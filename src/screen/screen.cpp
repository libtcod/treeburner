/*
 * Copyright (c) 2009 Jice
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Jice may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Jice ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Jice BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "screen.hpp"

#include "constants.hpp"
#include "main.hpp"

namespace screen {

// chapter pictures
static constexpr int chapNbPix[] = {
    1,
    1,
};
static constexpr char* chapter_pictures[] = {
    "data/img/g32.png",
    "data/img/f22.png",
};

TCODImage* Screen::loadChapterPicture(bool big) {
  int picture_i = 0;
  for (int ch = 0; ch < saveGame.chapter; ch++) {
    picture_i += chapNbPix[ch];
  }
  char tmp[128];
  strcpy(tmp, chapter_pictures[picture_i]);
  if (big) {
    char* ptr = strrchr(tmp, '.');
    strcpy(ptr, "b.png");
  }
  TCODImage* ret = new TCODImage(tmp);
  prepareImage(ret);
  return ret;
}

void Screen::prepareImage(TCODImage* img) const {
  static constexpr auto KEY_LENGTH = 1024;
  static constexpr auto KEY_MASK = 1023;
  static std::array<uint8_t, KEY_LENGTH> key;
  static bool init = false;

  if (!init) {
    uint8_t seed = (uint8_t)(666 & 0xFF);
    for (auto& it : key) {
      it = seed;
      seed = seed * 1103515245 + 12345;
    }
    init = true;
  }
  int w, h;
  img->getSize(&w, &h);
  int key_i = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      TCODColor col = img->getPixel(x, y);
      col.r ^= key.at(key_i++);
      key_i &= KEY_MASK;
      col.g ^= key.at(key_i++);
      key_i &= KEY_MASK;
      col.b ^= key.at(key_i++);
      key_i &= KEY_MASK;
      img->putPixel(x, y, col);
    }
  }
}

bool Screen::update() {
  if (time_fix_ > 0) {
    // this is the frame where activate has been called.
    // it might be unusually long. skip update to avoid jerky animation
    time_fix_ = 0.0f;
    return true;
  }
  static const float timeScale = config.getFloatProperty("config.gameplay.timeScale");
  const float elapsed = TCODSystem::getLastFrameLength() * timeScale;
  if (fade_ == FADE_UP) {
    fade_level_ += elapsed * 1000.0f / fade_in_length_ms_;
    if (fade_level_ >= 1.0f) {
      TCODConsole::setFade(255, fade_in_color_);
      fade_ = FADE_OFF;
      fade_level_ = 1.0f;
    } else {
      TCODConsole::setFade((int)(fade_level_ * 255), fade_in_color_);
    }
  } else if (fade_ == FADE_DOWN) {
    fade_level_ -= elapsed * 1000.0f / fade_out_length_ms_;
    if (fade_level_ < 0.0f) {
      TCODConsole::setFade(0, fade_out_color_);
      fade_level_ = 0.0f;
    } else {
      TCODConsole::setFade((int)(fade_level_ * 255), fade_out_color_);
    }
    bool fadeEnded = true;
    for (int x = 0; x < CON_W && fadeEnded; x++) {
      for (int y = 0; y < CON_H && fadeEnded; y++) {
        TCODColor fg_color = TCODConsole::root->getCharForeground(x, y);
        fg_color = TCODColor::lerp(fg_color, fade_out_color_, fade_level_);
        if (fg_color.r != fade_out_color_.r || fg_color.g != fade_out_color_.g || fg_color.b != fade_out_color_.b) {
          fadeEnded = false;
        }
        TCODColor bg_color = TCODConsole::root->getCharBackground(x, y);
        bg_color = TCODColor::lerp(bg_color, fade_out_color_, fade_level_);
        if (bg_color.r != fade_out_color_.r || bg_color.g != fade_out_color_.g || bg_color.b != fade_out_color_.b) {
          fadeEnded = false;
        }
      }
    }
    if (fadeEnded) fade_level_ = 0.0f;
  }
  return update(elapsed, key_, ms_);
}

void Screen::setFadeIn(int lengthInMilli, TCODColor col) {
  fade_in_length_ms_ = lengthInMilli;
  fade_in_color_ = col;
}

void Screen::setFadeOut(int lengthInMilli, TCODColor col) {
  fade_out_length_ms_ = lengthInMilli;
  fade_out_color_ = col;
}
}  // namespace screen

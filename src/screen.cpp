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

int SCREEN_MAIN_MENU;
int SCREEN_STORY;
int SCREEN_CHAPTER_1;
int SCREEN_GAME_OVER;
int SCREEN_GAME_WON;

// chapter pictures
static const int chapNbPix[] = {
    1,
    1,
};
static const char* chapPix[] = {
    "data/img/g32.png",
    "data/img/f22.png",
};

TCODImage* Screen::loadChapterPicture(bool big) {
  int pixnum = 0;
  for (int ch = 0; ch < saveGame.chapter; ch++) {
    pixnum += chapNbPix[ch];
  }
  char tmp[128];
  strcpy(tmp, chapPix[pixnum]);
  if (big) {
    char* ptr = strrchr(tmp, '.');
    strcpy(ptr, "b.png");
  }
  TCODImage* ret = new TCODImage(tmp);
  prepareImage(ret);
  return ret;
}

void Screen::prepareImage(TCODImage* img) const {
  static uint8_t key[1024];
  static bool init = false;
#define KEYLEN 1024
#define KEYMASK 1023

  if (!init) {
    uint8_t seed = (uint8_t)(666 & 0xFF);
    for (int i = 0; i < KEYLEN; i++) {
      key[i] = seed;
      seed = seed * 1103515245 + 12345;
    }
    init = true;
  }
  int k = 0, w, h;
  img->getSize(&w, &h);
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      TCODColor col = img->getPixel(x, y);
      col.r ^= key[k++];
      k &= KEYMASK;
      col.g ^= key[k++];
      k &= KEYMASK;
      col.b ^= key[k++];
      k &= KEYMASK;
      img->putPixel(x, y, col);
    }
  }
}

bool Screen::update() {
  if (timefix > 0) {
    // this is the frame where activate has been called.
    // it might be unusually long. skip update to avoid jerky animation
    timefix = 0.0f;
    return true;
  }
  static float timeScale = config.getFloatProperty("config.gameplay.timeScale");
  float elapsed = TCODSystem::getLastFrameLength() * timeScale;
  if (fade == FADE_UP) {
    fadeLvl += elapsed * 1000.0f / fadeInLength;
    if (fadeLvl >= 1.0f) {
      TCODConsole::setFade(255, fadeInColor);
      fade = FADE_OFF;
      fadeLvl = 1.0f;
    } else {
      TCODConsole::setFade((int)(fadeLvl * 255), fadeInColor);
    }
  } else if (fade == FADE_DOWN) {
    fadeLvl -= elapsed * 1000.0f / fadeOutLength;
    if (fadeLvl < 0.0f) {
      TCODConsole::setFade(0, fadeOutColor);
      fadeLvl = 0.0f;
    } else {
      TCODConsole::setFade((int)(fadeLvl * 255), fadeOutColor);
    }
    bool fadeEnded = true;
    for (int x = 0; x < CON_W && fadeEnded; x++) {
      for (int y = 0; y < CON_H && fadeEnded; y++) {
        TCODColor fcol = TCODConsole::root->getCharForeground(x, y);
        fcol = TCODColor::lerp(fcol, fadeOutColor, fadeLvl);
        if (fcol.r != fadeOutColor.r || fcol.g != fadeOutColor.g || fcol.b != fadeOutColor.b) {
          fadeEnded = false;
        }
        TCODColor bcol = TCODConsole::root->getCharBackground(x, y);
        bcol = TCODColor::lerp(bcol, fadeOutColor, fadeLvl);
        if (bcol.r != fadeOutColor.r || bcol.g != fadeOutColor.g || bcol.b != fadeOutColor.b) {
          fadeEnded = false;
        }
      }
    }
    if (fadeEnded) fadeLvl = 0.0f;
  }
  return update(elapsed, key, ms);
}

void Screen::setFadeIn(int lengthInMilli, TCODColor col) {
  fadeInLength = lengthInMilli;
  fadeInColor = col;
}

void Screen::setFadeOut(int lengthInMilli, TCODColor col) {
  fadeOutLength = lengthInMilli;
  fadeOutColor = col;
}

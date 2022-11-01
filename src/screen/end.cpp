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
#include "screen/end.hpp"

#include <SDL.h>

#include "constants.hpp"
#include "main.hpp"

namespace screen {
EndScreen::EndScreen(const char* txt, float fadeLvl, bool stats)
    : Screen(fadeLvl), txt(strdup(txt)), noiseZ(0.0f), stats(stats) {
  fadeInLength = fadeOutLength = (int)(config.getFloatProperty("config.display.fadeTime") * 1000);
  alignment = TCOD_CENTER;
}

void EndScreen::onInitialise() {
  img = new TCODImage(CON_W * 2, CON_H * 2);
  onFontChange();
}

void EndScreen::onActivate() {
  // set keyboard mode to RELEASED only
  engine.setKeyboardMode(UMBRA_KEYBOARD_RELEASED);
  Screen::onActivate();
  version = getStringParam("version");
  const char* al = getStringParam("alignment");
  if (al) {
    if (strcmp(al, "left") == 0)
      alignment = TCOD_LEFT;
    else if (strcmp(al, "right") == 0)
      alignment = TCOD_RIGHT;
    else if (strcmp(al, "center") == 0)
      alignment = TCOD_CENTER;
  }
}

void EndScreen::render() {
  static TCODNoise noise(3);
  img->clear(TCODColor::black);
  for (int x = 0; x < CON_W * 2; x++) {
    for (int y = 0; y < CON_H * 2; y++) {
      float f[3] = {(float)(x) / CON_W, (float)(y) / (2 * CON_H) + 2 * noiseZ, noiseZ};
      float v = 0.5f * (1.0f + noise.getFbm(f, 5.0f)) * y / (CON_H * 2);
      img->putPixel(x, y, TCODColor::red * v);
    }
  }
  img->blit2x(TCODConsole::root, 0, 0);
  // TCODConsole::root->setDefaultBackground(TCODColor::black);
  // TCODConsole::root->clear();
  TCODConsole::root->setDefaultForeground(TCODColor::lightRed);
  switch (alignment) {
    case TCOD_CENTER:
      TCODConsole::root->printRectEx(CON_W / 2, CON_H / 2, 70, 0, TCOD_BKGND_NONE, TCOD_CENTER, txt);
      break;
    case TCOD_LEFT:
      renderText(5, 2, 70, txt);
      // TCODConsole::root->printRectEx(5,CON_H/2,70,0,TCOD_BKGND_NONE,TCOD_LEFT,txt);
      break;
    case TCOD_RIGHT:
      TCODConsole::root->printRectEx(CON_W - 5, CON_H / 2, 70, 0, TCOD_BKGND_NONE, TCOD_RIGHT, txt);
      break;
  }

  if (version != NULL && version[0] != 0)
    TCODConsole::root->printEx(CON_W - 5, CON_H - 2, TCOD_BKGND_NONE, TCOD_RIGHT, "v%s", version);

  /*
  TODO
if ( gameEngine && ((Game *)gameEngine)->bossIsDead ) {
  int score = gameEngine->stats.nbCreatureKilled - 10 * gameEngine->stats.nbEaten[ITEM_HEALTH_POTION];
  TCODConsole::root->setDefaultForeground(TCODColor::white);
  TCODConsole::root->printEx(CON_W/2,CON_H/2+5,TCOD_BKGND_NONE,TCOD_CENTER,
          "Score : %5d",score);
}
*/

  if (stats) {
    TCODConsole::root->setDefaultForeground(TCODColor::red);
    TCODConsole::root->print(
        2,
        CON_H - 7,
        "   creatures killed :%4d\n"
        "      spells casted :%4d\n"
        "           fireball :%4d\n"
        "         fire burst :%4d\n"
        "      incandescence :%4d\n"
        "  distance traveled :%4d",
        gameEngine->stats.nbCreatureKilled,
        gameEngine->stats.nbSpellStandard + gameEngine->stats.nbSpellBurst + gameEngine->stats.nbSpellIncandescence,
        gameEngine->stats.nbSpellStandard,
        gameEngine->stats.nbSpellBurst,
        gameEngine->stats.nbSpellIncandescence,
        gameEngine->stats.nbSteps);
    int thy = CON_H - 7;
    TCODConsole::root->printEx(78, thy++, TCOD_BKGND_NONE, TCOD_RIGHT, "Table hunting");
    for (mob::CreatureTypeId id = (mob::CreatureTypeId)0; id != mob::NB_CREATURE_TYPES;
         id = (mob::CreatureTypeId)(id + 1)) {
      if (gameEngine->stats.creatureDeath[id] > 0) {
        mob::Creature* cr = mob::Creature::getCreature(id);  // yeah this sucks...
        TCODConsole::root->printEx(
            78, thy++, TCOD_BKGND_NONE, TCOD_RIGHT, "%s : %d", cr->name, gameEngine->stats.creatureDeath[id]);
      }
    }
  }
}

bool EndScreen::update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) {
  if (fade == FADE_DOWN) {
    if (fadeLvl <= 0.0f) {
      sound.setVolume(0.0f);
      //           	sound.unload();
      return false;
    } else
      sound.setVolume(fadeLvl);
  }
  noiseZ += elapsed * 0.1f;
  if ((k.vk != TCODK_NONE && k.vk != TCODK_ALT) || mouse.lbutton_pressed || mouse.rbutton_pressed) {
    fade = FADE_DOWN;
  }
  return true;
}

void EndScreen::onFontChange() {
  int charw, charh;
  TCODSystem::getCharSize(&charw, &charh);
  if (charw == charh) {
    // secondary font mappings
    TCODConsole::mapAsciiCodesToFont(128, 28, 0, 5);  // 128 - 155 : A-Z,. upper part
    TCODConsole::mapAsciiCodesToFont(233, 28, 0, 6);  // 233 - 260 : A-Z,. lower part
    TCODConsole::mapAsciiCodesToFont(233 + 28, 28, 0, 7);  // 261 - 288 : a-z-' upper part
    TCODConsole::mapAsciiCodesToFont(233 + 28 * 2, 28, 0, 8);  // 289 - 317 : a-z-' lower part
  }
}

// render text with secondary font (non square font on square console)
void EndScreen::renderText(int x, int y, int w, const char* txt) {
  int curx = x;
  while (*txt && y < CON_H) {
    char c = *txt;
    int ascii = c, ascii2 = 0;
    if (c == '\n') {
      curx = x;
      y += 2;
    } else {
      if (c >= 'A' && c <= 'Z') {
        ascii = 128 + (*txt - 'A');
        ascii2 = ascii + 233 - 128;
      } else if (c >= 'a' && c <= 'z') {
        ascii = 233 + 28 + (*txt - 'a');
        ascii2 = ascii + 28;
      } else if (c == ',') {
        ascii = 128 + 26;
        ascii2 = ascii + 233 - 128;
      } else if (c == '.') {
        ascii = 128 + 27;
        ascii2 = ascii + 233 - 128;
      } else if (c == '-') {
        ascii = 233 + 28 + 26;
        ascii2 = ascii + 28;
      } else if (c == '\'') {
        ascii = 233 + 28 + 27;
        ascii2 = ascii + 28;
      }
      if (y >= 0) TCODConsole::root->putChar(curx, y, ascii);
      if (ascii2 && y + 1 >= 0 && y + 1 < CON_H) TCODConsole::root->putChar(curx, y + 1, ascii2);
      curx++;
      if (curx >= CON_W || curx >= x + w) {
        while (*txt != ' ') {
          curx--;
          if (y >= 0) TCODConsole::root->setChar(curx, y, ' ');
          if (y + 1 >= 0 && y + 1 < CON_H) TCODConsole::root->setChar(curx, y + 1, ' ');
          txt--;
        }
        curx = x;
        y += 2;
      }
    }
    txt++;
  }
}

// ok. things are getting nasty here.
// I'm using the SDL callback to display a picture with a higher resolution than subcell
// this is cheating but even subcell was too big
SDL_Surface* TCOD_sys_get_surface(int width, int height, bool alpha) {
  if (alpha) return SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
  return SDL_CreateRGBSurfaceWithFormat(0, width, height, 24, SDL_PIXELFORMAT_RGB24);
}

PaperScreen::PaperScreen(const char* txgfile, const char* titlegen, const char* textgen, int chapter)
    : EndScreen("", 0.0f, false), txgfile(txgfile), titlegen(titlegen), textgen(textgen), chapter(chapter) {
  title = NULL;
  pixx = pixy = 0;
  scrolltimer = 0.0f;
}

TCODImage* PaperScreen::paper = NULL;
int PaperScreen::paperHeight = 0;
void PaperScreen::onInitialise() {
  tcodpix = loadChapterPicture(false);
  TCODRandom tmpRng(saveGame.seed);
  util::TextGenerator txtgen(txgfile, &tmpRng);
  txtgen.setLocalFunction("RANDOM_INT", new util::RandomIntFunc(&tmpRng));
  txtgen.setLocalFunction("RANDOM_NAME", new util::RandomNameFunc(&tmpRng));
  title = (const char*)strdup(txtgen.generate(titlegen, "${OUTPUT}"));
  txt = (const char*)strdup(txtgen.generate(textgen, "${OUTPUT}"));
  if (!paper) {
    paper = new TCODImage("data/img/paper.png");
    int w;
    paper->getSize(&w, &paperHeight);
  }
  onFontChange();
}

void PaperScreen::onFontChange() {
  EndScreen::onFontChange();
  tcodpix->getSize(&pixw, &pixh);
  SDL_Surface* surf = TCOD_sys_get_surface(pixw, pixh, false);

  int charw, charh;
  float ratio = (float)(pixh) / pixw;
  TCODSystem::getCharSize(&charw, &charh);
  int ridx = surf->format->Rshift / 8;
  int gidx = surf->format->Gshift / 8;
  int bidx = surf->format->Bshift / 8;
  for (int y = 0; y < pixh; y++) {
    for (int x = 0; x < pixw; x++) {
      TCODColor col = tcodpix->getPixel(x, y);
      uint8_t* p = (uint8_t*)surf->pixels + x * surf->format->BytesPerPixel + y * surf->pitch;
      p[ridx] = col.r;
      p[gidx] = col.g;
      p[bidx] = col.b;
    }
  }
  pixw = CON_W * charw / 2;
  pixh = (int)(CON_W * charw / 2 * ratio);
  SDL_Surface* surf2 = TCOD_sys_get_surface(pixw, pixh, false);
  SDL_SoftStretch(surf, NULL, surf2, NULL);
  SDL_FreeSurface(surf);
  if (pix) SDL_FreeSurface(pix);
  pix = surf2;
  int offx = 0, offy = 0;

  if (TCODConsole::isFullscreen()) TCODSystem::getFullscreenOffsets(&offx, &offy);
  pixx = offx + CON_W * charw / 4 - 18;
  pixy = offy + charh * 13;

  overlaph = offseth = 0;
  int texth = TCODConsole::root->getHeightRect(0, 0, 50, 0, txt) * 2;
  overlaph = texth + (15 + pixh / charh) - CON_H;
  overlaph = MAX(0, overlaph);
  overlaph = MAX(overlaph, paperHeight / 2 - CON_H);
}

void PaperScreen::render() {
  int charw, charh;
  TCODSystem::getCharSize(&charw, &charh);
  paper->blit2x(TCODConsole::root, 0, 0, 0, offseth * 2, CON_W * 2, CON_H * 2);
  TCODConsole::root->setDefaultForeground(TCODColor::darkestRed);
  if (charw != charh) {
    if (10 - offseth >= 0) TCODConsole::root->printEx(CON_W / 2, 10, TCOD_BKGND_NONE, TCOD_CENTER, title);
    TCODConsole::root->printRect(13, 15 + pixh / charh - offseth, 50, 0, txt);
  } else {
    // wall of text looks ugly with square font.
    // use secondary, non square font
    if (10 - offseth >= -1) renderText(CON_W / 3, 10 - offseth, CON_W, title);
    renderText(13, 15 + pixh / charh - offseth, 50, txt);
  }
  if (overlaph > 0) {
    float coef = TCODSystem::getElapsedSeconds() * 3;
    coef = coef - (int)coef;
    TCODColor col = TCODColor::lerp(TCODColor(227, 240, 180), TCODColor::darkestRed, coef);
    TCODConsole::root->setDefaultForeground(col);
    if (offseth < overlaph) TCODConsole::root->putChar(CON_W - 2, CON_H - 1, TCOD_CHAR_ARROW_S);
    if (offseth > 0) TCODConsole::root->putChar(CON_W - 2, 1, TCOD_CHAR_ARROW_N);
  }
}

void PaperScreen::render(void* sdlSurface) {
  int charw, charh;
  TCODSystem::getCharSize(&charw, &charh);
  SDL_Rect dst = {pixx, pixy + charh / 2 - offseth * charh, 0, 0};
  SDL_SetSurfaceBlendMode(pix, SDL_BLENDMODE_BLEND);
  SDL_SetSurfaceAlphaMod(pix, TCODConsole::getFade());
  SDL_BlitSurface(pix, NULL, static_cast<SDL_Surface*>(sdlSurface), &dst);
}

#define SCROLL_DELAY 0.020f
bool PaperScreen::update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) {
  scrolltimer += elapsed;
  if (overlaph > 0) {
    if (TCODConsole::isKeyPressed(TCODK_UP)) {
      if (scrolltimer > SCROLL_DELAY && offseth > 0) {
        offseth--;
        scrolltimer = 0.0f;
        TCODConsole::root->setDirty(0, 0, CON_W, CON_H);
      }
    } else if (TCODConsole::isKeyPressed(TCODK_DOWN)) {
      if (scrolltimer > SCROLL_DELAY && offseth < overlaph) {
        offseth++;
        scrolltimer = 0.0f;
        TCODConsole::root->setDirty(0, 0, CON_W, CON_H);
      }
    }
  }
  if (k.vk == TCODK_UP || k.vk == TCODK_DOWN) k.vk = TCODK_NONE;
  return EndScreen::update(elapsed, k, mouse);
}

void PaperScreen::onActivate() {
  saveGame.chapter = chapter;
  TCODSystem::registerSDLRenderer(this);
}

void PaperScreen::onDeactivate() {
  TCODSystem::registerSDLRenderer(NULL);
  TCODConsole::root->setDirty(0, 0, CON_W, CON_H);
}
}  // namespace screen

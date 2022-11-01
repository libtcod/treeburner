/*
 * Copyright (c) 2010 Jice
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
#include "screen/mainmenu.hpp"

#include <math.h>
#include <stdio.h>

#include "constants.hpp"
#include "main.hpp"
#include "util/subcell.hpp"

namespace screen {
MainMenu* MainMenu::instance = NULL;
static const TCODColor PAPER_COLOR(46, 28, 18);
// static const TCODColor TEXT_COLOR(205,165,96);
static const TCODColor TEXT_COLOR(184, 148, 86);
static const TCODColor HIGHLIGHTED_COLOR(241, 221, 171);
static TCODImage* title = NULL;
static TCODImage* rock = NULL;
static TCODImage* rockNormal = NULL;
static const int MENUX = 6;
static const int MENUW = 10;
static const int MENUY = CON_H / 2 + 17;

static const char* menuNames[MENU_NB_ITEMS] = {"New game", "Continue", "Exit"};

MainMenu::MainMenu() : Screen(0), selectedItem(0), elapsed(0.0f), noiseZ(0.0f) {
  instance = this;
  worldGenJobId = -1;
  fadeInLength = (int)(config.getFloatProperty("config.display.fadeTime") * 1000);
  fadeOutLength = fadeInLength / 2;
}

struct WorldGenThreadData {
  TCOD_semaphore_t worldDone;
  uint32_t seed;
};

int generateWorld(void* dat) {
  uint32_t seed = (uint32_t)(uintptr_t)dat;
  if (config.getBoolProperty("config.debug")) {
    printf("World seed : %d\n", seed);
  }
  // DISABLED
  //	SchoolScreen::instance->generateWorld(seed);
  //	TCODSystem::unlockSemaphore(data->worldDone);
  return 0;
}

int generateForest(void* dat) {
  uint32_t seed = (uint32_t)(uintptr_t)dat;
  if (config.getBoolProperty("config.debug")) {
    printf("Forest seed : %d\n", seed);
  }
  //	ForestScreen::instance->generateMap(seed);
  //	TCODSystem::unlockSemaphore(data->worldDone);
  return 0;
}

int loadForest(void* dat) {
  uint32_t seed = (uint32_t)(uintptr_t)dat;
  if (config.getBoolProperty("config.debug")) {
    printf("Forest seed : %d\n", seed);
  }
  //	ForestScreen::instance->loadMap(seed);
  //	TCODSystem::unlockSemaphore(data->worldDone);
  return 0;
}

void MainMenu::onInitialise() {
  Screen::onInitialise();
  img = new TCODImage(CON_W * 2, CON_H * 2);
  title = new TCODImage("data/img/title.png");
  title->getSize(&titlew, &titleh);
  titlex = CON_W - titlew / 2;
  titley = CON_H / 3 + 28;
  fire = new util::Fire(titlew, titleh + 10);
  worldDone = TCODSystem::newSemaphore(0);
  rock = new TCODImage("data/img/rock.png");
  rockNormal = new TCODImage("data/img/rock_n.png");
}

void MainMenu::onActivate() {
  // revert keyboard mode to RELEASED only
  engine.setKeyboardMode(UMBRA_KEYBOARD_RELEASED);
  Screen::onActivate();
  sound.load("data/snd/menu.ogg");
  sound.playLoop();
  sound.setVolume(1.0f);
  elapsed = 0.0f;
  smokeElapsed = 0.0f;
  noiseZ += 0.1f;
  static WorldGenThreadData data;
  data.worldDone = worldDone;
  data.seed = saveGame.seed;
  forestGenJobId = threadPool->addJob(newGame ? generateForest : loadForest, (void*)data.seed);
  worldGenJobId = threadPool->addJob(generateWorld, &data);
  menu.push(MENU_NEW_GAME);
  if (!newGame) {
    menu.push(MENU_CONTINUE);
    selectedItem = 1;
  }
  menu.push(MENU_EXIT);
  /*
  SchoolScreen sc;
  sc.generateWorld(0xdeadbeee);
  std::abort();
  */
}

void MainMenu::render() {
  // smocking background
  if (elapsed > 0.0f) {
    float elcoef = 0.4f, titleCoef = 0.0f, chapterPixCoef = 0.0f, fireCoef = 0.0f;
    if (elapsed < 20.0f) {
      elcoef = (elapsed - 10.0f) * 0.04f;
    }
    if (elapsed >= 0.0f && elapsed < 12.0f) {
      fireCoef = (elapsed - 0.0f) / 12.0f;
    } else if (elapsed >= 12.0f)
      fireCoef = 1.0f;
    if (elapsed >= 12.0f && elapsed < 20.0f) {
      titleCoef = (elapsed - 12.0f) / 8.0f;
    } else if (elapsed >= 20.0f)
      titleCoef = 1.0f;
    if (userPref.nbLaunches > 1 && !newGame) {
      if (elapsed >= 18.0f && elapsed < 28.0f) {
        chapterPixCoef = 0.7f * (elapsed - 18.0f) / 10.0f;
      } else if (elapsed >= 28.0f)
        chapterPixCoef = 0.7f;
    }

    for (int x = 0; x < titlew; x++) {
      for (int y = 0; y < titleh; y++) {
        TCODColor c = title->getPixel(x, y);
        int v = (int)(fireCoef * (c.r - 32 - (1.0f - fireCoef) * 32) / 16);
        fire->softspark(x, y + 10, v);
      }
    }
    fire->generateImage();
    float f = elapsed * 8.0f;
    float lightCoef = noise1d.getFbm(&f, 3.0f);
    f = elapsed * 1.5f;
    float torchposx = noise1d.getFbm(&f, 3.0f);
    f += 50.0f;
    float torchposy = noise1d.getFbm(&f, 3.0f);
    int lightx = (int)(titlex + torchposx * 30 + titleCoef * 20);
    int lighty = (int)(titley + titleh / 2 + torchposy * 30);
    lightCoef = (titleCoef + fireCoef) * 0.25f + 0.15f * lightCoef;
    for (int x = 0; x < CON_W * 2; x++) {
      for (int y = 0; y < CON_H * 2; y++) {
        TCODColor smokeCol;
        if (x >= titlex && x < titlex + titlew) {
          if (y >= titley && y < titley + titleh) {
            smokeCol = smokeCol + titleCoef * title->getPixel(x - titlex, y - titley);
          }
          if (y >= titley - 10 && y < titley + titleh) {
            smokeCol = smokeCol + fire->img->getPixel(x - titlex, y - titley + 10);
          }
        }
        TCODColor rockN = rockNormal->getPixel(x, y);
        TCODColor rockColor = rock->getPixel(x, y);
        float lightDir[3] = {
            static_cast<float>(x - lightx), static_cast<float>(y - lighty), 20.0f + 8.0f * (torchposx + torchposy)};
        float l = lightDir[0] * lightDir[0] + lightDir[1] * lightDir[1] + lightDir[2] * lightDir[2];
        l = base::Entity::fastInvSqrt(l);
        lightDir[0] *= l;
        lightDir[1] *= l;
        lightDir[2] *= l;

        rockColor = rockColor * (TCODColor(10, 10, 50) * fireCoef) +
                    rockColor * TCODColor(184, 148, 86) *
                        ((lightDir[0] * rockN.r + lightDir[1] * rockN.g + lightDir[2] * rockN.b) * lightCoef / 255);
        smokeCol = smokeCol + rockColor;
        img->putPixel(x, y, smokeCol);
      }
    }
    img->blit2x(TCODConsole::root, 0, 0);
  } else {
    TCODConsole::root->setDefaultBackground(TCODColor::black);
    TCODConsole::root->clear();
  }

  // menu
  if (elapsed > 2.0f) {
    float elcoef = 1.0f;
    if (elapsed < 10.0f) elcoef = (elapsed - 2.0f) / 8.0f;

    util::darken(MENUX - 5, MENUY + selectedItem * 2, MENUW * 2, 1, 1.0f - elcoef * 0.5f);
    TCODConsole::root->setDefaultForeground(TEXT_COLOR * elcoef);
    int itemNum = 0;
    for (MenuItemId* it = menu.begin(); it != menu.end(); it++, itemNum++) {
      if (selectedItem == itemNum) {
        TCODConsole::root->setDefaultForeground(HIGHLIGHTED_COLOR * elcoef);
      } else {
        TCODConsole::root->setDefaultForeground(TEXT_COLOR * elcoef);
      }
      TCODConsole::root->print(MENUX, MENUY + 2 * itemNum, menuNames[*it]);
    }
  }

  // credits
  static struct Credit {
    const char* title;
    float startTime;
    float endTime;
  } credits[] = {
      {"Jice presents", 0.0f, 6.0f},
      {"a libtcod/umbra powered game", 6.0f, 12.0f},
      {"Music by Mingos", 12.0f, 18.0f},
      {"Words by Tim Pruett", 18.0f, 24.0f},
      {NULL, 24.0f, 1E10f}};
  Credit* cred = credits;
  while (elapsed >= cred->endTime) cred++;
  if (cred->title) {
    float elcoef = sinf(3.14159f * (elapsed - cred->startTime) / (cred->endTime - cred->startTime));
    TCODConsole::root->setDefaultForeground(HIGHLIGHTED_COLOR * elcoef);
    TCODConsole::root->printEx(CON_W / 2, CON_H / 3, TCOD_BKGND_NONE, TCOD_CENTER, cred->title);
  }
  if (elapsed > 18.0f && userPref.nbLaunches > 5) {
    /*
    static uint8_t cdata[] = {
            0xba, 0x79, 0xa9, 0xfd, 0xcb, 0x79,
            0xd8, 0x09, 0x19, 0x69, 0xc9, 0x39,
            0xa9, 0xc8, 0xfd, 0x09, 0x99, 0xfd,
            0xbb, 0x09, 0xd8, 0x68, 0xa9, 0x19,
            0xfd, 0x2d, 0xfd, 0xdb, 0x09, 0x09,
            0x49, 0xfd, 0x6b, 0x00
    };
    */
    static uint8_t cdata[] = {0xfd, 0x79, 0xfd, 0xfd, 0xfd, 0x79, 0xd8, 0xfd, 0xfd, 0xfd, 0xfd, 0x39,
                              0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xd8, 0xfd, 0xa9, 0xfd,
                              0xfd, 0x2d, 0xfd, 0xdb, 0xfd, 0x09, 0x49, 0xfd, 0x6b, 0x00};
    static float coefs[sizeof(cdata)];
    static TCODRandom chaprnd(TCOD_RNG_CMWC, 0xdeadbeef);
    static bool first = true;
    float chapcoef = (elapsed - 18.0f) / 10.0f;
    chapcoef = MIN(1.0f, chapcoef);
    unsigned char* curc = cdata;
    int x = CON_W / 2 - sizeof(cdata) / 2;
    if (first) {
      int count = userPref.nbLaunches - 5;
      // float gcoef=MIN(1.0f,count/180.0f + 0.06f);
      float gcoef = MIN(0.05f, count / 20.0f + 0.06f);
      int nbcoef = 0;
      while (*curc) {
        if (*curc != 223) nbcoef++;
        curc++;
      }
      curc = cdata;
      if (count < nbcoef)
        count >>= 1;
      else
        count = nbcoef;
      memset(coefs, 0, sizeof(float) * sizeof(cdata));
      do {
        int num = chaprnd.getInt(0, sizeof(cdata) - 1);
        while (cdata[num] == 223 || coefs[num] > 0.0f) {
          num++;
          if (cdata[num] == 0) num = 0;
        }
        coefs[num] = gcoef;
        count--;
      } while (count > 0);
      first = false;
    }
    float* ccoef = coefs;
    TCODColor chapcol = TEXT_COLOR * chapcoef;
    while (*curc) {
      if (*curc != 223 && *ccoef > 0.0f) {
        unsigned char c = *curc;
        TCODConsole::root->setChar(x, 9, 0xFF & (~((c >> 4) | ((c << 4) & 0xF0))));
        TCODColor col = TCODColor::lerp(TCODConsole::root->getCharBackground(x, 9), chapcol, *ccoef);
        TCODConsole::root->setCharForeground(x, 9, col);
      }
      curc++;
      ccoef++;
      x++;
    }
  }

  TCODConsole::root->setDefaultForeground(TEXT_COLOR);
  TCODConsole::root->printEx(CON_W - 2, CON_H - 2, TCOD_BKGND_NONE, TCOD_RIGHT, "v" VERSION);

  if (userPref.nbLaunches == 1) {
    TCODConsole::root->printEx(
        CON_W / 2, CON_H - 2, TCOD_BKGND_NONE, TCOD_CENTER, "PageUp/PageDown to change font size");
  }

  sound.endFrame();
}

bool MainMenu::update(float el, TCOD_key_t k, TCOD_mouse_t mouse) {
  // update fmod
  elapsed += el;
  fire->update(el);
  sound.update();
  if (fade == FADE_DOWN) {
    MenuItemId id = menu.get(selectedItem);
    if (fadeLvl <= 0.0f) {
      switch (id) {
        case MENU_NEW_GAME: {
          engine.deactivateAll();
          // new game
          if (!newGame) {
            // cancel the background generation for the saved game
            if (threadPool->isMultiThreadEnabled()) {
              // TODO : being able to cancel the jobs
              waitForForestGen();
              waitForWorldGen();
            }
            saveGame.init();
            newGame = true;
            delete rng;
            if (config.getBoolProperty("config.debug")) {
              printf("New random seed : %d\n", saveGame.seed);
            }
            rng = new TCODRandom(saveGame.seed, TCOD_RNG_CMWC);
            static WorldGenThreadData data;
            data.worldDone = worldDone;
            data.seed = saveGame.seed;
            forestGenJobId = threadPool->addJob(generateForest, (void*)data.seed);
            worldGenJobId = threadPool->addJob(generateWorld, &data);
          }
          engine.activateModule("chapter1Story");
          return false;
        } break;
        case MENU_CONTINUE: {
          // continue
          engine.activateModule("chapter1");
          sound.setVolume(0.0f);
          return false;
        } break;
        case MENU_EXIT: {
          // exit game
          engine.deactivateAll();
          return false;
        } break;
        default:
          break;
      }
    } else {
      // fade music
      if (id != MENU_NEW_GAME) sound.setVolume(fadeLvl);
    }
  }
  if (fade != FADE_DOWN) {
    if ((mouse.dx != 0 || mouse.dy != 0) && mouse.cx >= MENUX - 5 && mouse.cx <= MENUX - 5 + MENUW) {
      int item = mouse.cy - MENUY;
      if (item >= 0 && (item & 1) == 0 && item / 2 < menu.size()) selectedItem = item / 2;
    }
    bool up = false, down = false, left = false, right = false;
    k.pressed = true;
    mob::Player::getMoveKey(k, &up, &down, &left, &right);
    if (down || right)
      selectedItem = (selectedItem + 1) % menu.size();
    else if (up || left)
      selectedItem = (menu.size() + selectedItem - 1) % menu.size();
  }
  if (((mouse.lbutton_pressed && (mouse.cy == MENUY || mouse.cy == MENUY + 2 || mouse.cy == MENUY + 4) &&
        mouse.cx >= MENUX - 5 && mouse.cx <= MENUX - 5 + MENUW) ||
       k.vk == TCODK_ENTER || k.vk == TCODK_KPENTER) &&
      selectedItem != -1) {
    fade = FADE_DOWN;
  }
  return true;
}

void MainMenu::waitForWorldGen() {
  // TCODSystem::lockSemaphore(worldDone);
  // threadPool->waitUntilFinished(worldGenJobId);
}
void MainMenu::waitForForestGen() {
  // TCODSystem::lockSemaphore(worldDone);
  // threadPool->waitUntilFinished(forestGenJobId);
}
}  // namespace screen

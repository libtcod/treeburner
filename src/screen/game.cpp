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
#include "screen/game.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "base/aidirector.hpp"
#include "main.hpp"
#include "util/powerup.hpp"

namespace screen {
Game::Game() : level(0), helpOn(false) {}

void Game::onInitialise() {
  util::PowerupGraph::instance->setFontSize(8 + engine.getFontID() * 2);
  lightMap.fogRange = 15.0f;
}

void Game::onActivate() {
  static int playerLightRange = 2 * config.getIntProperty("config.display.playerLightRange");
  static TCODColor playerLightColor = config.getColorProperty("config.display.playerLightColor");
  // set keyboard mode to RELEASED + PRESSED
  init();  // init game engine
  base::GameEngine::onActivate();
  initLevel();
  boss = NULL;
  finalExplosion = 2.0f;
  stats = {};
  item::Item* wand = item::Item::getRandomWeapon("wand", item::ITEM_CLASS_STANDARD);
  wand->name_ = strdup("pyromancer wand");
  player.addToInventory(wand);
  player.equip(wand);
  player.setLightRange(playerLightRange);
  player.setLightColor(playerLightColor);
}

void Game::render() {
  static int nbLevels = config.getIntProperty("config.gameplay.nbLevels");
  static bool debug = config.getBoolProperty("config.debug");
  static TCODColor memoryWallColor = config.getColorProperty("config.display.memoryWallColor");
  static int bossLife = config.getIntProperty("config.creatures.boss.life");

  TCODConsole::root->setDefaultBackground(TCODColor::black);
  TCODConsole::root->clear();

  // render the memory map
  ground.clear(TCODColor::black);
  for (int x = 0; x < CON_W; x++) {
    for (int y = 0; y < CON_H; y++) {
      int dungeonx = x + xOffset;
      int dungeony = y + yOffset;
      if (IN_RECTANGLE(dungeonx, dungeony, dungeon->width, dungeon->height)) {
        if (dungeon->getMemory(dungeonx, dungeony)) {
          int dungeonx2x = dungeonx * 2;
          int dungeony2x = dungeony * 2;
          if (!dungeon->map2x->isTransparent(dungeonx2x, dungeony2x)) ground.putPixel(x * 2, y * 2, memoryWallColor);
          if (!dungeon->map2x->isTransparent(dungeonx2x + 1, dungeony2x))
            ground.putPixel(x * 2 + 1, y * 2, memoryWallColor);
          if (!dungeon->map2x->isTransparent(dungeonx2x, dungeony2x + 1))
            ground.putPixel(x * 2, y * 2 + 1, memoryWallColor);
          if (!dungeon->map2x->isTransparent(dungeonx2x + 1, dungeony2x + 1))
            ground.putPixel(x * 2 + 1, y * 2 + 1, memoryWallColor);
        }
      }
    }
  }

  // variables to store the part of the dungeon than needs
  // subcell rendering from lightmap (part which gets light)
  int minx2x, maxx2x, miny2x, maxy2x;

  // render the lights on 2x lightmap
  dungeon->renderLightsToLightMap(lightMap, &minx2x, &miny2x, &maxx2x, &maxy2x, true);

  // render the fireballs
  for (spell::FireBall** it = fireballs.begin(); it != fireballs.end(); it++) {
    (*it)->render(lightMap);
  }

  // final explosion
  if (bossIsDead && finalExplosion > 0.0f && finalExplosion <= 1.0f) {
    float radius = 2 * CON_W * (1.0f - finalExplosion);
    float minRadius = MAX(0, MIN(radius - 3, 0.6f * radius));
    float medRadius, radiusdiv = 1.0f;
    int minx = dungeon->stairx * 2 - (int)radius;
    int miny = dungeon->stairy * 2 - (int)radius;
    int maxx = dungeon->stairx * 2 + (int)radius;
    int maxy = dungeon->stairy * 2 + (int)radius;
    int xOffset2 = xOffset * 2;
    int yOffset2 = yOffset * 2;
    int conExploX = dungeon->stairx * 2 - xOffset2;
    int conExploY = dungeon->stairy * 2 - yOffset2;
    minx = MAX(0, minx);
    miny = MAX(0, miny);
    maxx = MIN(dungeon->width * 2 - 1, maxx);
    maxy = MIN(dungeon->height * 2 - 1, maxy);
    radius = radius * radius;
    minRadius = minRadius * minRadius;
    medRadius = (radius + minRadius) * 0.5f;
    if (radius - minRadius > 1E-5) radiusdiv = 2.0f / (radius - minRadius);
    minx -= xOffset2;
    miny -= yOffset2;
    maxx -= xOffset2;
    maxy -= yOffset2;
    minx = MAX(0, minx);
    miny = MAX(0, miny);
    maxx = MIN(CON_W * 2 - 1, maxx);
    maxy = MIN(CON_H * 2 - 1, maxy);
    minx2x = MIN(minx, minx2x);
    miny2x = MIN(miny, miny2x);
    maxx2x = MAX(maxx, maxx2x);
    maxy2x = MAX(maxy, maxy2x);
    for (int x = minx; x <= maxx; x++) {
      int dx2 = (conExploX - x) * (conExploX - x);
      for (int y = miny; y <= maxy; y++) {
        if (dungeon->map2x->isInFov(x + xOffset2, y + yOffset2) &&
            dungeon->map->isWalkable(x / 2 + xOffset, y / 2 + yOffset)) {
          int dy = conExploY - y;
          float r = dx2 + dy * dy;
          if (r <= radius && r > minRadius) {
            float midr = (r - medRadius) * radiusdiv;
            float rcoef = 1.0f - ABS(midr);
            float f[2] = {(float)(3 * x) / CON_W, (float)(3 * y) / CON_H};
            float ncoef = 0.5f * (1.0f + noise2d.getFbm(f, 3.0f));
            //						ground.putPixel(x,y,TCODColor::lerp(TCODColor::yellow,TCODColor::red,coef));
            TCODColor col = lightMap.getColor2x(x, y);
            col = TCODColor::lerp(col, TCODColor::lerp(TCODColor::darkRed, TCODColor::yellow, ncoef), rcoef * ncoef);
            lightMap.setColor2x(x, y, col);
          }
        }
      }
    }
  }

  // convert it to lightmap (consolex2) coordinates
  minx2x -= 2 * xOffset;
  maxx2x -= 2 * xOffset;
  miny2x -= 2 * yOffset;
  maxy2x -= 2 * yOffset;
  minx2x = MAX(0, minx2x);
  maxx2x = MIN(CON_W * 2 - 1, maxx2x);
  miny2x = MAX(0, miny2x);
  maxy2x = MIN(CON_H * 2 - 1, maxy2x);

  // shade the 2xground
  lightMap.applyToImage(ground, minx2x, miny2x, maxx2x, maxy2x);

  // render boss health bar
  if (bossSeen && !bossIsDead) {
    float lifeper = (float)(boss->life) / bossLife;
    for (int x = 120; x < 140; x++) {
      TCODColor col = (x - 120) < (int)(lifeper * 20) ? TCODColor::red : TCODColor::darkerRed;
      ground.putPixel(x, 5, col);
      ground.putPixel(x, 4, col);
    }
  }

  // blit it on console
  ground.blit2x(TCODConsole::root, 0, 0);

  // render the corpses
  dungeon->renderCorpses(lightMap);
  // render the items
  dungeon->renderItems(lightMap);
  // render the creatures
  dungeon->renderCreatures(lightMap);
  // render the player
  player.render(lightMap);

  // render the stair
  int stairx = dungeon->stairx - xOffset;
  int stairy = dungeon->stairy - yOffset;
  if (IN_RECTANGLE(stairx, stairy, CON_W, CON_H) && dungeon->getMemory(dungeon->stairx, dungeon->stairy)) {
    if (level < nbLevels - 1) {
      TCODConsole::root->setChar(stairx, stairy, '<');
      TCODConsole::root->setCharForeground(stairx, stairy, TCODColor::white);
    } else if (bossIsDead && finalExplosion == 2.0f) {
      TCODConsole::root->setChar(stairx, stairy, '(');
      TCODConsole::root->setCharForeground(stairx, stairy, TCODColor::lightYellow);
    }
  }

  // render level
  if (fade_ == FADE_UP) {
    TCODColor lvlCol = TCODColor::white * (1.0f - fade_level_);
    TCODConsole::root->setDefaultForeground(lvlCol);
    TCODConsole::root->printEx(CON_W / 2, CON_H - 5, TCOD_BKGND_NONE, TCOD_CENTER, "Level %d", level + 1);
  }

  // render items under the mouse cursor
  if (!isGamePaused()) {
    int dungeonx = mousex + xOffset;
    int dungeony = mousey + yOffset;
    item::Item* mouseItem = dungeon->getFirstItem(dungeonx, dungeony);
    if (mouseItem) {
      mouseItem->renderDescription(mousex, mousey);
    } else if (dungeonx == dungeon->stairx && dungeony == dungeon->stairy) {
      int my = mousey + 1;
      if (my == CON_H) my = CON_H - 2;
      if (level < nbLevels - 1) {
        TCODConsole::root->setDefaultForeground(TCODColor::white);
        TCODConsole::root->printEx(mousex, my, TCOD_BKGND_NONE, TCOD_CENTER, "Stairs to next level");
      } else if (bossIsDead && finalExplosion == 2.0f) {
        TCODConsole::root->setDefaultForeground(TCODColor::lightYellow);
        TCODConsole::root->printEx(mousex, my, TCOD_BKGND_NONE, TCOD_CENTER, "Amulet of Zeepoh");
      }
    }
  }
  // render messages
  // gui.log.render();

  // various UI stuff
  if (bossSeen && !bossIsDead) {
    TCODConsole::root->print(53, 1, "Zeepoh");
  }
  if (!isGamePaused() || helpOn)
    TCODConsole::root->printEx(CON_W / 2, 2, TCOD_BKGND_NONE, TCOD_CENTER, helpOn ? "? to resume game" : "? for help");

  if (isGamePaused()) {
    TCODConsole::root->printEx(CON_W / 2, CON_H / 2 + 10, TCOD_BKGND_NONE, TCOD_CENTER, "-= pause =-");
  }
  if (debug) {
    TCODConsole::root->print(2, CON_H - 4, "%d", dungeon->creatures.size());
  }
  /*
  if ( isGamePaused() && pauseScreen ) {
          util::blitTransparent(pauseScreen,0,0,CON_W-20,CON_H-20,TCODConsole::root,10,5);
  }
  */
}

bool Game::update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) {
  static int nbLevels = config.getIntProperty("config.gameplay.nbLevels");
  static bool debug = config.getBoolProperty("config.debug");
  static base::AiDirector aiDirector;
  static float finalExplosionTime = config.getFloatProperty("config.display.finalExplosionTime");

  mousex = mouse.cx;
  mousey = mouse.cy;

  GameEngine::update(elapsed, k, mouse);

  if (bossIsDead && finalExplosion <= 1.0f) finalExplosion -= elapsed / finalExplosionTime;

  // update items
  dungeon->updateItems(elapsed, k, &mouse);

  if (helpOn && (k.c == '?' || k.c == ' ') && !k.pressed) {
    helpOn = false;
    resumeGame();
    return true;
  }
  if (k.c == ' ' && !k.pressed && gui.mode == ui::GUI_NONE) {
    if (isGamePaused())
      resumeGame();
    else
      pauseGame();
  }

  // update player
  player.update(elapsed, k, &mouse);
  if (isGamePaused()) return true;

  xOffset = (int)(player.x - CON_W / 2);
  yOffset = (int)(player.y - CON_H / 2);

  if (player.life <= 0 && fade_ != FADE_DOWN) {
    setFadeOut(fade_out_length_ms_, TCODColor::darkRed);
    fade_ = FADE_DOWN;
  }

  // update fading
  if (fade_ == FADE_DOWN && fade_level_ <= 0.0f) {
    if (player.life <= 0) {
      // death
      engine.activateModule("pyroGameOver");
      return false;
    }
    if (level < nbLevels - 1) {
      // go to next level
      fade_ = FADE_UP;
      termLevel();
      level++;
      initLevel();
    } else {
      // victory
      engine.activateModule("pyroVictory");
      return false;
    }
  } else if (fade_ == FADE_OFF) {
    if (finalExplosion <= 0.0f) {
      setFadeOut(fade_out_length_ms_, TCODColor::white);
      fade_ = FADE_DOWN;
      fade_level_ = 1.0f;
    }
  }

  // level ending condition
  if (player.x == dungeon->stairx && player.y == dungeon->stairy && fade_ == FADE_OFF) {
    if (level == nbLevels - 1) {
      if (bossIsDead) {
        if (finalExplosion == 2.0f) {
          // triggers final explosion
          finalExplosion = 1.0f;
        }
      }
    } else {
      fade_ = FADE_DOWN;
      fade_level_ = 1.0f;
    }
  }

  // calculate player fov
  dungeon->computeFov((int)player.x, (int)player.y);

  // update monsters
  if (fade_ != FADE_DOWN) {
    if (bossIsDead && finalExplosion > 0.0f && finalExplosion <= 1.0f) {
      int radius = (int)(2 * CON_H * (1.0f - finalExplosion)) - 10;
      if (radius > 0) dungeon->killCreaturesAtRange(radius);
    }

    aiDirector.update(elapsed);
    dungeon->updateCreatures(elapsed);
  }

  // non player related keyboard handling
  if (debug) {
    // debug/cheat shortcuts
    if (k.c == 'd' && k.lalt && !k.pressed) {
      // debug mode : Alt-d = player takes 'd'amages
      player.takeDamage(20);
    }
    if (k.c == 'b' && k.lalt && !k.pressed) {
      // debug mode : Alt-b = burn
      for (mob::Creature** cr = dungeon->creatures.begin(); cr != dungeon->creatures.end(); cr++) {
        (*cr)->burn = true;
      }
    }
    if (k.c == 'i' && k.lalt && !k.pressed) {
      // debug mode : Alt-i = item
      dungeon->addItem(item::Item::getItem("knife", player.x, player.y - 1));
    }
    if (k.c == 'm' && k.lalt && !k.pressed) {
      // debug mode : Alt-m : max spells
      TCODList<util::Powerup*> list;
      bool again = true;
      do {
        util::Powerup::getAvailable(&list);
        again = false;
        while (!list.isEmpty()) {
          util::Powerup* sel = list.pop();
          sel->apply();
          again = true;
        }
      } while (again);
      k.c = 0;
    }
    if (k.c == 'w' && k.lalt && !k.pressed) {
      // debug mode : Alt-w : weapon
      static int i = 0;
      item::Item* w = item::Item::getRandomWeapon("staff", (item::ItemClass)i++);
      w->setPos(player.x, player.y - 1);
      w->name_ = strdup("Pyromancer's staff");
      if (i > item::ITEM_CLASS_GOLD) i = 0;
      dungeon->addItem(w);
    }
    if (k.c == 's' && k.lalt && !k.pressed) {
      // debug mode : Alt-s = go to stairs
      player.setPath(dungeon->stairx, dungeon->stairy, false);
    }
    if (k.c == 'f' && k.lalt && !k.pressed) {
      // debug mode : Alt-f = final explosion
      bossIsDead = true;
      finalExplosion = 1.0f;
      k.c = 0;
    }
    // debug : change level with numpad +/-
    if (k.vk == TCODK_KPSUB && level > 0 && !k.pressed) {
      termLevel();
      level--;
      initLevel();
    } else if (k.vk == TCODK_KPADD && level < nbLevels - 1 && !k.pressed) {
      termLevel();
      level++;
      initLevel();
    }
  }
  if (k.c == '?' && !k.pressed) {
    // help screen
    static TCODConsole help(CON_W - 20, CON_H - 20);
    static bool init = false;
    if (!init) {
      static int moveUpKey = toupper(config.getCharProperty("config.creatures.player.moveUpKey"));
      static int moveDownKey = toupper(config.getCharProperty("config.creatures.player.moveDownKey"));
      static int moveLeftKey = toupper(config.getCharProperty("config.creatures.player.moveLeftKey"));
      static int moveRightKey = toupper(config.getCharProperty("config.creatures.player.moveRightKey"));
      // create an offscreen console for the help screen
      help.setDefaultBackground(TCODColor::darkRed);
      help.rect(0, 0, CON_W - 20, CON_H - 20, false, TCOD_BKGND_SET);
      for (int x = 0; x < CON_W - 20; x++) {
        help.setCharForeground(x, 0, TCODColor::lightRed);
        help.setChar(x, 0, TCOD_CHAR_HLINE);
        help.setCharForeground(x, CON_H - 21, TCODColor::lightRed);
        help.setChar(x, CON_H - 21, TCOD_CHAR_HLINE);
      }
      help.setDefaultForeground(TCODColor::lightOrange);
      int y = 2;
      help.printEx(CON_W / 2 - 10, y++, TCOD_BKGND_NONE, TCOD_CENTER, "Help");
      y++;
      help.printEx(CON_W / 2 - 10, y++, TCOD_BKGND_NONE, TCOD_CENTER, "-= Movement =-");
      y++;
      help.print(12, y++, "%c%c%c%c / arrows              : move", moveUpKey, moveLeftKey, moveDownKey, moveRightKey);
      help.print(12, y++, "LEFT BUTTON / RIGHT BUT    : attack");
      y++;
      help.printEx(CON_W / 2 - 10, y++, TCOD_BKGND_NONE, TCOD_CENTER, "-= Miscellaneous =-");
      y++;
      help.print(12, y++, "SPACE           : pause/resume game");
      help.print(12, y++, "PRINTSCREEN     : take screenshot");
      help.print(12, y++, "PGUP/PGDOWN     : change font size");
      help.print(12, y++, "ALT-ENTER       : fullscreen/windowed");
      init = true;
    }
    helpOn = true;
    // pauseScreen=&help;
    pauseGame();
  }

  // update lightmap (fog)
  lightMap.update(elapsed);

  // update messages
  // gui.log.update(k,mouse,elapsed);

  // update lights
  dungeon->updateLights(elapsed);

  // update fireballs
  updateFireballs(elapsed);

  return true;
}

// protected stuff
void Game::initLevel() {
  static int nbLevels = config.getIntProperty("config.gameplay.nbLevels");
  static TCODColor playerLightColor = config.getColorProperty("config.display.playerLightColor");
  static TCODColor playerLightColorEnd = config.getColorProperty("config.display.playerLightColorEnd");

  player.setLightColor(TCODColor::lerp(playerLightColor, playerLightColorEnd, (float)(level + 1) / nbLevels));

  util::CaveGenerator caveGen(level);
  dungeon = new map::Dungeon(level, &caveGen);
  if (level == nbLevels - 1) {
    // boss
    boss = (mob::Boss*)mob::Creature::getCreature(mob::CREATURE_ZEEPOH);
    int bx = dungeon->stairx;
    int by = dungeon->stairy;
    dungeon->getClosestWalkable(&bx, &by, false);
    boss->setPos(bx, by);
    dungeon->addCreature(boss);
  }
}

void Game::termLevel() {
  player.termLevel();
  delete dungeon;
  fireballs.clearAndDelete();
  base::AiDirector::instance->termLevel();
}
}  // namespace screen

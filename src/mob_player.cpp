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
#include "mob_player.hpp"

#include <assert.h>
#include <ctype.h>

#include "main.hpp"
#include "util_subcell.hpp"

// maximum sprint : 2 times faster
#define MIN_SPRINT_COEF 0.5f

static TCODColor healthColor(127, 127, 255);

Player::Player()
    : healPoints(0),
      lbuttonDelay(0.0f),
      lWalkDelay(0.0f),
      rbuttonDelay(0.0f),
      lbutton(false),
      rbutton(false),
      curHeal(0),
      initDungeon(true) {
  static char playerChar = config.getCharProperty("config.creatures.player.ch");
  static TCODColor playerColor = config.getColorProperty("config.creatures.player.col");
  static float healthIntensityDelay = config.getFloatProperty("config.creatures.player.healIntensityDelay");
  static const char* healthIntensityPattern =
      strdup(config.getStringProperty("config.creatures.player.healIntensityPattern"));
  static float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");

  ch = playerChar;
  col = playerColor;
  healLight.color = healthColor;
  healLight.range = 7;
  healLight.randomRad = false;
  healLight.setup(healthColor, healthIntensityDelay, healthIntensityPattern, NULL);

  light.range = 0;
  fovRange = 0;
  maxLife = 100.0f;
  sprintDelay = sprintLength;
  stealth = 1.0f;
  crouch = false;
  isSprinting = false;
}

Player::~Player() {}

void Player::init() {
  static float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  speed = playerSpeed;
  life = maxLife;
  up = down = left = right = false;
  averageSpeed = speedElapsed = speedDist = 0.0f;
  sprintDelay = sprintLength;
  // cannot do this. Game::lights not created yet...
  // gameEngine->addLight(&light);
}

float Player::getHealth() { return life / maxLife; }

float Player::getHealing() {
  float ret = (life + healPoints) / maxLife;
  return MIN(ret, 1.0f);
}

void Player::termLevel() {
  if (path) delete path;
  path = NULL;
  walkTimer = 0.0f;
  initDungeon = true;
}

void Player::takeDamage(float amount) {
  float oldLife = life;
  Creature::takeDamage(amount);
  if (life < oldLife) gameEngine->hitFlash();
}

void Player::heal(int healPoints) {
  bool heal = this->healPoints > 0;
  this->healPoints += healPoints;
  if (!heal) gameEngine->dungeon->addLight(&healLight);
}

bool Player::setPath(int xDest, int yDest, bool limitPath) {
  static int maxPathFinding = config.getIntProperty("config.creatures.player.maxPathFinding");

  Dungeon* dungeon = gameEngine->dungeon;
  if (!IN_RECTANGLE(xDest, yDest, dungeon->width, dungeon->height)) return false;
  // check if right clicked on a wall
  if (!dungeon->map->isWalkable(xDest, yDest)) {
    // walk toward the player and see if no other wall blocks the path
    float dx = x - xDest;
    float dy = y - yDest;
    if (xDest < dungeon->width - 1 && dx > 0 && dungeon->map->isWalkable(xDest + 1, yDest)) {
      xDest++;
    } else if (xDest > 0 && dx < 0 && dungeon->map->isWalkable(xDest - 1, yDest)) {
      xDest--;
    } else if (yDest < dungeon->height - 1 && dy > 0 && dungeon->map->isWalkable(xDest, yDest + 1)) {
      yDest++;
    } else if (yDest > 0 && dy < 0 && dungeon->map->isWalkable(xDest, yDest - 1)) {
      yDest--;
    }
    // to a known cell
    TCODLine::init(xDest, yDest, (int)x, (int)y);
    bool wall = true;
    int wx, wy;
    while (!TCODLine::step(&wx, &wy)) {
      if (dungeon->getMemory(wx, wy) && !wall) break;
      if (dungeon->map->isWalkable(wx, wy)) {
        // found a ground cell
        if (wall) {
          wall = false;
          xDest = wx;
          yDest = wy;
        }
      } else if (!wall)
        return false;  // hit another wall. no path
    }
  }
  if (!path) path = new TCODPath(dungeon->width, dungeon->height, this, NULL);
  ignoreCreatures = false;
  bool ok = path->compute((int)x, (int)y, xDest, yDest);
  if (!ok) {
    ignoreCreatures = true;
    ok = path->compute((int)x, (int)y, xDest, yDest);
  }
  ignoreCreatures = true;
  if (!ok) return false;
  if (limitPath && !dungeon->getMemory(xDest, yDest) && path->size() > maxPathFinding) {
    delete path;
    path = NULL;
    return false;
  }
  return true;
}

void Player::render(LightMap& lightMap) {
  static float longButtonDelay = config.getFloatProperty("config.creatures.player.longButtonDelay");
  static float longSpellDelay = config.getFloatProperty("config.creatures.player.longSpellDelay");
  static float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static bool blink = false;

  Creature::render(lightMap);
  if ((FireBall::incandescence && lbuttonDelay > longButtonDelay) ||
      (FireBall::sparkle && rbuttonDelay > longButtonDelay)) {
    // spell charging progress bar
    int barLength = 0;
    float delay = MAX(rbuttonDelay, lbuttonDelay);
    if (delay >= longSpellDelay) {
      barLength = 3;
    } else
      barLength = 1 + (int)((delay - longButtonDelay) * 1.99 / (longSpellDelay - longButtonDelay));
    blink = !blink;
    if (barLength == 3 && blink) barLength = 0;
    int bary = CON_H / 2 - 1;
    if (gameEngine->mousey <= CON_H / 2) bary = CON_H / 2 + 1;
    for (int i = CON_W / 2 - 1; i < CON_W / 2 + barLength - 1; i++) {
      TCODConsole::root->setChar(i, bary, TCOD_CHAR_PROGRESSBAR);
      TCODConsole::root->setCharForeground(i, bary, TCODColor::lightRed);
    }
  }

  // sprint bar
  if (isSprinting && !hasCondition(CRIPPLED)) {
    if (sprintDelay < sprintLength) {
      float sprintCoef = sprintDelay / sprintLength;
      static TCODImage sprintBar(10, 2);
      for (int x = 0; x < 10; x++) {
        float coef = (x * 0.1f - sprintCoef) * 5;
        coef = CLAMP(0.0f, 1.0f, coef);
        TCODColor col = TCODColor::lerp(TCODColor::blue, TCODColor::white, coef);
        sprintBar.putPixel(x, 0, col);
        sprintBar.putPixel(x, 1, col);
      }
      transpBlit2x(&sprintBar, 0, 0, 10, 2, TCODConsole::root, CON_W / 2 - 2, CON_H / 2 + 2, 0.4f);
    }
  }

  // stealth bar
  if (crouch || stealth < 1.0f) {
    static TCODImage stealthBar(2, 10);
    for (int y = 0; y < 10; y++) {
      float coef = (y * 0.1f - stealth) * 5;
      coef = CLAMP(0.0f, 1.0f, coef);
      TCODColor col = TCODColor::lerp(TCODColor::white, TCODColor::darkViolet, coef);
      stealthBar.putPixel(0, y, col);
      stealthBar.putPixel(1, y, col);
      transpBlit2x(&stealthBar, 0, 0, 2, 10, TCODConsole::root, CON_W / 2 - 3, CON_H / 2 - 3, 0.4f);
    }
  }
}

// handle movement keys
// supported layouts:
// hjklyubn (vi keys)
// arrows
// numpad 12346789
// WSAD / ZSQD (fps keys)
void Player::getMoveKey(TCOD_key_t key, bool* up, bool* down, bool* left, bool* right) {
  static int moveUpKey = toupper(config.getCharProperty("config.creatures.player.moveUpKey"));
  static int moveDownKey = toupper(config.getCharProperty("config.creatures.player.moveDownKey"));
  static int moveLeftKey = toupper(config.getCharProperty("config.creatures.player.moveLeftKey"));
  static int moveRightKey = toupper(config.getCharProperty("config.creatures.player.moveRightKey"));

  int kc = toupper(key.c);
  if (kc == moveUpKey || key.vk == TCODK_UP || kc == 'Z' || kc == 'W' || kc == 'K' || key.vk == TCODK_KP8) {
    *up = key.pressed;
  } else if (kc == moveDownKey || key.vk == TCODK_DOWN || kc == 'S' || kc == 'J' || key.vk == TCODK_KP2) {
    *down = key.pressed;
  } else if (kc == moveLeftKey || key.vk == TCODK_LEFT || kc == 'Q' || kc == 'A' || kc == 'H' || key.vk == TCODK_KP4) {
    *left = key.pressed;
  } else if (kc == moveRightKey || key.vk == TCODK_RIGHT || kc == 'D' || kc == 'L' || key.vk == TCODK_KP6) {
    *right = key.pressed;
  } else if (kc == 'Y' || key.vk == TCODK_KP7) {
    *up = key.pressed;
    *left = key.pressed;
  } else if (kc == 'U' || key.vk == TCODK_KP9) {
    *up = key.pressed;
    *right = key.pressed;
  } else if (kc == 'B' || key.vk == TCODK_KP1) {
    *down = key.pressed;
    *left = key.pressed;
  } else if (kc == 'N' || key.vk == TCODK_KP3) {
    *down = key.pressed;
    *right = key.pressed;
  }
}

void Player::computeStealth(float elapsed) {
  Dungeon* dungeon = gameEngine->dungeon;
  float shadow = dungeon->getShadow(x * 2, y * 2);
  float cloud = dungeon->getCloudCoef(x * 2, y * 2);
  shadow = MIN(shadow, cloud);
  // increase shadow. TODO should be in outdoor only!
  float shadowcoef = crouch ? 4.0f : 2.0f;
  shadow = 1.0f - shadowcoef * (1.0f - shadow);
  stealth -= (stealth - shadow) * elapsed;
  float speedcoef = crouch ? 0.6f : 1.0f;
  stealth += speedcoef * averageSpeed * elapsed * 0.1f;
  stealth = CLAMP(0.0f, 3.0f, stealth);
  // printf ("shadow %g stealth %g\n",shadow,stealth);
}

bool Player::activateCell(int dungeonx, int dungeony, bool lbut_pressed, bool walk, bool* activated) {
  // click on adjacent non walkable item = activate it
  // clink on adjacent pickable item = pick it up
  bool useWeapon = true;
  Dungeon* dungeon = gameEngine->dungeon;
  auto* items = dungeon->getItems(dungeonx, dungeony);
  if (activated) *activated = false;
  if (items->size() > 0) {
    useWeapon = false;
    // if ( lbut_pressed ) {
    std::vector<Item*> toPick;
    std::vector<Item*> toUse;
    for (Item* it : *items) {
      if (!it->isPickable())
        toUse.push_back(it);
      else if (it->speed == 0.0f)
        toPick.push_back(it);
    }
    for (Item* it : toPick) {
      it->putInInventory(this);
    }
    for (Item* it : toUse) {
      bool deleteOnUse = (it->typeData->flags & ITEM_DELETE_ON_USE) != 0;
      it->use();
      if (activated) *activated = true;
    }
    //}
  } else if (dungeon->hasCreature(dungeonx, dungeony)) {
    // click on adjacent catchable creature = catch it
    Creature* crea = dungeon->getCreature(dungeonx, dungeony);
    if (crea && crea->isCatchable()) {
      useWeapon = false;
      if (lbut_pressed) {
        dungeon->removeCreature(crea, false);
        crea->initItem();
        assert(crea->asItem);
        crea->asItem->putInInventory(this, 0, "catch");
      }
    }
  }
  return useWeapon;
}

bool Player::update(float elapsed, TCOD_key_t key, TCOD_mouse_t* mouse) {
  static float longSpellDelay = config.getFloatProperty("config.creatures.player.longSpellDelay");
  // static float playerSpeed=config.getFloatProperty("config.creatures.player.speed");
  static float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static char quickslot1Key = config.getCharProperty("config.creatures.player.quickslot1");
  static float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  static float playerSpeedDiag = playerSpeed / 1.2f;
  /*
  static char quickslot2Key=config.getCharProperty("config.creatures.player.quickslot2");
  static char quickslot3Key=config.getCharProperty("config.creatures.player.quickslot3");
  static char quickslot4Key=config.getCharProperty("config.creatures.player.quickslot4");
  static char quickslot5Key=config.getCharProperty("config.creatures.player.quickslot5");
  static char quickslot6Key=config.getCharProperty("config.creatures.player.quickslot6");
  static char quickslot7Key=config.getCharProperty("config.creatures.player.quickslot7");
  static char quickslot8Key=config.getCharProperty("config.creatures.player.quickslot8");
  static char quickslot9Key=config.getCharProperty("config.creatures.player.quickslot9");
  static char quickslot10Key=config.getCharProperty("config.creatures.player.quickslot10");
  */

  Dungeon* dungeon = gameEngine->dungeon;
  if (initDungeon) {
    initDungeon = false;
    dungeon->addLight(&light);
  }

  if (life <= 0) return false;
  light.setPos(x * 2, y * 2);
  updateConditions(elapsed);

  walkTimer += elapsed;

  // special key status
  bool ctrl = TCODConsole::isKeyPressed(TCODK_CONTROL);
  // if ( key.vk == TCODK_SHIFT ) isSprinting=key.pressed;
  isSprinting = TCODConsole::isKeyPressed(TCODK_SHIFT);

  // user input
  if (gameEngine->isGamePaused()) {
    up = down = left = right = false;
    return true;
  }
  // update items in inventory
  inventory.erase(
      std::remove_if(inventory.begin(), inventory.end(), [&elapsed](Item* it) { return it->age(elapsed); }),
      inventory.end());

  // crouching
  crouch = ctrl;

  // update breath recovery after sprint
  updateSprintDelay(elapsed, isSprinting);
  // compute average speed during last second
  computeAverageSpeed(elapsed);
  // update fov according to breath
  computeFovRange(elapsed);

  Player::getMoveKey(key, &up, &down, &left, &right);

  // mouse coordinates
  int dungeonx = mouse->cx + gameEngine->xOffset;
  int dungeony = mouse->cy + gameEngine->yOffset;

  bool useWeapon = true;
  if (mouse->lbutton_pressed && ABS(dungeonx - x) <= 1 && ABS(dungeony - y) <= 1) {
    // click on the player or near him in water=ripples
    if (mouse->lbutton_pressed && dungeon->hasRipples(dungeonx, dungeony)) gameEngine->startRipple(dungeonx, dungeony);
    if (dungeonx != x || dungeony != y) {
      useWeapon = activateCell(dungeonx, dungeony, mouse->lbutton_pressed, false);
    }
  }
  if (useWeapon) {
    if (mainHand) mainHand->update(elapsed, key, mouse);
    if (offHand) offHand->update(elapsed, key, mouse);
  }

  if (mouse->lbutton) {
    lbuttonDelay += elapsed;
    lWalkDelay += elapsed;
  }
  if (mouse->rbutton) rbuttonDelay += elapsed;

  // right mouse button
  if (mouse->rbutton_pressed) {
    if (!FireBall::sparkle || rbuttonDelay < longSpellDelay) {
      // quick click : standard fireball
      if (dungeonx != x || dungeony != y) {
        FireBall* fb = new FireBall(x, y, dungeonx, dungeony, FB_STANDARD);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellStandard++;
        stealth = MIN(3.0f, stealth + 0.5f);
      }
    }
    if (FireBall::sparkle && rbuttonDelay >= longSpellDelay) {
      // long right click : sparkle
      if (dungeonx != x || dungeony != y) {
        FireBall* fb = new FireBall(x, y, dungeonx, dungeony, FB_BURST);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellBurst++;
        stealth = MIN(3.0f, stealth + 0.5f);
      }
    }
    rbuttonDelay = 0.0f;
  }

  // left mouse button
  if (mouse->lbutton_pressed) {
    FireBallType type = FB_STANDARD;
    bool cast = false;
    const char* subtype = "fireball";
    if (lbuttonDelay < longSpellDelay) {
      // quick left click : cast standard fireball
      cast = true;
      gameEngine->stats.nbSpellStandard++;
      stealth = MIN(3.0f, stealth + 0.5f);
    } else {
      // long click : incandescence
      type = FB_INCANDESCENCE;
      gameEngine->stats.nbSpellIncandescence++;
      stealth = MIN(3.0f, stealth + 0.5f);
      subtype = "fireball2";
      cast = true;
    }

    lWalkDelay = 0.0f;
    lbuttonDelay = 0.0f;
    if (cast) {
      if (dungeonx != x || dungeony != y) {
        FireBall* fb = new FireBall(x, y, dungeonx, dungeony, type, subtype);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellStandard++;
      }
    }
  }

  if (!key.pressed && key.c == quickslot1Key) {
    // cast fireball
    if (dungeonx != x || dungeony != y) {
      FireBallType type = FB_SPARK;
      FireBall* fb = new FireBall(x, y, dungeonx, dungeony, type);
      gameEngine->addFireball(fb);
      gameEngine->stats.nbSpellStandard++;
    } else {
      takeDamage(1);
      gameEngine->gui.log.warn("You get hurt by your own spark spell !");
    }
  }

  // walk
  float maxInvSpeed = 1.0f / speed;
  if (isSprinting && sprintDelay > 0.0f && sprintDelay < sprintLength) {
    float sprintCoef = 1.0f - 4 * (sprintLength - sprintDelay) / sprintLength;
    sprintCoef = MAX(MIN_SPRINT_COEF, sprintCoef);
    maxInvSpeed *= sprintCoef;
  }
  if (hasCondition(CRIPPLED)) {
    float crippleCoef = getMinConditionAmount(CRIPPLED);
    maxInvSpeed /= crippleCoef;
  }
  if (crouch) {
    maxInvSpeed *= 2.0f;
  }
  // update stealth level
  if (crouch || stealth < 1.0f) {
    computeStealth(elapsed);
  }
  TerrainId terrainId = dungeon->getTerrainType((int)x, (int)y);
  float walkTime = terrainTypes[terrainId].walkCost * maxInvSpeed;
  if (walkTimer >= 0) {
    bool hasWalked = false;
    int newx = (int)x, oldx = (int)x, newy = (int)y, oldy = (int)y;
    if (up)
      newy--;
    else if (down)
      newy++;
    if (left)
      newx--;
    else if (right)
      newx++;
    int dx = newx - (int)x;
    int dy = newy - (int)y;
    speed = playerSpeed;
    if (dx != 0 || dy != 0) {
      int oldnewx = newx;
      int oldnewy = newy;
      if (path) {
        delete path;
        path = NULL;
      }
      if (IN_RECTANGLE(newx, newy, dungeon->width, dungeon->height) && !dungeon->hasCreature(newx, newy) &&
          dungeon->map->isWalkable(newx, newy)) {
        x = newx;
        y = newy;
        if (dx != 0 && dy != 0) {
          speedDist += 1.41f;
          speed = playerSpeedDiag;
        } else {
          speedDist += 1.0f;
        }
        gameEngine->stats.nbSteps++;
        hasWalked = true;
      } else if (
          IN_RECTANGLE(newx, newy, dungeon->width, dungeon->height) && !dungeon->hasCreature(newx, newy) &&
          dungeon->hasActivableItem(newx, newy)) {
        // activate some item by bumping on it (like a chest)
        activateCell(newx, newy, false, true);
        up = down = left = right = false;
        hasWalked = true;
      } else {
        // try to slide against walls
        if (dx != 0 && dy != 0) {
          newx = (int)x + dx;
          newy = (int)y;
          // horizontal slide
          if (IN_RECTANGLE(newx, newy, dungeon->width, dungeon->height) && dungeon->map->isWalkable(newx, newy) &&
              (!dungeon->hasCreature(newx, newy) || !dungeon->getCreature(newx, newy)->isBlockingPath())) {
            x = newx;
            y = newy;
            gameEngine->stats.nbSteps++;
            speedDist += 1.0f;
            hasWalked = true;
          } else {
            // vertical slide
            newx = (int)x;
            newy = (int)y + dy;
            if (IN_RECTANGLE(newx, newy, dungeon->width, dungeon->height) && dungeon->map->isWalkable(newx, newy) &&
                (!dungeon->hasCreature(newx, newy) || !dungeon->getCreature(newx, newy)->isBlockingPath())) {
              x = newx;
              y = newy;
              gameEngine->stats.nbSteps++;
              speedDist += 1.0f;
              hasWalked = true;
            }
          }
        } else if (dx != 0) {
          static int dy = 1;
          if (IN_RECTANGLE(x + dx, y + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x + dx, (int)y + dy) &&
              (!dungeon->hasCreature((int)x + dx, (int)y + dy) ||
               !dungeon->getCreature((int)x + dx, (int)y + dy)->isBlockingPath())) {
            newx = (int)x + dx;
            newy = (int)y + dy;
            x = newx;
            y = newy;
            gameEngine->stats.nbSteps++;
            dy = -dy;
            speedDist += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          } else if (
              IN_RECTANGLE(x + dx, y - dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x + dx, (int)y - dy) &&
              (!dungeon->hasCreature((int)x + dx, (int)y - dy) ||
               !dungeon->getCreature((int)x + dx, (int)y - dy)->isBlockingPath())) {
            newx = (int)x + dx;
            newy = (int)y - dy;
            x = newx;
            y = newy;
            gameEngine->stats.nbSteps++;
            dy = -dy;
            speedDist += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          }
        } else if (dy != 0) {
          static int dx = 1;
          if (IN_RECTANGLE(x + dx, y + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x + dx, (int)y + dy) &&
              (!dungeon->hasCreature((int)x + dx, (int)y + dy) ||
               !dungeon->getCreature((int)x + dx, (int)y + dy)->isBlockingPath())) {
            newx = (int)x + dx;
            newy = (int)y + dy;
            x = newx;
            y = newy;
            gameEngine->stats.nbSteps++;
            dx = -dx;
            speedDist += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          } else if (
              IN_RECTANGLE(x - dx, y + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x - dx, (int)y + dy) &&
              (!dungeon->hasCreature((int)x - dx, (int)y + dy) ||
               !dungeon->getCreature((int)x - dx, (int)y + dy)->isBlockingPath())) {
            newx = (int)x - dx;
            newy = (int)y + dy;
            x = newx;
            y = newy;
            gameEngine->stats.nbSteps++;
            dx = -dx;
            speedDist += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          }
        }
        if (oldx == x && oldy == y && IN_RECTANGLE(oldnewx, oldnewy, dungeon->width, dungeon->height)) {
          // could not walk. activate item ?
          bool activated = false;
          activateCell(oldnewx, oldnewy, false, false, &activated);
          if (activated) {
            up = down = left = right = false;
            hasWalked = true;
          }
        }
      }
    } else if (path && !path->isEmpty()) {
      path->get(0, &newx, &newy);
      if (!dungeon->hasCreature(newx, newy)) {
        path->walk(&newx, &newy, false);
        setPos(newx, newy);
        gameEngine->stats.nbSteps++;
        hasWalked = true;
      } else {
        // the path is obstructed. cancel it
        delete path;
        path = NULL;
      }
    }
    // auto pickup items
    if (oldx != x || oldy != y) {
      auto* items = dungeon->getItems((int)x, (int)y);
      std::vector<Item*> toPick;
      for (Item* it : *items) {
        if (!it->speed > 0 && it->isPickable()) {
          toPick.push_back(it);
        }
      }
      for (Item* it : toPick) {
        it->putInInventory(this);
      }
      if (dungeon->hasRipples(x, y)) {
        gameEngine->startRipple(x, y);
      }
    }
    if (hasWalked) walkTimer = -walkTime;
  }
  // healing effect
  updateHealing(elapsed);
  healLight.setPos(x * 2, y * 2);
  return true;
}

void Player::computeFovRange(float elapsed) {
  static float rangeAccomodation = config.getFloatProperty("config.creatures.player.rangeAccomodation");
  static float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  float fovRangeTarget = maxFovRange;
  if (averageSpeed > playerSpeed / 2) {
    float fovSpeed = averageSpeed - playerSpeed / 2;
    float fovRefSpeed = playerSpeed / 2;
    fovRangeTarget = maxFovRange - (fovSpeed * 0.5 / fovRefSpeed) * 0.8 * maxFovRange;
  }
  if (crouch) fovRangeTarget *= 1.15f;
  if (fovRange > fovRangeTarget)
    fovRange += (fovRangeTarget - fovRange) * elapsed;
  else
    fovRange += (fovRangeTarget - fovRange) * elapsed / rangeAccomodation;
}

void Player::computeAverageSpeed(float elapsed) {
  speedElapsed += elapsed;
  if (speedElapsed > 0.5f) {
    averageSpeed = speedDist * 2;
    speedElapsed = speedDist = 0.0f;
  }
}

// update breath recovery after sprint
// sprintDelay < 0 : recovery. cannot sprint
void Player::updateSprintDelay(float elapsed, bool isSprinting) {
  static float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static float sprintRecovery = config.getFloatProperty("config.creatures.player.sprintRecovery");
  if (sprintDelay > 0.0f && isSprinting && averageSpeed > 0.1f) {
    sprintDelay -= elapsed;
    if (sprintDelay < 0.0f) {
      // exhausted
      Condition* cond = new Condition(CRIPPLED, sprintRecovery, 0.5f, "exhausted");
      addCondition(cond);
    }
  } else if (sprintDelay < 0.0f) {
    if (!hasCondition(CRIPPLED, "exhausted")) sprintDelay = sprintLength;
  } else if (sprintDelay > 0.0f && sprintDelay < sprintLength) {
    sprintDelay += elapsed;
    if (sprintDelay > sprintLength) sprintDelay = sprintLength;
  }
}

void Player::updateHealing(float elapsed) {
  static float healRate = config.getFloatProperty("config.creatures.player.healRate");
  Dungeon* dungeon = gameEngine->dungeon;
  if (healPoints > 0) {
    float amount = elapsed * healRate;
    healPoints -= amount;
    healLight.color = healthColor * (healPoints / (maxLife / 10));
    if (healPoints < 0) {
      amount += healPoints;
      healPoints = 0;
      dungeon->removeLight(&healLight);
    }
    curHeal += amount;
    int iHeal = (int)curHeal;
    life += iHeal;
    curHeal -= iHeal;
    life = MIN(maxLife, life);
  }
}

#define PLAY_CHUNK_VERSION 2
void Player::saveData(uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(PLAY_CHUNK_ID, PLAY_CHUNK_VERSION);

  // save player specific data
  zip->putFloat(stealth);

  Creature::saveData(CREA_CHUNK_ID, zip);
}

bool Player::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != PLAY_CHUNK_VERSION) return false;

  // load player specific data
  stealth = zip->getFloat();

  saveGame.loadChunk(&chunkId, &chunkVersion);
  bool ret = Creature::loadData(chunkId, chunkVersion, zip);
  if (ret) {
    TextGenerator::addGlobalValue("PLAYER_NAME", name);
  }
  return ret;
}

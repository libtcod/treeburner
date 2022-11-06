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
#include "mob/player.hpp"

#include <assert.h>
#include <ctype.h>

#include "base/movement.hpp"
#include "main.hpp"
#include "util/subcell.hpp"

namespace mob {
// maximum sprint : 2 times faster
static constexpr auto MIN_SPRINT_COEF = 0.5f;

static constexpr TCODColor healthColor{127, 127, 255};

Player::Player() {
  static const char playerChar = config.getCharProperty("config.creatures.player.ch");
  static const TCODColor playerColor = config.getColorProperty("config.creatures.player.col");
  static const float healthIntensityDelay = config.getFloatProperty("config.creatures.player.healIntensityDelay");
  static const char* healthIntensityPattern =
      strdup(config.getStringProperty("config.creatures.player.healIntensityPattern"));
  static const float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");

  ch = playerChar;
  color_ = playerColor;
  heal_light_.color = healthColor;
  heal_light_.range = 7;
  heal_light_.randomRad = false;
  heal_light_.setup(healthColor, healthIntensityDelay, healthIntensityPattern, nullptr);

  maxLife = 100.0f;
  sprint_delay_ = sprintLength;
}

void Player::init() {
  static const float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static const float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  speed = playerSpeed;
  life = maxLife;
  sprint_delay_ = sprintLength;
  // cannot do this. screen::Game::lights not created yet...
  // gameEngine->addLight(&light);
}

float Player::getHealth() { return life / maxLife; }

float Player::getHealing() { return std::min((life + heal_points_) / maxLife, 1.0f); }

void Player::termLevel() {
  if (path) delete path;
  path = nullptr;
  walkTimer = 0.0f;
  init_dungeon_ = true;
}

void Player::takeDamage(float amount) {
  const float oldLife = life;
  Creature::takeDamage(amount);
  if (life < oldLife) gameEngine->hitFlash();
}

void Player::heal(int healPoints) {
  const bool heal = heal_points_ > 0;
  heal_points_ += healPoints;
  if (!heal) gameEngine->dungeon->addLight(&heal_light_);
}

bool Player::setPath(int xDest, int yDest, bool limitPath) {
  static const int maxPathFinding = config.getIntProperty("config.creatures.player.maxPathFinding");

  map::Dungeon* dungeon = gameEngine->dungeon;
  if (!IN_RECTANGLE(xDest, yDest, dungeon->width, dungeon->height)) return false;
  // check if right clicked on a wall
  if (!dungeon->map->isWalkable(xDest, yDest)) {
    // walk toward the player and see if no other wall blocks the path
    const float dx = x_ - xDest;
    const float dy = y_ - yDest;
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
    TCODLine::init(xDest, yDest, (int)x_, (int)y_);
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
  if (!path) path = new TCODPath(dungeon->width, dungeon->height, this, nullptr);
  ignoreCreatures = false;
  bool ok = path->compute((int)x_, (int)y_, xDest, yDest);
  if (!ok) {
    ignoreCreatures = true;
    ok = path->compute((int)x_, (int)y_, xDest, yDest);
  }
  ignoreCreatures = true;
  if (!ok) return false;
  if (limitPath && !dungeon->getMemory(xDest, yDest) && path->size() > maxPathFinding) {
    delete path;
    path = nullptr;
    return false;
  }
  return true;
}

void Player::render(map::LightMap& lightMap) {
  static const float longButtonDelay = config.getFloatProperty("config.creatures.player.longButtonDelay");
  static const float longSpellDelay = config.getFloatProperty("config.creatures.player.longSpellDelay");
  static const float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static bool blink = false;

  Creature::render(lightMap);
  if ((spell::FireBall::incandescence && left_button_delay_ > longButtonDelay) ||
      (spell::FireBall::sparkle && right_button_delay_ > longButtonDelay)) {
    // spell charging progress bar
    int barLength = 0;
    const float delay = std::max(right_button_delay_, left_button_delay_);
    if (delay >= longSpellDelay) {
      barLength = 3;
    } else
      barLength = 1 + (int)((delay - longButtonDelay) * 1.99 / (longSpellDelay - longButtonDelay));
    blink = !blink;
    if (barLength == 3 && blink) barLength = 0;
    int bar_y = CON_H / 2 - 1;
    if (gameEngine->mousey <= CON_H / 2) bar_y = CON_H / 2 + 1;
    for (int i = CON_W / 2 - 1; i < CON_W / 2 + barLength - 1; i++) {
      TCODConsole::root->setChar(i, bar_y, TCOD_CHAR_PROGRESSBAR);
      TCODConsole::root->setCharForeground(i, bar_y, TCODColor::lightRed);
    }
  }

  // sprint bar
  if (is_sprinting_ && !hasCondition(CRIPPLED)) {
    if (sprint_delay_ < sprintLength) {
      const float sprintCoef = sprint_delay_ / sprintLength;
      static TCODImage sprintBar(10, 2);
      for (int x = 0; x < 10; x++) {
        float coef = (x * 0.1f - sprintCoef) * 5;
        coef = std::clamp(coef, 0.0f, 1.0f);
        TCODColor col = TCODColor::lerp(TCODColor::blue, TCODColor::white, coef);
        sprintBar.putPixel(x, 0, col);
        sprintBar.putPixel(x, 1, col);
      }
      util::transpBlit2x(&sprintBar, 0, 0, 10, 2, TCODConsole::root, CON_W / 2 - 2, CON_H / 2 + 2, 0.4f);
    }
  }

  // stealth bar
  if (crouched_ || stealth_ < 1.0f) {
    static TCODImage stealthBar(2, 10);
    for (int y = 0; y < 10; y++) {
      const float coef = std::clamp((y * 0.1f - stealth_) * 5, 0.0f, 1.0f);
      TCODColor col = TCODColor::lerp(TCODColor::white, TCODColor::darkViolet, coef);
      stealthBar.putPixel(0, y, col);
      stealthBar.putPixel(1, y, col);
      util::transpBlit2x(&stealthBar, 0, 0, 2, 10, TCODConsole::root, CON_W / 2 - 3, CON_H / 2 - 3, 0.4f);
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
  static const int moveUpKey = toupper(config.getCharProperty("config.creatures.player.moveUpKey"));
  static const int moveDownKey = toupper(config.getCharProperty("config.creatures.player.moveDownKey"));
  static const int moveLeftKey = toupper(config.getCharProperty("config.creatures.player.moveLeftKey"));
  static const int moveRightKey = toupper(config.getCharProperty("config.creatures.player.moveRightKey"));

  const int kc = toupper(key.c);
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
  map::Dungeon* dungeon = gameEngine->dungeon;
  float shadow = dungeon->getShadow(x_ * 2, y_ * 2);
  const float cloud = dungeon->getCloudCoef(x_ * 2, y_ * 2);
  shadow = std::min(shadow, cloud);
  // increase shadow. TODO should be in outdoor only!
  const float shadowcoef = crouched_ ? 4.0f : 2.0f;
  shadow = 1.0f - shadowcoef * (1.0f - shadow);
  stealth_ -= (stealth_ - shadow) * elapsed;
  const float speedcoef = crouched_ ? 0.6f : 1.0f;
  stealth_ += speedcoef * average_speed_ * elapsed * 0.1f;
  stealth_ = std::clamp(stealth_, 0.0f, 3.0f);
  // printf ("shadow %g stealth %g\n",shadow,stealth);
}

bool Player::activateCell(int dungeonx, int dungeony, bool lbut_pressed, bool walk, bool* activated) {
  // click on adjacent non walkable item = activate it
  // clink on adjacent pickable item = pick it up
  bool useWeapon = true;
  map::Dungeon* dungeon = gameEngine->dungeon;
  auto* items = dungeon->getItems(dungeonx, dungeony);
  if (activated) *activated = false;
  if (items->size() > 0) {
    useWeapon = false;
    // if ( lbut_pressed ) {
    std::vector<item::Item*> toPick;
    std::vector<item::Item*> toUse;
    for (item::Item* it : *items) {
      if (!it->isPickable())
        toUse.push_back(it);
      else if (it->speed_ == 0.0f)
        toPick.push_back(it);
    }
    for (item::Item* it : toPick) {
      it->putInInventory(this);
    }
    for (item::Item* it : toUse) {
      bool deleteOnUse = (it->typeData->flags & item::ITEM_DELETE_ON_USE) != 0;
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
  static const float longSpellDelay = config.getFloatProperty("config.creatures.player.longSpellDelay");
  // static float playerSpeed=config.getFloatProperty("config.creatures.player.speed");
  static const float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static const char quickslot1Key = config.getCharProperty("config.creatures.player.quickslot1");
  static const float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  static const float playerSpeedDiag = playerSpeed / 1.2f;
  /*
  static const char quickslot2Key=config.getCharProperty("config.creatures.player.quickslot2");
  static const char quickslot3Key=config.getCharProperty("config.creatures.player.quickslot3");
  static const char quickslot4Key=config.getCharProperty("config.creatures.player.quickslot4");
  static const char quickslot5Key=config.getCharProperty("config.creatures.player.quickslot5");
  static const char quickslot6Key=config.getCharProperty("config.creatures.player.quickslot6");
  static const char quickslot7Key=config.getCharProperty("config.creatures.player.quickslot7");
  static const char quickslot8Key=config.getCharProperty("config.creatures.player.quickslot8");
  static const char quickslot9Key=config.getCharProperty("config.creatures.player.quickslot9");
  static const char quickslot10Key=config.getCharProperty("config.creatures.player.quickslot10");
  */

  map::Dungeon* dungeon = gameEngine->dungeon;
  if (init_dungeon_) {
    init_dungeon_ = false;
    dungeon->addLight(&light_);
  }

  if (life <= 0) return false;
  light_.setPos(x_ * 2, y_ * 2);
  updateConditions(elapsed);

  walkTimer += elapsed;

  // special key status
  const bool ctrl = TCODConsole::isKeyPressed(TCODK_CONTROL);
  // if ( key.vk == TCODK_SHIFT ) isSprinting=key.pressed;
  is_sprinting_ = TCODConsole::isKeyPressed(TCODK_SHIFT);

  // user input
  if (gameEngine->isGamePaused()) {
    up_ = down_ = left_ = right_ = false;
    return true;
  }
  // update items in inventory
  inventory.erase(
      std::remove_if(inventory.begin(), inventory.end(), [&elapsed](item::Item* it) { return it->age(elapsed); }),
      inventory.end());

  // crouching
  crouched_ = ctrl;

  // update breath recovery after sprint
  updateSprintDelay(elapsed, is_sprinting_);
  // compute average speed during last second
  computeAverageSpeed(elapsed);
  // update fov according to breath
  computeFovRange(elapsed);

  {
    // Hack to fix movement.
    const auto [new_dx, new_dy] = base::get_current_movement_dir();
    left_ = new_dx < 0;
    right_ = new_dx > 0;
    up_ = new_dy < 0;
    down_ = new_dy > 0;
  }

  // mouse coordinates
  const int dungeon_x = mouse->cx + gameEngine->xOffset;
  const int dungeon_y = mouse->cy + gameEngine->yOffset;

  bool useWeapon = true;
  if (mouse->lbutton_pressed && ABS(dungeon_x - x_) <= 1 && ABS(dungeon_y - y_) <= 1) {
    // click on the player or near him in water=ripples
    if (mouse->lbutton_pressed && dungeon->hasRipples(dungeon_x, dungeon_y))
      gameEngine->startRipple(dungeon_x, dungeon_y);
    if (dungeon_x != x_ || dungeon_y != y_) {
      useWeapon = activateCell(dungeon_x, dungeon_y, mouse->lbutton_pressed, false);
    }
  }
  if (useWeapon) {
    if (mainHand) mainHand->update(elapsed, key, mouse);
    if (offHand) offHand->update(elapsed, key, mouse);
  }

  if (mouse->lbutton) {
    left_button_delay_ += elapsed;
    left_walk_delay_ += elapsed;
  }
  if (mouse->rbutton) right_button_delay_ += elapsed;

  // right mouse button
  if (mouse->rbutton_pressed) {
    if (!spell::FireBall::sparkle || right_button_delay_ < longSpellDelay) {
      // quick click : standard fireball
      if (dungeon_x != x_ || dungeon_y != y_) {
        spell::FireBall* fb = new spell::FireBall(x_, y_, dungeon_x, dungeon_y, spell::FB_STANDARD);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellStandard++;
        stealth_ = std::min(3.0f, stealth_ + 0.5f);
      }
    }
    if (spell::FireBall::sparkle && right_button_delay_ >= longSpellDelay) {
      // long right click : sparkle
      if (dungeon_x != x_ || dungeon_y != y_) {
        spell::FireBall* fb = new spell::FireBall(x_, y_, dungeon_x, dungeon_y, spell::FB_BURST);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellBurst++;
        stealth_ = std::min(3.0f, stealth_ + 0.5f);
      }
    }
    right_button_delay_ = 0.0f;
  }

  // left mouse button
  if (mouse->lbutton_pressed) {
    spell::FireBallType type = spell::FB_STANDARD;
    bool cast = false;
    const char* subtype = "fireball";
    if (left_button_delay_ < longSpellDelay) {
      // quick left click : cast standard fireball
      cast = true;
      gameEngine->stats.nbSpellStandard++;
      stealth_ = std::min(3.0f, stealth_ + 0.5f);
    } else {
      // long click : incandescence
      type = spell::FB_INCANDESCENCE;
      gameEngine->stats.nbSpellIncandescence++;
      stealth_ = std::min(3.0f, stealth_ + 0.5f);
      subtype = "fireball2";
      cast = true;
    }

    left_walk_delay_ = 0.0f;
    left_button_delay_ = 0.0f;
    if (cast) {
      if (dungeon_x != x_ || dungeon_y != y_) {
        spell::FireBall* fb = new spell::FireBall(x_, y_, dungeon_x, dungeon_y, type, subtype);
        gameEngine->addFireball(fb);
        gameEngine->stats.nbSpellStandard++;
      }
    }
  }

  if (!key.pressed && key.c == quickslot1Key) {
    // cast fireball
    if (dungeon_x != x_ || dungeon_y != y_) {
      spell::FireBallType type = spell::FB_SPARK;
      spell::FireBall* fb = new spell::FireBall(x_, y_, dungeon_x, dungeon_y, type);
      gameEngine->addFireball(fb);
      gameEngine->stats.nbSpellStandard++;
    } else {
      takeDamage(1);
      gameEngine->gui.log.warn("You get hurt by your own spark spell !");
    }
  }

  // walk
  float maxInvSpeed = 1.0f / speed;
  if (is_sprinting_ && sprint_delay_ > 0.0f && sprint_delay_ < sprintLength) {
    float sprintCoef = 1.0f - 4 * (sprintLength - sprint_delay_) / sprintLength;
    sprintCoef = std::max(MIN_SPRINT_COEF, sprintCoef);
    maxInvSpeed *= sprintCoef;
  }
  if (hasCondition(CRIPPLED)) {
    const float crippleCoef = getMinConditionAmount(CRIPPLED);
    maxInvSpeed /= crippleCoef;
  }
  if (crouched_) {
    maxInvSpeed *= 2.0f;
  }
  // update stealth level
  if (crouched_ || stealth_ < 1.0f) {
    computeStealth(elapsed);
  }
  map::TerrainId terrainId = dungeon->getTerrainType((int)x_, (int)y_);
  const float walkTime = map::terrainTypes[terrainId].walkCost * maxInvSpeed;
  if (walkTimer >= 0) {
    bool hasWalked = false;
    const int old_x = (int)x_;
    const int old_y = (int)y_;
    int new_x = (int)x_;
    int new_y = (int)y_;
    if (up_) --new_y;
    if (down_) ++new_y;
    if (left_) --new_x;
    if (right_) ++new_x;
    int dx = new_x - (int)x_;
    int dy = new_y - (int)y_;
    speed = playerSpeed;
    if (dx != 0 || dy != 0) {
      int old_new_x = new_x;
      int old_new_y = new_y;
      if (path) {
        delete path;
        path = nullptr;
      }
      if (IN_RECTANGLE(new_x, new_y, dungeon->width, dungeon->height) && !dungeon->hasCreature(new_x, new_y) &&
          dungeon->map->isWalkable(new_x, new_y)) {
        x_ = new_x;
        y_ = new_y;
        if (dx != 0 && dy != 0) {
          speed_dist_ += 1.41f;
          speed = playerSpeedDiag;
        } else {
          speed_dist_ += 1.0f;
        }
        gameEngine->stats.nbSteps++;
        hasWalked = true;
      } else if (
          IN_RECTANGLE(new_x, new_y, dungeon->width, dungeon->height) && !dungeon->hasCreature(new_x, new_y) &&
          dungeon->hasActivableItem(new_x, new_y)) {
        // activate some item by bumping on it (like a chest)
        activateCell(new_x, new_y, false, true);
        up_ = down_ = left_ = right_ = false;
        hasWalked = true;
      } else {
        // try to slide against walls
        if (dx != 0 && dy != 0) {
          new_x = (int)x_ + dx;
          new_y = (int)y_;
          // horizontal slide
          if (IN_RECTANGLE(new_x, new_y, dungeon->width, dungeon->height) && dungeon->map->isWalkable(new_x, new_y) &&
              (!dungeon->hasCreature(new_x, new_y) || !dungeon->getCreature(new_x, new_y)->isBlockingPath())) {
            x_ = new_x;
            y_ = new_y;
            gameEngine->stats.nbSteps++;
            speed_dist_ += 1.0f;
            hasWalked = true;
          } else {
            // vertical slide
            new_x = (int)x_;
            new_y = (int)y_ + dy;
            if (IN_RECTANGLE(new_x, new_y, dungeon->width, dungeon->height) && dungeon->map->isWalkable(new_x, new_y) &&
                (!dungeon->hasCreature(new_x, new_y) || !dungeon->getCreature(new_x, new_y)->isBlockingPath())) {
              x_ = new_x;
              y_ = new_y;
              gameEngine->stats.nbSteps++;
              speed_dist_ += 1.0f;
              hasWalked = true;
            }
          }
        } else if (dx != 0) {
          static int a = 1;
          if (IN_RECTANGLE(x_ + dx, y_ + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x_ + dx, (int)y_ + dy) &&
              (!dungeon->hasCreature((int)x_ + dx, (int)y_ + dy) ||
               !dungeon->getCreature((int)x_ + dx, (int)y_ + dy)->isBlockingPath())) {
            new_x = (int)x_ + dx;
            new_y = (int)y_ + dy;
            x_ = new_x;
            y_ = new_y;
            gameEngine->stats.nbSteps++;
            dy = -dy;
            speed_dist_ += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          } else if (
              IN_RECTANGLE(x_ + dx, y_ - dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x_ + dx, (int)y_ - dy) &&
              (!dungeon->hasCreature((int)x_ + dx, (int)y_ - dy) ||
               !dungeon->getCreature((int)x_ + dx, (int)y_ - dy)->isBlockingPath())) {
            new_x = (int)x_ + dx;
            new_y = (int)y_ - dy;
            x_ = new_x;
            y_ = new_y;
            gameEngine->stats.nbSteps++;
            dy = -dy;
            speed_dist_ += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          }
        } else if (dy != 0) {
          static int dx = 1;
          if (IN_RECTANGLE(x_ + dx, y_ + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x_ + dx, (int)y_ + dy) &&
              (!dungeon->hasCreature((int)x_ + dx, (int)y_ + dy) ||
               !dungeon->getCreature((int)x_ + dx, (int)y_ + dy)->isBlockingPath())) {
            new_x = (int)x_ + dx;
            new_y = (int)y_ + dy;
            x_ = new_x;
            y_ = new_y;
            gameEngine->stats.nbSteps++;
            dx = -dx;
            speed_dist_ += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          } else if (
              IN_RECTANGLE(x_ - dx, y_ + dy, dungeon->width, dungeon->height) &&
              dungeon->map->isWalkable((int)x_ - dx, (int)y_ + dy) &&
              (!dungeon->hasCreature((int)x_ - dx, (int)y_ + dy) ||
               !dungeon->getCreature((int)x_ - dx, (int)y_ + dy)->isBlockingPath())) {
            new_x = (int)x_ - dx;
            new_y = (int)y_ + dy;
            x_ = new_x;
            y_ = new_y;
            gameEngine->stats.nbSteps++;
            dx = -dx;
            speed_dist_ += 1.41f;
            speed = playerSpeedDiag;
            hasWalked = true;
          }
        }
        if (old_x == x_ && old_y == y_ && IN_RECTANGLE(old_new_x, old_new_y, dungeon->width, dungeon->height)) {
          // could not walk. activate item ?
          bool activated = false;
          activateCell(old_new_x, old_new_y, false, false, &activated);
          if (activated) {
            up_ = down_ = left_ = right_ = false;
            hasWalked = true;
          }
        }
      }
    } else if (path && !path->isEmpty()) {
      path->get(0, &new_x, &new_y);
      if (!dungeon->hasCreature(new_x, new_y)) {
        path->walk(&new_x, &new_y, false);
        setPos(new_x, new_y);
        gameEngine->stats.nbSteps++;
        hasWalked = true;
      } else {
        // the path is obstructed. cancel it
        delete path;
        path = nullptr;
      }
    }
    // auto pickup items
    if (old_x != x_ || old_y != y_) {
      auto* items = dungeon->getItems((int)x_, (int)y_);
      std::vector<item::Item*> toPick;
      for (item::Item* it : *items) {
        if (!it->speed_ > 0 && it->isPickable()) {
          toPick.push_back(it);
        }
      }
      for (item::Item* it : toPick) {
        it->putInInventory(this);
      }
      if (dungeon->hasRipples(x_, y_)) {
        gameEngine->startRipple(x_, y_);
      }
    }
    if (hasWalked) walkTimer = -walkTime;
  }
  // healing effect
  updateHealing(elapsed);
  heal_light_.setPos(x_ * 2, y_ * 2);
  return true;
}

void Player::computeFovRange(float elapsed) {
  static const float rangeAccomodation = config.getFloatProperty("config.creatures.player.rangeAccomodation");
  static const float playerSpeed = gameEngine->getFloatParam("playerSpeed");
  float fovRangeTarget = max_fov_range_;
  if (average_speed_ > playerSpeed / 2) {
    const float fovSpeed = average_speed_ - playerSpeed / 2;
    const float fovRefSpeed = playerSpeed / 2;
    fovRangeTarget = max_fov_range_ - (fovSpeed * 0.5 / fovRefSpeed) * 0.8 * max_fov_range_;
  }
  if (crouched_) fovRangeTarget *= 1.15f;
  if (fovRange > fovRangeTarget)
    fovRange += (fovRangeTarget - fovRange) * elapsed;
  else
    fovRange += (fovRangeTarget - fovRange) * elapsed / rangeAccomodation;
}

void Player::computeAverageSpeed(float elapsed) {
  speed_elapsed_ += elapsed;
  if (speed_elapsed_ > 0.5f) {
    average_speed_ = speed_dist_ * 2;
    speed_elapsed_ = speed_dist_ = 0.0f;
  }
}

// update breath recovery after sprint
// sprint_delay_ < 0 : recovery. cannot sprint
void Player::updateSprintDelay(float elapsed, bool isSprinting) {
  static const float sprintLength = config.getFloatProperty("config.creatures.player.sprintLength");
  static const float sprintRecovery = config.getFloatProperty("config.creatures.player.sprintRecovery");
  if (sprint_delay_ > 0.0f && isSprinting && average_speed_ > 0.1f) {
    sprint_delay_ -= elapsed;
    if (sprint_delay_ < 0.0f) {
      // exhausted
      Condition* cond = new Condition(CRIPPLED, sprintRecovery, 0.5f, "exhausted");
      addCondition(cond);
    }
  } else if (sprint_delay_ < 0.0f) {
    if (!hasCondition(CRIPPLED, "exhausted")) sprint_delay_ = sprintLength;
  } else if (sprint_delay_ > 0.0f && sprint_delay_ < sprintLength) {
    sprint_delay_ += elapsed;
    if (sprint_delay_ > sprintLength) sprint_delay_ = sprintLength;
  }
}

void Player::updateHealing(float elapsed) {
  static const float healRate = config.getFloatProperty("config.creatures.player.healRate");
  map::Dungeon* dungeon = gameEngine->dungeon;
  if (heal_points_ > 0) {
    float amount = elapsed * healRate;
    heal_points_ -= amount;
    heal_light_.color = healthColor * (heal_points_ / (maxLife / 10));
    if (heal_points_ < 0) {
      amount += heal_points_;
      heal_points_ = 0;
      dungeon->removeLight(&heal_light_);
    }
    current_heal_ += amount;
    const int iHeal = (int)current_heal_;
    life += iHeal;
    current_heal_ -= iHeal;
    life = std::min(maxLife, life);
  }
}

#define PLAY_CHUNK_VERSION 2
void Player::saveData(uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(PLAY_CHUNK_ID, PLAY_CHUNK_VERSION);

  // save player specific data
  zip->putFloat(stealth_);

  Creature::saveData(CREA_CHUNK_ID, zip);
}

bool Player::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != PLAY_CHUNK_VERSION) return false;

  // load player specific data
  stealth_ = zip->getFloat();

  saveGame.loadChunk(&chunkId, &chunkVersion);
  const bool ret = Creature::loadData(chunkId, chunkVersion, zip);
  if (ret) {
    util::TextGenerator::addGlobalValue("PLAYER_NAME", name);
  }
  return ret;
}
}  // namespace mob

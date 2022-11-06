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
#include "mob/minion.hpp"

#include <math.h>

#include "main.hpp"

namespace mob {
float Villager::talkDelay = 0.0f;

Archer::Archer() {
  static TCODColor archerColor = config.getColorProperty("config.creatures.archer.col");
  static char archerChar = config.getCharProperty("config.creatures.archer.ch");
  static int archerLife = config.getIntProperty("config.creatures.archer.life");
  ch = archerChar;
  color_ = archerColor;
  life = archerLife;
  strcpy(name, "archer");
  pathTimer = 0.0f;
}

bool Archer::update(float elapsed) {
  static float arrowSpeed = config.getFloatProperty("config.gameplay.arrowSpeed");
  pathTimer += elapsed;
  if (!Creature::update(elapsed)) return false;
  if (gameEngine->dungeon->map->isInFov((int)x, (int)y)) {
    // see player
    if (pathTimer > 1.0f) {
      pathTimer = 0.0f;
      item::Item* arrow = item::Item::getItem("arrow", x + 0.5f, y + 0.5f, false);
      arrow->dx = gameEngine->player.x - x;
      arrow->dy = gameEngine->player.y - y;
      arrow->speed = arrowSpeed;
      float l = sqrt(arrow->dx * arrow->dx + arrow->dy * arrow->dy);
      arrow->dx /= l;
      arrow->dy /= l;
      arrow->duration = l / arrow->speed;
      gameEngine->dungeon->addItem(arrow);
    }
  }
  return true;
}

Villager::Villager() {
  static TCODColor villagerColor = config.getColorProperty("config.creatures.villager.col");
  static char villagerChar = config.getCharProperty("config.creatures.villager.ch");
  static int villagerLife = config.getIntProperty("config.creatures.villager.life");
  ch = villagerChar;
  color_ = villagerColor;
  life = villagerLife;
  strcpy(name, "villager");
}

bool Villager::update(float elapsed) {
  static util::TextGenerator talkGenerator("data/cfg/villager.txg");
  bool oldSeen = seen;
  if (!Minion::update(elapsed)) return false;
  if (!oldSeen && seen && talkDelay > 10.0f) {
    talkDelay = 0.0f;
    talk(talkGenerator.generate("villager", "${SPOTTED}"));
  }
  if (Creature::creatureByType[CREATURE_VILLAGER].get(0) == this) {
    talkDelay += elapsed;
  }
  return true;
}

Minion::Minion() {
  static char minionChar = config.getCharProperty("config.creatures.minion.ch");
  static TCODColor minionColor = config.getColorProperty("config.creatures.minion.col");
  static int minionLife = config.getIntProperty("config.creatures.minion.life");
  static float pathDelay = config.getFloatProperty("config.creatures.pathDelay");
  ch = minionChar;
  color_ = minionColor;
  life = minionLife;
  seen = false;
  speed = 1.0f;
  pathTimer = TCODRandom::getInstance()->getFloat(0.0f, pathDelay);
}

void Minion::setSeen() {
  static float minionSpeed = config.getFloatProperty("config.creatures.minion.speed");
  seen = true;
  speed = minionSpeed;
}

void Minion::onReplace() { seen = false; }

bool Minion::update(float elapsed) {
  static float minionDamage = config.getFloatProperty("config.creatures.minion.damage");
  static float pathDelay = config.getFloatProperty("config.creatures.pathDelay");

  base::GameEngine* game = gameEngine;
  if (!Creature::update(elapsed)) return false;
  pathTimer += elapsed;
  if (!seen && game->dungeon->map->isInFov((int)x, (int)y) && game->dungeon->getMemory(x, y)) {
    float dist = squaredDistance(game->player);
    if (dist < 1.0f || game->player.stealth_ >= 1.0f - 1.0f / dist) {
      // creature is seen by player
      setSeen();
    }
  }
  if (burn || !seen) {
    randomWalk(elapsed);
  } else {
    // track player
    if (!path) {
      path = new TCODPath(game->dungeon->width, game->dungeon->height, this, game);
    }
    if (pathTimer > pathDelay) {
      int dx, dy;
      path->getDestination(&dx, &dy);
      if (dx != game->player.x || dy != game->player.y) {
        // path is no longer valid (the player moved)
        path->compute((int)x, (int)y, (int)game->player.x, (int)game->player.y);
        pathTimer = 0.0f;
      }
    }
    walk(elapsed);
  }
  float dx = ABS(game->player.x - x);
  float dy = ABS(game->player.y - y);
  if (dx <= 1.0f && dy <= 1.0f) {
    // at melee range. attack
    game->player.takeDamage(minionDamage * elapsed);
  }
  return true;
}
}  // namespace mob

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
  ch_ = archerChar;
  color_ = archerColor;
  life_ = archerLife;
  name_ = "archer";
  pathTimer = 0.0f;
}

bool Archer::update(float elapsed) {
  static float arrowSpeed = config.getFloatProperty("config.gameplay.arrowSpeed");
  pathTimer += elapsed;
  if (!Creature::update(elapsed)) return false;
  if (gameEngine->dungeon->map->isInFov((int)x_, (int)y_)) {
    // see player
    if (pathTimer > 1.0f) {
      pathTimer = 0.0f;
      item::Item* arrow = item::Item::getItem("arrow", x_ + 0.5f, y_ + 0.5f, false);
      arrow->dx_ = gameEngine->player.x_ - x_;
      arrow->dy_ = gameEngine->player.y_ - y_;
      arrow->speed_ = arrowSpeed;
      float l = sqrt(arrow->dx_ * arrow->dx_ + arrow->dy_ * arrow->dy_);
      arrow->dx_ /= l;
      arrow->dy_ /= l;
      arrow->duration_ = l / arrow->speed_;
      gameEngine->dungeon->addItem(arrow);
    }
  }
  return true;
}

Villager::Villager() {
  static TCODColor villagerColor = config.getColorProperty("config.creatures.villager.col");
  static char villagerChar = config.getCharProperty("config.creatures.villager.ch");
  static int villagerLife = config.getIntProperty("config.creatures.villager.life");
  ch_ = villagerChar;
  color_ = villagerColor;
  life_ = villagerLife;
  name_ = "villager";
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
  ch_ = minionChar;
  color_ = minionColor;
  life_ = minionLife;
  seen = false;
  speed_ = 1.0f;
  pathTimer = TCODRandom::getInstance()->getFloat(0.0f, pathDelay);
}

void Minion::setSeen() {
  static float minionSpeed = config.getFloatProperty("config.creatures.minion.speed");
  seen = true;
  speed_ = minionSpeed;
}

void Minion::onReplace() { seen = false; }

bool Minion::update(float elapsed) {
  static float minionDamage = config.getFloatProperty("config.creatures.minion.damage");
  static float pathDelay = config.getFloatProperty("config.creatures.pathDelay");

  base::GameEngine* game = gameEngine;
  if (!Creature::update(elapsed)) return false;
  pathTimer += elapsed;
  if (!seen && game->dungeon->map->isInFov((int)x_, (int)y_) && game->dungeon->getMemory(x_, y_)) {
    float dist = squaredDistance(game->player);
    if (dist < 1.0f || game->player.stealth_ >= 1.0f - 1.0f / dist) {
      // creature is seen by player
      setSeen();
    }
  }
  if (burn_ || !seen) {
    randomWalk(elapsed);
  } else {
    // track player
    if (!path_) {
      path_ = std::make_unique<TCODPath>(game->dungeon->width, game->dungeon->height, this, game);
    }
    if (pathTimer > pathDelay) {
      int dx, dy;
      path_->getDestination(&dx, &dy);
      if (dx != game->player.x_ || dy != game->player.y_) {
        // path is no longer valid (the player moved)
        path_->compute((int)x_, (int)y_, (int)game->player.x_, (int)game->player.y_);
        pathTimer = 0.0f;
      }
    }
    walk(elapsed);
  }
  float dx = fabsf(game->player.x_ - x_);
  float dy = fabsf(game->player.y_ - y_);
  if (dx <= 1.0f && dy <= 1.0f) {
    // at melee range. attack
    game->player.takeDamage(minionDamage * elapsed);
  }
  return true;
}
}  // namespace mob

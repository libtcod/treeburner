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

#include "main.hpp"

VillageHead::VillageHead() {
  static TCODColor villageHeadColor = config.getColorProperty("config.creatures.villageHead.col");
  static char villageHeadChar = config.getCharProperty("config.creatures.villageHead.ch");
  static int villageHeadLife = config.getIntProperty("config.creatures.villageHead.life");
  ch = villageHeadChar;
  col = villageHeadColor;
  life = villageHeadLife;
  strcpy(name, "village head");
  summonMinions = true;
  stayInLair = false;
}

Boss::Boss() {
  static char bossChar = config.getCharProperty("config.creatures.boss.ch");
  static TCODColor bossColor = config.getColorProperty("config.creatures.boss.col");
  static int bossLife = config.getIntProperty("config.creatures.boss.life");
  static float pathDelay = config.getFloatProperty("config.creatures.pathDelay");
  static float treasureLightRange = config.getFloatProperty("config.display.treasureLightRange");
  static TCODColor treasureLightColor = config.getColorProperty("config.display.treasureLightColor");
  static float treasureIntensityDelay = config.getFloatProperty("config.display.treasureIntensityDelay");
  static const char* treasureIntensityPattern =
      strdup(config.getStringProperty("config.display.treasureIntensityPattern"));

  treasureLight = new ExtendedLight();
  treasureLight->color = treasureLightColor;
  treasureLight->range = treasureLightRange * 2;
  treasureLight->setup(treasureLightColor, treasureIntensityDelay, treasureIntensityPattern, NULL);
  gameEngine->dungeon->addLight(treasureLight);

  ch = bossChar;
  col = bossColor;
  life = bossLife;
  seen = false;
  speed = 1.0f;
  pathTimer = TCODRandom::getInstance()->getFloat(0.0f, pathDelay);
  summonTimer = 0.0f;
  ignoreCreatures = false;
  summonMinions = true;
  stayInLair = true;
}

void Boss::setSeen() {
  static float bossSpeed = config.getFloatProperty("config.creatures.boss.speed");

  seen = true;
  ((Game*)(gameEngine))->bossSeen = true;
  AiDirector::instance->bossSeen();
  speed = bossSpeed;
}

// boss can't be stunned
void Boss::stun(float delay) {}

void Boss::takeDamage(float amount) {
  Creature::takeDamage(amount);
  if (life <= 0.0f) {
    // the boss dies
    gameEngine->bossIsDead = true;
    AiDirector::instance->bossDead();
    gameEngine->dungeon->stairx = (int)x;
    gameEngine->dungeon->stairy = (int)y;
  }
}

bool Boss::update(float elapsed) {
  static float pathDelay = config.getFloatProperty("config.creatures.pathDelay");
  static float summonTime = config.getFloatProperty("config.creatures.boss.summonTime");
  static int minionCount = config.getIntProperty("config.creatures.boss.minionCount");
  static float burnDamage = config.getFloatProperty("config.creatures.burnDamage");

  treasureLight->setPos(x * 2, y * 2);
  if (life <= 0) {
    gameEngine->dungeon->removeLight(treasureLight);
    return false;
  }
  if (burn) {
    takeDamage(burnDamage * elapsed);
  }
  pathTimer += elapsed;
  if (!seen) {
    if (gameEngine->dungeon->isCellInFov(x, y) && gameEngine->dungeon->getMemory(x, y)) {
      // creature is seen by player
      setSeen();
    }
    return true;
  }
  summonTimer += elapsed;
  if (summonMinions && summonTimer > summonTime) {
    // summon some minions to protect the boss
    summonTimer = 0.0f;
    for (int i = 0; i < minionCount; i++) {
      AiDirector::instance->spawnMinion(true, (int)x, (int)y);
    }
  }
  if (pathTimer > pathDelay) {
    if (!path || path->isEmpty()) {
      // stay away from player
      // while staying in lair
      int destx, desty;
      if (stayInLair) {
        destx = gameEngine->dungeon->stairx + TCODRandom::getInstance()->getInt(-15, 15);
        desty = gameEngine->dungeon->stairy + TCODRandom::getInstance()->getInt(-15, 15);
      } else {
        destx = (int)(x + TCODRandom::getInstance()->getInt(-15, 15));
        desty = (int)(y + TCODRandom::getInstance()->getInt(-15, 15));
      }
      destx = CLAMP(0, gameEngine->dungeon->width - 1, destx);
      desty = CLAMP(0, gameEngine->dungeon->height - 1, desty);
      gameEngine->dungeon->getClosestWalkable(&destx, &desty, true, true);
      if (!path) {
        path = new TCODPath(gameEngine->dungeon->width, gameEngine->dungeon->height, this, gameEngine);
      }
      path->compute((int)x, (int)y, destx, desty);
      pathTimer = 0.0f;
    } else
      walk(elapsed);
  } else {
    walk(elapsed);
  }
  return life > 0;
}

float Boss::getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const {
  static int secureDist = config.getIntProperty("config.creatures.boss.secureDist");
  static float secureCoef = config.getFloatProperty("config.creatures.boss.secureCoef");

  // the boss don't like to be near player
  if (!gameEngine->dungeon->map->isWalkable(xTo, yTo)) return 0.0f;
  if (ignoreCreatures) return 1.0f;
  float pdist = SQRDIST(gameEngine->player.x, gameEngine->player.y, xTo, yTo);
  if (pdist < secureDist) return 1.0f + secureCoef * (secureDist - pdist);
  return 1.0f;
}

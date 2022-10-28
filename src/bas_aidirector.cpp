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
#include "bas_aidirector.hpp"

#include <math.h>
#include <stdio.h>

#include <libtcod.hpp>

#include "main.hpp"
#include "mob_minion.hpp"
#include "util_powerup.hpp"

AiDirector* AiDirector::instance = NULL;

AiDirector::AiDirector() : spawnCount(0), spawnTimer(0.0f), waitTimer(0.0f), timer(-M_PI / 2), nbScrolls(0) {
  static int hordeDelay = config.getIntProperty("config.aidirector.hordeDelay");
  static int itemKillCount = config.getIntProperty("config.aidirector.itemKillCount");

  lowLevel = config.getFloatProperty("config.aidirector.lowLevel");
  medLevel = config.getFloatProperty("config.aidirector.medLevel");

  instance = this;
  status = STATUS_CALM;
  killCount = TCODRandom::getInstance()->getInt((int)(itemKillCount * 0.75), (int)(itemKillCount * 1.25));
  hordeTimer = hordeDelay;
  baseCreature = CREATURE_MINION;
}

void AiDirector::bossSeen() {
  // no more respite once the boss has been seen
  lowLevel = -1.0f;
  medLevel = -1.0f;
}

void AiDirector::bossDead() {
  // no more creatures
  lowLevel = 1.0f;
  medLevel = 2.0f;
}

void AiDirector::update(float elapsed) {
  static float waveLength = config.getFloatProperty("config.aidirector.waveLength");
  static float medRate = config.getFloatProperty("config.aidirector.medRate");
  static float highRate = config.getFloatProperty("config.aidirector.highRate");
  static int maxCreatures = config.getIntProperty("config.aidirector.maxCreatures");
  static int hordeDelay = config.getIntProperty("config.aidirector.hordeDelay");
  static bool debug = config.getBoolProperty("config.debug");

  GameEngine* game = gameEngine;
  hordeTimer -= elapsed;
  timer += elapsed * (2 * M_PI) / waveLength;
  spawnTimer += elapsed;
  float wavePos = 0.5f * (1.0f + sinf(timer));
  if (wavePos < lowLevel) {
    if (wavePos >= lowLevel / 2) status = STATUS_CALM;
    return;
  }
  if (waitTimer > 0.0f) {
    waitTimer -= elapsed;
    return;
  }
  if (wavePos < medLevel)
    status = STATUS_MED;
  else
    status = STATUS_HIGH;
  int nbCreatures = Creature::creatureByType[baseCreature].size();
  if (nbCreatures >= maxCreatures) return;
  if (spawnTimer > 60.0f) {
    spawnTimer -= 60.0f;
    spawnCount = 0;
  }
  if (wavePos < medLevel || hordeTimer > 0.0f) {
    float rate = (wavePos < medLevel) ? medRate : highRate;
    float curRate = spawnCount / spawnTimer;
    if (curRate < rate) {
      float timeRemaining = 60.0f - spawnTimer;
      int nbMissing = (int)(rate - spawnCount);
      if (nbMissing > 0) {
        if (debug)
          game->gui.log.debug(
              "minion (%s) wavepos %d horde in %d",
              (wavePos < medLevel ? "med" : "high"),
              (int)(wavePos * 100),
              (int)(hordeTimer));
        spawnMinion(false);
        waitTimer = timeRemaining / nbMissing;
      }
    }
  } else {
    status = STATUS_HORDE;
    float timeRemaining = 60.0f - spawnTimer;
    int nbMissing = (int)(highRate - spawnCount);
    if (debug) game->gui.log.debug("Horde! %d", nbMissing);
    while (nbMissing > 0 && game->dungeon->creatures.size() < maxCreatures) {
      spawnMinion(true);
      nbMissing--;
    }
    hordeTimer = hordeDelay;
    waitTimer = timeRemaining;
  }
}

void AiDirector::spawnMinion(bool chase, int x, int y) {
  int sx, sy;
  GameEngine* game = gameEngine;

  spawnCount++;
  if (x != -1 && y != -1) {
    sx = x;
    sy = y;
  } else {
    game->dungeon->getClosestSpawnSource(game->player.x, game->player.y, &sx, &sy);
  }
  Minion* cr = (Minion*)Creature::getCreature(baseCreature);
  game->dungeon->getClosestWalkable(&sx, &sy, true, false);
  cr->setPos(sx, sy);
  if (chase) cr->setSeen();
  game->dungeon->addCreature(cr);
}

// remove a creature that is too far from player
void AiDirector::replace(Creature* cr) {
  int oldx = (int)cr->x, oldy = (int)cr->y;
  cr->burn = false;
  int newx, newy;
  gameEngine->dungeon->getClosestSpawnSource(gameEngine->player.x, gameEngine->player.y, &newx, &newy);
  cr->setPos(newx, newy);
  gameEngine->dungeon->moveCreature(cr, oldx, oldy, newx, newy);
  cr->onReplace();
}

void AiDirector::killCreature(Creature* cr) {
  static int itemKillCount = config.getIntProperty("config.aidirector.itemKillCount");
  static bool debug = config.getBoolProperty("config.debug");

  gameEngine->stats.nbCreatureKilled++;
  if (debug) gameEngine->gui.log.debug("%d kills", gameEngine->stats.nbCreatureKilled);
  killCount--;
  if (killCount == 0) {
    TCODList<Powerup*> list;
    Powerup::getAvailable(&list);

    killCount = TCODRandom::getInstance()->getInt((int)(itemKillCount * 0.75), (int)(itemKillCount * 1.25));
    if (list.size() - nbScrolls <= 0 ||
        TCODRandom::getInstance()->getFloat(0.0f, 1.0f) > gameEngine->player.getHealth()) {
      Item* it = Item::getItem("health", 0, 0);
      Item* bottle = Item::getItem("bottle", cr->x, cr->y);
      it->putInContainer(bottle);
      gameEngine->dungeon->addItem(bottle);
    } else {
      nbScrolls++;
      // TODO
      // gameEngine->dungeon->addItem(Item::getItem(ITEM_SCROLL,cr->x,cr->y));
    }
  }
}

void AiDirector::termLevel() { nbScrolls = 0; }

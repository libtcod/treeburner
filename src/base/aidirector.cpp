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
#include "base/aidirector.hpp"

#include <math.h>
#include <stdio.h>

#include <gsl/gsl>
#include <libtcod.hpp>

#include "main.hpp"
#include "mob/minion.hpp"
#include "util_powerup.hpp"

namespace base {
AiDirector* AiDirector::instance = NULL;

AiDirector::AiDirector() {
  static const float hordeDelay = config.getFloatProperty("config.aidirector.hordeDelay");
  static const int itemKillCount = config.getIntProperty("config.aidirector.itemKillCount");

  lowLevel = config.getFloatProperty("config.aidirector.lowLevel");
  medLevel = config.getFloatProperty("config.aidirector.medLevel");

  instance = this;
  killCount = TCODRandom::getInstance()->getInt((int)(itemKillCount * 0.75), (int)(itemKillCount * 1.25));
  hordeTimer = hordeDelay;
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
  static const float waveLength = config.getFloatProperty("config.aidirector.waveLength");
  static const float medRate = config.getFloatProperty("config.aidirector.medRate");
  static const float highRate = config.getFloatProperty("config.aidirector.highRate");
  static const int maxCreatures = config.getIntProperty("config.aidirector.maxCreatures");
  static const int hordeDelay = config.getIntProperty("config.aidirector.hordeDelay");
  static const bool debug = config.getBoolProperty("config.debug");

  GameEngine& game = *gameEngine;
  hordeTimer -= elapsed;
  timer += elapsed * (2.0f * float{M_PI}) / waveLength;
  spawnTimer += elapsed;
  const float wavePos = 0.5f * (1.0f + sinf(timer));
  if (wavePos < lowLevel) {
    if (wavePos >= lowLevel / 2.0f) status = STATUS_CALM;
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
  const int nbCreatures = Creature::creatureByType[baseCreature].size();
  if (nbCreatures >= maxCreatures) return;
  if (spawnTimer > 60.0f) {
    spawnTimer -= 60.0f;
    spawnCount = 0;
  }
  if (wavePos < medLevel || hordeTimer > 0.0f) {
    const float rate = (wavePos < medLevel) ? medRate : highRate;
    const float curRate = spawnCount / spawnTimer;
    if (curRate < rate) {
      const float timeRemaining = 60.0f - spawnTimer;
      const int nbMissing = gsl::narrow_cast<int>(rate) - spawnCount;
      if (nbMissing > 0) {
        if (debug)
          game.gui.log.debug(
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
    const float timeRemaining = 60.0f - spawnTimer;
    int nbMissing = gsl::narrow_cast<int>(highRate) - spawnCount;
    if (debug) game.gui.log.debug("Horde! %d", nbMissing);
    while (nbMissing > 0 && game.dungeon->creatures.size() < maxCreatures) {
      spawnMinion(true);
      --nbMissing;
    }
    hordeTimer = gsl::narrow_cast<float>(hordeDelay);
    waitTimer = timeRemaining;
  }
}

void AiDirector::spawnMinion(bool chase) {
  const GameEngine& game = *gameEngine;
  auto [x, y] =
      game.dungeon->getClosestSpawnSource(gsl::narrow_cast<int>(game.player.x), gsl::narrow_cast<int>(game.player.y));
  spawnMinion(chase, x, y);
}

void AiDirector::spawnMinion(bool chase, int x, int y) {
  assert(x >= 0);
  assert(y >= 0);
  GameEngine& game = *gameEngine;

  ++spawnCount;
  Minion* cr = static_cast<Minion*>(Creature::getCreature(baseCreature));
  game.dungeon->getClosestWalkable(&x, &y, true, false);
  cr->setPos(x, y);
  if (chase) cr->setSeen();
  game.dungeon->addCreature(cr);
}

// remove a creature that is too far from player
void AiDirector::replace(Creature* cr) {
  const int old_x = (int)cr->x;
  const int old_y = (int)cr->y;
  cr->burn = false;
  int newx{};
  int newy{};
  gameEngine->dungeon->getClosestSpawnSource(gameEngine->player.x, gameEngine->player.y, &newx, &newy);
  cr->setPos(newx, newy);
  gameEngine->dungeon->moveCreature(cr, old_x, old_y, newx, newy);
  cr->onReplace();
}

void AiDirector::killCreature(Creature* cr) {
  static const int itemKillCount = config.getIntProperty("config.aidirector.itemKillCount");
  static const bool debug = config.getBoolProperty("config.debug");

  gameEngine->stats.nbCreatureKilled++;
  if (debug) gameEngine->gui.log.debug("%d kills", gameEngine->stats.nbCreatureKilled);
  --killCount;
  if (killCount == 0) {
    TCODList<Powerup*> list;
    Powerup::getAvailable(&list);

    killCount = TCODRandom::getInstance()->getInt((int)(itemKillCount * 0.75), (int)(itemKillCount * 1.25));
    if (list.size() - nbScrolls <= 0 ||
        TCODRandom::getInstance()->getFloat(0.0f, 1.0f) > gameEngine->player.getHealth()) {
      item::Item* it = item::Item::getItem("health", 0, 0);
      item::Item* bottle = item::Item::getItem("bottle", cr->x, cr->y);
      it->putInContainer(bottle);
      gameEngine->dungeon->addItem(bottle);
    } else {
      ++nbScrolls;
      // TODO
      // gameEngine->dungeon->addItem(Item::getItem(ITEM_SCROLL,cr->x,cr->y));
    }
  }
}

void AiDirector::termLevel() { nbScrolls = 0; }
}  // namespace base

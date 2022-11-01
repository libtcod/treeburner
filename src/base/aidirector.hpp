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
#pragma once
#include <cmath>

#include "mob/creature.hpp"

namespace base {
// the class that manages monsters spawning
class AiDirector {
 public:
  static AiDirector* instance;
  AiDirector();
  void setBaseCreature(mob::CreatureTypeId id) { baseCreature = id; }
  void update(float elapsed);
  void replace(mob::Creature* cr);
  void killCreature(mob::Creature* cr);
  void bossSeen();  // enter final fight mode
  void bossDead();  // exit final fight mode
  void termLevel();
  void spawnMinion(bool chase);
  void spawnMinion(bool chase, int x, int y);
  enum { STATUS_CALM, STATUS_MED, STATUS_HIGH, STATUS_HORDE } status{STATUS_CALM};

 protected:
  int spawnCount{};
  float spawnTimer{};
  float waitTimer{};
  float timer{float{-M_PI} / 2.0f};
  float hordeTimer{};
  int killCount{};
  float lowLevel{};
  float medLevel{};
  int nbScrolls{};
  mob::CreatureTypeId baseCreature{mob::CREATURE_MINION};
};
}  // namespace base

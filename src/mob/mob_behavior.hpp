/*
 * Copyright (c) 2009,2010 Jice
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
#include <libtcod.hpp>

#include "entity.hpp"

class WalkPattern : public ITCODPathCallback {
  float getWalkCost(int, int, int, int, void*) const { return 1.0f; }
};

class WaterOnlyWalkPattern : public WalkPattern {
  float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const;
};

class AvoidWaterWalkPattern : public WalkPattern {
  float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const;
};

class Creature;

class Behavior {
 public:
  Behavior(WalkPattern* walkPattern) : walkPattern(walkPattern) {}
  virtual bool update(Creature* crea, float elapsed) = 0;

 protected:
  WalkPattern* walkPattern = nullptr;
};

class FollowBehavior : public Behavior {
 public:
  FollowBehavior(WalkPattern* walkPattern) : Behavior(walkPattern), leader_(NULL), standDelay(0.0f) {}
  void setLeader(Creature* leader) { leader_ = leader; }
  bool update(Creature* crea, float elapsed) override;

 protected:
  Creature* leader_ = nullptr;
  float standDelay;
};

// default duration of a scare point in seconds
#define SCARE_LIFE 1.0

class ScarePoint : public base::Entity {
 public:
  float life;
  ScarePoint(float x, float y, float life = SCARE_LIFE) : base::Entity(x, y), life(life) {}
};

class HerdBehavior : public Behavior {
 public:
  HerdBehavior(WalkPattern* walkPattern) : Behavior(walkPattern) {}
  virtual ~HerdBehavior() = default;
  bool update(Creature* crea, float elapsed) override;
  static void addScarePoint(int x, int y, float life = SCARE_LIFE);
  static void updateScarePoints(float elapsed);
  static void recomputeHerds();

 protected:
  static TCODList<ScarePoint*> scare;
  TCODList<Creature*> herd;  // current herd for this creature
};

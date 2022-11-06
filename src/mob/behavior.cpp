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
#include "mob/behavior.hpp"

#include <math.h>

#include "main.hpp"
#include "map/dungeon.hpp"

namespace mob {
TCODList<ScarePoint*> HerdBehavior::scare;

#define FOLLOW_DIST 5

float WaterOnlyWalkPattern::getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const {
  map::Dungeon* dungeon = gameEngine->dungeon;
  if (!dungeon->map->isWalkable(xTo, yTo)) return 0.0f;
  if (!dungeon->hasRipples(xTo, yTo)) return 0.0f;
  return 1.0f;
}

float AvoidWaterWalkPattern::getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const {
  map::Dungeon* dungeon = gameEngine->dungeon;
  if (!dungeon->map->isWalkable(xTo, yTo)) return 0.0f;
  float cost = map::terrainTypes[dungeon->getTerrainType(xTo, yTo)].walkCost;
  if (dungeon->hasRipples(xTo, yTo)) cost *= 3;  // try to avoid getting wet!
  return cost;
}

bool FollowBehavior::update(Creature* crea, float elapsed) {
  int pdist = (int)crea->distance(*leader_);
  map::Dungeon* dungeon = gameEngine->dungeon;
  standDelay += elapsed;
  if ((pdist > FOLLOW_DIST || standDelay > 10.0f) && (!crea->path_ || crea->path_->isEmpty())) {
    // go near the leader
    int destx = (int)(leader_->x_ + TCODRandom::getInstance()->getInt(-FOLLOW_DIST, FOLLOW_DIST));
    int desty = (int)(leader_->y_ + TCODRandom::getInstance()->getInt(-FOLLOW_DIST, FOLLOW_DIST));
    destx = CLAMP(0, dungeon->width - 1, destx);
    desty = CLAMP(0, dungeon->height - 1, desty);
    dungeon->getClosestWalkable(&destx, &desty, true, true, false);
    if (!crea->path_) {
      crea->path_ = new TCODPath(dungeon->width, dungeon->height, walkPattern, NULL);
    }
    crea->path_->compute((int)crea->x_, (int)crea->y_, destx, desty);
    crea->path_timer_ = 0.0f;
  } else {
    if (crea->walk(elapsed)) {
      standDelay = 0.0f;
    }
  }
  return true;
}

#define SCARE_RANGE 10.0f
// range below which fishes try to get away from each other
#define CLOSE_RANGE 2.0f
// range below which fishes try to get closer from each other
#define FAR_RANGE 10.0f

void HerdBehavior::recomputeHerds() {}

void HerdBehavior::updateScarePoints(float elapsed) {
  for (ScarePoint** spit = scare.begin(); spit != scare.end(); spit++) {
    (*spit)->life -= elapsed;
    if ((*spit)->life <= 0.0f) {
      delete *spit;
      spit = scare.remove(spit);
    }
  }
}

bool HerdBehavior::update(Creature* crea1, float elapsed) {
  TCODList<Creature*>* herd = &Creature::creatureByType[crea1->type_];
  //	printf ("=> %d\n",crea1);
  for (Creature** f2 = herd->begin(); f2 != herd->end(); f2++) {
    Creature* crea2 = *f2;
    if (crea1 != crea2) {
      float dx = crea2->x_ - crea1->x_;
      if (fabs(dx) >= FAR_RANGE) continue;
      float dy = crea2->y_ - crea1->y_;
      if (fabs(dy) >= FAR_RANGE) continue;  // too far to interact
      float invDist = crea1->fastInvDistance(*crea2);
      //	printf ("==> %d\n",crea2);
      if (invDist > 1E4f) {
      } else if (invDist > 1.0f / CLOSE_RANGE) {
        // get away from other creature
        crea1->dx_ -= elapsed * 5.0f * dx * invDist;
        crea1->dy_ -= elapsed * 5.0f * dy * invDist;
      } else if (invDist > 1.0f / FAR_RANGE) {
        // get closer to other creature
        crea1->dx_ += elapsed * 1.2f * dx * invDist;
        crea1->dy_ += elapsed * 1.2f * dy * invDist;
      }
    }
  }

  crea1->dx_ = CLAMP(-crea1->speed_, crea1->speed_, crea1->dx_);
  crea1->dy_ = CLAMP(-crea1->speed_, crea1->speed_, crea1->dy_);
  // interaction with scare points
  for (ScarePoint** spit = scare.begin(); spit != scare.end(); spit++) {
    float dx = (*spit)->x_ - crea1->x_;
    float dy = (*spit)->y_ - crea1->y_;
    float dist = base::Entity::fastInvSqrt(dx * dx + dy * dy);
    if (dist < 1E4f && dist > 1.0f / SCARE_RANGE) {
      float coef = (SCARE_RANGE - 1.0f / dist) * SCARE_RANGE;
      crea1->dx_ -= elapsed * crea1->speed_ * 10 * coef * dx * dist;
      crea1->dy_ -= elapsed * crea1->speed_ * 10 * coef * dy * dist;
    }
  }
  crea1->dx_ = CLAMP(-crea1->speed_ * 2, crea1->speed_ * 2, crea1->dx_);
  crea1->dy_ = CLAMP(-crea1->speed_ * 2, crea1->speed_ * 2, crea1->dy_);

  float newx = crea1->x_ + crea1->dx_;
  float newy = crea1->y_ + crea1->dy_;
  map::Dungeon* dungeon = gameEngine->dungeon;
  newx = CLAMP(0.0, dungeon->width - 1, newx);
  newy = CLAMP(0.0, dungeon->height - 1, newy);
  crea1->walk_timer_ += elapsed;
  if ((int)crea1->x_ != (int)newx || (int)crea1->y_ != (int)newy) {
    if (dungeon->isCellWalkable(newx, newy)) {
      map::TerrainId terrainId = gameEngine->dungeon->getTerrainType((int)newx, (int)newy);
      float walkTime = map::terrainTypes[terrainId].walkCost / crea1->speed_;
      if (crea1->walk_timer_ >= walkTime) {
        crea1->walk_timer_ = 0;
        dungeon->moveCreature(crea1, (int)crea1->x_, (int)crea1->y_, (int)newx, (int)newy);
        crea1->x_ = newx;
        crea1->y_ = newy;
      }
    }
  }
  return true;
}

void HerdBehavior::addScarePoint(int x, int y, float life) { scare.push(new ScarePoint(x, y, life)); }
}  // namespace mob

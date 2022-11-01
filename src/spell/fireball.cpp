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
#include "spell/fireball.hpp"

#include <fmt/core.h>
#include <math.h>
#include <stdio.h>

#include "main.hpp"

namespace spell {
float FireBall::incanRange = 0.0f;
float FireBall::incanLife = 0.0f;
float FireBall::sparkleSpeed = 0.0f;
int FireBall::nbSparkles = 0;
float FireBall::damage = 0;
float FireBall::range = 0;
bool FireBall::sparkleThrough = false;
bool FireBall::sparkleBounce = false;
bool FireBall::incandescence = false;
bool FireBall::sparkle = false;
TCODList<FireBall*> FireBall::incandescences;

FireBall::FireBall(float xFrom, float yFrom, int xTo, int yTo, FireBallType type, const char* subtype) : type(type) {
  if (damage == 0) {
    damage = config.getFloatProperty("config.spells.fireball.baseDamage");
    range = config.getFloatProperty("config.spells.fireball.baseRange");
  }
  x = xFrom;
  fx_ = x;
  y = yFrom;
  fy_ = y;

  light.x = x * 2;
  light.y = y * 2;

  dx_ = xTo - xFrom;
  dy_ = yTo - yFrom;
  float l = 1.0f / sqrt(dx_ * dx_ + dy_ * dy_);
  dx_ *= l;
  dy_ *= l;
  gameEngine->dungeon->addLight(&light);

  type_data_ = getType(subtype);
  light.color = type_data_->lightColor;
  light.range = type_data_->lightRange * range;
  light.randomRad = type_data_->lightRandomRad;
}

FireBall::Type* FireBall::getType(const char* name) {
  static TCODList<const char*> names;
  static TCODList<Type*> types;
  int i = 0;
  for (const char** it = names.begin(); it != names.end(); it++, i++) {
    if (strcmp(name, *it) == 0) return types.get(i);
  }
  Type* type = new Type();
  // this sucks. libtcod parser should have variadic functions
  std::string buf = fmt::format("config.spells.{}.trailLength", name);
  type->trailLength = config.getIntProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.lightColor", name);
  type->lightColor = getHDRColorProperty(config, buf.c_str());

  buf = fmt::format("config.spells.{}.lightRange", name);
  type->lightRange = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.lightRandomRad", name);
  type->lightRandomRad = config.getBoolProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.speed", name);
  type->speed = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.standardLife", name);
  type->standardLife = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.sparkLife", name);
  type->sparkLife = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.sparkleLife", name);
  type->sparkleLife = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.sparkleSpeed", name);
  type->sparkleSpeed = config.getFloatProperty(buf.c_str());

  buf = fmt::format("config.spells.{}.stunDelay", name);
  type->stunDelay = config.getFloatProperty(buf.c_str());

  names.push(strdup(name));
  types.push(type);
  return type;
}

FireBall::~FireBall() { gameEngine->dungeon->removeLight(&light); }

void FireBall::render(map::LightMap& lightMap) {
  if (effect == FIREBALL_MOVE) {
    float curx = fx_ * 2 - gameEngine->xOffset * 2;
    float cury = fy_ * 2 - gameEngine->yOffset * 2;
    map::HDRColor col = type_data_->lightColor;
    for (int i = 0; i < type_data_->trailLength; i++) {
      int icurx = (int)curx;
      int icury = (int)cury;
      if (IN_RECTANGLE(icurx, icury, lightMap.width, lightMap.height)) {
        map::HDRColor lcol = lightMap.getColor2x(icurx, icury);
        lcol = lcol + col;
        lightMap.setColor2x(icurx, icury, lcol);
      }
      curx -= dx_;
      cury -= dy_;
      col = col * 0.8f;
    }
  } else if (effect == FIREBALL_SPARKLE) {
    for (Sparkle** it = sparkles_.begin(); it != sparkles_.end(); it++) {
      int lmx = (int)((*it)->x) - gameEngine->xOffset * 2;
      int lmy = (int)((*it)->y) - gameEngine->yOffset * 2;
      if (IN_RECTANGLE(lmx, lmy, lightMap.width, lightMap.height)) {
        if (gameEngine->dungeon->map2x->isInFov((int)((*it)->x), (int)((*it)->y))) {
          map::HDRColor lcol = lightMap.getColor2x(lmx, lmy);
          lcol = lcol + light.color;
          lightMap.setColor2x(lmx, lmy, lcol);
        }
      }
    }
  }
}

void FireBall::render(TCODImage& ground) {
  if (effect == FIREBALL_MOVE) {
    float curx = fx_ * 2 - gameEngine->xOffset * 2;
    float cury = fy_ * 2 - gameEngine->yOffset * 2;
    TCODColor col = type_data_->lightColor;
    for (int i = 0; i < type_data_->trailLength; i++) {
      int icurx = (int)curx;
      int icury = (int)cury;
      if (IN_RECTANGLE(icurx, icury, CON_W * 2, CON_H * 2)) {
        TCODColor lcol = ground.getPixel(icurx, icury);
        lcol = lcol + col;
        ground.putPixel(icurx, icury, lcol);
      }
      curx -= dx_;
      cury -= dy_;
      col = col * 0.8f;
    }
  } else if (effect == FIREBALL_SPARKLE) {
    for (Sparkle** it = sparkles_.begin(); it != sparkles_.end(); it++) {
      int lmx = (int)((*it)->x) - gameEngine->xOffset * 2;
      int lmy = (int)((*it)->y) - gameEngine->yOffset * 2;
      if (IN_RECTANGLE(lmx, lmy, CON_W * 2, CON_H * 2)) {
        if (gameEngine->dungeon->map2x->isInFov((int)((*it)->x), (int)((*it)->y))) {
          TCODColor lcol = ground.getPixel(lmx, lmy);
          lcol = lcol + light.color;
          ground.putPixel(lmx, lmy, lcol);
        }
      }
    }
  }
}

bool FireBall::updateMove(float elapsed) {
  base::GameEngine* game = gameEngine;
  map::Dungeon* dungeon = game->dungeon;
  int oldx = (int)x;
  int oldy = (int)y;
  fx_ += dx_ * type_data_->speed;
  fy_ += dy_ * type_data_->speed;
  x = (int)fx_;
  y = (int)fy_;
  light.x = x * 2;
  light.y = y * 2;
  if (type == FB_SPARK) {
    fx_life_ -= elapsed / type_data_->sparkLife;
    if (fx_life_ < 0.0f) return false;
  }

  // check if we hit a wall
  TCODLine::init(oldx, oldy, (int)x, (int)y);
  int oldoldx = oldx;
  int oldoldy = oldy;
  while (!TCODLine::step(&oldx, &oldy)) {
    bool end = false, wallhit = false;
    if (!IN_RECTANGLE(oldx, oldy, dungeon->width, dungeon->height)) {
      // wall hit
      // last ground cell before the wall
      x = oldoldx;
      y = oldoldy;
      end = true;
      wallhit = true;
    } else {
      static int deltax[] = {0, 1, -1, 0, 0};
      static int deltay[] = {0, 0, 0, 1, -1};
      if (type == FB_STANDARD) {
        for (FireBall** fb = gameEngine->fireballs.begin(); fb != gameEngine->fireballs.end(); fb++) {
          if ((*fb)->effect == FIREBALL_MOVE && (*fb)->type == FB_INCANDESCENCE &&
              ABS((*fb)->x - x) < (*fb)->light.range / 2 && ABS((*fb)->y - y) < (*fb)->light.range / 2) {
            float newdx = (*fb)->dx_;
            float newdy = (*fb)->dy_;
            float angle = atan2f(newdy, newdx);
            angle += 0.78f;
            (*fb)->dx_ = cosf(angle);
            (*fb)->dy_ = sinf(angle);
            type = FB_INCANDESCENCE;
            type_data_ = getType("fireball2");
            light.color = type_data_->lightColor;
            light.range = type_data_->lightRange * range;
            light.randomRad = type_data_->lightRandomRad;
            angle -= 2 * 0.78f;
            dx_ = cosf(angle);
            dy_ = sinf(angle);
            x = (*fb)->x;
            y = (*fb)->y;
            end = true;
            break;
          }
        }
      }
      if (end) return true;
      if (dungeon->hasCreature(oldx, oldy) || dungeon->hasCreature(oldx + 1, oldy) ||
          dungeon->hasCreature(oldx - 1, oldy) || dungeon->hasCreature(oldx, oldy + 1) ||
          dungeon->hasCreature(oldx, oldy - 1)) {
        // creature hit
        for (int i = 0; i < 5; i++) {
          mob::Creature* cr = dungeon->getCreature(oldx + deltax[i], oldy + deltay[i]);
          if (cr) {
            float dmg = TCODRandom::getInstance()->getFloat(damage / 2, damage);
            if (type == FB_BURST) dmg *= 4;
            cr->takeDamage(dmg);
            cr->stun(type_data_->stunDelay);
            end = true;
          }
        }
        x = oldx;
        y = oldy;
      }
      if (dungeon->hasItem(oldx, oldy) || dungeon->hasItem(oldx + 1, oldy) || dungeon->hasItem(oldx - 1, oldy) ||
          dungeon->hasItem(oldx, oldy + 1) || dungeon->hasItem(oldx, oldy - 1)) {
        // item hit
        for (int i = 0; i < 5; i++) {
          auto* items = dungeon->getItems(oldx + deltax[i], oldy + deltay[i]);
          if (items && items->size() > 0) {
            for (item::Item* it : *items) {
              item::ItemFeature* feat = it->getFeature(item::ITEM_FEAT_FIRE_EFFECT);
              if (feat) {
                float dmg = TCODRandom::getInstance()->getFloat(damage / 2, damage);
                it->fire_resistance_ -= dmg;
              }
              end = true;
            }
          }
        }
      }
      if (!end && !dungeon->map->isWalkable(oldx, oldy)) {
        // wall hit
        // last ground cell before the wall
        x = oldoldx;
        y = oldoldy;
        end = true;
        wallhit = true;
      }
      if (end && dungeon->hasRipples(x, y)) {
        gameEngine->startRipple((int)x, (int)y, damage / 5);
      }
    }
    if (end) {
      light.x = x * 2;
      light.y = y * 2;
      // start effect
      fx_life_ = 1.0f;
      switch (type) {
        case FB_SPARK:
          return false;
          break;
        case FB_STANDARD:
          effect = FIREBALL_STANDARD;
          break;
        case FB_INCANDESCENCE:
          effect = FIREBALL_TORCH;
          light.color = light.color * 1.5f;
          current_range_ = 0.0f;
          incandescences.push(this);
          break;
        case FB_BURST:
          effect = FIREBALL_SPARKLE;
          for (int i = 0; i < nbSparkles; i++) {
            Sparkle* sparkle = new Sparkle();
            sparkle->x = x * 2;
            sparkle->y = y * 2;
            float sparkleAngle = atan2f(-dy_, -dx_);
            if (wallhit) {
              sparkleAngle += TCODRandom::getInstance()->getFloat(-1.5f, 1.5f);
            } else {
              sparkleAngle += TCODRandom::getInstance()->getFloat(-M_PI, M_PI);
            }
            sparkle->dx = cosf(sparkleAngle) * sparkleSpeed;
            sparkle->dy = sinf(sparkleAngle) * sparkleSpeed;
            if (IN_RECTANGLE(sparkle->x, sparkle->y, dungeon->width * 2, dungeon->height * 2)) {
              sparkles_.push(sparkle);
            } else {
              delete sparkle;
            }
          }
          break;
      }
      break;  // exit bresenham loop
    } else {
      oldoldx = oldx;
      oldoldy = oldy;
    }
  }
  return true;
}

bool FireBall::updateStandard(float elapsed) {
  fx_life_ -= elapsed * type_data_->standardLife;
  if (fx_life_ < 0.0f) return false;
  light.range = range * (3.0 - 2 * fx_life_);
  light.color = type_data_->lightColor * fx_life_;
  return true;
}

bool FireBall::updateTorch(float elapsed) {
  base::GameEngine* game = gameEngine;
  map::Dungeon* dungeon = game->dungeon;
  float f;
  fx_life_ -= elapsed / incanLife;
  if (fx_life_ < 0.0f) {
    incandescences.removeFast(this);
    return false;
  }
  f = noiseOffset + fx_life_ * 250.0f;
  float var = 0.2 * (4.0f + noise1d.get(&f));
  current_range_ = incanRange * (2.0 - fx_life_) * var;
  light.range = 2 * current_range_;
  /*
  for (mob::Creature **cr=dungeon->creatures.begin(); cr != dungeon->creatures.end(); cr++) {
          if ( ABS((*cr)->x-x)<curRange && ABS((*cr)->y-y)< curRange ) {
                  // do not set fire through walls
                  if ( dungeon->hasLos((int)((*cr)->x),(int)((*cr)->y),(int)x,(int)y,true) ) (*cr)->burn=true;
          }
  }
  */
  heat_timer_ += elapsed;
  if (heat_timer_ > 1.0f) {
    // warm up adjacent items
    heat_timer_ = 0.0f;
    float radius = current_range_;
    for (int tx = -(int)floor(radius); tx <= (int)ceil(radius); tx++) {
      if ((int)(x) + tx >= 0 && (int)(x) + tx < dungeon->width) {
        int dy = (int)(sqrtf(radius * radius - tx * tx));
        for (int ty = -dy; ty <= dy; ty++) {
          if ((int)(y) + ty >= 0 && (int)(y) + ty < dungeon->height) {
            auto* items = dungeon->getItems((int)(x) + tx, (int)(y) + ty);
            for (item::Item* it : *items) {
              // found an adjacent item
              item::ItemFeature* fireFeat = it->getFeature(item::ITEM_FEAT_FIRE_EFFECT);
              if (fireFeat) {
                // item is affected by fire
                it->fire_resistance_ -= damage / 4;
              }
            }
            mob::Creature* cr = dungeon->getCreature((int)(x) + tx, (int)(y) + ty);
            if (cr) {
              cr->burn = true;
              cr->takeDamage(damage / 4);
            }
          }
        }
      }
    }
  }
  if (fx_life_ < 0.25f) {
    light.color = type_data_->lightColor * fx_life_ * 4;
  }
  return true;
}

bool FireBall::updateSparkle(float elapsed) {
  base::GameEngine* game = gameEngine;
  map::Dungeon* dungeon = game->dungeon;
  bool firstFrame = (fx_life_ == 1.0f);
  fx_life_ -= elapsed / type_data_->sparkleLife;
  if (fx_life_ < 0.0f) {
    sparkles_.clearAndDelete();
    return false;
  }
  if (firstFrame) {
    // burst flash
    light.range = range * 4.0;
    light.color = type_data_->lightColor * 3.0f;
  } else {
    light.range = range * (3.0 - 2 * fx_life_);
    light.color = type_data_->lightColor * fx_life_;
  }
  for (Sparkle** it = sparkles_.begin(); it != sparkles_.end(); it++) {
    Sparkle* sparkle = *it;
    float speed = type_data_->sparkleSpeed * fx_life_;
    sparkle->x += sparkle->dx * speed;
    sparkle->y += sparkle->dy * speed;
    int dungeonx = (int)(sparkle->x) / 2;
    int dungeony = (int)(sparkle->y) / 2;
    mob::Creature* cr = NULL;
    bool del = false;
    for (FireBall** fb = incandescences.begin(); fb != incandescences.end(); fb++) {
      if (ABS((*fb)->x - sparkle->x / 2) < (*fb)->current_range_ &&
          ABS((*fb)->y - sparkle->y / 2) < (*fb)->current_range_) {
        FireBall* fb = new FireBall(
            sparkle->x / 2,
            sparkle->y / 2,
            (int)(sparkle->x / 2 + sparkle->dx * 10),
            (int)(sparkle->y / 2 + sparkle->dy * 10),
            FB_STANDARD);
        gameEngine->addFireball(fb);
        it = sparkles_.removeFast(it);
        delete sparkle;
        del = true;
        break;
      }
    }
    if (del) continue;
    if (dungeon->hasCreature(dungeonx, dungeony)) {
      cr = dungeon->getCreature(dungeonx, dungeony);
      cr->takeDamage(TCODRandom::getInstance()->getFloat(damage, 3 * damage / 2));
    }
    if ((cr && !sparkleThrough) || !IN_RECTANGLE(sparkle->x, sparkle->y, dungeon->width * 2, dungeon->height * 2) ||
        (!sparkleBounce && !dungeon->map->isWalkable(dungeonx, dungeony))) {
      // sparkle hit an obstacle. delete it
      it = sparkles_.removeFast(it);
      delete sparkle;
    } else if (sparkleBounce && !dungeon->map->isWalkable(dungeonx, dungeony)) {
      // sparkle bounces
      int oldx = (int)(sparkle->x - sparkle->dx * speed) / 2;
      int oldy = (int)(sparkle->y - sparkle->dy * speed) / 2;
      int newx = dungeonx;
      int newy = dungeony;
      int cdx = newx - oldx;
      int cdy = newy - oldy;
      if (cdx == 0) {
        // hit horizontal wall
        sparkle->dy = -sparkle->dy;
      } else if (cdy == 0) {
        // hit vertical wall
        sparkle->dx = -sparkle->dx;
      } else {
        bool xwalk = dungeon->map->isWalkable(oldx + cdx, oldy);
        bool ywalk = dungeon->map->isWalkable(oldx, oldy + cdy);
        if (xwalk && ywalk) {
          // outer corner bounce. detect which side of the cell is hit
          // TODO : this does not work
          float fdx = newx + 0.5f - (sparkle->x - sparkle->dx * speed);
          float fdy = newy + 0.5f - (sparkle->y - sparkle->dy * speed);
          fdx = ABS(fdx);
          fdy = ABS(fdy);
          if (fdx >= fdy) sparkle->dx = -sparkle->dx;
          if (fdy >= fdx) sparkle->dy = -sparkle->dy;
        } else if (!xwalk) {
          if (ywalk) {
            // vertical wall bounce
            sparkle->dx = -sparkle->dx;
          } else {
            // inner corner bounce
            sparkle->dx = -sparkle->dx;
            sparkle->dy = -sparkle->dy;
          }
        } else {
          // horizontal wall bounce
          sparkle->dy = -sparkle->dy;
        }
      }
    }
  }
  return true;
}

bool FireBall::update(float elapsed) {
  switch (effect) {
    case FIREBALL_MOVE:
      return updateMove(elapsed);
      break;
    case FIREBALL_TORCH:
      return updateTorch(elapsed);
      break;
    case FIREBALL_SPARKLE:
      return updateSparkle(elapsed);
      break;
    case FIREBALL_STANDARD:
      return updateStandard(elapsed);
      break;
  }
  return true;
}
}  // namespace spell

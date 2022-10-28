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
#include <math.h>
#include <stdio.h>

#include "main.hpp"
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
  fx = x;
  y = yFrom;
  fy = y;

  light.x = x * 2;
  light.y = y * 2;

  dx = xTo - xFrom;
  dy = yTo - yFrom;
  float l = 1.0f / sqrt(dx * dx + dy * dy);
  dx *= l;
  dy *= l;
  effect = FIREBALL_MOVE;
  gameEngine->dungeon->addLight(&light);
  fxLife = 1.0f;

  typeData = getType(subtype);
  light.color = typeData->lightColor;
  light.range = typeData->lightRange * range;
  light.randomRad = typeData->lightRandomRad;
  heatTimer = 0.0f;
}

FireBall::Type* FireBall::getType(const char* name) {
  static TCODList<const char*> names;
  static TCODList<Type*> types;
  int i = 0;
  for (const char** it = names.begin(); it != names.end(); it++, i++) {
    if (strcmp(name, *it) == 0) return types.get(i);
  }
  Type* type = new Type();
  char buf[128];
  // this sucks. libtcod parser should have variadic functions
  sprintf(buf, "config.spells.%s.trailLength", name);
  type->trailLength = config.getIntProperty(buf);

  sprintf(buf, "config.spells.%s.lightColor", name);
  type->lightColor = getHDRColorProperty(config, buf);

  sprintf(buf, "config.spells.%s.lightRange", name);
  type->lightRange = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.lightRandomRad", name);
  type->lightRandomRad = config.getBoolProperty(buf);

  sprintf(buf, "config.spells.%s.speed", name);
  type->speed = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.standardLife", name);
  type->standardLife = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.sparkLife", name);
  type->sparkLife = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.sparkleLife", name);
  type->sparkleLife = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.sparkleSpeed", name);
  type->sparkleSpeed = config.getFloatProperty(buf);

  sprintf(buf, "config.spells.%s.stunDelay", name);
  type->stunDelay = config.getFloatProperty(buf);

  names.push(strdup(name));
  types.push(type);
  return type;
}

FireBall::~FireBall() { gameEngine->dungeon->removeLight(&light); }

void FireBall::render(LightMap* lightMap) {
  if (effect == FIREBALL_MOVE) {
    float curx = fx * 2 - gameEngine->xOffset * 2;
    float cury = fy * 2 - gameEngine->yOffset * 2;
    HDRColor col = typeData->lightColor;
    for (int i = 0; i < typeData->trailLength; i++) {
      int icurx = (int)curx;
      int icury = (int)cury;
      if (IN_RECTANGLE(icurx, icury, lightMap->width, lightMap->height)) {
        HDRColor lcol = lightMap->getColor2x(icurx, icury);
        lcol = lcol + col;
        lightMap->setColor2x(icurx, icury, lcol);
      }
      curx -= dx;
      cury -= dy;
      col = col * 0.8f;
    }
  } else if (effect == FIREBALL_SPARKLE) {
    for (Sparkle** it = sparkles.begin(); it != sparkles.end(); it++) {
      int lmx = (int)((*it)->x) - gameEngine->xOffset * 2;
      int lmy = (int)((*it)->y) - gameEngine->yOffset * 2;
      if (IN_RECTANGLE(lmx, lmy, lightMap->width, lightMap->height)) {
        if (gameEngine->dungeon->map2x->isInFov((int)((*it)->x), (int)((*it)->y))) {
          HDRColor lcol = lightMap->getColor2x(lmx, lmy);
          lcol = lcol + light.color;
          lightMap->setColor2x(lmx, lmy, lcol);
        }
      }
    }
  }
}

void FireBall::render(TCODImage* ground) {
  if (effect == FIREBALL_MOVE) {
    float curx = fx * 2 - gameEngine->xOffset * 2;
    float cury = fy * 2 - gameEngine->yOffset * 2;
    TCODColor col = typeData->lightColor;
    for (int i = 0; i < typeData->trailLength; i++) {
      int icurx = (int)curx;
      int icury = (int)cury;
      if (IN_RECTANGLE(icurx, icury, CON_W * 2, CON_H * 2)) {
        TCODColor lcol = ground->getPixel(icurx, icury);
        lcol = lcol + col;
        ground->putPixel(icurx, icury, lcol);
      }
      curx -= dx;
      cury -= dy;
      col = col * 0.8f;
    }
  } else if (effect == FIREBALL_SPARKLE) {
    for (Sparkle** it = sparkles.begin(); it != sparkles.end(); it++) {
      int lmx = (int)((*it)->x) - gameEngine->xOffset * 2;
      int lmy = (int)((*it)->y) - gameEngine->yOffset * 2;
      if (IN_RECTANGLE(lmx, lmy, CON_W * 2, CON_H * 2)) {
        if (gameEngine->dungeon->map2x->isInFov((int)((*it)->x), (int)((*it)->y))) {
          TCODColor lcol = ground->getPixel(lmx, lmy);
          lcol = lcol + light.color;
          ground->putPixel(lmx, lmy, lcol);
        }
      }
    }
  }
}

bool FireBall::updateMove(float elapsed) {
  GameEngine* game = gameEngine;
  Dungeon* dungeon = game->dungeon;
  int oldx = (int)x;
  int oldy = (int)y;
  fx += dx * typeData->speed;
  fy += dy * typeData->speed;
  x = (int)fx;
  y = (int)fy;
  light.x = x * 2;
  light.y = y * 2;
  if (type == FB_SPARK) {
    fxLife -= elapsed / typeData->sparkLife;
    if (fxLife < 0.0f) return false;
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
            float newdx = (*fb)->dx;
            float newdy = (*fb)->dy;
            float angle = atan2f(newdy, newdx);
            angle += 0.78f;
            (*fb)->dx = cosf(angle);
            (*fb)->dy = sinf(angle);
            type = FB_INCANDESCENCE;
            typeData = getType("fireball2");
            light.color = typeData->lightColor;
            light.range = typeData->lightRange * range;
            light.randomRad = typeData->lightRandomRad;
            angle -= 2 * 0.78f;
            dx = cosf(angle);
            dy = sinf(angle);
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
          Creature* cr = dungeon->getCreature(oldx + deltax[i], oldy + deltay[i]);
          if (cr) {
            float dmg = TCODRandom::getInstance()->getFloat(damage / 2, damage);
            if (type == FB_BURST) dmg *= 4;
            cr->takeDamage(dmg);
            cr->stun(typeData->stunDelay);
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
          TCODList<Item*>* items = dungeon->getItems(oldx + deltax[i], oldy + deltay[i]);
          if (items && items->size() > 0) {
            for (Item** it = items->begin(); it != items->end(); it++) {
              ItemFeature* feat = (*it)->getFeature(ITEM_FEAT_FIRE_EFFECT);
              if (feat) {
                float dmg = TCODRandom::getInstance()->getFloat(damage / 2, damage);
                (*it)->fireResistance -= dmg;
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
      fxLife = 1.0f;
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
          curRange = 0.0f;
          incandescences.push(this);
          break;
        case FB_BURST:
          effect = FIREBALL_SPARKLE;
          for (int i = 0; i < nbSparkles; i++) {
            Sparkle* sparkle = new Sparkle();
            sparkle->x = x * 2;
            sparkle->y = y * 2;
            float sparkleAngle = atan2f(-dy, -dx);
            if (wallhit) {
              sparkleAngle += TCODRandom::getInstance()->getFloat(-1.5f, 1.5f);
            } else {
              sparkleAngle += TCODRandom::getInstance()->getFloat(-M_PI, M_PI);
            }
            sparkle->dx = cosf(sparkleAngle) * sparkleSpeed;
            sparkle->dy = sinf(sparkleAngle) * sparkleSpeed;
            if (IN_RECTANGLE(sparkle->x, sparkle->y, dungeon->width * 2, dungeon->height * 2)) {
              sparkles.push(sparkle);
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
  fxLife -= elapsed * typeData->standardLife;
  if (fxLife < 0.0f) return false;
  light.range = range * (3.0 - 2 * fxLife);
  light.color = typeData->lightColor * fxLife;
  return true;
}

bool FireBall::updateTorch(float elapsed) {
  GameEngine* game = gameEngine;
  Dungeon* dungeon = game->dungeon;
  float f;
  fxLife -= elapsed / incanLife;
  if (fxLife < 0.0f) {
    incandescences.removeFast(this);
    return false;
  }
  f = noiseOffset + fxLife * 250.0f;
  float var = 0.2 * (4.0f + noise1d.get(&f));
  curRange = incanRange * (2.0 - fxLife) * var;
  light.range = 2 * curRange;
  /*
  for (Creature **cr=dungeon->creatures.begin(); cr != dungeon->creatures.end(); cr++) {
          if ( ABS((*cr)->x-x)<curRange && ABS((*cr)->y-y)< curRange ) {
                  // do not set fire through walls
                  if ( dungeon->hasLos((int)((*cr)->x),(int)((*cr)->y),(int)x,(int)y,true) ) (*cr)->burn=true;
          }
  }
  */
  heatTimer += elapsed;
  if (heatTimer > 1.0f) {
    // warm up adjacent items
    heatTimer = 0.0f;
    float radius = curRange;
    for (int tx = -(int)floor(radius); tx <= (int)ceil(radius); tx++) {
      if ((int)(x) + tx >= 0 && (int)(x) + tx < dungeon->width) {
        int dy = (int)(sqrtf(radius * radius - tx * tx));
        for (int ty = -dy; ty <= dy; ty++) {
          if ((int)(y) + ty >= 0 && (int)(y) + ty < dungeon->height) {
            TCODList<Item*>* items = dungeon->getItems((int)(x) + tx, (int)(y) + ty);
            for (Item** it = items->begin(); it != items->end(); it++) {
              // found an adjacent item
              ItemFeature* fireFeat = (*it)->getFeature(ITEM_FEAT_FIRE_EFFECT);
              if (fireFeat) {
                // item is affected by fire
                (*it)->fireResistance -= damage / 4;
              }
            }
            Creature* cr = dungeon->getCreature((int)(x) + tx, (int)(y) + ty);
            if (cr) {
              cr->burn = true;
              cr->takeDamage(damage / 4);
            }
          }
        }
      }
    }
  }
  if (fxLife < 0.25f) {
    light.color = typeData->lightColor * fxLife * 4;
  }
  return true;
}

bool FireBall::updateSparkle(float elapsed) {
  GameEngine* game = gameEngine;
  Dungeon* dungeon = game->dungeon;
  bool firstFrame = (fxLife == 1.0f);
  fxLife -= elapsed / typeData->sparkleLife;
  if (fxLife < 0.0f) {
    sparkles.clearAndDelete();
    return false;
  }
  if (firstFrame) {
    // burst flash
    light.range = range * 4.0;
    light.color = typeData->lightColor * 3.0f;
  } else {
    light.range = range * (3.0 - 2 * fxLife);
    light.color = typeData->lightColor * fxLife;
  }
  for (Sparkle** it = sparkles.begin(); it != sparkles.end(); it++) {
    Sparkle* sparkle = *it;
    float speed = typeData->sparkleSpeed * fxLife;
    sparkle->x += sparkle->dx * speed;
    sparkle->y += sparkle->dy * speed;
    int dungeonx = (int)(sparkle->x) / 2;
    int dungeony = (int)(sparkle->y) / 2;
    Creature* cr = NULL;
    bool del = false;
    for (FireBall** fb = incandescences.begin(); fb != incandescences.end(); fb++) {
      if (ABS((*fb)->x - sparkle->x / 2) < (*fb)->curRange && ABS((*fb)->y - sparkle->y / 2) < (*fb)->curRange) {
        FireBall* fb = new FireBall(
            sparkle->x / 2,
            sparkle->y / 2,
            (int)(sparkle->x / 2 + sparkle->dx * 10),
            (int)(sparkle->y / 2 + sparkle->dy * 10),
            FB_STANDARD);
        gameEngine->addFireball(fb);
        it = sparkles.removeFast(it);
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
      it = sparkles.removeFast(it);
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

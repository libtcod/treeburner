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
#include <libtcod.hpp>

#include "map_lightmap.hpp"
#include "mob_creature.hpp"
#include "screen_school.hpp"

class Dungeon;

class Player : public Creature {
 public:
  Player();
  virtual ~Player();
  void init();
  bool setPath(int xDest, int yDest, bool limitPath = true);
  bool update(float elapsed, TCOD_key_t key, TCOD_mouse_t* mouse);
  void takeDamage(float amount);
  void termLevel();
  void heal(int healPoints);
  void render(LightMap* lightMap);
  void setLightRange(float range) { light.range = range; }
  void setLightColor(TCODColor col) { light.color = col; }
  float getHealing();
  float getHealth();
  static void getMoveKey(TCOD_key_t key, bool* up, bool* down, bool* left, bool* right);
  inline float getAverageSpeed() { return averageSpeed; }
  void computeStealth(float elapsed);

  // SaveListener
  bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip);
  void saveData(uint32_t chunkId, TCODZip* zip);

  float maxFovRange;
  School school;
  float stealth;
  bool crouch;

 protected:
  float healPoints, averageSpeed, speedElapsed, speedDist, sprintDelay;
  float lbuttonDelay, lWalkDelay, rbuttonDelay;
  bool lbutton, rbutton;
  float curHeal;
  Light light;
  bool up, down, left, right;
  ExtendedLight healLight;
  bool initDungeon;
  bool isSprinting;

  void computeFovRange(float elapsed);
  void computeAverageSpeed(float elapsed);
  void updateSprintDelay(float elapsed, bool isSprinting);
  void updateHealing(float elapsed);
  bool activateCell(int dungeonx, int dungeony, bool lbut_pressed, bool walk, bool* activated = NULL);
};

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

#include "base/entity.hpp"
#include "base/noisything.hpp"
#include "map/light.hpp"
#include "map/lightmap.hpp"

typedef enum { FB_SPARK, FB_STANDARD, FB_BURST, FB_INCANDESCENCE } FireBallType;

class FireBall : public base::Entity, public base::NoisyThing {
 public:
  Light light;  // light associated with this fireball

  static float incanRange;  // incandescence cloud radius
  static float incanLife;  // incandescence cloud life in seconds
  static float sparkleSpeed;  // sparkles speed (cell/second)
  static int nbSparkles;  // number of sparkles
  static float damage;  // fireball base damage
  static float range;  // fireball light range
  static bool sparkle;  // wether long rmb triggers sparkles
  static bool incandescence;  // wether long lmb triggers incandescence
  static bool sparkleThrough;  // wether sparkles can go though creatures
  static bool sparkleBounce;  // wether sparkles bounce against walls

  FireBall(float xFrom, float yFrom, int xTo, int yTo, FireBallType type, const char* subtype = "fireball");
  ~FireBall();

  void render(LightMap& lightMap);
  void render(TCODImage& ground);
  bool update(float elapsed);

 protected:
  // type data
  struct Type {
    int trailLength;
    HDRColor lightColor;
    float speed;
    float standardLife;
    float sparkLife;
    float sparkleLife;
    float sparkleSpeed;
    float stunDelay;
    float lightRange;
    bool lightRandomRad;
  };
  struct Sparkle {
    float x, y, dx, dy;
  };

  static TCODList<FireBall*> incandescences;

  Type* getType(const char* name);
  bool updateMove(float elapsed);
  bool updateStandard(float elapsed);
  bool updateTorch(float elapsed);
  bool updateSparkle(float elapsed);

  float dx_{}, dy_{};
  float fx_{}, fy_{};
  FireBallType type{};
  enum { FIREBALL_MOVE, FIREBALL_STANDARD, FIREBALL_TORCH, FIREBALL_SPARKLE } effect{FIREBALL_MOVE};
  float fx_life_{1.0f};
  float heat_timer_{};
  float current_range_{};
  Type* type_data_{};
  TCODList<Sparkle*> sparkles_{};
};

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
#include "map_lightmap.hpp"


class Light : public base::Entity, public base::NoisyThing {
 public:
  Light() : randomRad(false), range(0.0f), color{tcod::ColorRGB{255, 255, 255}} {}
  Light(float range, TCODColor color = TCODColor::white, bool randomRad = false)
      : randomRad(randomRad), range(range), color(color) {}
  void addToLightMap(LightMap& map);
  void addToImage(TCODImage& img);
  void getDungeonPart(int* minx, int* miny, int* maxx, int* maxy);
  virtual void update([[maybe_unused]] float elapsed) {}

  bool randomRad;
  float range;
  HDRColor color;

 protected:
  void add(LightMap* l, TCODImage* i);
  virtual float getIntensity() { return 1.0f; }
  virtual HDRColor getColor([[maybe_unused]] float rad) { return color; }
  float getFog(int x, int y);
};

class ExtendedLight : public Light {
 public:
  void setup(HDRColor outColor, float intensityPatternDelay, const char* intensityPattern, const char* colorPattern);
  void update(float elapsed) override;

 protected:
  HDRColor outColor;
  const char* intensityPattern = nullptr;
  const char* colorPattern = nullptr;
  float intensityPatternDelay;
  int intensityPatternLen;
  int colorPatternLen;
  float intensityTimer;
  bool noiseIntensity;

  float getIntensity() override;
  HDRColor getColor(float rad) override;
};

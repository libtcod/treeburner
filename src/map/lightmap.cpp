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
#include "map/lightmap.hpp"

#include "main.hpp"
#include "map/dungeon.hpp"

HDRColor operator*(float value, const HDRColor& c) { return c * value; }

LightMap::LightMap(int width, int height) : width(width), height(height) {
  data = new HDRColor[width * height / 4];
  data2x = new HDRColor[width * height];
  // initialise fog
  fogNoise = new TCODNoise(3);
  fogZ = 0.0f;
  fogRange = 0.0f;
}

void LightMap::clear(const TCODColor& col) {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      setColor2x(x, y, col);
    }
  }
}

void LightMap::update(float elapsed) {
  static float fogSpeed = config.getFloatProperty("config.fog.speed");

  fogZ += elapsed * fogSpeed;
}

float LightMap::getFog(int x, int y) {
  static float fogMaxLevel = config.getFloatProperty("config.fog.maxLevel");
  static float fogScale = config.getFloatProperty("config.fog.scale");
  static float fogOctaves = config.getFloatProperty("config.fog.octaves");
  static float coefx = fogScale / CON_W;
  static float coefy = fogScale / CON_H;

  float f[3] = {x * coefx, y * coefy, fogZ};
  return fogMaxLevel * 0.5f * (fogNoise->getFbm(f, fogOctaves) + 1.0f);
}

float LightMap::getPlayerFog(int x, int y) {
  if (fogRange == 0.0f) return 0.0f;
  float maxDistDiv = 1.0f / (fogRange * fogRange);
  float fogLevel = gameEngine->getFog(x, y);  // amount of fog, between 0 and fogMaxLevel
  float playerdx = gameEngine->player.x * 2 - x;
  float playerdy = gameEngine->player.y * 2 - y;
  float fogDist = playerdx * playerdx + playerdy * playerdy;  // distance from player
  float fogCoef = fogDist * maxDistDiv;
  fogCoef = MIN(1.0f, fogCoef);  // increase fog with distance
  fogLevel = fogLevel * fogCoef;
  return fogLevel;
}

// apply a light map to an image.
void LightMap::applyToImage(TCODImage& image, int minx2x, int miny2x, int maxx2x, int maxy2x, bool playerFog) {
  static TCODColor fogColor = config.getColorProperty("config.fog.col");
  static TCODColor memoryWallColor = config.getColorProperty("config.display.memoryWallColor");
  static int memoryWallIntensity = (int)(memoryWallColor.r) + memoryWallColor.g + memoryWallColor.b;
  static TCODColor wallColor = config.getColorProperty("config.display.wallColor");
  static TCODColor groundColor = config.getColorProperty("config.display.groundColor");
  Dungeon* dungeon = gameEngine->dungeon;
  if (maxx2x == 0) maxx2x = width - 1;
  if (maxy2x == 0) maxy2x = height - 1;
  for (int x = minx2x; x <= maxx2x; x++) {
    for (int y = miny2x; y < maxy2x; y++) {
      int dungeonx = x + gameEngine->xOffset * 2;
      int dungeony = y + gameEngine->yOffset * 2;
      if (!IN_RECTANGLE(dungeonx, dungeony, dungeon->width * 2, dungeon->height * 2)) {
        image.putPixel(x, y, TCODColor::black);  // out of the map
      } else {
        // visible cell. shade it
        TCODColor col = dungeon->getGroundColor(dungeonx, dungeony);  // wall?wallColor:groundColor;
        TCODColor lmcol = playerFog ? TCODColor::lerp(data2x[x + y * width], fogColor, getPlayerFog(dungeonx, dungeony))
                                    : (TCODColor)data2x[x + y * width];

        int lightIntensity = (int)(lmcol.r) + lmcol.g + lmcol.b;
        float coef = 1.0f;

        if (!dungeon->map2x->isInFov(dungeonx, dungeony) || lightIntensity < memoryWallIntensity) {
          if (dungeon->getMemory(dungeonx / 2, dungeony / 2) && !dungeon->map2x->isTransparent(dungeonx, dungeony)) {
            lmcol = memoryWallColor;
            col = TCODColor::white;
          }
        } else {
          // anti-aliased fov
          static int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
          static int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
          int cnt = 0;
          // count number of adjacent out of fov cells
          for (int i = 0; i < 8; i++) {
            int cx = dungeonx + dx[i];
            int cy = dungeony + dy[i];
            if (IN_RECTANGLE(cx, cy, dungeon->width * 2, dungeon->height * 2)) {
              if (!dungeon->map2x->isInFov(cx, cy)) cnt++;
            } else {
              cnt++;
            }
          }
          coef = 1.0f - cnt * 0.125f;
        }
        if (lightIntensity > 30) {
          dungeon->setMemory(dungeonx / 2, dungeony / 2);
        }
        image.putPixel(x, y, col * lmcol * coef);
      }
    }
  }
}

void LightMap::applyToImageOutdoor(TCODImage& image) {
  Dungeon* dungeon = gameEngine->dungeon;
  int maxx2x = width - 1;
  int maxy2x = height - 1;
  for (int x = 0; x <= maxx2x; x++) {
    for (int y = 0; y < maxy2x; y++) {
      int dungeonx = x + gameEngine->xOffset * 2;
      int dungeony = y + gameEngine->yOffset * 2;
      if (!IN_RECTANGLE(dungeonx, dungeony, dungeon->width * 2, dungeon->height * 2)) {
        image.putPixel(x, y, TCODColor::black);  // out of the map
      } else {
        // visible cell. shade it
        HDRColor col = image.getPixel(x, y);
        HDRColor lmcol = data2x[x + y * width];
        lmcol = lmcol * col;
        int lightIntensity = (int)(lmcol.r + lmcol.g + lmcol.b);
        image.putPixel(x, y, lmcol);
        if (lightIntensity > 30 && dungeon->map2x->isInFov(dungeonx, dungeony)) {
          dungeon->setMemory(dungeonx / 2, dungeony / 2);
        }
      }
    }
  }
}

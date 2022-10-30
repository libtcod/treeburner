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

// world generator
// this was mostly generated with libtcod 1.4.2 heightmap tool !
#include "util_worldgen.hpp"

#include <SDL.h>

#include <cmath>
#include <cstdio>

#include "main.hpp"

/// Workaround function until the time profiling code can be refactored to use uint64.
static auto getTime() -> float { return SDL_GetTicks64() * 0.001f; }

static constexpr auto WHITE = tcod::ColorRGB{255, 255, 255};
static constexpr auto BLACK = tcod::ColorRGB{255, 255, 255};

// temperature / precipitation Biome diagram (Whittaker diagram)
static constexpr EBiome biomeDiagram[5][5] = {
    // artic/alpine climate (below -5°C)
    {
        TUNDRA,
        TUNDRA,
        TUNDRA,
        TUNDRA,
        TUNDRA,
    },
    // cold climate (-5 / 5°C)
    {
        COLD_DESERT,
        GRASSLAND,
        BOREAL_FOREST,
        BOREAL_FOREST,
        BOREAL_FOREST,
    },
    // temperate climate (5 / 15 °C)
    {
        COLD_DESERT,
        GRASSLAND,
        TEMPERATE_FOREST,
        TEMPERATE_FOREST,
        TROPICAL_MONTANE_FOREST,
    },
    // warm climate (15 - 2°C)
    {
        HOT_DESERT,
        SAVANNA,
        TROPICAL_DRY_FOREST,
        TROPICAL_EVERGREEN_FOREST,
        TROPICAL_EVERGREEN_FOREST,
    },
    // tropical climate (above 20°C)
    {
        HOT_DESERT,
        THORN_FOREST,
        TROPICAL_DRY_FOREST,
        TROPICAL_EVERGREEN_FOREST,
        TROPICAL_EVERGREEN_FOREST,
    },
};

static constexpr auto sandHeight = 0.12f;
static constexpr auto grassHeight = 0.16f;  // 0.315f;
static constexpr auto rockHeight = 0.655f;
static constexpr auto snowHeight = 0.905f;  // 0.785f;
// TCOD's land color map
static constexpr auto MAX_COLOR_KEY = 10;
static constexpr auto COLOR_KEY_MAX_SEA = (static_cast<int>(sandHeight * 255) - 1);
static constexpr auto COLOR_KEY_MIN_LAND = (static_cast<int>(sandHeight * 255));
static constexpr int keyIndex[MAX_COLOR_KEY] = {
    0,
    COLOR_KEY_MAX_SEA,
    COLOR_KEY_MIN_LAND,
    static_cast<int>(grassHeight * 255),
    static_cast<int>(grassHeight * 255) + 10,
    static_cast<int>(rockHeight * 255),
    static_cast<int>(rockHeight * 255) + 10,
    static_cast<int>(snowHeight * 255),
    static_cast<int>(snowHeight * 255) + 10,
    255};
static constexpr TCODColor keyColor[MAX_COLOR_KEY] = {
    TCODColor(0, 0, 50),  // deep water
    TCODColor(20, 20, 200),  // water-sand transition
    TCODColor(134, 180, 101),  // sand
    TCODColor(80, 120, 10),  // sand-grass transition
    TCODColor(17, 109, 7),  // grass
    TCODColor(30, 85, 12),  // grass-rock transisiton
    TCODColor(64, 70, 20),  // rock
    TCODColor(120, 140, 40),  // rock-snow transisiton
    TCODColor(208, 208, 239),  // snow
    TCODColor(255, 255, 255)};

// altitude color map
static constexpr int MAX_ALT_KEY = 8;
static constexpr int altIndexes[MAX_ALT_KEY] = {
    0, 15, (int)(sandHeight * 255), (int)(sandHeight * 255) + 1, 80, 130, 195, 255};
static constexpr float altitudes[MAX_ALT_KEY] = {
    -2000, -1000, -100, 0, 500, 1000, 2500, 4000  // in meters
};
static constexpr TCODColor altColors[MAX_ALT_KEY] = {
    TCODColor(24, 165, 255),  // -2000
    TCODColor(132, 214, 255),  // -1000
    TCODColor(247, 255, 255),  // -100
    TCODColor(49, 149, 44),  // 0
    TCODColor(249, 209, 151),  // 500
    TCODColor(165, 148, 24),  // 1000
    TCODColor(153, 110, 6),  // 2500
    TCODColor(172, 141, 138),  // 4000
};

// precipitation color map
static constexpr int MAX_PREC_KEY = 19;
static constexpr int precIndexes[MAX_PREC_KEY] = {
    4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 50, 60, 70, 80, 100, 120, 140, 160, 255};
static constexpr float precipitations[MAX_PREC_KEY] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 15, 18, 20, 25, 30, 35, 40  // cm / m� / year
};
static constexpr TCODColor precColors[MAX_PREC_KEY] = {
    TCODColor(128, 0, 0),  // < 4
    TCODColor(173, 55, 0),  // 4-8
    TCODColor(227, 102, 0),  // 8-12
    TCODColor(255, 149, 0),  // 12-16
    TCODColor(255, 200, 0),  // 16-20
    TCODColor(255, 251, 0),  // 20-24
    TCODColor(191, 255, 0),  // 24-28
    TCODColor(106, 251, 0),  // 28-32
    TCODColor(25, 255, 48),  // 32-36
    TCODColor(48, 255, 141),  // 36-40
    TCODColor(28, 255, 232),  // 40-50
    TCODColor(54, 181, 255),  // 50-60
    TCODColor(41, 71, 191),  // 60-70
    TCODColor(38, 0, 255),  // 70-80
    TCODColor(140, 0, 255),  // 80-100
    TCODColor(221, 0, 255),  // 100-120
    TCODColor(255, 87, 255),  // 120-140
    TCODColor(255, 173, 255),  // 140-160
    TCODColor(255, 206, 255),  // > 160
};

// temperature color map
static constexpr int MAX_TEMP_KEY = 7;
static constexpr int tempIndexes[MAX_TEMP_KEY] = {0, 42, 84, 126, 168, 210, 255};
static constexpr int temperatures[MAX_TEMP_KEY] = {-30, -20, -10, 0, 10, 20, 30};
static constexpr TCODColor tempKeyColor[MAX_TEMP_KEY] = {
    TCODColor(180, 8, 130),  // -30 �C
    TCODColor(32, 1, 139),  // -20 �C
    TCODColor(0, 65, 252),  // -10 �C
    TCODColor(37, 255, 236),  // 0 �C
    TCODColor(255, 255, 1),  // 10 �C
    TCODColor(255, 29, 4),  // 20 �C
    TCODColor(80, 3, 0),  // 30 �C
};

int WorldGenerator::getWidth() const { return HM_WIDTH; }

int WorldGenerator::getHeight() const { return HM_HEIGHT; }

float WorldGenerator::getAltitude(int x, int y) const { return heightmap_.getValue(x, y); }

float WorldGenerator::getRealAltitude(float x, float y) const {
  int ih = gsl::narrow_cast<int>(256 * getInterpolatedAltitude(x, y));
  ih = std::clamp(0, 255, ih);
  int idx;
  for (idx = 0; idx < MAX_ALT_KEY - 1; idx++) {
    if (altIndexes[idx + 1] > ih) break;
  }
  const float alt = altitudes[idx] + (altitudes[idx + 1] - altitudes[idx]) * (ih - altIndexes[idx]) /
                                         (altIndexes[idx + 1] - altIndexes[idx]);
  return alt;
}

float WorldGenerator::getPrecipitations(float x, float y) const {
  int iprec = gsl::narrow_cast<int>(256 * precipitation_.getValue(gsl::narrow_cast<int>(x), gsl::narrow_cast<int>(y)));
  iprec = std::clamp(0, 255, iprec);
  int idx;
  for (idx = 0; idx < MAX_PREC_KEY - 1; idx++) {
    if (precIndexes[idx + 1] > iprec) break;
  }
  float prec = precipitations[idx] + (precipitations[idx + 1] - precipitations[idx]) * (iprec - precIndexes[idx]) /
                                         (precIndexes[idx + 1] - precIndexes[idx]);
  return prec;
}

float WorldGenerator::getTemperature(float x, float y) const { return temperature_.getValue((int)x, (int)y); }

EBiome WorldGenerator::getBiome(float x, float y) const { return biome_map_[(int)x + (int)y * HM_WIDTH]; }

float WorldGenerator::getInterpolatedAltitude(float x, float y) const { return heightmap_.getInterpolatedValue(x, y); }

auto WorldGenerator::getInterpolatedNormal(float x, float y) -> std::array<float, 3> const {
  std::array<float, 3> normal;
  heightmap_no_erosion_.getNormal(x, y, normal.data(), sandHeight);
  return normal;
}

float WorldGenerator::getSandHeight() const { return sandHeight; }

bool WorldGenerator::isOnSea(float x, float y) const { return getInterpolatedAltitude(x, y) <= sandHeight; }

void WorldGenerator::addHill(int nbHill, float baseRadius, float radiusVar, float height) {
  for (int i = 0; i < nbHill; i++) {
    const float hillMinRadius = baseRadius * (1.0f - radiusVar);
    const float hillMaxRadius = baseRadius * (1.0f + radiusVar);
    const float radius = wg_rng_->getFloat(hillMinRadius, hillMaxRadius);
    const int xh = wg_rng_->getInt(0, HM_WIDTH - 1);
    const int yh = wg_rng_->getInt(0, HM_HEIGHT - 1);
    heightmap_.addHill(gsl::narrow_cast<float>(xh), gsl::narrow_cast<float>(yh), radius, height);
  }
}

void WorldGenerator::setLandMass(float landMass, float waterLevel) {
  // fix land mass. We want a proportion of landMass above sea level
#ifndef NDEBUG
  const float t0 = getTime();
#endif
  auto heightcount = std::array<int, 256>{};
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float h = heightmap_.getValue(x, y);
      const int ih = std::clamp(0, 255, gsl::narrow_cast<int>(h * 255));
      ++heightcount.at(ih);
    }
  }
  int i = 0, totalcount = 0;
  while (totalcount < HM_WIDTH * HM_HEIGHT * (1.0f - landMass)) {
    totalcount += heightcount.at(i);
    ++i;
  }
  const float newWaterLevel = i / 255.0f;
  const float landCoef = (1.0f - waterLevel) / (1.0f - newWaterLevel);
  const float waterCoef = waterLevel / newWaterLevel;
  // water level should be rised/lowered to newWaterLevel
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      float h = heightmap_.getValue(x, y);
      if (h > newWaterLevel) {
        h = waterLevel + (h - newWaterLevel) * landCoef;
      } else {
        h = h * waterCoef;
      }
      heightmap_.setValue(x, y, h);
    }
  }
#ifndef NDEBUG
  const float t1 = getTime();
  DBG(("  Landmass... %g\n", t1 - t0));
#endif
}

// function building the heightmap
void WorldGenerator::buildBaseMap() {
  float t0 = getTime();
  addHill(600, 16.0f * HM_WIDTH / 200, 0.7f, 0.3f);
  heightmap_.normalize();
  float t1 = getTime();
  DBG(("  Hills... %g\n", t1 - t0));
  t0 = t1;

  heightmap_.addFbm(&noise_, 2.20f * HM_WIDTH / 400, 2.20f * HM_WIDTH / 400, 0, 0, 10.0f, 1.0f, 2.05f);
  heightmap_.normalize();
  heightmap_no_erosion_.copy(&heightmap_);
  t1 = getTime();
  DBG(("  Fbm... %g\n", t1 - t0));
  t0 = t1;

  setLandMass(0.6f, sandHeight);

  // fix land/mountain ratio using x^3 curve above sea level
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      float h = heightmap_.getValue(x, y);
      if (h >= sandHeight) {
        const float coef = (h - sandHeight) / (1.0f - sandHeight);
        h = sandHeight + coef * coef * coef * (1.0f - sandHeight);
        heightmap_.setValue(x, y, h);
      }
    }
  }
  t1 = getTime();
  DBG(("  Flatten plains... %g\n", t1 - t0));
  t0 = t1;

  // we use a custom erosion algo
  // hm->rainErosion(15000*HM_WIDTH/400,0.03,0.01,wg_rng_);
  // t1=getTime();
  // DBG(("  Erosion... %g\n", t1-t0 ));
  // t0=t1;
  // compute clouds
  for (int y = 0; y < HM_HEIGHT; ++y) {
    std::array<float, 2> sample_pos;
    sample_pos.at(0) = 6.0f * (gsl::narrow_cast<float>(y) / HM_WIDTH);
    for (int x = 0; x < HM_WIDTH; ++x) {
      sample_pos.at(1) = 6.0f * (gsl::narrow_cast<float>(x) / HM_HEIGHT);
      clouds_[x][y] = 0.5f * (1.0f + 0.8f * noise_.getFbm(sample_pos.data(), 4.0f));
    }
  }
  t1 = getTime();
  DBG(("  Init clouds... %g\n", t1 - t0));
  t0 = t1;
}

// function blurring the heightmap
void WorldGenerator::smoothMap() {
  // 3x3 kernel for smoothing operations
  static constexpr int smoothKernelSize = 9;
  static constexpr int smoothKernelDx[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
  static constexpr int smoothKernelDy[9] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
  static constexpr float smoothKernelWeight[9] = {2, 8, 2, 8, 20, 8, 2, 8, 2};

#ifndef NDEBUG
  float t0 = getTime();
#endif
  heightmap_.kernelTransform(smoothKernelSize, smoothKernelDx, smoothKernelDy, smoothKernelWeight, -1000, 1000);
  heightmap_no_erosion_.kernelTransform(
      smoothKernelSize, smoothKernelDx, smoothKernelDy, smoothKernelWeight, -1000, 1000);
  heightmap_.normalize();
#ifndef NDEBUG
  float t1 = getTime();
  DBG(("  Blur... %g\n", t1 - t0));
#endif
}

static constexpr int DIR_X[9] = {0, -1, 0, 1, -1, 1, -1, 0, 1};
static constexpr int DIR_Y[9] = {0, -1, -1, -1, 0, 0, 1, 1, 1};
static constexpr float dircoef[9] = {
    1.0f, 1.0f / 1.414f, 1.0f, 1.0f / 1.414f, 1.0f, 1.0f, 1.0f / 1.414f, 1.0f, 1.0f / 1.414f};
static constexpr int oppdir[9] = {0, 8, 7, 6, 5, 4, 3, 2, 1};

// erosion parameters
static constexpr auto EROSION_FACTOR = 0.01f;
static constexpr auto SEDIMENTATION_FACTOR = 0.01f;
static constexpr auto MAX_EROSION_ALT = 0.9f;
static constexpr auto MUDSLIDE_COEF = 0.4f;

void WorldGenerator::erodeMap() {
  TCODHeightMap newMap(HM_WIDTH, HM_HEIGHT);
  for (int pass = 5; pass != 0; --pass) {
    // compute flow and slope maps
    MapData* md = map_data_.data();
    for (int y = 0; y < HM_HEIGHT; ++y) {
      for (int x = 0; x < HM_WIDTH; ++x) {
        const float height = heightmap_.getValue(x, y);
        float height_min = height;
        float height_max = height;
        uint8_t min_dir = 0;
        uint8_t max_dir = 0;
        for (uint8_t dir_i = 1; dir_i < 9; ++dir_i) {
          const int ix = x + DIR_X[dir_i];
          const int iy = y + DIR_Y[dir_i];
          if (IN_RECTANGLE(ix, iy, HM_WIDTH, HM_HEIGHT)) {
            float h2 = heightmap_.getValue(ix, iy);
            if (h2 < height_min) {
              height_min = h2;
              min_dir = dir_i;
            } else if (h2 > height_max) {
              height_max = h2;
              max_dir = dir_i;
            }
          }
        }
        md->flowDir = min_dir;
        md->up_dir = max_dir;
        float slope = height_min - height;  // this is negative
        slope *= dircoef[min_dir];
        md->slope = slope;
        ++md;
      }
    }

    md = map_data_.data();
    for (int y = 0; y < HM_HEIGHT; ++y) {
      for (int x = 0; x < HM_WIDTH; ++x) {
        float sediment = 0.0f;
        bool end = false;
        int ix = x;
        int iy = y;
        uint8_t oldFlow = md->flowDir;
        MapData* md2 = md;
        while (!end) {
          float h = heightmap_.getValue(ix, iy);
          if (h < sandHeight - 0.01f) break;
          if (md2->flowDir == oppdir[oldFlow]) {
            h += SEDIMENTATION_FACTOR * sediment;
            heightmap_.setValue(ix, iy, h);
            end = true;
          } else {
            // remember, slope is negative
            h += precipitation_.getValue(ix, iy) * EROSION_FACTOR * md2->slope;
            h = std::max(h, sandHeight);
            sediment -= md2->slope;
            heightmap_.setValue(ix, iy, h);
            oldFlow = md2->flowDir;
            ix += DIR_X[oldFlow];
            iy += DIR_Y[oldFlow];
            md2 = &map_data_[ix + iy * HM_WIDTH];
          }
        }
        ++md;
      }
    }
    DBG(("  Erosion pass %d\n", pass));

    // mudslides (smoothing)
    const float sandCoef = 1.0f / (1.0f - sandHeight);
    for (int y = 0; y < HM_HEIGHT; ++y) {
      for (int x = 0; x < HM_WIDTH; ++x) {
        const float h = heightmap_.getValue(x, y);
        if (h < sandHeight - 0.01f || h >= MAX_EROSION_ALT) {
          newMap.setValue(x, y, h);
          continue;
        }
        float sumDelta1 = 0.0f, sumDelta2 = 0.0f;
        int nb1 = 1, nb2 = 1;
        for (int i = 1; i < 9; ++i) {
          const int ix = x + DIR_X[i];
          const int iy = y + DIR_Y[i];
          if (IN_RECTANGLE(ix, iy, HM_WIDTH, HM_HEIGHT)) {
            float ih = heightmap_.getValue(ix, iy);
            if (ih < h) {
              if (i == 1 || i == 3 || i == 6 || i == 8) {
                // diagonal neighbour
                sumDelta1 += (ih - h) * 0.4f;
                ++nb1;
              } else {
                // adjacent neighbour
                sumDelta2 += (ih - h) * 1.6f;
                ++nb2;
              }
            }
          }
        }
        // average height difference with lower neighbours
        float dh = sumDelta1 / nb1 + sumDelta2 / nb2;
        dh *= MUDSLIDE_COEF;
        const float hcoef = (h - sandHeight) * sandCoef;
        dh *= (1.0f - hcoef * hcoef * hcoef);  // less smoothing at high altitudes

        newMap.setValue(x, y, h + dh);
      }
    }
    heightmap_.copy(&newMap);
  }
}

// interpolated cloud thickness
float WorldGenerator::getCloudThickness(float x, float y) const {
  x += cloud_dx_;
  const int ix = gsl::narrow_cast<int>(x);
  const int iy = gsl::narrow_cast<int>(y);
  const int ix1 = std::min(HM_WIDTH - 1, ix + 1);
  const int iy1 = std::min(HM_HEIGHT - 1, iy + 1);
  const float fdx = x - ix;
  const float fdy = y - iy;
  const float v1 = clouds_[ix][iy];
  const float v2 = clouds_[ix1][iy];
  const float v3 = clouds_[ix][iy1];
  const float v4 = clouds_[ix1][iy1];
  const float vx1 = (1.0f - fdx) * v1 + fdx * v2;
  const float vx2 = (1.0f - fdx) * v3 + fdx * v4;
  const float v = (1.0f - fdy) * vx1 + fdy * vx2;
  return v;
}

TCODColor WorldGenerator::getMapColor(float h) {
  int colorIdx{};
  if (h < sandHeight)
    colorIdx = gsl::narrow_cast<int>(h / sandHeight * COLOR_KEY_MAX_SEA);
  else
    colorIdx =
        COLOR_KEY_MIN_LAND + gsl::narrow_cast<int>((h - sandHeight) / (1.0f - sandHeight) * (255 - COLOR_KEY_MIN_LAND));
  colorIdx = std::clamp(0, 255, colorIdx);
  return map_gradient_[colorIdx];
}

void WorldGenerator::computeSunLight(float lightDir[3]) {
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      light_intensity_[x + y * HM_WIDTH] = getMapIntensity(x + 0.5f, y + 0.5f, lightDir);
    }
  }
}

float WorldGenerator::getMapIntensity(float worldX, float worldY, float lightDir[3]) {
  // sun color & direction
  static constexpr TCODColor sunCol(255, 255, 160);
  float wx = std::clamp<float>(0.0f, HM_WIDTH - 1, worldX);
  float wy = std::clamp<float>(0.0f, HM_HEIGHT - 1, worldY);
  // apply sun light
  auto normal = getInterpolatedNormal(wx, wy);
  normal[2] *= 3.0f;
  float intensity = 0.75f - (normal[0] * lightDir[0] + normal[1] * lightDir[1] + normal[2] * lightDir[2]) * 0.75f;
  intensity = std::clamp(0.75f, 1.5f, intensity);
  return intensity;
}

TCODColor WorldGenerator::getInterpolatedColor(float worldX, float worldY) {
  return getInterpolatedColor(worldmap_, worldX, worldY);
}

TCODColor WorldGenerator::getInterpolatedColor(TCODImage& img, float x, float y) {
  int w, h;
  img.getSize(&w, &h);
  const float wx = std::clamp(0.0f, gsl::narrow_cast<float>(w - 1), x);
  const float wy = std::clamp(0.0f, gsl::narrow_cast<float>(h - 1), y);
  const int iwx = gsl::narrow_cast<int>(wx);
  const int iwy = gsl::narrow_cast<int>(wy);
  const float dx = wx - iwx;
  const float dy = wy - iwy;

  const TCODColor colNW = img.getPixel(iwx, iwy);
  const TCODColor colNE = (iwx < w - 1 ? img.getPixel(iwx + 1, iwy) : colNW);
  const TCODColor colSW = (iwy < h - 1 ? img.getPixel(iwx, iwy + 1) : colNW);
  const TCODColor colSE = (iwx < w - 1 && iwy < h - 1 ? img.getPixel(iwx + 1, iwy + 1) : colNW);
  const TCODColor colN = TCODColor::lerp(colNW, colNE, dx);
  const TCODColor colS = TCODColor::lerp(colSW, colSE, dx);
  const TCODColor col = TCODColor::lerp(colN, colS, dy);
  return col;
}

float WorldGenerator::getInterpolatedIntensity(float worldX, float worldY) {
  return getInterpolatedFloat(light_intensity_.data(), worldX, worldY, HM_WIDTH, HM_HEIGHT);
}

void WorldGenerator::updateClouds(float elapsedTime) {
  cloud_total_dx_ += elapsedTime * 5;
  cloud_dx_ += elapsedTime * 5;
  if (cloud_dx_ >= 1.0f) {
    const int colsToTranslate = gsl::narrow_cast<int>(cloud_dx_);
    cloud_dx_ -= colsToTranslate;
    // translate the cloud map
    for (int x = colsToTranslate; x < HM_WIDTH; ++x) {
      for (int y = 0; y < HM_HEIGHT; ++y) {
        clouds_[x - colsToTranslate][y] = clouds_[x][y];
      }
    }
    // compute a new column
    const float cdx = floorf(cloud_total_dx_);
    for (int x = HM_WIDTH - colsToTranslate; x < HM_WIDTH; ++x) {
      for (int y = 0; y < HM_HEIGHT; ++y) {
        const std::array<float, 2> sample_pos{
            6.0f * (gsl::narrow_cast<float>(x + cdx) / HM_WIDTH),
            6.0f * (gsl::narrow_cast<float>(y) / HM_HEIGHT),
        };
        clouds_[x][y] = 0.5f * (1.0f + 0.8f * noise_.getFbm(sample_pos.data(), 4.0f));
      }
    }
  }
}

class RiverPathCbk : public ITCODPathCallback {
 public:
  float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const {
    const WorldGenerator& world = *static_cast<WorldGenerator*>(userData);
    const float h1 = world.heightmap_.getValue(xFrom, yFrom);
    const float h2 = world.heightmap_.getValue(xTo, yTo);
    if (h2 < sandHeight) return 0.0f;
    //        float f[2] = {xFrom*10.0f/HM_WIDTH,yFrom*10.0f/HM_HEIGHT};
    //        return (1.0f+h2-h1)*10+5*(1.0f+noise2d.get(f));
    return (0.01f + h2 - h1) * 100;
  }
};

/*
void WorldGenerator::generateRivers() {
    static int riverId=0;
    static RiverPathCbk cbk;
//    static TCODPath *path=NULL;
    static TCODDijkstra *path=NULL;
        // the source
        int sx,sy;
        // the destination
        int dx=-1,dy=-1;
        // get a random point near the coast
        sx = wg_rng_->getInt(0,HM_WIDTH-1);
        sy = wg_rng_->getInt(0,HM_HEIGHT-1);
        float h = hm->getValue(sx,sy);
        while ( h <  sandHeight - 0.02 || h >= sandHeight ) {
                sx++;
                if ( sx == HM_WIDTH ) {
                        sx=0;
                        sy++;
                        if ( sy == HM_HEIGHT ) sy=0;
                }
                h = hm->getValue(sx,sy);
        }
        riverId++;
        // get a closes mountain point
        float minDist=1E10f;
        int minx = sx - HM_WIDTH/4;
        int maxx = sx + HM_WIDTH/4;
        int miny = sy - HM_HEIGHT/4;
        int maxy = sy + HM_HEIGHT/4;
        minx = std::max(0,minx);
        maxx = std::min(HM_WIDTH-1,maxx);
        miny = std::max(0,miny);
        maxy = std::min(HM_HEIGHT-1,maxy);
        h = std::min(snowHeight,h + wg_rng_->getFloat(0.1f,0.5f));
        for (int y=miny; y < maxy; y++) {
        for (int x=minx; x < maxx; x++) {
            float dh=hm->getValue(x,y);
            if ( dh >= h ) {
                dx=x;
                dy=y;
                break;
            }
        }
        }

        if (! path) {
//	    path = new TCODPath(HM_WIDTH,HM_HEIGHT,&cbk,this);
            path = new TCODDijkstra(HM_WIDTH,HM_HEIGHT,&cbk,this);
        }
        path->compute(dx,dy);
//	if ( dx >= 0 && path->compute(dx,dy,sx,sy) ) {
        if ( dx >= 0 ) { path->setPath(sx,sy) ;
        DBG( ("river : %d %d -> %d %d\n",sx,sy,dx,dy));
        int x,y;
            while (path->walk(&x,&y)) {
            MapData *md=&map_data_[x+y*HM_WIDTH];
            if ( md->riverId != 0 ) break;
            md->riverId = riverId;
            }
        }

}
*/

void WorldGenerator::generateRivers() {
  static int river_id = 0;
  // get a random point near the coast
  int source_x = wg_rng_->getInt(0, HM_WIDTH - 1);
  int source_y = wg_rng_->getInt(HM_HEIGHT / 5, 4 * HM_HEIGHT / 5);
  {
    float height = heightmap_.getValue(source_x, source_y);
    while (height < sandHeight - 0.02 || height >= sandHeight) {
      ++source_x;
      if (source_x == HM_WIDTH) {
        source_x = 0;
        ++source_y;
        if (source_y == HM_HEIGHT) source_y = 0;
      }
      height = heightmap_.getValue(source_x, source_y);
    }
  }
  std::vector<int> tree;
  std::vector<int> random_points;
  tree.push_back(source_x + source_y * HM_WIDTH);
  ++river_id;
  const int dest_x = source_x;
  const int dest_y = source_y;
  for (int i = 0; i < wg_rng_->getInt(50, 200); i++) {
    const int rx = wg_rng_->getInt(source_x - 200, source_x + 200);
    const int ry = wg_rng_->getInt(source_y - 200, source_y + 200);
    //	    if ( IN_RECTANGLE(rx,ry,HM_WIDTH,HM_HEIGHT) ) {
    //	        float h=hm->getValue(rx,ry);
    //	        if ( h >= sandHeight ) {
    random_points.push_back(rx + ry * HM_WIDTH);
    //	        }
    //	    }
  }
  for (int i = 0; i < random_points.size(); i++) {
    const int rx = random_points.at(i) % HM_WIDTH;
    const int ry = random_points.at(i) / HM_WIDTH;

    float minDist = 1E10;
    int best_x = -1;
    int best_y = -1;
    for (int j = 0; j < tree.size(); j++) {
      const int tx = tree.at(j) % HM_WIDTH;
      const int ty = tree.at(j) / HM_WIDTH;
      const float dist = gsl::narrow_cast<float>((tx - rx) * (tx - rx) + (ty - ry) * (ty - ry));
      if (dist < minDist) {
        minDist = dist;
        best_x = tx;
        best_y = ty;
      }
    }
    TCODLine::init(best_x, best_y, rx, ry);
    int len = 3;
    int cx = best_x;
    int cy = best_y;
    MapData* md = &map_data_[cx + cy * HM_WIDTH];
    if (md->river_id == river_id) md->river_id = 0;
    do {
      md = &map_data_[cx + cy * HM_WIDTH];
      if (md->river_id > 0) return;
      const float height = heightmap_.getValue(cx, cy);
      if (height >= sandHeight) {
        md->river_id = river_id;
        precipitation_.setValue(cx, cy, 1.0f);
      }
      if (cx == 0 || cx == HM_WIDTH - 1 || cy == 0 || cy == HM_HEIGHT - 1) {
        len = 0;
      } else if (TCODLine::step(&cx, &cy)) {
        len = 0;
      }
      --len;
    } while (len > 0);
    const int newNode = cx + cy * HM_WIDTH;
    if (newNode != best_x + best_y * HM_WIDTH) {
      tree.push_back(newNode);
    }
  }
}

/*
void WorldGenerator::generateRivers() {
    static int riverId=0;
        // the source
        int sx,sy;
        // the destination
        int dx,dy;
        // get a random point near the coast
        sx = wg_rng_->getInt(0,HM_WIDTH-1);
        sy = wg_rng_->getInt(HM_HEIGHT/5,4*HM_HEIGHT/5);
        float h = hm->getValue(sx,sy);
        MapData *md=&map_data_[sx+sy*HM_WIDTH];
        while ( md->riverId == 0 && (h <  sandHeight - 0.02 || h >= sandHeight) ) {
                sx++;
                if ( sx == HM_WIDTH ) {
                        sx=0;
                        sy++;
                        if ( sy == HM_HEIGHT ) sy=0;
                }
                h = hm->getValue(sx,sy);
                md=&map_data_[sx+sy*HM_WIDTH];
        }
        riverId++;
        dx = sx;
        dy = sy;
        DBG( ("source : %d %d\n",sx,sy));
        // travel down to the see
        // get the hiwest point around current position
        bool deadEnd=false;
        int len=0;
        River *river=new River();
        rivers_.push(river);
        int maxlen=HM_WIDTH,lastdx=1,lastdy=1;
        do {
        int coord = sx + sy*HM_WIDTH;
            MapData *md=&map_data_[coord];
            if ( md->riverId != 0 ) {
                River *joined = rivers_.get(md->riverId-1);
                int i=0;
                while (joined->coords.get(i) != coord ) i++;
                while ( i < joined->coords.size() ) {
                    int newStrength=joined->strength.get(i)+1;
                    joined->strength.set(newStrength,i);
                    i++;
                }
                break;
            }
        md->riverId = riverId;
        md->river_length = len++;
        river->coords.push(coord);
        river->strength.push(1);
                if ( md->up_dir != 0 ) {
                    lastdx=dirx[md->up_dir];
                        sx += lastdx;
                        lastdy=diry[md->up_dir];
                        sy += lastdy;
                        deadEnd=false;
                } else if ( deadEnd ) {
                    break;
                } else {
                        sx += lastdx;
                        sy += lastdy;
                        if ( ! IN_RECTANGLE(sx,sy,HM_WIDTH,HM_HEIGHT ) ) break;
                        deadEnd=true;
                }
                h=hm->getValue(sx,sy);
                maxlen--;
        } while ( maxlen > 0 && h <= snowHeight);

}
*/

EClimate WorldGenerator::getClimateFromTemp(float temp) {
  if (temp <= -5) return ARTIC_ALPINE;
  if (temp <= 5) return COLD;
  if (temp <= 15) return TEMPERATE;
  if (temp <= 20) return WARM;
  return TROPICAL;
}

float WorldGenerator::getInterpolatedFloat(float* arr, float x, float y, int width, int height) {
  const float wx = std::clamp(0.0f, gsl::narrow_cast<float>(width - 1), x);
  const float wy = std::clamp(0.0f, gsl::narrow_cast<float>(height - 1), y);
  const int iwx = gsl::narrow_cast<int>(wx);
  const int iwy = gsl::narrow_cast<int>(wy);
  const float dx = wx - iwx;
  const float dy = wy - iwy;

  const float iNW = arr[iwx + iwy * width];
  const float iNE = (iwx < width - 1 ? arr[iwx + 1 + iwy * width] : iNW);
  const float iSW = (iwy < height - 1 ? arr[iwx + (iwy + 1) * width] : iNW);
  const float iSE = (iwx < width - 1 && iwy < height - 1 ? arr[iwx + 1 + (iwy + 1) * width] : iNW);
  const float iN = (1.0f - dx) * iNW + dx * iNE;
  const float iS = (1.0f - dx) * iSW + dx * iSE;
  return (1.0f - dy) * iN + dy * iS;
}

int WorldGenerator::getRiverStrength(int riverId) {
  // River *river = rivers_.get(riverId-1);
  // return river->strength.get(river->strength.size()-1);
  return 2;
}

void WorldGenerator::computePrecipitations() {
  static constexpr float water_add = 0.03f;
  static constexpr float slope_coef = 2.0f;
  static constexpr float base_precipitation = 0.01f;  // precipitation coef when slope == 0
  float t0 = getTime();
  // north/south winds
  for (int dir_y = -1; dir_y <= 1; dir_y += 2) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float noise_x = gsl::narrow_cast<float>(x) * 5 / HM_WIDTH;
      float water_amount = (1.0f + noise1d.getFbm(&noise_x, 3.0f));
      const int start_y = (dir_y == -1 ? HM_HEIGHT - 1 : 0);
      const int end_y = (dir_y == -1 ? -1 : HM_HEIGHT);
      for (int y = start_y; y != end_y; y += dir_y) {
        float h = heightmap_.getValue(x, y);
        if (h < sandHeight) {
          water_amount += water_add;
        } else if (water_amount > 0.0f) {
          float slope;
          if ((unsigned)(y + dir_y) < (unsigned)HM_HEIGHT)
            slope = heightmap_.getValue(x, y + dir_y) - h;
          else
            slope = h - heightmap_.getValue(x, y - dir_y);
          if (slope >= 0.0f) {
            const float precip = water_amount * (base_precipitation + slope * slope_coef);
            precipitation_.setValue(x, y, precipitation_.getValue(x, y) + precip);
            water_amount -= precip;
            water_amount = std::max(0.0f, water_amount);
          }
        }
      }
    }
  }
  float t1 = getTime();
  DBG(("  North/south winds... %g\n", t1 - t0));
  t0 = t1;

  // east/west winds
  for (int dir_x = -1; dir_x <= 1; dir_x += 2) {
    for (int y = 0; y < HM_HEIGHT; ++y) {
      const float noise_y = gsl::narrow_cast<float>(y) * 5 / HM_HEIGHT;
      float water_amount = (1.0f + noise1d.getFbm(&noise_y, 3.0f));
      const int start_x = (dir_x == -1 ? HM_WIDTH - 1 : 0);
      const int end_x = (dir_x == -1 ? -1 : HM_WIDTH);
      for (int x = start_x; x != end_x; x += dir_x) {
        float h = heightmap_.getValue(x, y);
        if (h < sandHeight) {
          water_amount += water_add;
        } else if (water_amount > 0.0f) {
          float slope;
          if ((unsigned)(x + dir_x) < (unsigned)HM_WIDTH)
            slope = heightmap_.getValue(x + dir_x, y) - h;
          else
            slope = h - heightmap_.getValue(x - dir_x, y);
          if (slope >= 0.0f) {
            const float precip = water_amount * (base_precipitation + slope * slope_coef);
            precipitation_.setValue(x, y, precipitation_.getValue(x, y) + precip);
            water_amount -= precip;
            water_amount = std::max(0.0f, water_amount);
          }
        }
      }
    }
  }
  t1 = getTime();
  DBG(("  East/west winds... %g\n", t1 - t0));
  t0 = t1;

  float min, max;
  precipitation_.getMinMax(&min, &max);

  // latitude impact
  for (int y = HM_HEIGHT / 4; y < 3 * HM_HEIGHT / 4; ++y) {
    // latitude (0 : equator, -1/1 : pole)
    const float lat = gsl::narrow_cast<float>(y - HM_HEIGHT / 4) * 2 / HM_HEIGHT;
    const float coef = sinf(2 * 3.1415926f * lat);
    for (int x = 0; x < HM_WIDTH; x++) {
      const std::array<float, 2> sample_pos = {
          gsl::narrow_cast<float>(x) / HM_WIDTH,
          gsl::narrow_cast<float>(y) / HM_HEIGHT,
      };
      const float xcoef = coef + 0.5f * noise2d.getFbm(sample_pos.data(), 3.0f);
      float precip = precipitation_.getValue(x, y);
      precip += (max - min) * xcoef * 0.1f;
      precipitation_.setValue(x, y, precip);
    }
  }
  t1 = getTime();
  DBG(("  latitude... %g\n", t1 - t0));
  t0 = t1;

  // very fast blur by scaling down and up
  static constexpr int factor = 8;
  static constexpr int small_width = (HM_WIDTH + factor - 1) / factor;
  static constexpr int small_height = (HM_HEIGHT + factor - 1) / factor;
  auto lowResMap = std::array<float, small_width * small_height>{};
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float v = precipitation_.getValue(x, y);
      const int ix = x / factor;
      const int iy = y / factor;
      lowResMap[ix + iy * small_width] += v;
    }
  }
  const float coef = 1.0f / factor;
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float v = getInterpolatedFloat(lowResMap.data(), x * coef, y * coef, small_width, small_height);
      precipitation_.setValue(x, y, v);
    }
  }
}

void WorldGenerator::smoothPrecipitations() {
  float t0 = getTime();

  // better quality polishing blur using a 5x5 kernel
  // faster than TCODHeightmap kernelTransform function
  TCODHeightMap temp_heightmap(HM_WIDTH, HM_HEIGHT);
  temp_heightmap.copy(&precipitation_);
  for (int i = 4; i != 0; --i) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const int min_x = std::max(0, x - 2);
      const int max_x = std::min(HM_WIDTH - 1, x + 2);
      const int min_y = 0;
      const int max_y = 2;
      float sum = 0.0f;
      int count = 0;
      // compute the kernel sum at x,0
      for (int ix = min_x; ix <= max_x; ++ix) {
        for (int iy = min_y; iy <= max_y; ++iy) {
          sum += precipitation_.getValue(ix, iy);
          ++count;
        }
      }
      temp_heightmap.setValue(x, 0, sum / count);
      for (int y = 1; y < HM_HEIGHT; ++y) {
        if (y - 2 >= 0) {
          // remove the top-line sum
          for (int ix = min_x; ix <= max_x; ++ix) {
            sum -= precipitation_.getValue(ix, y - 2);
            count--;
          }
        }
        if (y + 2 < HM_HEIGHT) {
          // add the bottom-line sum
          for (int ix = min_x; ix <= max_x; ++ix) {
            sum += precipitation_.getValue(ix, y + 2);
            count++;
          }
        }
        temp_heightmap.setValue(x, y, sum / count);
      }
    }
  }
  precipitation_.copy(&temp_heightmap);

  float t1 = getTime();
  DBG(("  Blur... %g\n", t1 - t0));
  t0 = t1;

  precipitation_.normalize();
  t1 = getTime();
  DBG(("  Normalization... %g\n", t1 - t0));
  t0 = t1;
}

void WorldGenerator::computeTemperaturesAndBiomes() {
  // temperature shift with altitude : -25�C at 6000 m
  // mean temp at sea level : 25�C at lat 0  5�C at lat 45 -25�C at lat 90 (sinusoide)
  const float sand_coef = 1.0f / (1.0f - sandHeight);
  const float water_coef = 1.0f / sandHeight;
  for (int y = 0; y < HM_HEIGHT; y++) {
    const float lat = (float)(y - HM_HEIGHT / 2) * 2 / HM_HEIGHT;
    float lat_temp = 0.5f * (1.0f + powf(sinf(3.1415926f * (lat + 0.5f)), 5));  // between 0 and 1
    if (lat_temp > 0.0f) lat_temp = sqrt(lat_temp);
    lat_temp = -30 + lat_temp * 60;
    for (int x = 0; x < HM_WIDTH; x++) {
      const float h0 = heightmap_.getValue(x, y);
      float h = h0 - sandHeight;
      if (h < 0.0f)
        h *= water_coef;
      else
        h *= sand_coef;
      const float altShift = -35 * h;
      const float temp = lat_temp + altShift;
      temperature_.setValue(x, y, temp);
      const float humid = precipitation_.getValue(x, y);
      // compute biome
      const EClimate climate = getClimateFromTemp(temp);
      int humid_index = gsl::narrow_cast<int>(humid * 5);
      humid_index = std::min(4, humid_index);
      const EBiome biome = biomeDiagram[climate][humid_index];
      biome_map_[x + y * HM_WIDTH] = biome;
    }
  }
  float min, max;
  temperature_.getMinMax(&min, &max);
  DBG(("Temperatures min/max: %g / %g\n", min, max));
}

TCODColor WorldGenerator::getBiomeColor(EBiome biome, int x, int y) {
  static constexpr tcod::ColorRGB biomeColors[] = {
      // TUNDRA,
      tcod::ColorRGB{200, 240, 255},
      // COLD_DESERT,
      tcod::ColorRGB{180, 210, 210},
      // GRASSLAND,
      tcod::ColorRGB{0, 255, 127},
      // BOREAL_FOREST,
      tcod::ColorRGB{14, 93, 43},
      // TEMPERATE_FOREST,
      tcod::ColorRGB{44, 177, 83},
      // TROPICAL_MONTANE_FOREST,
      tcod::ColorRGB{185, 232, 164},
      // HOT_DESERT,
      tcod::ColorRGB{235, 255, 210},
      // SAVANNA,
      tcod::ColorRGB{255, 205, 20},
      // TROPICAL_DRY_FOREST,
      tcod::ColorRGB{60, 130, 40},
      // TROPICAL_EVERGREEN_FOREST,
      tcod::ColorRGB{0, 255, 0},
      // THORN_FOREST,
      tcod::ColorRGB{192, 192, 112},
  };
  int r = 0, g = 0, b = 0;
  int count = 1;
  r += biomeColors[biome].r;
  g += biomeColors[biome].g;
  b += biomeColors[biome].b;
  for (int i = 0; i < 4; ++i) {
    int ix = x + wg_rng_->getInt(-10, 10);
    int iy = y + wg_rng_->getInt(-10, 10);
    if (IN_RECTANGLE(ix, iy, HM_WIDTH, HM_HEIGHT)) {
      TCODColor c = biomeColors[biome_map_[ix + iy * HM_WIDTH]];
      r += c.r + wg_rng_->getInt(-10, 10);
      g += c.g + wg_rng_->getInt(-10, 10);
      b += c.b + wg_rng_->getInt(-10, 10);
      ++count;
    }
  }
  r /= count;
  g /= count;
  b /= count;
  r = std::clamp(0, 255, r);
  g = std::clamp(0, 255, g);
  b = std::clamp(0, 255, b);
  return TCODColor(r, g, b);
}

void WorldGenerator::computeColors() {
  // alter map color using temperature & precipitation maps
  MapData* md = map_data_.data();
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float height = heightmap_.getValue(x, y);
      float tempature = temperature_.getValue(x, y);
      EBiome biome = biome_map_[x + y * HM_WIDTH];
      TCODColor c = getMapColor(height);
      if (height >= sandHeight) {
        c = TCODColor::lerp(c, getBiomeColor(biome, x, y), 0.5f);
      }

      // snow near poles
      tempature += 10 * (clouds_[HM_WIDTH - 1 - x][HM_HEIGHT - 1 - y]);  // cheap 2D noise ;)
      if (tempature < -10.0f && height < sandHeight) {
        worldmap_.putPixel(x, y, TCODColor::lerp(WHITE, c, 0.3f));
      } else if (tempature < -8.0f && height < sandHeight) {
        worldmap_.putPixel(x, y, TCODColor::lerp(WHITE, c, 0.3f + 0.7f * (10.0f + tempature) / 2.0f));
      } else if (tempature < -2.0f && height >= sandHeight) {
        worldmap_.putPixel(x, y, WHITE);
      } else if (tempature < 2.0f && height >= sandHeight) {
        // TCODColor snow = map_gradient_[(int)(snowHeight*255) + (int)((255 - (int)(snowHeight*255)) *
        // (0.6f-tempature)/0.4f)];
        c = TCODColor::lerp(WHITE, c, (tempature + 2) / 4.0f);
        worldmap_.putPixel(x, y, c);
      } else {
        worldmap_.putPixel(x, y, c);
      }
      ++md;
    }
  }
  // draw rivers
  /*
  for (River **it=rivers_.begin(); it != rivers_.end(); it++) {
      for (int i=0; i < (*it)->coords.size(); i++ ) {
          int coord = (*it)->coords.get(i);
          int strength = (*it)->strength.get(i);
          int x = coord % HM_WIDTH;
          int y = coord / HM_WIDTH;
          TCODColor c= worldmap_.getPixel(x,y);
          c = TCODColor::lerp(c,TCODColor::blue,(float)(strength)/5.0f);
          worldmap_.putPixel(x,y,c);
      }
  }
  */
  md = map_data_.data();
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      if (md->river_id > 0) {
        TCODColor c = worldmap_.getPixel(x, y);
        c = TCODColor::lerp(c, tcod::ColorRGB{0, 0, 255}, 0.3f);
        worldmap_.putPixel(x, y, c);
      }
      ++md;
    }
  }
  // blur
  static constexpr int dx[] = {0, -1, 0, 1, 0};
  static constexpr int dy[] = {0, 0, -1, 0, 1};
  static constexpr int coef[] = {1, 2, 2, 2, 2};
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      int r = 0, g = 0, b = 0, count = 0;
      for (int i = 0; i < 5; ++i) {
        int ix = x + dx[i];
        int iy = y + dy[i];
        if (IN_RECTANGLE(ix, iy, HM_WIDTH, HM_HEIGHT)) {
          TCODColor c = worldmap_.getPixel(ix, iy);
          r += coef[i] * c.r;
          g += coef[i] * c.g;
          b += coef[i] * c.b;
          count += coef[i];
        }
      }
      r /= count;
      g /= count;
      b /= count;
      worldmap_.putPixel(x, y, TCODColor(r, g, b));
    }
  }
  drawCoasts(worldmap_);
}

void WorldGenerator::generate(TCODRandom* wRng) {
  float t00, t0 = getTime();
  t00 = t0;
  cloud_dx_ = cloud_total_dx_ = 0.0f;
  TCODColor::genMap(map_gradient_.data(), MAX_COLOR_KEY, keyColor, keyIndex);
  if (wRng == NULL) wRng = TCODRandom::getInstance();
  wg_rng_ = wRng;
  noise_ = TCODNoise(2, wg_rng_);
  heightmap_.clear();
  heightmap_no_erosion_.clear();
  worldmap_.clear(BLACK);
  light_intensity_.fill(0);
  temperature_.clear();
  precipitation_.clear();
  biome_map_.fill(EBiome{});
  map_data_.fill(MapData{});
  float t1 = getTime();
  DBG(("Initialization... %g\n", t1 - t0));
  t0 = t1;

  buildBaseMap();
  t1 = getTime();
  DBG(("Heightmap construction... %g\n", t1 - t0));
  t0 = t1;

  computePrecipitations();
  t1 = getTime();
  DBG(("Precipitation map... %g\n", t1 - t0));
  t0 = t1;

  erodeMap();
  t1 = getTime();
  DBG(("Erosion... %g\n", t1 - t0));
  t0 = t1;

  smoothMap();
  t1 = getTime();
  DBG(("Smooth... %g\n", t1 - t0));
  t0 = t1;

  setLandMass(0.6f, sandHeight);

  for (int i = 0; i < HM_WIDTH * HM_HEIGHT / 3000; i++) {
    //	for (int i=0; i < 1; i++) {
    generateRivers();
  }
  t1 = getTime();
  DBG(("Rivers... %g\n", t1 - t0));
  t0 = t1;

  smoothPrecipitations();
  t1 = getTime();
  DBG(("Smooth precipitations... %g\n", t1 - t0));
  t0 = t1;

  computeTemperaturesAndBiomes();
  t1 = getTime();
  DBG(("Temperature map... %g\n", t1 - t0));
  t0 = t1;

  computeColors();
  t1 = getTime();
  DBG(("Color map... %g\n", t1 - t0));
  t0 = t1;

  t1 = getTime();
  DBG(("TOTAL TIME... %g\n", t1 - t00));
}

void WorldGenerator::drawCoasts(TCODImage& img) {
  // detect coasts
  for (int y = 0; y < HM_HEIGHT - 1; ++y) {
    for (int x = 0; x < HM_WIDTH - 1; ++x) {
      float h = heightmap_.getValue(x, y);
      float h2 = heightmap_.getValue(x + 1, y);
      if ((h < sandHeight && h2 >= sandHeight) || (h2 < sandHeight && h >= sandHeight)) {
        img.putPixel(x, y, BLACK);
      } else {
        h = heightmap_.getValue(x, y);
        h2 = heightmap_.getValue(x, y + 1);
        if ((h < sandHeight && h2 >= sandHeight) || (h2 < sandHeight && h >= sandHeight)) {
          img.putPixel(x, y, BLACK);
        }
      }
    }
  }
}

void WorldGenerator::saveBiomeMap(const char* filename) {
  static TCODImage legend{"data/img/legend_biome.png"};
  static constexpr tcod::ColorRGB biomeColors[] = {
      // TUNDRA,
      tcod::ColorRGB{88, 234, 250},
      // COLD_DESERT,
      tcod::ColorRGB{129, 174, 170},
      // GRASSLAND,
      tcod::ColorRGB{0, 255, 127},
      // BOREAL_FOREST,
      tcod::ColorRGB{14, 93, 43},
      // TEMPERATE_FOREST,
      tcod::ColorRGB{44, 177, 83},
      // TROPICAL_MONTANE_FOREST,
      tcod::ColorRGB{185, 232, 164},
      // HOT_DESERT,
      tcod::ColorRGB{229, 247, 184},
      // SAVANNA,
      tcod::ColorRGB{255, 127, 0},
      // TROPICAL_DRY_FOREST,
      tcod::ColorRGB{191, 191, 0},
      // TROPICAL_EVERGREEN_FOREST,
      tcod::ColorRGB{0, 255, 0},
      // THORN_FOREST,
      tcod::ColorRGB{192, 192, 112},
  };
  int legend_height, legend_width;
  legend.getSize(&legend_width, &legend_height);
  if (filename == NULL) filename = "world_biome.png";
  TCODImage img(std::max(HM_WIDTH, legend_width), HM_HEIGHT + legend_height);
  // draw biome map
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      const float h = heightmap_.getValue(x, y);
      if (h < sandHeight)
        img.putPixel(x, y, TCODColor(100, 100, 255));
      else
        img.putPixel(x, y, biomeColors[biome_map_[x + y * HM_WIDTH]]);
    }
  }
  drawCoasts(img);
  // blit legend
  const int legend_x = std::max(HM_WIDTH, legend_width) / 2 - legend_width / 2;
  for (int y = 0; y < legend_height; y++) {
    for (int x = 0; x < legend_width; x++) {
      img.putPixel(legend_x + x, HM_HEIGHT + y, legend.getPixel(x, y));
    }
  }
  // fill legend colors
  for (int i = 0; i < 6; ++i) {
    for (int x = 17; x < 47; ++x) {
      for (int y = 4 + i * 14; y < 14 + i * 14; ++y) {
        img.putPixel(legend_x + x, HM_HEIGHT + y, biomeColors[i]);
      }
    }
  }
  for (int i = 6; i < NB_BIOMES; ++i) {
    for (int x = 221; x < 251; ++x) {
      for (int y = 4 + (i - 6) * 14; y < 14 + (i - 6) * 14; y++) {
        img.putPixel(legend_x + x, HM_HEIGHT + y, biomeColors[i]);
      }
    }
  }
  img.save(filename);
}

void WorldGenerator::saveTemperatureMap(const char* filename) {
  static TCODColor tempature_gradient[256];

  static TCODImage* legend = NULL;
  static int legend_height, legend_width;
  if (legend == NULL) {
    legend = new TCODImage("data/img/legend_temperature.png");
    legend->getSize(&legend_width, &legend_height);
    TCODColor::genMap(tempature_gradient, MAX_TEMP_KEY, tempKeyColor, tempIndexes);
  }

  if (filename == NULL) filename = "world_temperature.png";
  TCODImage img(std::max(HM_WIDTH, legend_width), HM_HEIGHT + legend_height);
  float min_tempature, max_tempature;
  temperature_.getMinMax(&min_tempature, &max_tempature);
  // render temperature map
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      float h = heightmap_.getValue(x, y);
      if (h < sandHeight)
        img.putPixel(x, y, TCODColor(100, 100, 255));
      else {
        float tempature = temperature_.getValue(x, y);
        tempature = (tempature - min_tempature) / (max_tempature - min_tempature);
        int color_index = (int)(tempature * 255);
        color_index = std::clamp(0, 255, color_index);
        img.putPixel(x, y, tempature_gradient[color_index]);
      }
    }
  }
  drawCoasts(img);

  // blit legend
  int legend_x = std::max(HM_WIDTH, legend_width) / 2 - legend_width / 2;
  for (int y = 0; y < legend_height; y++) {
    for (int x = 0; x < legend_width; x++) {
      img.putPixel(legend_x + x, HM_HEIGHT + y, legend->getPixel(x, y));
    }
  }
  img.save(filename);
}

void WorldGenerator::savePrecipitationMap(const char* filename) {
  static TCODImage* legend = NULL;
  static int legend_height, legend_width;
  if (legend == NULL) {
    legend = new TCODImage("data/img/legend_precipitation.png");
    legend->getSize(&legend_width, &legend_height);
  }

  if (filename == NULL) filename = "world_precipitation.png";
  TCODImage img(std::max(HM_WIDTH, legend_width), HM_HEIGHT + legend_height);
  // render precipitation map
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      float h = heightmap_.getValue(x, y);
      if (h < sandHeight)
        img.putPixel(x, y, TCODColor(100, 100, 255));
      else {
        float prec = precipitation_.getValue(x, y);
        int iprec = (int)(prec * 180);
        int color_index = 0;
        while (color_index < MAX_PREC_KEY && iprec > precIndexes[color_index]) ++color_index;
        color_index = std::clamp(0, MAX_PREC_KEY, color_index);
        img.putPixel(x, y, precColors[color_index]);
      }
    }
  }
  drawCoasts(img);

  // blit legend
  int legend_x = std::max(HM_WIDTH, legend_width) / 2 - legend_width / 2;
  for (int y = 0; y < legend_height; ++y) {
    for (int x = 0; x < legend_width; ++x) {
      img.putPixel(legend_x + x, HM_HEIGHT + y, legend->getPixel(x, y));
    }
  }
  img.save(filename);
}

void WorldGenerator::saveAltitudeMap(const char* filename) {
  static TCODColor altGradient[256];

  static TCODImage* legend = NULL;
  static int legend_height, legend_width;
  if (legend == NULL) {
    legend = new TCODImage("data/img/legend_altitude.png");
    legend->getSize(&legend_width, &legend_height);
    TCODColor::genMap(altGradient, MAX_ALT_KEY, altColors, altIndexes);
  }

  if (filename == NULL) filename = "world_altitude.png";
  TCODImage img(HM_WIDTH + legend_width, std::max(HM_HEIGHT, legend_height));
  // render altitude map
  for (int y = 0; y < HM_HEIGHT; ++y) {
    for (int x = 0; x < HM_WIDTH; ++x) {
      float h = heightmap_.getValue(x, y);
      int alt_index = (int)(h * 256);
      alt_index = std::clamp(0, 255, alt_index);
      img.putPixel(x, y, altGradient[alt_index]);
    }
  }

  // blit legend
  int legendy = std::max(HM_HEIGHT, legend_height) / 2 - legend_height / 2;
  for (int y = 0; y < legend_height; ++y) {
    for (int x = 0; x < legend_width; ++x) {
      img.putPixel(HM_WIDTH + x, legendy + y, legend->getPixel(x, y));
    }
  }
  img.save(filename);
}

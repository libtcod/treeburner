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
#include <array>
#include <libtcod.hpp>
#include <vector>

namespace util {
// size of the heightmap
static constexpr auto HM_WIDTH = 800;
static constexpr auto HM_HEIGHT = 800;

// biome and climate list. based on Whittaker Biome Diagram
enum EClimate { ARTIC_ALPINE, COLD, TEMPERATE, WARM, TROPICAL, NB_CLIMATES };

// grassland : might be either grassland, shrubland or woodland depending on the vegetation level
// savanna : might be either savanna or thorn forest depending on the vegetation level
enum EBiome {
  TUNDRA,
  COLD_DESERT,
  GRASSLAND,
  BOREAL_FOREST,
  TEMPERATE_FOREST,
  TROPICAL_MONTANE_FOREST,
  HOT_DESERT,
  SAVANNA,
  TROPICAL_DRY_FOREST,
  TROPICAL_EVERGREEN_FOREST,
  THORN_FOREST,
  NB_BIOMES
};

class WorldGenerator {
 public:
  void generate(TCODRandom* wRng);

  // getters
  [[nodiscard]] int getWidth() const;
  [[nodiscard]] int getHeight() const;
  [[nodiscard]] float getAltitude(int x, int y) const;  // heightmap. between 0 and 1
  [[nodiscard]] float getInterpolatedAltitude(float x, float y) const;
  [[nodiscard]] float getSandHeight() const;
  [[nodiscard]] bool isOnSea(float x, float y) const;
  [[nodiscard]] float getCloudThickness(float x, float y) const;
  [[nodiscard]] auto getInterpolatedNormal(float x, float y) -> std::array<float, 3> const;
  [[nodiscard]] TCODColor getInterpolatedColor(float worldX, float worldY);
  [[nodiscard]] float getInterpolatedIntensity(float worldX, float worldY);

  // update
  void updateClouds(float elapsedTime);
  void computeSunLight(float lightDir[3]);

  // data
  [[nodiscard]] float getRealAltitude(float x, float y) const;  // altitude in meters
  [[nodiscard]] float getPrecipitations(float x, float y) const;  // in centimeter/m²/year
  [[nodiscard]] float getTemperature(float x, float y) const;  // in °C
  [[nodiscard]] EBiome getBiome(float x, float y) const;

  // map generators
  void saveBiomeMap(const char* filename = NULL);
  void saveAltitudeMap(const char* filename = NULL);
  void saveTemperatureMap(const char* filename = NULL);
  void savePrecipitationMap(const char* filename = NULL);

  // altitude->color map
  std::array<TCODColor, 256> map_gradient_{};
  // world height map (0.0 - 1.0)
  TCODHeightMap heightmap_{HM_WIDTH, HM_HEIGHT};
  // height map without erosion
  TCODHeightMap heightmap_no_erosion_{HM_WIDTH, HM_HEIGHT};
  // complete world map (not shaded)
  TCODImage worldmap_{HM_WIDTH, HM_HEIGHT};
  // temperature map (in °C)
  TCODHeightMap temperature_{HM_WIDTH, HM_HEIGHT};
  // precipitation map (0.0 - 1.0)
  TCODHeightMap precipitation_{HM_WIDTH, HM_HEIGHT};
  // biome map
  std::vector<EBiome> biome_map_{std::vector<EBiome>(HM_WIDTH * HM_HEIGHT)};

 protected:
  friend class RiverPathCbk;
  struct MapData {
    float slope;
    // number of cells flowing into this cell
    uint32_t area;
    // direction of lowest neighbour
    uint8_t flowDir;
    // inverse flow direction
    uint8_t up_dir;
    uint8_t in_flags;  // incoming flows
    int river_id;
    int river_length;
  };
  struct River {
    std::vector<int> coords;
    std::vector<int> strength;
  };

  void addHill(int nbHill, float baseRadius, float radiusVar, float height);
  void buildBaseMap();
  void erodeMap();
  void smoothMap();
  // compute the ground color from the heightmap
  [[nodiscard]] TCODColor getMapColor(float h);
  // get sun light intensity on a point of the map
  [[nodiscard]] float getMapIntensity(float worldX, float worldY, float lightDir[3]);
  [[nodiscard]] TCODColor getInterpolatedColor(TCODImage& img, float x, float y);
  [[nodiscard]] float getInterpolatedFloat(float* arr, float x, float y, int width, int height);
  void generateRivers();
  void smoothPrecipitations();
  [[nodiscard]] int getRiverStrength(int riverId);
  void setLandMass(float percent, float waterLevel);
  void computeTemperaturesAndBiomes();
  [[nodiscard]] TCODColor getBiomeColor(EBiome biome, int x, int y);
  void computePrecipitations();
  void computeColors();
  void drawCoasts(TCODImage& img);
  [[nodiscard]] EClimate getClimateFromTemp(float temp);

  TCODNoise noise_{2};
  // cloud thickness
  float clouds_[HM_WIDTH][HM_HEIGHT]{};
  float cloud_dx_{};  // horizontal offset for smooth scrolling
  float cloud_total_dx_{};
  // world light intensity map (shadow map)
  std::vector<float> light_intensity_{std::vector<float>(HM_WIDTH * HM_HEIGHT)};
  std::vector<MapData> map_data_{std::vector<MapData>(HM_WIDTH * HM_HEIGHT)};
  std::vector<River> rivers_{};
  TCODRandom* wg_rng_{};
};
}  // namespace util

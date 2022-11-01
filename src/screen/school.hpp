/*
 * Copyright (c) 2010 Jice
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

#include "constants.hpp"
#include "screen.hpp"
#include "util/textgen.hpp"
#include "util/worldgen.hpp"

namespace screen {
enum ESchool { SCHOOL_FIRE, SCHOOL_WATER, NB_SCHOOLS };
extern const char* schoolTypeName[NB_SCHOOLS];
#define MAP_WIDTH CON_W * 4 / 3
#define MAP_HEIGHT (CON_H - 2) * 2

class School {
 public:
  enum { SHORE, PLAIN, FOREST_EDGE, FOREST, MOUNTAIN, SNOW } terrain;
  int x, y;
  ESchool type;
  char name[32];
  char desc[512];
};

class SchoolScreen : public Screen {
 public:
  static SchoolScreen* instance;
  SchoolScreen();
  void render() override;
  bool update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) override;
  void generateWorld(uint32_t seed);

 protected:
  School school[NB_SCHOOLS];
  WorldGenerator worldGen;
  // world map current offset and destination offset
  float offx, offy, dx, dy;
  // rendered world
  TCODImage* world;
  // orb transparency map
  TCODImage* mapmask;
  // orb reflection map
  TCODImage* mapreflection;
  TCODConsole* con;
  int selectedSchool;
  // precomputed fisheye distorsion
  float fisheyex[MAP_WIDTH][MAP_HEIGHT];
  float fisheyey[MAP_WIDTH][MAP_HEIGHT];
  bool worldGenerated;
  TextGenerator* textGen;
  TCODRandom* schoolRng;

  bool isPosOk(int schoolNum) const;
  int getTerrainType(int x, int y, int range) const;
  void selectSchool(int num);
  const char* genSchoolDescription(School* sch);
  char* goatSoup(const char* source, School* sch, char* buf);
  void setContextSchool(School* sch);
  void onActivate() override;
  // apply sun light & cloud shadow to interpolated color
  TCODColor getMapShadedColor(float worldX, float worldY, bool clouds);
};
}  // namespace screen

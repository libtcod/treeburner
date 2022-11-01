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

// TODO :
// compute 4 regions and use a name type (norse, mesopotamian, fantasy, celtic) for each region.
// add 'on the border of x region' in the goat soup
#include "screen/school.hpp"

#include <math.h>
#include <stdio.h>

#include "main.hpp"
#include "screen/mainmenu.hpp"
#include "util_subcell.hpp"

#define SQR(x) ((x) * (x))
#define MENUY 5

static const TCODColor PAPER_COLOR(46, 28, 18);
static const TCODColor TEXT_COLOR(184, 148, 86);
static const TCODColor HIGHLIGHTED_COLOR(241, 221, 171);

#define MENU_WIDTH CON_W - MAP_WIDTH / 2 + 3

SchoolScreen* SchoolScreen::instance = NULL;

const char* schoolTypeName[NB_SCHOOLS] = {
    "pyromancy",
    "hydromancy",
};

const char* schoolDesc[NB_SCHOOLS] = {
    "The art of pyromancy is about dealing the maximum damages through burns and exploding projectiles. Pyromancers "
    "are amongst the most feared fighters on Doryen. They become vulnerable only at melee range but you are generally "
    "turned into ashes before reaching this point...",
    "Hydromancers excel in disabling their enemies by freezing them or knocking them with magical water jets. They "
    "finish their immobilized enemy off with deadly ice shards. Their defensive spells make them less vulnerable to "
    "melee attacks than pyromancer, at the expense of having fewer offensive spells.",
};

SchoolScreen::SchoolScreen()
    : Screen(0), offx(0), offy(0), dx(0), dy(0), world(NULL), selectedSchool(0), worldGenerated(false), textGen(NULL) {
  instance = this;
}

void SchoolScreen::generateWorld(uint32_t seed) {
  static float lightDir[3] = {1.0f, 1.0f, 0.0f};

  if (!world) {
    // load resources
    world = new TCODImage(MAP_WIDTH, MAP_HEIGHT);
    con = new TCODConsole(MAP_WIDTH / 2, MAP_HEIGHT / 2);
    mapmask = new TCODImage("data/img/mapmask.png");
    mapreflection = new TCODImage("data/img/mapreflection.png");

    // precompute fisheye distorsion
    for (int px = 0; px < MAP_WIDTH; px++) {
      for (int py = 0; py < MAP_HEIGHT; py++) {
        int deltax = px - MAP_WIDTH / 2;
        int deltay = py - MAP_HEIGHT / 2;
        // distance from map center
        float rad = MAX(ABS(deltax) / 53.0, ABS(deltay) / 46.0);
        rad = CLAMP(0.0f, 1.0f, rad);  //  map sphere radius = 22 console cell = 44 map subcells
        float cs = cosf(rad * 3.14159f * 0.5f);
        float dist;
        if (ABS(cs) < 0.0001f)
          dist = 1000;
        else
          dist = (1.0f / cs);
        // angle
        float angle = (float)atan2(deltay, deltax);
        fisheyex[px][py] = dist * cosf(angle);
        fisheyey[px][py] = dist * sinf(angle);
      }
    }
  }
  schoolRng = new TCODRandom(seed);
  worldGen.generate(schoolRng);
  worldGen.computeSunLight(lightDir);
  static bool firstActivation = true;
  if (config.getBoolProperty("config.debug") && firstActivation) {
    firstActivation = false;
    TCODImage worldimg(HM_WIDTH, HM_HEIGHT);
    // altitude map
    worldGen.saveAltitudeMap();
    // temperature map of the world
    worldGen.saveTemperatureMap();
    // precipitation map of the world
    worldGen.savePrecipitationMap();
    // generate a PNG with the world map
    worldGen.worldmap_.save("world.png");
    // generate the biome map
    worldGen.saveBiomeMap();
    // and the shaded version
    for (int x = 0; x < HM_WIDTH; x++) {
      for (int y = 0; y < HM_HEIGHT; y++) {
        worldimg.putPixel(x, y, getMapShadedColor(x + 0.5f, y + 0.5f, false));
      }
    }
    worldimg.save("world_shaded.png");
  }
  textGen = new TextGenerator("data/cfg/school.txg", schoolRng);
  textGen->setLocalFunction("RANDOM_INT", new RandomIntFunc(schoolRng));
  textGen->setLocalFunction("RANDOM_NAME", new RandomNameFunc(schoolRng));

  // find suitable position for schools
  for (int i = 0; i < NB_SCHOOLS; i++) {
    School* sch = &school[i];
    sch->x = rng->getInt(44, worldGen.getWidth() - 44);
    sch->y = rng->getInt(44, worldGen.getHeight() - 44);
    sch->type = (ESchool)i;
    while (!isPosOk(i)) {
      sch->x++;
      if (sch->x >= worldGen.getWidth() - 44) {
        sch->x = 44;
        sch->y++;
        if (sch->y >= worldGen.getHeight() - 44) {
          sch->y = 44;
        }
      }
    }
    // generate school name
    strcpy(sch->name, NameGenerator::generateRandomName(schoolRng));
    // determin terrain type
    int terrain1 = getTerrainType(sch->x, sch->y, 16);
    int terrain2 = getTerrainType(sch->x, sch->y, 4);
    if (terrain1 <= 2)
      sch->terrain = School::SHORE;
    else if (terrain1 <= 4)
      sch->terrain = School::PLAIN;
    else if (terrain1 <= 7)
      sch->terrain = School::FOREST_EDGE;
    else if (terrain1 <= 9)
      sch->terrain = School::FOREST;
    else if (terrain2 <= 12)
      sch->terrain = School::MOUNTAIN;
    else
      sch->terrain = School::SNOW;
    strcpy(sch->desc, genSchoolDescription(sch));
  }
  worldGenerated = true;
}

void SchoolScreen::onActivate() {
  engine.setKeyboardMode(UMBRA_KEYBOARD_RELEASED);
  if (!worldGenerated) MainMenu::instance->waitForWorldGen();
  selectSchool(0);
  offx = rng->getFloat(MAP_WIDTH, HM_WIDTH - MAP_WIDTH - 1);
  offy = rng->getFloat(MAP_WIDTH, HM_HEIGHT - MAP_HEIGHT - 1);
  offx = rng->getFloat(MAP_WIDTH, HM_WIDTH - MAP_WIDTH - 1);
  offy = rng->getFloat(MAP_WIDTH, HM_HEIGHT - MAP_HEIGHT - 1);
  Screen::onActivate();
}

// check if the position of the school is ok
bool SchoolScreen::isPosOk(int schoolNum) const {
#define MIN_SCHOOL_DIST 100
  // in water ?
  if (worldGen.getAltitude(school[schoolNum].x, school[schoolNum].y) <= worldGen.getSandHeight()) return false;
  // to close from another school
  for (int i = 0; i < schoolNum; i++) {
    int d = SQR(school[i].x - school[schoolNum].x) + SQR(school[i].y - school[schoolNum].y);
    if (d < MIN_SCHOOL_DIST) return false;  // school i too close
  }
  return true;
}

int SchoolScreen::getTerrainType(int x, int y, int range) const {
  int num = 0;
  float h = 0.0f;
  for (int px = x - range; px <= x + range; px++) {
    for (int py = y - range; py <= y + range; py++) {
      if (IN_RECTANGLE(px, py, worldGen.getWidth(), worldGen.getHeight())) {
        num++;
        h += worldGen.getAltitude(px, py);
      }
    }
  }
  int typeidx = (int)(h * 16 / num);
  typeidx = CLAMP(0, 16, typeidx);
  return typeidx;
}

TCODColor SchoolScreen::getMapShadedColor(float worldX, float worldY, bool clouds) {
  // sun color & direction
  static TCODColor sunCol(255, 255, 200);
  float wx = CLAMP(0.0f, worldGen.getWidth() - 1, worldX);
  float wy = CLAMP(0.0f, worldGen.getHeight() - 1, worldY);
  // apply cloud shadow
  float cloudAmount = clouds ? worldGen.getCloudThickness(wx, wy) : 0.0f;
  TCODColor col = worldGen.getInterpolatedColor(worldX, worldY);

  // apply sun light
  float intensity = worldGen.getInterpolatedIntensity(wx, wy);
  intensity = MIN(intensity, 1.5f - cloudAmount);
  int cr = (int)(intensity * (int)(col.r) * sunCol.r / 255);
  int cg = (int)(intensity * (int)(col.g) * sunCol.g / 255);
  int cb = (int)(intensity * (int)(col.b) * sunCol.b / 255);
  TCODColor col2;
  col2.r = CLAMP(0, 255, cr);
  col2.g = CLAMP(0, 255, cg);
  col2.b = CLAMP(0, 255, cb);
  col2.r = MAX(col2.r, col.r / 2);
  col2.g = MAX(col2.g, col.g / 2);
  col2.b = MAX(col2.b, col.b / 2);
  return col2;
}

void SchoolScreen::render() {
  background.blit2x(TCODConsole::root, 0, 0);
  // compute the map image
  for (int px = 0; px < MAP_WIDTH; px++) {
    for (int py = 0; py < MAP_HEIGHT; py++) {
      // world texel coordinate (with fisheye distorsion)
      float wx = px + offx + fisheyex[px][py];
      float wy = py + offy + fisheyey[px][py];
      world->putPixel(px, py, getMapShadedColor(wx, wy, true));
    }
  }
  // apply map mask
  for (int x = 0; x < MAP_WIDTH; x++) {
    for (int y = 0; y < MAP_HEIGHT; y++) {
      // mask color
      TCODColor mask = mapmask->getPixel(x, y);
      // actual world color
      TCODColor col = world->getPixel(x, y);
      // reflection color
      TCODColor refl = mapreflection->getPixel(x, y);
      if (refl.r > 65)
        col = TCODColor::lerp(col, TCODColor::white, (refl.r - 65) / 380.0f);
      else if (refl.r < 65)
        col = TCODColor::lerp(col, TCODColor::black, (65 - refl.r) / 130.0f);
      // color from the background
      TCODColor bkcol = background.getPixel((CON_W - 1 - MAP_WIDTH / 2) * 2 + x, (CON_H * 2 - MAP_HEIGHT) / 2 + y);
      col = TCODColor::lerp(col, bkcol, mask.r / 255.0);
      world->putPixel(x, y, col);
    }
  }

  // blit the map on the offscreen console
  world->blit2x(con, 0, 0);

  // add school markers and names
  for (int i = 0; i < NB_SCHOOLS; i++) {
    int screenx = (int)(school[i].x - offx) / 2;
    int screeny = (int)(school[i].y - offy) / 2;
    if (screenx < 2 || screenx > MAP_WIDTH / 2 - 2) continue;
    if (screeny < 2 || screeny > MAP_HEIGHT / 2 - 2) continue;

    int l = strlen(school[i].name);
    int labelx = screenx - l / 2;
    labelx = CLAMP(0, CON_W - l, labelx);
    if (school[i].terrain == School::SNOW) {
      con->setDefaultForeground(TCODColor::darkerGrey);
    } else {
      con->setDefaultForeground(TCODColor::white);
    }
    con->putChar(screenx, screeny, '+', TCOD_BKGND_NONE);
    con->print(labelx, screeny == MAP_HEIGHT / 2 - 1 ? screeny - 1 : screeny + 1, school[i].name);
  }

  // blit offscreen console on root console
  TCODConsole::blit(
      con,
      0,
      0,
      MAP_WIDTH / 2,
      MAP_HEIGHT / 2,
      TCODConsole::root,
      CON_W - 1 - MAP_WIDTH / 2,
      (CON_H * 2 - MAP_HEIGHT) / 4);

  // render the menu
  darken(1, MENUY + 3 + selectedSchool, MENU_WIDTH - 10, 1, 0.5);
  TCODConsole::root->setDefaultForeground(TEXT_COLOR);
  TCODConsole::root->print(1, MENUY + 1, "Select a school");
  for (int i = 0; i < NB_SCHOOLS; i++) {
    if (i == selectedSchool) {
      TCODConsole::root->setDefaultForeground(HIGHLIGHTED_COLOR);
    } else {
      TCODConsole::root->setDefaultForeground(TEXT_COLOR);
    }
    TCODConsole::root->print(2, MENUY + 3 + i, schoolTypeName[i]);
  }
  TCODConsole::root->setDefaultForeground(TEXT_COLOR);
  int schoolDescHeight =
      TCODConsole::root->printRect(2, MENUY + 3 + NB_SCHOOLS + 1, MENU_WIDTH - 4, 0, school[selectedSchool].desc);
  TCODConsole::root->printRect(
      2, MENUY + 3 + NB_SCHOOLS + 2 + schoolDescHeight, MENU_WIDTH - 4, 0, schoolDesc[selectedSchool]);
  TCODConsole::root->print(2, CON_H - 2, "Click on the map to scroll");
}

void SchoolScreen::setContextSchool(School* sch) {
  gameEngine->player.school = *sch;
  TextGenerator::deleteGlobalValue("SCHOOL_NAME");
  TextGenerator::deleteGlobalValue("SCHOOL_TYPE");
  TextGenerator::deleteGlobalValue("PLAYER_CLASS");
  TextGenerator::addGlobalValue("SCHOOL_NAME", sch->name);
  TextGenerator::addGlobalValue("SCHOOL_TYPE", schoolTypeName[sch->type]);
  // convert pyromancy to pyromancer
  char tmp[128];
  strcpy(tmp, schoolTypeName[sch->type]);
  tmp[strlen(tmp) - 1] = 0;
  TextGenerator::addGlobalValue("PLAYER_CLASS", "%ser", tmp);
}

bool SchoolScreen::update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) {
  if (fade == FADE_DOWN && fadeLvl <= 0.0f) {
    return false;
  }
  bool up = false, down = false, left = false, right = false;
  mob::Player::getMoveKey(k, &up, &down, &left, &right);
  if (down || right) {
    selectSchool(selectedSchool + 1);
  } else if (up || left) {
    selectSchool(selectedSchool - 1);
  } else if (
      ((k.vk == TCODK_ENTER || k.vk == TCODK_KPENTER) && k.vk != TCODK_ALT) ||
      (mouse.cy >= MENUY + 3 && mouse.cy < MENUY + 3 + NB_SCHOOLS && mouse.cx >= 2 && mouse.cx < 2 + MENU_WIDTH &&
       (mouse.lbutton_pressed || mouse.rbutton_pressed))) {
    if (fade != FADE_DOWN) {
      fade = FADE_DOWN;
      setContextSchool(&school[selectedSchool]);
    }
  } else if (
      mouse.lbutton && mouse.cx >= CON_W - 1 - MAP_WIDTH / 2 && mouse.cx >= (CON_H * 2 - MAP_HEIGHT) / 4 &&
      mouse.cx < (CON_H * 2 - MAP_HEIGHT) / 4 + MAP_HEIGHT) {
    dx = 2 * (mouse.cx - (CON_W - 1 - MAP_WIDTH / 2)) + offx - MAP_WIDTH / 2;
    dy = 2 * (mouse.cy - (CON_H * 2 - MAP_HEIGHT) / 4) + offy - MAP_HEIGHT / 2;
    dx = CLAMP(0, HM_WIDTH - MAP_WIDTH - 1, dx);
    dy = CLAMP(0, HM_HEIGHT - MAP_HEIGHT - 1, dy);
  }
  // update current offset
  elapsed = MIN(0.1f, elapsed);
  offx += (dx - offx) * elapsed * 1.5f;
  offy += (dy - offy) * elapsed * 1.5f;
  // update cloud layer
  worldGen.updateClouds(elapsed);
  return true;
}

void SchoolScreen::selectSchool(int num) {
  if (num < 0)
    num = NB_SCHOOLS - 1;
  else if (num >= NB_SCHOOLS)
    num = 0;
  selectedSchool = num;
  // compute destination offset
  int dw = worldGen.getWidth() - MAP_WIDTH;
  int dh = worldGen.getHeight() - MAP_HEIGHT;
  dx = school[num].x - MAP_WIDTH / 2;
  dy = school[num].y - MAP_HEIGHT / 2;
  dx = CLAMP(0, dw, dx);
  dy = CLAMP(0, dh, dy);
  gameEngine->player.school = school[selectedSchool];
}

const char* SchoolScreen::genSchoolDescription(School* sch) {
  setContextSchool(sch);
  switch (sch->terrain) {
    case School::SHORE:
      return textGen->generate("school", "${SHORE}");
      break;
    case School::PLAIN:
      return textGen->generate("school", "${PLAIN}");
      break;
    case School::FOREST_EDGE:
      return textGen->generate("school", "${FOREST_EDGE}");
      break;
    case School::FOREST:
      return textGen->generate("school", "${FOREST}");
      break;
    case School::MOUNTAIN:
      return textGen->generate("school", "${MOUNTAIN}");
      break;
    case School::SNOW:
      return textGen->generate("school", "${SNOW}");
      break;
    default:
      return NULL;
      break;
  }
}

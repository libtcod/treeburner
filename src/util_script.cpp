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

#include "main.hpp"
#ifndef NO_LUA
#include <lua.hpp>

int scHasCreatureItemType(lua_State* L) {
  Creature* cr = NULL;
  const char* creatureName = lua_tostring(L, 1);
  cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    lua_pushboolean(L, 0);
    return 1;
  }
  const char* typeName = lua_tostring(L, 2);
  for (Item** it = cr->inventoryBegin(); it != cr->inventoryEnd(); it++) {
    if ((*it)->isA(typeName)) {
      lua_pushboolean(L, 1);
      return 1;
    }
  }
  lua_pushboolean(L, 0);
  return 1;
}

int scGetBestCreatureFoodName(lua_State* L) {
  Creature* cr = NULL;
  const char* creatureName = lua_tostring(L, 1);
  cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    lua_pushboolean(L, 0);
    return 1;
  }
  int best = 0;
  const char* bestName = "";
  for (Item** it = cr->inventoryBegin(); it != cr->inventoryEnd(); it++) {
    if ((*it)->isA("food")) {
      ItemFeature* foodFeat = (*it)->getFeature(ITEM_FEAT_FOOD);
      if (foodFeat && foodFeat->food.health > best) {
        best = foodFeat->food.health;
        bestName = (*it)->typeData->name;
      }
    }
  }
  lua_pushstring(L, bestName);
  return 1;
}

int scGetBestCreatureFoodHP(lua_State* L) {
  Creature* cr = NULL;
  const char* creatureName = lua_tostring(L, 1);
  cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    lua_pushboolean(L, 0);
    return 1;
  }
  int best = 0;
  for (Item** it = cr->inventoryBegin(); it != cr->inventoryEnd(); it++) {
    if ((*it)->isA("food")) {
      ItemFeature* foodFeat = (*it)->getFeature(ITEM_FEAT_FOOD);
      if (foodFeat && foodFeat->food.health > best) best = foodFeat->food.health;
    }
  }
  lua_pushnumber(L, best);
  return 1;
}

int scLog(MessageSeverity sev, lua_State* L) {
  const char* msg = lua_tostring(L, 1);
  switch (sev) {
    case DEBUG:
      gameEngine->gui.log.debug(msg);
      break;
    case INFO:
      gameEngine->gui.log.info(msg);
      break;
    case WARN:
      gameEngine->gui.log.warn(msg);
      break;
    case CRITICAL:
      gameEngine->gui.log.critical(msg);
      break;
    default:
      break;
  }
  return 0;
}
int scDebug(lua_State* L) { return scLog(DEBUG, L); }
int scInfo(lua_State* L) { return scLog(INFO, L); }
int scWarn(lua_State* L) { return scLog(WARN, L); }
int scError(lua_State* L) { return scLog(CRITICAL, L); }

int scAddObjectiveStep(lua_State* L) {
  const char* msg = lua_tostring(L, 1);
  gameEngine->gui.objectives.addStep(msg);
  return 0;
}

int scActivateObjective(lua_State* L) {
  if (lua_gettop(L) == 1) {
    const char* title = lua_tostring(L, 1);
    gameEngine->gui.objectives.activateObjective(title);
  } else {
    gameEngine->gui.objectives.activateCurrent();
  }
  return 0;
}

int scCloseObjective(lua_State* L) {
  bool success = (bool)lua_toboolean(L, 1);
  gameEngine->gui.objectives.closeCurrent(success);
  return 0;
}

int scTextGen(lua_State* L) {
  const char* filename = lua_tostring(L, 1);
  const char* generator = lua_tostring(L, 2);
  const char* msg = lua_tostring(L, 3);
  TextGenerator txtGen(filename);
  const char* buf = txtGen.generate(generator, msg);
  lua_pushstring(L, buf);
  return 1;
}

int scCreatureTalk(lua_State* L) {
  const char* creatureName = lua_tostring(L, 1);
  const char* msg = lua_tostring(L, 2);
  Creature* cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    return 0;
  }
  cr->talk(msg);
  return 0;
}

int scCreaturePos(lua_State* L) {
  const char* creatureName = lua_tostring(L, 1);
  Creature* cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    return 0;
  }
  lua_pushnumber(L, cr->x);
  lua_pushnumber(L, cr->y);
  return 2;
}

int scCreatureLife(lua_State* L) {
  const char* creatureName = lua_tostring(L, 1);
  Creature* cr = gameEngine->dungeon->getCreature(creatureName);
  if (!cr) {
    return 0;
  }
  lua_pushnumber(L, cr->life);
  return 1;
}
#endif

Script::Script() {
#ifndef NO_LUA
  lua_State* L = lua_open();
  luaL_openlibs(L);
  data = (void*)L;
#endif
  ref = -1;
  init();
}
Script::~Script() {
#ifndef NO_LUA
  lua_close((lua_State*)data);
#endif
}

bool Script::execute() {
#ifndef NO_LUA
  lua_State* L = (lua_State*)data;
  lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
  if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
    const char* err = lua_tostring(L, lua_gettop((lua_State*)data));
    lua_pop(L, 1);
    fprintf(stderr, "FATAL error while executing script : %s", err);
    gameEngine->gui.log.critical("SCRIPT ERROR : %s", err);
    return false;
  }
#endif
  return true;
}

bool Script::parse(const char* txt, ...) {
#ifndef NO_LUA
  static char buf[1024];
  va_list ap;
  va_start(ap, txt);
  vsprintf(buf, txt, ap);
  va_end(ap);
  lua_State* L = (lua_State*)data;
  if (luaL_loadstring(L, buf) != 0) {
    const char* err = lua_tostring(L, lua_gettop(L));
    lua_pop(L, 1);
    fprintf(stderr, "FATAL error while loading script : %s\n%s", err, buf);
    gameEngine->gui.log.critical("SCRIPT ERROR : %s", err);
    return false;
  }
  ref = luaL_ref(L, LUA_REGISTRYINDEX);
#endif
  return true;
}

void Script::init() {
#ifndef NO_LUA
  lua_State* L = (lua_State*)data;
  lua_register(L, "debug", scDebug);
  lua_register(L, "info", scInfo);
  lua_register(L, "warn", scWarn);
  lua_register(L, "error", scError);
  lua_register(L, "addObjectiveStep", scAddObjectiveStep);
  lua_register(L, "activateObjective", scActivateObjective);
  lua_register(L, "closeObjective", scCloseObjective);
  lua_register(L, "hasCreatureItemType", scHasCreatureItemType);
  lua_register(L, "getBestCreatureFoodName", scGetBestCreatureFoodName);
  lua_register(L, "getBestCreatureFoodHP", scGetBestCreatureFoodHP);
  lua_register(L, "textGen", scTextGen);
  lua_register(L, "creatureTalk", scCreatureTalk);
  lua_register(L, "creaturePos", scCreaturePos);
  lua_register(L, "creatureLife", scCreatureLife);
#endif
}

float Script::getFloatVariable(const char* name) {
  float ret = 0.0f;
#ifndef NO_LUA
  lua_State* L = (lua_State*)data;
  lua_getglobal(L, name);
  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 1);
    gameEngine->gui.log.critical("SCRIPT ERROR : try to read variable %s as float", name);
    return 0.0f;
  }
  ret = (float)lua_tonumber(L, -1);
  lua_pop(L, 1);
#endif
  return ret;
}

void Script::setFloatVariable(const char* name, float val) {
#ifndef NO_LUA
  lua_State* L = (lua_State*)data;
  lua_pushnumber(L, val);
  lua_setglobal(L, name);
#endif
}

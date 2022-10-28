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
#include <umbra/umbra.hpp>
#ifndef NO_SOUND
#include "fmod/fmod.h"
#include "fmod/fmod_errors.h"
#endif

#define VERSION "0.0.2"

// console size
#define CON_W 80
#define CON_H 50

#include "bas_aidirector.hpp"
#include "bas_entity.hpp"
#include "bas_gameengine.hpp"
#include "bas_noisything.hpp"
#include "bas_savegame.hpp"
#include "bas_userpref.hpp"
#include "item.hpp"
#include "item_modifier.hpp"
#include "map_building.hpp"
#include "map_cell.hpp"
#include "map_dungeon.hpp"
#include "map_light.hpp"
#include "map_lightmap.hpp"
#include "mob_behavior.hpp"
#include "mob_boss.hpp"
#include "mob_creature.hpp"
#include "mob_fish.hpp"
#include "mob_friend.hpp"
#include "mob_minion.hpp"
#include "mob_player.hpp"
#include "screen.hpp"
#include "screen_end.hpp"
#include "screen_forest.hpp"
#include "screen_game.hpp"
#include "screen_mainmenu.hpp"
#include "screen_school.hpp"
#include "screen_treeBurner.hpp"
#include "spell_fireball.hpp"
#include "ui_craft.hpp"
#include "ui_descriptor.hpp"
#include "ui_dialog.hpp"
#include "ui_gui.hpp"
#include "ui_input.hpp"
#include "ui_inventory.hpp"
#include "ui_messages.hpp"
#include "ui_objectives.hpp"
#include "ui_status.hpp"
#include "ui_tuto.hpp"
#include "util_carver.hpp"
#include "util_cavegen.hpp"
#include "util_cellular.hpp"
#include "util_clouds.hpp"
#include "util_fire.hpp"
#include "util_namegen.hpp"
#include "util_packer.hpp"
#include "util_powerup.hpp"
#include "util_ripples.hpp"
#include "util_script.hpp"
#include "util_sound.hpp"
#include "util_subcell.hpp"
#include "util_textgen.hpp"
#include "util_threadpool.hpp"
#include "util_worldgen.hpp"

#define IN_RECTANGLE(x, y, w, h) ((unsigned)(x) < (unsigned)(w) && (unsigned)(y) < (unsigned)(h))
#define SQRDIST(x1, y1, x2, y2) (((x1) - (x2)) * ((x1) - (x2)) + ((y1) - (y2)) * ((y1) - (y2)))
#define TCOD_CHAR_PROGRESSBAR 1

#ifndef NDEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

HDRColor getHDRColorProperty(const TCODParser& parser, const char* name);

extern TCODNoise noise1d;
extern TCODNoise noise2d;
extern TCODNoise noise3d;
extern TCODRandom* rng;
extern bool mouseControl;
extern bool newGame;
extern SaveGame saveGame;
extern UserPref userPref;
extern UmbraEngine engine;
extern GameEngine* gameEngine;
extern TCODImage background;
extern Sound sound;
extern ThreadPool* threadPool;
extern TerrainType terrainTypes[NB_TERRAINS];
extern TCODParser config;

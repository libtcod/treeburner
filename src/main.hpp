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
#include <umbra/umbra.hpp>

#include "bas_gameengine.hpp"
#include "bas_savegame.hpp"
#include "bas_userpref.hpp"
#include "map_cell.hpp"
#include "map_lightmap.hpp"
#include "util_sound.hpp"
#include "util_threadpool.hpp"

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

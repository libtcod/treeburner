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
#include <gsl/gsl>
#include <libtcod.hpp>
#include <umbra/umbra.hpp>

#include "gameengine.hpp"
#include "map_cell.hpp"
#include "map_lightmap.hpp"
#include "savegame.hpp"
#include "userpref.hpp"
#include "util_sound.hpp"
#include "util_threadpool.hpp"

/// @brief Return true if x and y in the bounds of a rectable shaped w and h.
template <typename T>
static constexpr auto IN_RECTANGLE(T x, T y, T w, T h) -> bool {
  return 0 <= x && 0 <= y && x < w && y < h;
}
static constexpr auto IN_RECTANGLE(float x, float y, int w, int h) -> bool {
  return IN_RECTANGLE(x, y, gsl::narrow<float>(w), gsl::narrow<float>(h));
}

/// @brief Return the squared distance of a vector.
template <typename T>
static constexpr auto SQRDIST(T x, T y) -> T {
  return x * x + y * y;
}
/// @brief Return the squared distance between two points.
template <typename T>
static constexpr auto SQRDIST(T x1, T y1, T x2, T y2) -> T {
  return SQRDIST(x1 - x2, y1 - y2);
}

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
extern TCODParser config;

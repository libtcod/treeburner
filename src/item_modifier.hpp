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

typedef enum {
  ITEM_MOD_FIRE_BEGIN,
  ITEM_MOD_FIRE_BLAST = ITEM_MOD_FIRE_BEGIN,
  ITEM_MOD_FIRE_SPARKLE_COUNT,
  ITEM_MOD_FIRE_SPARKLE_POWER,
  ITEM_MOD_FIRE_INCANDESCENCE_LIFE,
  ITEM_MOD_FIRE_INCANDESCENCE_SIZE,
  ITEM_MOD_FIRE_END = ITEM_MOD_FIRE_INCANDESCENCE_SIZE,

  ITEM_MOD_WATER_BEGIN,
  ITEM_MOD_WATER_END = ITEM_MOD_WATER_BEGIN,
  ITEM_MOD_NUMBER
} ItemModifierId;

extern const char* modifierName[ITEM_MOD_NUMBER];

class ItemModifier {
 public:
  ItemModifier(ItemModifierId id, float value) : id(id), value(value) {}
  ItemModifierId id;
  float value;

  static void renderDescription(TCODConsole* con, int x, int y, const TCODList<ItemModifier*>& list);
};

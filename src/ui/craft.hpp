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
#include <vector>

#include "item.hpp"
#include "mob/creature.hpp"
#include "ui/dialog.hpp"

namespace ui {
class Craft : public ui::Dialog, public ui::UIListener, public ui::Scrollable {
 public:
  Craft();
  void initialize(mob::Creature* owner, bool soft = false);
  void render() override;
  bool update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) override;

  // ui::UIListener
  bool onWidgetEvent(ui::Widget* widget, ui::EWidgetEvent event) override;

  // scrollable
  int getScrollTotalSize() override;
  const std::string& getScrollText(int idx) override;
  void getScrollColor(int idx, TCODColor* fore, TCODColor* back) override;

 protected:
  std::vector<item::Item*> items;
  int selectedItem;
  int selectedIngredient;
  bool selectedTool;
  ui::Scroller* scroller;
  Button clear;
  Button create;
  mob::Creature* owner;
  bool isDragging, isDraggingStart;
  int dragx, dragy, dragStartX, dragStartY;
  item::Item* dragItem;
  item::Item* tool;
  std::vector<item::Item*> ingredients;
  TCODList<item::ItemCombination*> recipes;
  item::Item* result;
  item::ItemCombination* recipe;

  void detectItem(TCOD_mouse_t& mouse);
  void computeResult();
  void computeRecipes();
};
}  // namespace ui

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
#include "ui/dialog.hpp"

namespace ui {
// gui skin colors
extern TCODColor guiBackground;
extern TCODColor guiHighlightedBackground;
extern TCODColor guiDisabledText;
extern TCODColor guiText;
extern TCODColor guiHighlightedText;

struct InventoryTab {
  //	int x;
  //	int len;
  int offset;
  std::vector<item::Item*> items;
};

class Inventory : public ui::Dialog, public ui::UIListener {
 public:
  Inventory();
  virtual ~Inventory() {}
  void initialize(mob::Creature* owner);
  void initialize(item::Item* container);
  void render() override;
  void onActivate() override;
  void onDeactivate() override;
  bool update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) override;

  // ui::UIListener
  bool onWidgetEvent(ui::Widget* widget, ui::EWidgetEvent event) override;

 protected:
  void activateItem();
  void runActionOnItem(item::ItemActionId id, item::Item* it);
  void checkDefaultAction(item::Item* it);

  InventoryTab tabs[item::NB_INV_TABS];
  // InventoryTabId curTabId,mouseTabId;
  ui::Tabs guiTabs;
  Button takeAll;
  Button craft;
  int selectedItem;
  bool closeOn;  // mouse hovers close button
  mob::Creature* owner;  // either owner or container is NULL
  item::Item* container;
  // for item combination
  item::ItemCombination* combination;
  item::Item* itemToCombine;
  item::Item* itemToCombine2;
  item::Item* combinationResult;
  bool firstOpen;  // launch tutorial the fist time inventory is open
  // drag & drop support
  bool isDragging, isDraggingStart, dragOut, cmenudrag;
  int dragx, dragy, dragStartX, dragStartY;
  item::Item* dragItem;
  // context (right-click) menu position
  int cmenux, cmenuy, cmenuitem, cmenuwidth, cmenuheight;
  bool cmenuon;
};
}  // namespace ui

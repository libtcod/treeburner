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

#include <stdio.h>

#include "main.hpp"

#define INV_WIDTH 70
#define INV_HEIGHT 40

// skin
TCODColor guiBackground(32, 16, 0);
TCODColor guiHighlightedBackground(128, 128, 0);
TCODColor guiDisabledText(95, 95, 95);
TCODColor guiText(255, 180, 0);
TCODColor guiHighlightedText(114, 114, 255);

static const char* tabNames[NB_INV_TABS] = {"All", "Arm", "Wea", "Foo", "Mis"};

Inventory::Inventory() : selectedItem(-1), closeOn(false) {
  con = new TCODConsole(INV_WIDTH, INV_HEIGHT);
  con->setKeyColor(TCODColor::black);
  rect.x = rect.y = 5;
  rect.w = INV_WIDTH / 2;
  rect.h = INV_HEIGHT;
  firstOpen = true;
  isDragging = false;
  dragOut = false;
  cmenuon = false;
  cmenudrag = false;
  flags = DIALOG_MODAL | DIALOG_CLOSABLE;
  closeButton.set(rect.w - 1, 0);
  for (int i = 0; i < NB_INV_TABS; i++) guiTabs.addTab(tabNames[i]);
  guiTabs.curTab = INV_ARMOR;
  takeAll.setLabel("Take all");
  takeAll.addListener(this);
  craft.setLabel("Craft");
  craft.addListener(this);
}

void Inventory::initialize(Creature* owner) {
  for (int i = 0; i < NB_INV_TABS; i++) {
    tabs[i].items.clear();
    tabs[i].offset = 0;
  }
  for (Item** it = owner->inventoryBegin(); it != owner->inventoryEnd(); it++) {
    tabs[(*it)->typeData->inventoryTab].items.push(*it);
    tabs[INV_ALL].items.push(*it);
  }
  for (; guiTabs.curTab < NB_INV_TABS && tabs[guiTabs.curTab].items.size() == 0; guiTabs.curTab++) {
  }
  if (guiTabs.curTab == NB_INV_TABS) guiTabs.curTab = 0;
  if (selectedItem >= tabs[guiTabs.curTab].items.size()) selectedItem = tabs[guiTabs.curTab].items.size() - 1;
  this->owner = owner;
  container = NULL;
  itemToCombine = itemToCombine2 = combinationResult = NULL;
  cmenuon = false;
  cmenudrag = false;
  rect.x = rect.y = 5;
  isDragging = dragOut = false;
  craft.setPos(rect.w / 2 - craft.w / 2, rect.h - 1);
}

void Inventory::initialize(Item* container) {
  for (int i = 0; i < NB_INV_TABS; i++) {
    tabs[i].items.clear();
    tabs[i].offset = 0;
  }
  for (Item** it = container->stack.begin(); it != container->stack.end(); it++) {
    tabs[(*it)->typeData->inventoryTab].items.push(*it);
    tabs[INV_ALL].items.push(*it);
  }
  for (; guiTabs.curTab < NB_INV_TABS && tabs[guiTabs.curTab].items.size() == 0; guiTabs.curTab++) {
  }
  if (guiTabs.curTab == NB_INV_TABS) guiTabs.curTab = 0;
  if (selectedItem >= tabs[guiTabs.curTab].items.size()) selectedItem = tabs[guiTabs.curTab].items.size() - 1;
  this->container = container;
  owner = NULL;
  itemToCombine = itemToCombine2 = combinationResult = NULL;
  cmenuon = false;
  cmenudrag = false;
  rect.x = 6 + INV_WIDTH / 2;
  rect.y = 5;
  isDragging = dragOut = false;
  takeAll.setPos(rect.w / 2 - takeAll.w / 2, rect.h - 1);
}

void Inventory::checkDefaultAction(Item* item) {
  // when looting, drop item is the default action instead of use
  // check that the default action is coherent with current mode (inventory or loot)
  if (container) return;  // concerns only inventory
  ItemActionId* dropAction = NULL;
  ItemActionId* useAction = NULL;
  bool dropFirst = false;
  for (ItemActionId* id = item->typeData->actions.begin(); id != item->typeData->actions.end(); id++) {
    ItemAction* action = ItemAction::getFromId(*id);
    if (action->onWater()) {
      if (!gameEngine->dungeon->hasRipples((int)gameEngine->player.x, (int)gameEngine->player.y)) {
        continue;
      }
    }
    if ((owner && action->onInventory()) || (container && action->onLoot())) {
      if (*id == ITEM_ACTION_USE) useAction = id;
      if (*id == ITEM_ACTION_DROP) {
        dropAction = id;
        if (useAction != NULL)
          dropFirst = false;
        else
          dropFirst = true;
      }
    }
  }
  if (useAction && dropAction) {
    if ((gameEngine->gui.loot.getActive() && !dropFirst) || (!gameEngine->gui.loot.getActive() && dropFirst)) {
      // bad default action. swap use and drop
      ItemActionId tmp = *useAction;
      *useAction = *dropAction;
      *dropAction = tmp;
    }
  }
}

bool Inventory::onWidgetEvent(Widget* widget, EWidgetEvent event) {
  if (widget == &takeAll && container) {
    InventoryTab* curTab = &tabs[guiTabs.curTab];
    while (!curTab->items.isEmpty()) {
      Item** it = curTab->items.begin();
      runActionOnItem(ITEM_ACTION_TAKE, *it);
      gameEngine->gui.inventory.guiTabs.curTab = INV_ALL;
    }
  } else if (widget == &craft && owner) {
    gameEngine->gui.setMode(GUI_CRAFT);
  }
  return false;
}

void Inventory::render() {
  int itemx = isDragging ? dragx : rect.x + INV_WIDTH / 4;
  int itemy = isDragging ? dragy + 1 : rect.y + selectedItem + 4;
  InventoryTab* curTab = &tabs[guiTabs.curTab];
  if (!dragOut) {
    // render the inventory frame
    con->setDefaultBackground(guiBackground);
    con->setDefaultForeground(guiText);
    int w2 = INV_WIDTH / 2;
    con->printFrame(
        0,
        0,
        w2,
        rect.h,
        true,
        TCOD_BKGND_SET,
        owner             ? "Inventory"
        : container->name ? container->name
                          : container->typeName);
    con->setChar(w2 - 1, 0, 'x');
    con->setCharForeground(w2 - 1, 0, closeOn ? guiHighlightedText : guiText);

    // render the tabs
    for (int i = 0; i < NB_INV_TABS; i++) {
      int numItems = tabs[i].items.size();
      if (numItems > 0) {
        char buf[128];
        sprintf(buf, "%s:%d", tabNames[i], numItems);
        guiTabs.setLabel(i, buf);
      } else {
        guiTabs.setLabel(i, tabNames[i]);
      }
    }
    guiTabs.render(con, 0, 0);

    // render the items list
    int ty = 3;
    int skip = curTab->offset;
    int num = 0;
    con->setDefaultForeground(guiText);
    for (Item** it = curTab->items.begin(); it != curTab->items.end() && ty < rect.y + rect.h - 2; it++, skip--) {
      if (skip <= 0) {
        con->setDefaultBackground(num == selectedItem ? guiHighlightedBackground : guiBackground);
        con->rect(2, ty, w2 - 4, 1, false, TCOD_BKGND_SET);
        con->setDefaultForeground(Item::classColor[(*it)->itemClass]);
        if ((*it)->isEquiped())
          con->print(2, ty++, "%s (equiped)", (*it)->aName());
        else
          con->print(2, ty++, (*it)->aName());
        num++;
      }
    }

    if (container)
      takeAll.render(con);
    else
      craft.render(con);

    blitSemiTransparent(con, 0, 0, INV_WIDTH / 2, INV_HEIGHT, TCODConsole::root, rect.x, rect.y, 0.8f, 1.0f);
  }
  if (cmenuon) {
    // render the context menu
    if (selectedItem >= 0) {
      Item* item = curTab->items.get(selectedItem);
      checkDefaultAction(item);
      cmenuheight = 0;
      cmenuwidth = 0;
      // compute context menu size
      for (ItemActionId* id = item->typeData->actions.begin(); id != item->typeData->actions.end(); id++) {
        ItemAction* action = ItemAction::getFromId(*id);
        if (action->onWater()) {
          if (!gameEngine->dungeon->hasRipples((int)gameEngine->player.x, (int)gameEngine->player.y)) {
            continue;
          }
        }
        if ((owner && action->onInventory()) || (container && action->onLoot())) {
          cmenuheight++;
          cmenuwidth = MAX(cmenuwidth, (int)strlen(action->name) + 2);
          if (*id == ITEM_ACTION_TAKE && item->count > 1) {
            // add one entry for 'Take all'
            cmenuheight++;
            ItemAction* takeAll = ItemAction::getFromId(ITEM_ACTION_TAKE_ALL);
            cmenuwidth = MAX(cmenuwidth, (int)strlen(takeAll->name) + 2);
          } else if (*id == ITEM_ACTION_DROP && item->count > 1) {
            // add one entry for 'Drop all'
            cmenuheight++;
            ItemAction* dropAll = ItemAction::getFromId(ITEM_ACTION_DROP_ALL);
            cmenuwidth = MAX(cmenuwidth, (int)strlen(dropAll->name) + 2);
          }
        }
      }
      int num = 0;
      if (cmenuwidth > 0) {
        TCODConsole::root->rect(cmenux, cmenuy, cmenuwidth, cmenuheight, true);
      }
      // print the actions names
      bool first = true;
      for (ItemActionId* id = item->typeData->actions.begin(); id != item->typeData->actions.end(); id++) {
        ItemAction* action = ItemAction::getFromId(*id);
        if (action->onWater()) {
          if (!gameEngine->dungeon->hasRipples((int)gameEngine->player.x, (int)gameEngine->player.y)) {
            continue;
          }
        }
        if ((owner && action->onInventory()) || (container && action->onLoot())) {
          TCODConsole::root->setDefaultForeground(
              num == cmenuitem ? TCODColor::white
              : first          ? guiHighlightedText
                               : guiText);
          TCODConsole::root->setDefaultBackground(num == cmenuitem ? guiHighlightedBackground : guiBackground);
          TCODConsole::root->printEx(
              cmenux,
              cmenuy + num,
              TCOD_BKGND_SET,
              TCOD_LEFT,
              " %s%*s",
              action->name,
              cmenuwidth - 1 - strlen(action->name),
              "");
          first = false;
          num++;
          if (*id == ITEM_ACTION_TAKE && item->count > 1) {
            TCODConsole::root->setDefaultForeground(
                num == cmenuitem ? TCODColor::white
                : first          ? guiHighlightedText
                                 : guiText);
            TCODConsole::root->setDefaultBackground(num == cmenuitem ? guiHighlightedBackground : guiBackground);
            ItemAction* takeAll = ItemAction::getFromId(ITEM_ACTION_TAKE_ALL);
            TCODConsole::root->printEx(
                cmenux,
                cmenuy + num,
                TCOD_BKGND_SET,
                TCOD_LEFT,
                " %s%*s",
                takeAll->name,
                cmenuwidth - 1 - strlen(takeAll->name),
                "");
            num++;
          } else if (*id == ITEM_ACTION_DROP && item->count > 1) {
            TCODConsole::root->setDefaultForeground(
                num == cmenuitem ? TCODColor::white
                : first          ? guiHighlightedText
                                 : guiText);
            TCODConsole::root->setDefaultBackground(num == cmenuitem ? guiHighlightedBackground : guiBackground);
            ItemAction* dropAll = ItemAction::getFromId(ITEM_ACTION_DROP_ALL);
            TCODConsole::root->printEx(
                cmenux,
                cmenuy + num,
                TCOD_BKGND_SET,
                TCOD_LEFT,
                " %s%*s",
                dropAll->name,
                cmenuwidth - 1 - strlen(dropAll->name),
                "");
            num++;
          }
        }
      }
    }
  } else {
    // render the item description
    if (dragOut && isDragging && dragItem) {
      TCODConsole::root->setChar(itemx, itemy - 2, dragItem->ch);
      TCODConsole::root->setCharForeground(itemx, itemy - 2, dragItem->col);
    } else if ((!isDragging && itemToCombine == NULL) || (isDragging && !dragOut && selectedItem == -1)) {
      Item* item = isDragging ? dragItem : selectedItem >= 0 ? curTab->items.get(selectedItem) : NULL;
      if (item) item->renderDescription(itemx, itemy);
    } else if (selectedItem >= 0) {
      if (itemToCombine2 == curTab->items.get(selectedItem)) {
        combinationResult->renderGenericDescription(itemx, itemy);
      } else if (itemToCombine || dragItem) {
        if (combinationResult) delete combinationResult;
        combinationResult = itemToCombine2 = NULL;
        combination = Item::getCombination(itemToCombine ? itemToCombine : dragItem, curTab->items.get(selectedItem));
        if (combination) {
          itemToCombine2 = curTab->items.get(selectedItem);
          combinationResult = Item::getItem(combination->resultType, 0, 0, false);
          combinationResult->count = combination->nbResult;
          combinationResult->addComponent(itemToCombine ? itemToCombine : dragItem);
          combinationResult->addComponent(curTab->items.get(selectedItem));
        } else {
          TCODConsole::root->setDefaultBackground(guiBackground);
          TCODConsole::root->setDefaultForeground(guiText);
          TCODConsole::root->printEx(itemx, itemy + 2, TCOD_BKGND_SET, TCOD_CENTER, "No combination");
        }
      }
    }
  }
}

void Inventory::activateItem() {
  Item* item = tabs[guiTabs.curTab].items.get(selectedItem);
  checkDefaultAction(item);
  if (combinationResult) {
    gameEngine->gui.log.info("You created %s", combinationResult->aName());
    combinationResult = gameEngine->player.addToInventory(combinationResult);
    // switch to the tab of the generated item
    if (guiTabs.curTab != INV_ALL) guiTabs.curTab = combinationResult->typeData->inventoryTab;
    combinationResult = NULL;
    bool destroy1 = false;
    // destroy items if needed
    TCODList<Item*> toDestroy;
    if (combination->ingredients[0].destroy) {
      if (itemToCombine->isA(combination->ingredients[0].type)) {
        toDestroy.push(itemToCombine);
        destroy1 = true;
      } else if (itemToCombine2->isA(combination->ingredients[0].type)) {
        toDestroy.push(itemToCombine2);
      }
    }
    if (combination->nbIngredients == 2 && combination->ingredients[1].destroy) {
      if (itemToCombine->isA(combination->ingredients[1].type)) {
        toDestroy.push(itemToCombine);
        destroy1 = true;
      } else if (itemToCombine2->isA(combination->ingredients[1].type)) {
        toDestroy.push(itemToCombine2);
      }
    }
    for (Item** it = toDestroy.begin(); it != toDestroy.end(); it++) {
      (*it)->toDelete = true;
    }
    // recompute inventory
    if (owner)
      initialize(owner);
    else
      initialize(container);
    itemToCombine2 = NULL;
    if (destroy1) {
      // itemToCombine was destroyed.
      itemToCombine = NULL;
    }
  } else {
    // check the first valid action
    ItemActionId* id = NULL;
    for (id = item->typeData->actions.begin(); id != item->typeData->actions.end(); id++) {
      ItemAction* action = ItemAction::getFromId(*id);
      if ((owner && action->onInventory()) || (container && action->onLoot())) break;
    }
    runActionOnItem(*id, item);
  }
}

void Inventory::onActivate() {
  Dialog::onActivate();
  if (firstOpen && gameEngine->gui.mode == GUI_INVENTORY) {
    firstOpen = false;
    gameEngine->gui.tutorial.startLiveTuto(TUTO_INVENTORY);
  }
}

void Inventory::onDeactivate() {
  Dialog::onDeactivate();
  if (combinationResult) delete combinationResult;
  combinationResult = NULL;
  if (owner) {
    if (gameEngine->gui.mode == GUI_INVLOOT)
      gameEngine->gui.mode = GUI_LOOT;
    else
      gameEngine->gui.mode = GUI_NONE;
  } else {
    if (gameEngine->gui.mode == GUI_INVLOOT)
      gameEngine->gui.mode = GUI_INVENTORY;
    else
      gameEngine->gui.mode = GUI_NONE;
  }
}

void Inventory::runActionOnItem(ItemActionId id, Item* item) {
  switch (id) {
    case ITEM_ACTION_USE:
      item->use();
      if (owner)
        initialize(owner);
      else
        initialize(container);
      break;
    case ITEM_ACTION_DROP:
    case ITEM_ACTION_DROP_ALL:
      if (gameEngine->gui.loot.getActive()) {
        Item* newItem = owner->removeFromInventory(item, id == ITEM_ACTION_DROP ? 1 : 0);
        newItem->putInContainer(gameEngine->gui.loot.container);
        gameEngine->gui.loot.initialize(gameEngine->gui.loot.container);
      } else {
        item->drop(id == ITEM_ACTION_DROP ? 1 : 0);
      }
      if (owner)
        initialize(owner);
      else
        initialize(container);
      break;
    case ITEM_ACTION_TAKE:
    case ITEM_ACTION_TAKE_ALL:
      item = item->putInInventory(&gameEngine->player, id == ITEM_ACTION_TAKE ? 1 : 0);
      initialize(container);
      if (gameEngine->gui.inventory.getActive()) {
        if (gameEngine->gui.inventory.guiTabs.curTab != INV_ALL)
          gameEngine->gui.inventory.guiTabs.curTab = item->typeData->inventoryTab;
        gameEngine->gui.inventory.initialize(&gameEngine->player);
      }
      break;
    case ITEM_ACTION_FILL: {
      Item* water = Item::getItem("water", 0, 0);
      water->putInContainer(item);
      item->computeBottleName();
    } break;
    case ITEM_ACTION_THROW:
      isDraggingStart = isDragging = true;
      dragItem = item;
      dragOut = true;
      cmenuon = false;
      cmenudrag = true;
      break;
    case ITEM_ACTION_DISASSEMBLE: {
      // create components
      for (Item** it = item->components.begin(); it != item->components.end(); it++) {
        if (owner)
          owner->addToInventory(*it);
        else
          (*it)->putInContainer(container);
      }
      // destroy item
      item->destroy(1);
      if (owner)
        initialize(owner);
      else
        initialize(container);
    } break;
    default:
      break;
  }
}

bool Inventory::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) {
  int mx = mouse.cx - rect.x;
  int my = mouse.cy - rect.y;
  closeOn = (mx == rect.w / 2 - 1 && my == 0);
  guiTabs.update(elapsed, k, mouse, rect.x, rect.y);
  if (container)
    takeAll.update(elapsed, k, mouse, rect.x, rect.y);
  else
    craft.update(elapsed, k, mouse, rect.x, rect.y);
  if (!isDragging && !cmenuon && mouse.rbutton) {
    // initialize context menu
    cmenuon = true;
    cmenux = mouse.cx;
    cmenuy = mouse.cy + 1;
    cmenuitem = -1;
    cmenuwidth = cmenuheight = 0;  // computed in render
  } else if (cmenuon && !mouse.rbutton) {
    cmenuon = false;
    if (cmenuitem >= 0 && selectedItem >= 0) {
      // execute context menu action
      Item* item = tabs[guiTabs.curTab].items.get(selectedItem);
      ItemActionId* id = NULL;
      int count = cmenuitem;
      ItemActionId actionId = (ItemActionId)0;
      for (id = item->typeData->actions.begin(); id != item->typeData->actions.end(); id++) {
        ItemAction* action = ItemAction::getFromId(*id);
        if ((owner && action->onInventory()) || (container && action->onLoot())) {
          if (count == 0) {
            actionId = *id;
            break;
          }
          count--;
          if (*id == ITEM_ACTION_TAKE && item->count > 1) {
            if (count == 0) {
              actionId = ITEM_ACTION_TAKE_ALL;
              break;
            }
            count--;
          } else if (*id == ITEM_ACTION_DROP && item->count > 1) {
            if (count == 0) {
              actionId = ITEM_ACTION_DROP_ALL;
              break;
            }
            count--;
          }
        }
      }

      runActionOnItem(actionId, item);
      if (actionId == ITEM_ACTION_THROW) {
        dragx = dragStartX = mouse.cx;
        dragy = dragStartY = mouse.cy;
      }
    }
  } else if (cmenuon) {
    if (mouse.cx >= cmenux && mouse.cx <= cmenux + cmenuwidth && mouse.cy >= cmenuy &&
        mouse.cy <= cmenuy + cmenuheight) {
      cmenuitem = mouse.cy - cmenuy;
    } else {
      cmenuitem = -1;
    }
  } else {
    selectedItem = -1;
    if (my >= 3 && my < 3 + tabs[guiTabs.curTab].items.size() && mx > 1 && mx < rect.x + rect.w / 2) {
      selectedItem = my - 3;
      if ((!isDragging || combinationResult) && mouse.lbutton_pressed) {
        activateItem();
      }
    }
  }
  if (!cmenuon && !isDraggingStart && mouse.lbutton && selectedItem >= 0) {
    // start dragging. to be confirmed
    isDraggingStart = true;
    dragItem = tabs[guiTabs.curTab].items.get(selectedItem);
    dragOut = false;
    dragStartX = mouse.cx;
    dragStartY = mouse.cy;
    dragx = mouse.cx;
    dragy = mouse.cy;
  } else if (isDraggingStart && !isDragging && mouse.lbutton) {
    dragx = mouse.cx;
    dragy = mouse.cy;
    if (dragx != dragStartX || dragy != dragStartY) {
      // dragging confirmed
      isDragging = true;
      itemToCombine = dragItem;
    }
  } else if (
      (isDragging || isDraggingStart) && ((!cmenudrag && !mouse.lbutton) || (cmenudrag && mouse.lbutton_pressed))) {
    // drop
    if (isDragging) {
      itemToCombine = NULL;
    }
    isDragging = isDraggingStart = false;
    if (dragOut) {
      int dungeonx = mouse.cx + gameEngine->xOffset;
      int dungeony = mouse.cy - 1 + gameEngine->yOffset;
      Dungeon* dungeon = gameEngine->dungeon;
      dragOut = false;
      if (!IN_RECTANGLE(dungeonx, dungeony, dungeon->width, dungeon->height) || mouse.rbutton) {
        // out of map or right click : cancel
      } else {
        dragItem->x = owner ? owner->x : container->x;
        dragItem->y = owner ? owner->y : container->y;
        if (dungeonx == gameEngine->player.x && dungeony == gameEngine->player.y) {
          if (owner) {
            // drag from inventory and drop on player cell = drop
            dragItem->drop();
            initialize(owner);
          } else {
            // drag from loot screen and drop on player cell = take
            dragItem->putInInventory(&gameEngine->player);
            initialize(container);
          }
        } else {
          // drag and drop on another cell = throw
          float dx = dungeonx - (owner ? owner->x : container->x);
          float dy = dungeony - (owner ? owner->y : container->y);
          float invLength = Entity::fastInvSqrt(dx * dx + dy * dy);
          dx *= invLength;
          dy *= invLength;
          Item* newItem = NULL;
          if (owner)
            newItem = owner->removeFromInventory(dragItem);
          else
            newItem = dragItem->removeFromList(&container->stack);
          newItem->dx = dx;
          newItem->dy = dy;
          newItem->speed = 1.0f / (invLength * 1.5f);
          newItem->speed = MIN(12.0f, newItem->speed);
          newItem->duration = 1.5f;
          dungeon->addItem(newItem);
          if (owner)
            initialize(owner);
          else
            initialize(container);
        }
        return false;
      }
    }
    dragItem = NULL;
  } else if (isDragging) {
    if (mouse.rbutton) {
      // cancel dragging
      isDragging = isDraggingStart = false;
      dragOut = cmenudrag = false;
      dragItem = NULL;
    } else if (mouse.lbutton || cmenudrag) {
      if (!dragOut && isDragging &&
          (dragx < rect.x || dragx >= rect.x + INV_WIDTH / 2 || dragy < rect.y || dragy >= rect.y + INV_HEIGHT)) {
        // dragging out of inventory frame
        dragOut = true;
      }
      dragx = mouse.cx;
      dragy = mouse.cy;
    }
  }
  if (!k.pressed) {
    if (k.c == 'A' || k.c == 'a')
      guiTabs.curTab = INV_ARMOR;
    else if (k.c == 'W' || k.c == 'w')
      guiTabs.curTab = INV_WEAPON;
    else if (k.c == 'F' || k.c == 'f')
      guiTabs.curTab = INV_FOOD;
    else if (k.c == 'M' || k.c == 'm')
      guiTabs.curTab = INV_MISC;
    if (strchr("AWFMawfm", k.c)) {
      k.vk = TCODK_NONE;
      k.c = 0;
    }
  }
  if (itemToCombine && ((k.vk == TCODK_ESCAPE && !k.pressed) || mouse.rbutton_pressed)) {
    // cancel item combination
    itemToCombine = NULL;
    if (combinationResult) delete combinationResult;
    combinationResult = NULL;
    k.vk = TCODK_NONE;
  }
  if ((k.vk == TCODK_ESCAPE && !k.pressed)) {
    return false;
  }
  return true;
}

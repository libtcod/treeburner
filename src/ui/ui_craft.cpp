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
#include "ui_craft.hpp"

#include "helpers.hpp"
#include "main.hpp"
#include "ui_inventory.hpp"
#include "util_subcell.hpp"

#define CRAFT_WIDTH 70
#define CRAFT_HEIGHT 40

Craft::Craft() {
  con = new TCODConsole(CRAFT_WIDTH, CRAFT_HEIGHT);
  rect.x = rect.y = 5;
  rect.w = CRAFT_WIDTH;
  rect.h = CRAFT_HEIGHT;
  flags = DIALOG_MODAL | DIALOG_CLOSABLE;
  closeButton.set(rect.w - 1, 0);
  scroller = new Scroller(this, CRAFT_WIDTH / 2 - 2, CRAFT_HEIGHT - 3);
  create.setLabel("Create");
  create.setPos(3 * rect.w / 4 + 1, rect.h - 1);
  create.addListener(this);
  clear.setLabel("Clear");
  clear.setPos(3 * rect.w / 4 - 5, rect.h - 1);
  clear.addListener(this);
}

void Craft::initialize(Creature* owner, bool soft) {
  items.clear();
  this->owner = owner;
  if (!soft) {
    tool = result = NULL;
    ingredients.clear();
  }
  computeRecipes();
  for (item::Item* it : owner->getInventory()) {
    if ((it->isIngredient() || it->isTool()) && !helpers::contains(ingredients, it) && tool != it) items.push_back(it);
  }
  selectedItem = -1;
  isDragging = isDraggingStart = false;
  dragItem = NULL;
  selectedIngredient = -1;
  selectedTool = false;
}

int Craft::getScrollTotalSize() { return items.size(); }

const std::string& Craft::getScrollText(int idx) { return items.at(idx)->aName(); }

void Craft::getScrollColor(int idx, TCODColor* fore, TCODColor* back) {
  item::Item* item = items.at(idx);
  // check if the item can be used in the current recipes list
  bool enabled = false;
  for (item::ItemCombination** it = recipes.begin(); it != recipes.end(); it++) {
    if ((*it)->isTool(item)) {
      enabled = true;
      break;
    }
    const item::ItemIngredient* ing = (*it)->getIngredient(item);
    if (ing && ing->quantity <= item->count_) {
      enabled = true;
      break;
    }
  }
  *fore = enabled ? item::Item::classColor[item->item_class_] : guiDisabledText;
  *back = (enabled && idx == selectedItem ? guiHighlightedBackground : guiBackground);
}

void Craft::render() {
  renderFrame(1.0f, "Craft");
  con->setDefaultBackground(guiBackground);
  con->setDefaultForeground(guiText);
  con->printFrame(0, 1, rect.w / 2 + 1, rect.h - 1, true, TCOD_BKGND_SET, "Inventory");
  if (dragItem && !dragItem->isTool()) con->setDefaultForeground(guiDisabledText);
  con->printFrame(rect.w / 2, 1, rect.w - rect.w / 2, rect.h - 1, true, TCOD_BKGND_SET, "Tool");
  if (dragItem && !dragItem->isIngredient())
    con->setDefaultForeground(guiDisabledText);
  else
    con->setDefaultForeground(guiText);
  con->printFrame(rect.w / 2, 4, rect.w - rect.w / 2, rect.h - 4, true, TCOD_BKGND_SET, "Ingredients");
  con->setDefaultForeground(guiText);
  con->printFrame(
      rect.w / 2, rect.h / 2 + 4, rect.w - rect.w / 2, rect.h - rect.h / 2 - 4, true, TCOD_BKGND_SET, "Result");
  con->setChar(rect.w / 2, 1, TCOD_CHAR_TEES);
  con->setChar(rect.w / 2, rect.h - 1, TCOD_CHAR_TEEN);
  con->setChar(rect.w - rect.w / 2, 4, TCOD_CHAR_TEEE);
  con->setChar(rect.w - 1, 4, TCOD_CHAR_TEEW);
  con->setChar(rect.w - rect.w / 2, rect.h / 2 + 4, TCOD_CHAR_TEEE);
  con->setChar(rect.w - 1, rect.h / 2 + 4, TCOD_CHAR_TEEW);

  // inventory
  scroller->render(con, 1, 2);
  scroller->renderScrollbar(con, 1, 2);

  // ingredient list
  int y = 5;
  for (item::Item* it : ingredients) {
    con->setDefaultForeground(item::Item::classColor[it->item_class_]);
    con->setDefaultBackground(y - 5 == selectedIngredient ? guiHighlightedBackground : guiBackground);
    con->printEx(rect.w / 2 + 1, y++, TCOD_BKGND_SET, TCOD_LEFT, it->aName().c_str());
  }

  // tool
  if (tool) {
    con->setDefaultForeground(item::Item::classColor[tool->item_class_]);
    con->setDefaultBackground(selectedTool ? guiHighlightedBackground : guiBackground);
    con->printEx(rect.w / 2 + 1, 2, TCOD_BKGND_SET, TCOD_LEFT, tool->aName().c_str());
  }

  // buttons
  if (result) create.render(con);
  if (tool || !ingredients.empty()) clear.render(con);

  blitSemiTransparent(con, 0, 0, rect.w, rect.h, TCODConsole::root, rect.x, rect.y, 0.8f, 1.0f);

  // result
  if (result) {
    result->renderGenericDescription(rect.x + 3 * rect.w / 4 + 1, rect.y + rect.h / 2 + 5, true, false);
  }

  // item under cursor
  if (selectedItem >= 0 || selectedIngredient >= 0 || selectedTool || isDragging) {
    int itemx = isDragging ? dragx : selectedItem >= 0 ? rect.x + CRAFT_WIDTH / 4 : rect.x + 3 * CRAFT_WIDTH / 4;
    int itemy = isDragging                ? dragy + 2
                : selectedItem >= 0       ? rect.y + selectedItem + 4 - scroller->getOffset()
                : selectedIngredient >= 0 ? rect.y + 7 + selectedIngredient
                                          : rect.y + 5;
    item::Item* item = isDragging                ? dragItem
                       : selectedItem >= 0       ? items.at(selectedItem)
                       : selectedIngredient >= 0 ? ingredients.at(selectedIngredient)
                                                 : tool;
    item->renderDescription(itemx, itemy);
  }
}

void Craft::detectItem(TCOD_mouse_t& mouse) {
  // detect item under cursor
  selectedItem = -1;
  selectedIngredient = -1;
  selectedTool = false;
  if (mouse.cy >= rect.y + 2 && mouse.cy < rect.y + rect.h - 1) {
    if (mouse.cx >= rect.x && mouse.cx < rect.x + rect.w / 2) {
      if (mouse.cy - rect.y - 2 < items.size() - scroller->getOffset()) selectedItem = mouse.cy - rect.y - 2;
    } else if (mouse.cx >= rect.x + rect.w / 2 && mouse.cx < rect.x + rect.w - 1) {
      if (mouse.cy >= rect.y + 1 && mouse.cy < rect.y + 4) {
        selectedTool = (tool != NULL);
      } else if (mouse.cy >= rect.y + 5 && mouse.cy < rect.y + 5 + ingredients.size()) {
        selectedIngredient = mouse.cy - rect.y - 5;
      }
    }
  }
}

bool Craft::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) {
  scroller->update(elapsed, k, mouse, rect.x + 1, rect.y + 1);
  if (result) create.update(elapsed, k, mouse, rect.x, rect.y);
  if (tool || !ingredients.empty()) clear.update(elapsed, k, mouse, rect.x, rect.y);

  detectItem(mouse);

  // item dragging
  if (!isDraggingStart && mouse.lbutton && (selectedItem >= 0 || selectedIngredient >= 0 || selectedTool)) {
    // start dragging. to be confirmed
    isDraggingStart = true;
    dragItem = selectedItem >= 0         ? items.at(selectedItem)
               : selectedIngredient >= 0 ? ingredients.at(selectedIngredient)
               : selectedTool            ? tool
                                         : NULL;
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
    }
  } else if ((isDragging || isDraggingStart) && ((!mouse.lbutton) || (mouse.lbutton_pressed))) {
    // drop
    isDragging = isDraggingStart = false;
    if (mouse.cx >= rect.x + rect.w / 2 && mouse.cx < rect.x + rect.w && mouse.cy >= rect.y + 1 &&
        mouse.cy < rect.y + 4) {
      // change tool
      if (dragItem->isTool() && tool != dragItem) {
        if (tool) {
          items.push_back(tool);
          if (tool->isA("liquid container") && !tool->stack_.empty()) {
            helpers::remove(ingredients, tool->stack_.at(0));
          }
        }
        tool = dragItem;
        if (helpers::contains(items, tool)) {
          helpers::remove(items, tool);
        } else if (helpers::contains(ingredients, tool)) {
          helpers::remove(ingredients, tool);
        }
        if (tool->isA("liquid container") && !tool->stack_.empty()) {
          ingredients.push_back(tool->stack_.at(0));
        }
        computeResult();
      }
    } else if (
        mouse.cx >= rect.x + rect.w / 2 && mouse.cx < rect.x + rect.w && mouse.cy >= rect.y + 4 &&
        mouse.cy < rect.y + rect.h / 2 + 4) {
      // add ingredient
      if (dragItem->isIngredient()) {
        if (helpers::contains(items, dragItem)) {
          helpers::remove(items, dragItem);
        } else if (helpers::contains(ingredients, dragItem)) {
          helpers::remove(ingredients, dragItem);
        } else if (tool == dragItem) {
          tool = NULL;
        }
        ingredients.push_back(dragItem);
        computeResult();
      }
    } else if (
        mouse.cx >= rect.x + 1 && mouse.cx < rect.x + rect.w / 2 && mouse.cy >= rect.y + 4 &&
        mouse.cy < rect.y + rect.h - 1) {
      // put back something in inventory
      if (!dragItem->container_ || !dragItem->container_->isA("liquid container")) {
        if (helpers::contains(items, dragItem)) {
          helpers::remove(items, dragItem);
        } else if (helpers::contains(ingredients, dragItem)) {
          helpers::remove(ingredients, dragItem);
        } else if (tool == dragItem) {
          if (tool->isA("liquid container") && !tool->stack_.empty()) {
            helpers::remove(ingredients, tool->stack_.at(0));
          }
          tool = NULL;
        }
        items.push_back(dragItem);
        computeResult();
      }
    }
    dragItem = NULL;
  } else if (isDragging) {
    if (mouse.rbutton) {
      // cancel dragging
      isDragging = isDraggingStart = false;
      dragItem = NULL;
    } else if (mouse.lbutton) {
      dragx = mouse.cx;
      dragy = mouse.cy;
    }
  }

  detectItem(mouse);
  if ((k.vk == TCODK_ESCAPE && !k.pressed)) {
    return false;
  }

  return true;
}

// UIListener
bool Craft::onWidgetEvent(Widget* widget, EWidgetEvent event) {
  if (widget == &create && result) {
    // delete ingredients
    const auto deleteIngredients = [&](item::Item* it) {
      const item::ItemIngredient* ing = recipe->getIngredient(it);
      if (ing->destroy) {
        if (it->count_ > ing->quantity) {
          it->count_ -= ing->quantity;
        } else {
          owner->removeFromInventory(it, 1);
          return true;  // Remove item.
        }
      }
      return false;
    };
    ingredients.erase(std::remove_if(ingredients.begin(), ingredients.end(), deleteIngredients), ingredients.end());
    // put result in player's inventory
    gameEngine->gui.log.info("You created %s", result->aName());
    owner->addToInventory(result);
    if (tool->isA("liquid container")) {
      owner->removeFromInventory(tool, true);
      tool = NULL;
    }
    result = NULL;
    computeResult();
    initialize(owner, true);
  } else if (widget == &clear) {
    if (tool) {
      items.push_back(tool);
      if (tool->isA("liquid container") && !tool->stack_.empty()) {
        helpers::remove(ingredients, tool->stack_.at(0));
      }
    }
    tool = NULL;
    for (item::Item* it : ingredients) items.push_back(it);
    ingredients.clear();
    result = NULL;
    computeRecipes();
  }
  return false;
}

void Craft::computeResult() {
  computeRecipes();
  for (item::ItemCombination** cur = item::Item::combinations.begin(); cur != item::Item::combinations.end(); cur++) {
    // check that the tool matches
    if ((!(*cur)->hasTool() && !tool) || (tool && (*cur)->isTool(tool))) {
      bool ingredientOk = true;
      bool checkIng[item::MAX_INGREDIENTS];
      memset(checkIng, 0, item::MAX_INGREDIENTS * sizeof(bool));
      // check that all proposed ingredients match
      for (auto it = ingredients.begin(); ingredientOk && it != ingredients.end(); ++it) {
        const item::ItemIngredient* ing = (*cur)->getIngredient(*it);
        if (!ing)
          ingredientOk = false;
        else if (!ing->optional && ing->quantity > (*it)->count_)
          ingredientOk = false;
        else
          checkIng[ing - (*cur)->ingredients] = true;
      }
      if (ingredientOk) {
        // check that no ingredient is missing
        for (int i = 0; ingredientOk && i < (*cur)->nbIngredients; i++) {
          if (!checkIng[i]) ingredientOk = false;
        }
        if (ingredientOk) {
          // generate the result
          if (result) {
            if (result->isA("liquid container")) delete result->stack_.at(0);
            delete result;
          }
          result = item::Item::getItem((*cur)->resultType, 0, 0, false);
          result->count_ = (*cur)->nbResult;
          for (item::Item* it : ingredients) {
            const item::ItemIngredient* ing = (*cur)->getIngredient(it);
            if (ing->revert) result->addComponent(it);
          }
          if (tool && tool->isA("liquid container")) {
            item::Item* bottle = item::Item::getItem("bottle", 0, 0, false);
            result->putInContainer(bottle);
            result = bottle;
            result->computeBottleName();
          }
          recipe = (*cur);
          return;
        }
      }
    }
  }
  if (result) {
    if (result->isA("liquid container")) delete result->stack_.at(0);
    delete result;
  }
  result = NULL;
  recipe = NULL;
}

// get the list of recipes that match the current tool/ingredients
void Craft::computeRecipes() {
  recipes.clear();
  for (item::ItemCombination** cur = item::Item::combinations.begin(); cur != item::Item::combinations.end(); cur++) {
    if ((!tool) || (tool && (*cur)->isTool(tool))) {
      bool ingredientOk = true;
      // check that all proposed ingredients match
      for (auto it = ingredients.begin(); ingredientOk && it != ingredients.end(); it++) {
        const item::ItemIngredient* ing = (*cur)->getIngredient(*it);
        if (!ing)
          ingredientOk = false;
        else if (!ing->optional && ing->quantity > (*it)->count_)
          ingredientOk = false;
      }
      if (ingredientOk) recipes.push(*cur);
    }
  }
}

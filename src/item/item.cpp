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
#include "item.hpp"

#include <fmt/core.h>
#include <math.h>
#include <stdio.h>

#include <libtcod.hpp>
#include <string>

#include "helpers.hpp"
#include "item.hpp"
#include "main.hpp"
#include "map/building.hpp"
#include "screen/game.hpp"
#include "ui/inventory.hpp"
#include "util/cellular.hpp"
#include "util/textgen.hpp"

// text generator for item names
static util::TextGenerator* textgen = NULL;

namespace item {
TCODConsole* Item::descCon = NULL;

TCODList<ItemActionId> featureActions[NB_ITEM_FEATURES];

TCODList<ItemType*> Item::types;

static ItemAction itemActions[NB_ITEM_ACTIONS] = {
    {"Take", ITEM_ACTION_LOOT},
    {"Take all", ITEM_ACTION_LOOT},
    {"Use", ITEM_ACTION_LOOT | ITEM_ACTION_INVENTORY},
    {"Drop", ITEM_ACTION_INVENTORY},
    {"Drop all", ITEM_ACTION_INVENTORY},
    {"Throw", ITEM_ACTION_LOOT | ITEM_ACTION_INVENTORY},
    {"Disassemble", ITEM_ACTION_LOOT | ITEM_ACTION_INVENTORY},
    {"Fill", ITEM_ACTION_LOOT | ITEM_ACTION_INVENTORY | ITEM_ACTION_ONWATER},
};

ItemAction* ItemAction::getFromId(ItemActionId id) { return &itemActions[id]; }

TCODList<ItemCombination*> Item::combinations;

TCODColor Item::classColor[NB_ITEM_CLASSES] = {
    TCODColor::white,
    TCODColor::chartreuse,
    TCODColor::orange,
    TCODColor::red,
    TCODColor::lightGrey,
    TCODColor::lightYellow,
};

ItemColor& ItemColor::operator=(const TCODColor& col) {
  r = col.r;
  g = col.g;
  b = col.b;
  return *this;
}

ItemFeature* ItemFeature::getProduce(float delay, float chance, ItemType* type) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_PRODUCES;
  ret->produce.delay = delay;
  ret->produce.chance = chance;
  ret->produce.type = type;
  return ret;
}

ItemFeature* ItemFeature::getFireEffect(float resistance, ItemType* type) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_FIRE_EFFECT;
  ret->fireEffect.resistance = resistance;
  ret->fireEffect.type = type;
  return ret;
}

ItemFeature* ItemFeature::getAgeEffect(float delay, ItemType* type) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_AGE_EFFECT;
  ret->ageEffect.delay = delay;
  ret->ageEffect.type = type;
  return ret;
}

ItemFeature* ItemFeature::getFood(int health) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_FOOD;
  ret->food.health = health;
  return ret;
}

ItemFeature* ItemFeature::getLight(float range, const TCODColor& color, float patternDelay, const char* pattern) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_LIGHT;
  ret->light.range = range;
  ret->light.color = color;
  ret->light.patternDelay = patternDelay;
  ret->light.pattern = strdup(pattern);
  return ret;
}

ItemFeature* ItemFeature::getAttack(
    AttackWieldType wield,
    float minCastDelay,
    float maxCastDelay,
    float minReloadDelay,
    float maxReloadDelay,
    float minDamagesCoef,
    float maxDamagesCoef,
    int flags,
    const char* spellCasted) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_ATTACK;
  ret->attack.wield = wield;
  ret->attack.minCastDelay = minCastDelay;
  ret->attack.maxCastDelay = maxCastDelay;
  ret->attack.minReloadDelay = minReloadDelay;
  ret->attack.maxReloadDelay = maxReloadDelay;
  ret->attack.minDamagesCoef = minDamagesCoef;
  ret->attack.maxDamagesCoef = maxDamagesCoef;
  ret->attack.flags = flags;
  ret->attack.spellCasted = spellCasted ? strdup(spellCasted) : NULL;
  return ret;
}

ItemFeature* ItemFeature::getHeat(float intensity, float radius) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_HEAT;
  ret->heat.intensity = intensity;
  ret->heat.radius = radius;
  return ret;
}

ItemFeature* ItemFeature::getContainer(int size) {
  ItemFeature* ret = new ItemFeature();
  ret->id = ITEM_FEAT_CONTAINER;
  ret->container.size = size;
  return ret;
}

Item* ItemType::produce(float rng) const {
  float odds = 0.0f;
  TCODList<ItemFeature*> produceFeatures;
  // select one of the produce features
  for (ItemFeature** it = features.begin(); it != features.end(); it++) {
    if ((*it)->id == ITEM_FEAT_PRODUCES) {
      produceFeatures.push(*it);
      odds += (*it)->produce.chance;
    }
  }
  rng *= odds;
  for (ItemFeature** it = produceFeatures.begin(); it != produceFeatures.end(); it++) {
    rng -= (*it)->produce.chance;
    if (rng < 0) {
      // this one wins!
      return Item::getItem((*it)->produce.type, 0, 0);
    }
  }
  return NULL;
}

bool ItemType::isA(const char* typeName) const { return isA(Item::getType(typeName)); }
bool ItemType::hasAction(ItemActionId id) const { return actions.contains(id); }

bool ItemType::isA(const ItemType* type) const {
  if (type == this) return true;
  if (type == NULL) return false;
  for (ItemType** father = inherits.begin(); father != inherits.end(); father++) {
    if ((*father)->isA(type)) return true;
  }
  return false;
}

ItemFeature* ItemType::getFeature(ItemFeatureId id) const {
  for (ItemFeature** it = features.begin(); it != features.end(); it++) {
    if ((*it)->id == id) return *it;
  }
  return NULL;
}

void Item::addFeature(const char* typeName, ItemFeature* feat) { Item::getType(typeName)->features.push(feat); }

// to handle item type forward references in items.cfg
TCODList<const char*> toDefine;

class ItemFileListener : public ITCODParserListener {
 public:
  bool parserNewStruct(TCODParser* parser, const TCODParserStruct* str, const char* name) {
    if (strcmp(str->getName(), "itemType") == 0) {
      type = NULL;
      for (const char** it = toDefine.begin(); it != toDefine.end(); it++) {
        if (strcmp(*it, name) == 0) {
          toDefine.removeFast(it);
          type = Item::getType(name);
          break;
        }
      }
      if (!type) {
        type = new ItemType();
        Item::types.push(type);
      }
      type->inventoryTab = INV_MISC;
      type->flags = 0;
      type->character = 0;
      type->onPick = {};
      // inheritance
      if (types.size() > 0) {
        type->inherits.push(types.peek());
        // copy features
        for (ItemType** father = types.begin(); father != types.end(); father++) {
          type->inventoryTab = (*father)->inventoryTab;
          type->flags |= ((*father)->flags & ~ITEM_ABSTRACT);
          type->character = (*father)->character;
          type->color = (*father)->color;
          for (ItemFeature** it = (*father)->features.begin(); it != (*father)->features.end(); it++) {
            ItemFeature* feat = type->getFeature((*it)->id);
            if (!feat) {
              feat = new ItemFeature();
              type->features.push(feat);
            }
            *feat = **it;
          }
        }
      }
      type->name = strdup(name);

      types.push(type);
    } else if (strcmp(str->getName(), "fireEffect") == 0) {
      feat.id = ITEM_FEAT_FIRE_EFFECT;
      feat.fireEffect.resistance = 0.0f;
      feat.fireEffect.type = NULL;
    } else if (strcmp(str->getName(), "heat") == 0) {
      feat.id = ITEM_FEAT_HEAT;
      feat.heat.intensity = 0.0f;
      feat.heat.radius = 0.0f;
    } else if (strcmp(str->getName(), "produces") == 0) {
      feat.id = ITEM_FEAT_PRODUCES;
      feat.produce.delay = 0.0f;
      feat.produce.chance = 0.0f;
      feat.produce.type = NULL;
    } else if (strcmp(str->getName(), "food") == 0) {
      feat.id = ITEM_FEAT_FOOD;
      feat.food.health = 0;
    } else if (strcmp(str->getName(), "ageEffect") == 0) {
      feat.id = ITEM_FEAT_AGE_EFFECT;
      feat.ageEffect.delay = 0.0f;
      feat.ageEffect.type = NULL;
    } else if (strcmp(str->getName(), "container") == 0) {
      feat.id = ITEM_FEAT_CONTAINER;
      feat.container.size = 0;
    } else if (strcmp(str->getName(), "attack") == 0) {
      feat.id = ITEM_FEAT_ATTACK;
      feat.attack.flags = 0;
      feat.attack.wield = WIELD_NONE;
      feat.attack.minCastDelay = 0.0f;
      feat.attack.maxCastDelay = 0.0f;
      feat.attack.minReloadDelay = 0.0f;
      feat.attack.maxReloadDelay = 0.0f;
      feat.attack.minDamagesCoef = 0.0f;
      feat.attack.maxDamagesCoef = 0.0f;
      feat.attack.spellCasted = NULL;
    }
    return true;
  }
  bool parserFlag(TCODParser* parser, const char* name) {
    if (strcmp(name, "projectile") == 0) {
      feat.attack.flags |= WEAPON_PROJECTILE;
    }
    return true;
  }
  bool parserProperty(TCODParser* parser, const char* name, TCOD_value_type_t vtype, TCOD_value_t value) {
    using namespace std::literals::string_literals;
    if ("inventoryTab"s == name) {
      if (strcmp(value.s, "armor") == 0)
        type->inventoryTab = INV_ARMOR;
      else if (strcmp(value.s, "weapon") == 0)
        type->inventoryTab = INV_WEAPON;
      else if (strcmp(value.s, "food") == 0)
        type->inventoryTab = INV_FOOD;
      else if (strcmp(value.s, "misc") == 0)
        type->inventoryTab = INV_MISC;
    } else if ("col"s == name) {
      type->color = value.col;
    } else if (strcmp(name, "onPick") == 0) {
      type->onPick = value.s;
    } else if (strcmp(name, "character") == 0) {
      type->character = value.c;
    } else if (strcmp(name, "flags") == 0) {
      TCODList<const char*> l(value.list);
      for (const char** it = l.begin(); it != l.end(); it++) {
        if (strcmp(*it, "notWalkable") == 0)
          type->flags |= ITEM_NOT_WALKABLE;
        else if (strcmp(*it, "notTransparent") == 0)
          type->flags |= ITEM_NOT_TRANSPARENT;
        else if (strcmp(*it, "notPickable") == 0)
          type->flags |= ITEM_NOT_PICKABLE;
        else if (strcmp(*it, "stackable") == 0)
          type->flags |= ITEM_STACKABLE;
        else if (strcmp(*it, "softStackable") == 0)
          type->flags |= ITEM_SOFT_STACKABLE;
        else if (strcmp(*it, "deleteOnUse") == 0)
          type->flags |= ITEM_DELETE_ON_USE;
        else if (strcmp(*it, "useWhenPicked") == 0)
          type->flags |= ITEM_USE_WHEN_PICKED;
        else if (strcmp(*it, "container") == 0)
          type->flags |= ITEM_CONTAINER;
        else if (strcmp(*it, "activateOnBump") == 0)
          type->flags |= ITEM_ACTIVATE_ON_BUMP;
        else if (strcmp(*it, "an") == 0)
          type->flags |= ITEM_AN;
        else if (strcmp(*it, "abstract") == 0)
          type->flags |= ITEM_ABSTRACT;
        else if (strcmp(*it, "notBlock") == 0)
          type->flags |= ITEM_BUILD_NOT_BLOCK;
        else
          parser->error("unknown flag '%s'", *it);
      }
    } else if (strcmp(name, "inherits") == 0) {
      TCODList<const char*> l(value.list);
      for (const char** it = l.begin(); it != l.end(); it++) {
        ItemType* result = Item::getType(*it);
        // forward reference to a type not already existing
        if (!result) {
          result = new ItemType();
          Item::types.push(result);
          toDefine.push(strdup(value.s));
          result->name = strdup(value.s);
        }
        if (!type->inherits.contains(result)) type->inherits.push(result);
      }
    } else if (strcmp(name, "resistance") == 0) {
      feat.fireEffect.resistance = value.f;
    } else if (strcmp(name, "itemType") == 0) {
      ItemType* result = Item::getType(value.s);
      // forward reference to a type not already existing
      if (!result) {
        result = new ItemType();
        Item::types.push(result);
        toDefine.push(strdup(value.s));
        result->name = strdup(value.s);
      }
      if (feat.id == ITEM_FEAT_FIRE_EFFECT) {
        feat.fireEffect.type = result;
      } else if (feat.id == ITEM_FEAT_PRODUCES) {
        feat.produce.type = result;
      } else if (feat.id == ITEM_FEAT_AGE_EFFECT) {
        feat.ageEffect.type = result;
      }
    } else if (strcmp(name, "intensity") == 0) {
      feat.heat.intensity = value.f;
    } else if (strcmp(name, "radius") == 0) {
      feat.heat.radius = value.f;
    } else if (strcmp(name, "delay") == 0) {
      if (feat.id == ITEM_FEAT_PRODUCES) {
        feat.produce.delay = getDelay(value.s);
      } else if (feat.id == ITEM_FEAT_AGE_EFFECT) {
        feat.ageEffect.delay = getDelay(value.s);
      }
    } else if (strcmp(name, "chance") == 0) {
      feat.produce.chance = value.f;
    } else if (strcmp(name, "health") == 0) {
      feat.food.health = value.i;
    } else if (strcmp(name, "size") == 0) {
      feat.container.size = value.i;
    } else if (strcmp(name, "wield") == 0) {
      feat.attack.wield = getWieldType(value.s);
    } else if (strcmp(name, "spellCasted") == 0) {
      feat.attack.spellCasted = strdup(value.s);
    } else if (strcmp(name, "castDelay") == 0) {
      getAttackCoef(value.s, &feat.attack.minCastDelay, &feat.attack.maxCastDelay);
    } else if (strcmp(name, "reloadDelay") == 0) {
      getAttackCoef(value.s, &feat.attack.minReloadDelay, &feat.attack.maxReloadDelay);
    } else if (strcmp(name, "damageCoef") == 0) {
      getAttackCoef(value.s, &feat.attack.minDamagesCoef, &feat.attack.maxDamagesCoef);
    }
    return true;
  }
  bool parserEndStruct(TCODParser* parser, const TCODParserStruct* str, const char* name) {
    if (strcmp(str->getName(), "itemType") == 0) {
      types.pop();
    } else if (strcmp(str->getName(), "fireEffect") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_FIRE_EFFECT);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getFireEffect(feat.fireEffect.resistance, feat.fireEffect.type));
      }
    } else if (strcmp(str->getName(), "heat") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_HEAT);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getHeat(feat.heat.intensity, feat.heat.radius));
      }
    } else if (strcmp(str->getName(), "produces") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_PRODUCES);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getProduce(feat.produce.delay, feat.produce.chance, feat.produce.type));
      }
    } else if (strcmp(str->getName(), "food") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_FOOD);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getFood(feat.food.health));
      }
    } else if (strcmp(str->getName(), "ageEffect") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_AGE_EFFECT);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getAgeEffect(feat.ageEffect.delay, feat.ageEffect.type));
      }
    } else if (strcmp(str->getName(), "container") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_CONTAINER);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getContainer(feat.container.size));
      }
    } else if (strcmp(str->getName(), "attack") == 0) {
      ItemFeature* itemFeat = type->getFeature(ITEM_FEAT_ATTACK);
      if (itemFeat) {
        *itemFeat = feat;
      } else {
        type->features.push(ItemFeature::getAttack(
            feat.attack.wield,
            feat.attack.minCastDelay,
            feat.attack.maxCastDelay,
            feat.attack.minReloadDelay,
            feat.attack.maxReloadDelay,
            feat.attack.minDamagesCoef,
            feat.attack.maxDamagesCoef,
            feat.attack.flags,
            feat.attack.spellCasted));
      }
    }
    return true;
  }
  void error(const char* msg) {
    fprintf(stderr, "Fatal error while loading items.cfg : %s", msg);
    std::abort();
  }

 protected:
  TCODList<ItemType*> types;
  ItemType* type;
  ItemFeature feat;
  float getDelay(const char* s) {
    float ret;
    char unit[32] = "";
    if (sscanf(s, "%f%s", &ret, unit) != 2) {
      error(fmt::format("bad format for delay '{}'. [0-9]*(s|m|h|d) expected", s).c_str());
    }
    if (strcmp(unit, "m") == 0)
      ret *= 60;
    else if (strcmp(unit, "h") == 0)
      ret *= 3600;
    else if (strcmp(unit, "d") == 0)
      ret *= 3600 * 24;
    else if (strcmp(unit, "s") != 0) {
      error(fmt::format("bad format for delay '{}'. [0-9]*(s|m|h|d) expected", s).c_str());
    }
    return ret;
  }

  AttackWieldType getWieldType(const char* s) {
    if (strcmp(s, "mainHand") == 0)
      return WIELD_MAIN_HAND;
    else if (strcmp(s, "oneHand") == 0)
      return WIELD_ONE_HAND;
    else if (strcmp(s, "offHand") == 0)
      return WIELD_OFF_HAND;
    else if (strcmp(s, "twoHands") == 0)
      return WIELD_TWO_HANDS;
    return WIELD_NONE;
  }

  void getAttackCoef(const char* s, float* fmin, float* fmax) {
    if (strchr(s, '-')) {
      if (sscanf(s, "%f-%f", fmin, fmax) != 2) {
        error(fmt::format("bad format '{}'.", s).c_str());
      }
    } else {
      *fmin = *fmax = atof(s);
    }
  }
};

class RecipeFileListener : public ITCODParserListener {
 public:
  bool parserNewStruct(TCODParser* parser, const TCODParserStruct* str, const char* name) {
    if (strcmp(str->getName(), "recipe") == 0) {
      recipe = new ItemCombination();
      recipe->resultType = NULL;
      recipe->nbResult = 1;
      recipe->tool = NULL;
      recipe->nbIngredients = 0;
      Item::combinations.push(recipe);
    } else if (strcmp(str->getName(), "tool") == 0) {
      mode = RECIPE_TOOL;
    } else if (strcmp(str->getName(), "result") == 0) {
      mode = RECIPE_RESULT;
    } else if (strcmp(str->getName(), "ingredient") == 0 || strcmp(str->getName(), "optionalIngredient") == 0) {
      if (recipe->nbIngredients == MAX_INGREDIENTS - 1) {
        error(fmt::format("too many ingredients/components (max {})", MAX_INGREDIENTS).c_str());
      }
      mode = RECIPE_INGREDIENT;
      ingredient = &recipe->ingredients[recipe->nbIngredients];
      ingredient->optional = (strcmp(str->getName(), "optionalIngredient") == 0);
      ingredient->destroy = true;
      ingredient->quantity = 1;
      ingredient->revert = false;
      ingredient->type = NULL;
      recipe->nbIngredients++;
    } else if (strcmp(str->getName(), "component") == 0 || strcmp(str->getName(), "optionalComponent") == 0) {
      if (recipe->nbIngredients == MAX_INGREDIENTS - 1) {
        error(fmt::format("too many ingredients/components (max {})", MAX_INGREDIENTS).c_str());
      }
      mode = RECIPE_INGREDIENT;
      ingredient = &recipe->ingredients[recipe->nbIngredients];
      ingredient->optional = (strcmp(str->getName(), "optionalComponent") == 0);
      ingredient->destroy = true;
      ingredient->quantity = 1;
      ingredient->revert = true;
      ingredient->type = NULL;
      recipe->nbIngredients++;
    }
    return true;
  }
  bool parserFlag(TCODParser* parser, const char* name) { return true; }
  bool parserProperty(TCODParser* parser, const char* name, TCOD_value_type_t vtype, TCOD_value_t value) {
    if (strcmp(name, "itemType") == 0) {
      ItemType* result = Item::getType(value.s);
      if (!result) {
        error(fmt::format("unknown item type '{}'", value.s).c_str());
      }
      switch (mode) {
        case RECIPE_TOOL:
          recipe->tool = result;
          break;
        case RECIPE_INGREDIENT:
          ingredient->type = result;
          break;
        case RECIPE_RESULT:
          recipe->resultType = result;
          break;
      }
    } else if (strcmp(name, "count") == 0) {
      recipe->nbResult = value.i;
    }
    return true;
  }
  bool parserEndStruct(TCODParser* parser, const TCODParserStruct* str, const char* name) { return true; }
  void error(const char* msg) {
    fprintf(stderr, "Fatal error while loading recipes.cfg : %s", msg);
    std::abort();
  }

 protected:
  enum { RECIPE_TOOL, RECIPE_INGREDIENT, RECIPE_RESULT } mode;
  ItemCombination* recipe;
  ItemIngredient* ingredient;
};

bool Item::initDatabase() {
  const char* inventoryTabs[] = {"armor", "weapon", "food", "misc", NULL};
  const char* wieldTypes[] = {"oneHand", "twoHands", "mainHand", "offHand", NULL};
  // parse item types
  TCODParser itemParser;
  TCODParserStruct* itemType = itemParser.newStructure("itemType");
  itemType->addValueList("inventoryTab", inventoryTabs, false);
  itemType->addProperty("col", TCOD_TYPE_COLOR, false);
  itemType->addProperty("character", TCOD_TYPE_CHAR, false);
  itemType->addProperty("onPick", TCOD_TYPE_STRING, false);
  itemType->addListProperty("flags", TCOD_TYPE_STRING, false);
  itemType->addListProperty("inherits", TCOD_TYPE_STRING, false);
  itemType->addStructure(itemType);

  TCODParserStruct* fireEffect = itemParser.newStructure("fireEffect");
  fireEffect->addProperty("resistance", TCOD_TYPE_FLOAT, true);
  fireEffect->addProperty("itemType", TCOD_TYPE_STRING, false);
  itemType->addStructure(fireEffect);

  TCODParserStruct* heat = itemParser.newStructure("heat");
  heat->addProperty("intensity", TCOD_TYPE_FLOAT, true);
  heat->addProperty("radius", TCOD_TYPE_FLOAT, true);
  itemType->addStructure(heat);

  TCODParserStruct* produces = itemParser.newStructure("produces");
  produces->addProperty("delay", TCOD_TYPE_STRING, true);
  produces->addProperty("chance", TCOD_TYPE_FLOAT, true);
  produces->addProperty("itemType", TCOD_TYPE_STRING, true);
  itemType->addStructure(produces);

  TCODParserStruct* food = itemParser.newStructure("food");
  food->addProperty("health", TCOD_TYPE_INT, true);
  itemType->addStructure(food);

  TCODParserStruct* ageEffect = itemParser.newStructure("ageEffect");
  ageEffect->addProperty("delay", TCOD_TYPE_STRING, true);
  ageEffect->addProperty("itemType", TCOD_TYPE_STRING, false);
  itemType->addStructure(ageEffect);

  TCODParserStruct* container = itemParser.newStructure("container");
  container->addProperty("size", TCOD_TYPE_INT, false);
  itemType->addStructure(container);

  TCODParserStruct* attack = itemParser.newStructure("attack");
  attack->addFlag("projectile");
  attack->addValueList("wield", wieldTypes, true);
  attack->addProperty("castDelay", TCOD_TYPE_STRING, true);
  attack->addProperty("reloadDelay", TCOD_TYPE_STRING, true);
  attack->addProperty("damageCoef", TCOD_TYPE_STRING, true);
  attack->addProperty("spellCasted", TCOD_TYPE_STRING, false);
  itemType->addStructure(attack);

  itemParser.run("data/cfg/items.cfg", new ItemFileListener());

  // check for unresolved forward references
  if (toDefine.size() > 0) {
    for (const char** it = toDefine.begin(); it != toDefine.end(); it++) {
      fprintf(stderr, "FATAL : no definition for item type '%s'\n", *it);
    }
    return false;
  }

  // addFeature("health potion",ItemFeature::getFood(15));
  // addFeature("health potion",ItemFeature::getLight(4.0f,TCODColor(38,38,76),1.0f,"56789876"));
  // addFeature("wand",ItemFeature::getAttack(WIELD_ONE_HAND, 0.2f,0.3f, 0.6f,0.8f, 0.8f,1.2f, 0));
  // addFeature("wand",ItemFeature::getLight(4.0f,TCODColor::white,1.0f,"56789876"));
  // addFeature("staff",ItemFeature::getAttack(WIELD_TWO_HANDS, 0.25f,1.0f, 0.5f,2.0f, 0.8f,1.2f, 0));
  // addFeature("staff",ItemFeature::getLight(4.0f,TCODColor::white,1.0f,"56789876"));

  // define available action on each item type
  for (ItemType** it = types.begin(); it != types.end(); it++) {
    (*it)->computeActions();
  }

  // parse recipes
  TCODParser recipeParser;
  TCODParserStruct* recipe = recipeParser.newStructure("recipe");
  TCODParserStruct* tool = recipeParser.newStructure("tool");
  recipe->addStructure(tool);
  tool->addProperty("itemType", TCOD_TYPE_STRING, true);

  TCODParserStruct* ingredient = recipeParser.newStructure("ingredient");
  recipe->addStructure(ingredient);
  ingredient->addProperty("itemType", TCOD_TYPE_STRING, true);
  ingredient->addProperty("count", TCOD_TYPE_INT, false);

  TCODParserStruct* optionalIngredient = recipeParser.newStructure("optionalIngredient");
  recipe->addStructure(optionalIngredient);
  optionalIngredient->addProperty("itemType", TCOD_TYPE_STRING, true);
  optionalIngredient->addProperty("count", TCOD_TYPE_INT, false);

  TCODParserStruct* component = recipeParser.newStructure("component");
  recipe->addStructure(component);
  component->addProperty("itemType", TCOD_TYPE_STRING, true);

  TCODParserStruct* optionalComponent = recipeParser.newStructure("optionalComponent");
  recipe->addStructure(optionalComponent);
  optionalComponent->addProperty("itemType", TCOD_TYPE_STRING, true);

  TCODParserStruct* result = recipeParser.newStructure("result");
  recipe->addStructure(result);
  result->addProperty("itemType", TCOD_TYPE_STRING, true);
  result->addProperty("count", TCOD_TYPE_INT, false);

  recipeParser.run("data/cfg/recipes.cfg", new RecipeFileListener());

  return true;
}

void ItemType::computeActions() {
  if ((flags & ITEM_NOT_PICKABLE) == 0) actions.push(ITEM_ACTION_TAKE);
  if (getFeature(ITEM_FEAT_FOOD) || getFeature(ITEM_FEAT_ATTACK)) actions.push(ITEM_ACTION_USE);
  if ((flags & ITEM_NOT_PICKABLE) == 0) actions.push(ITEM_ACTION_DROP);
  if ((flags & ITEM_NOT_PICKABLE) == 0) actions.push(ITEM_ACTION_THROW);
  if (hasComponents()) actions.push(ITEM_ACTION_DISASSEMBLE);
  if (isA("liquid container")) actions.push(ITEM_ACTION_FILL);
}

ItemType* Item::getType(const char* name) {
  if (name == NULL) return NULL;
  if (types.size() == 0) {
    if (!initDatabase()) std::abort();  // fatal error. cannot load items configuration
  }
  for (ItemType** it = types.begin(); it != types.end(); it++) {
    if ((*it)->name == name) return *it;
  }
  return NULL;
}

bool ItemType::hasComponents() const {
  for (ItemCombination** cur = Item::combinations.begin(); cur != Item::combinations.end(); cur++) {
    if ((*cur)->resultType == this) {
      // check if ingredients can be reverted
      for (int i = 0; i < (*cur)->nbIngredients; i++) {
        if ((*cur)->ingredients[i].revert) return true;
      }
    }
  }
  return false;
}

bool ItemCombination::isTool(const Item* item) const { return isTool(item->typeData); }

bool ItemCombination::isIngredient(const Item* item) const { return isIngredient(item->typeData); }

bool ItemCombination::isTool(const ItemType* itemType) const { return (itemType->isA(tool)); }

bool ItemCombination::isIngredient(const ItemType* itemType) const {
  for (int i = 0; i < nbIngredients; i++) {
    if (itemType->isA(ingredients[i].type)) return true;
  }
  return false;
}

const ItemIngredient* ItemCombination::getIngredient(Item* item) const {
  for (int i = 0; i < nbIngredients; i++) {
    if (item->isA(ingredients[i].type)) return &ingredients[i];
  }
  return NULL;
}

bool ItemType::isIngredient() const {
  for (ItemCombination** cur = Item::combinations.begin(); cur != Item::combinations.end(); cur++) {
    if ((*cur)->isIngredient(this)) return true;
  }
  return false;
}

bool ItemType::isTool() const {
  for (ItemCombination** cur = Item::combinations.begin(); cur != Item::combinations.end(); cur++) {
    if ((*cur)->isTool(this)) return true;
  }
  return false;
}

ItemCombination* ItemType::getCombination() const {
  for (ItemCombination** cur = Item::combinations.begin(); cur != Item::combinations.end(); cur++) {
    if ((*cur)->resultType == this) return *cur;
  }
  return NULL;
}

Item::Item(float x, float y, const ItemType& type) {
  if (descCon == NULL) {
    descCon = new TCODConsole(CON_W / 2, CON_H / 2);
    descCon->setAlignment(TCOD_CENTER);
    descCon->setDefaultBackground(ui::guiBackground);
  }
  setPos(x, y);
  typeName_ = type.name;
  name_ = {};
  typeData = &type;
  ItemFeature* feat = getFeature(ITEM_FEAT_LIGHT);
  if (feat) {
    initLight();
    light_->range = feat->light.range;
    light_->color = TCODColor(feat->light.color.r, feat->light.color.g, feat->light.color.b);
    light_->setup(light_->color, feat->light.patternDelay, feat->light.pattern, NULL);
  }
  color_ = type.color;
  ch_ = type.character;
  item_class_ = ITEM_CLASS_STANDARD;
  feat = typeData->getFeature(ITEM_FEAT_AGE_EFFECT);
  if (feat)
    life_ = feat->ageEffect.delay;
  else
    life_ = 0.0f;
  feat = typeData->getFeature(ITEM_FEAT_FIRE_EFFECT);
  if (feat)
    fire_resistance_ = feat->fireEffect.resistance;
  else
    fire_resistance_ = 0.0f;
  feat = typeData->getFeature(ITEM_FEAT_ATTACK);
  cast_delay_ = reload_delay_ = damages_ = 0.0f;
  if (feat) {
    cast_delay_ = rng->getFloat(feat->attack.minCastDelay, feat->attack.maxCastDelay);
    reload_delay_ = rng->getFloat(feat->attack.minReloadDelay, feat->attack.maxReloadDelay);
    damages_ =
        15 * (reload_delay_ + cast_delay_) * rng->getFloat(feat->attack.minDamagesCoef, feat->attack.maxDamagesCoef);
  }

  phase_ = IDLE;
  phase_timer_ = 0.0f;
  target_x_ = target_y_ = -1;
  heat_timer_ = 0.0f;
  toggle_ = false;
  an_ = false;
}

Item* Item::getItem(const char* typeName, float x, float y, bool createComponents) {
  ItemType* type = Item::getType(typeName);
  if (!type) {
    printf("FATAL : unknown item type '%s'\n", typeName);
    return NULL;
  }
  return Item::getItem(type, x, y, createComponents);
}

Item* Item::getItem(const ItemType* type, float x, float y, bool createComponents) {
  Item* ret = new Item(x, y, *type);
  if (createComponents) ret->generateComponents();
  return ret;
}

#define MAX_RELOAD_BONUS 0.2f
#define MAX_CAST_BONUS 0.2f
Item* Item::getRandomWeapon(const char* typeName, ItemClass itemClass) {
  if (!textgen) {
    textgen = new util::TextGenerator("data/cfg/weapon.txg", rng);
    textgen->parseFile();
  }
  ItemType* type = getType(typeName);
  if (!type) {
    printf("FATAL : unknown weapon type '%s'\n", typeName);
    return NULL;
  }
  Item* weapon = Item::getItem(type, -1, -1, false);
  weapon->item_class_ = itemClass;
  weapon->color_ = Item::classColor[itemClass];
  if (itemClass > ITEM_CLASS_STANDARD) {
    /*
    TODO
    int goatKey = 6 * gameEngine->player.school.type + id;
    weapon->name = strdup( textgen->generate("weapon","${%s}",
            textgen->generators.peek()->rules.get(goatKey)->name ));
    */
  }
  enum { MOD_RELOAD, MOD_CAST, MOD_MODIFIER };
  for (int i = 0; i < itemClass; i++) {
    int modType = rng->getInt(MOD_RELOAD, MOD_MODIFIER);
    switch (modType) {
      case MOD_RELOAD:
        weapon->reload_delay_ -= rng->getFloat(0.05f, MAX_RELOAD_BONUS);
        weapon->reload_delay_ = MAX(0.1f, weapon->reload_delay_);
        break;
      case MOD_CAST:
        weapon->cast_delay_ -= rng->getFloat(0.05f, MAX_CAST_BONUS);
        weapon->cast_delay_ = MAX(0.1f, weapon->reload_delay_);
        break;
      case MOD_MODIFIER:
        modifier::ItemModifierId id = (modifier::ItemModifierId)0;
        switch (gameEngine->player.school.type) {
          case screen::SCHOOL_FIRE:
            id = (modifier::ItemModifierId)rng->getInt(modifier::ITEM_MOD_FIRE_BEGIN, modifier::ITEM_MOD_FIRE_END);
            break;
          case screen::SCHOOL_WATER:
            id = (modifier::ItemModifierId)rng->getInt(modifier::ITEM_MOD_WATER_BEGIN, modifier::ITEM_MOD_WATER_END);
            break;
          default:
            break;
        }
        weapon->addModifier(id, rng->getFloat(0.1f, 0.2f));
        break;
    }
  }
  weapon->damages_ += weapon->damages_ * (int)(itemClass)*0.2f;  // 20% increase per color level
  weapon->damages_ = MIN(1.0f, weapon->damages_);
  // build components
  weapon->generateComponents();
  return weapon;
}

void Item::addComponent(Item* component) {
  components_.push_back(component);
  if (component->adjective_ && !adjective_) {
    adjective_ = component->adjective_;
  }
}

void Item::generateComponents() {
  // check if this item type has components
  ItemCombination* combination = getCombination();
  if (!combination) return;
  int maxOptionals = item_class_ - ITEM_CLASS_STANDARD;
  int i = rng->getInt(0, combination->nbIngredients - 1);
  for (int count = combination->nbIngredients; count > 0; count--) {
    if (combination->ingredients[i].revert && (!combination->ingredients[i].optional || maxOptionals > 0)) {
      if (combination->ingredients[i].optional) maxOptionals--;
      ItemType* componentType = combination->ingredients[i].type;
      if (componentType) {
        Item* component = Item::getItem(componentType, x, y);
        addComponent(component);
      }
    }
    i = (i + 1) % combination->nbIngredients;
  }
}

void Item::initLight() {
  light_ = new map::ExtendedLight();
  light_->x = x * 2;
  light_->y = y * 2;
  light_->randomRad = false;
}

Item::~Item() {
  if (light_) delete light_;
}

// look for a 2 items recipe
ItemCombination* Item::getCombination(const Item* it1, const Item* it2) {
  for (ItemCombination** cur = Item::combinations.begin(); cur != Item::combinations.end(); cur++) {
    if ((*cur)->nbIngredients == 1 && (*cur)->tool != NULL) {
      // tool + 1 ingredient
      if (it1->isA((*cur)->tool) && it2->isA((*cur)->ingredients[0].type)) return *cur;
      if (it2->isA((*cur)->tool) && it1->isA((*cur)->ingredients[0].type)) return *cur;
    } else if ((*cur)->nbIngredients == 2 && (*cur)->tool == NULL) {
      // 2 ingredients (no tool)
      if (it1->isA((*cur)->ingredients[0].type) && it2->isA((*cur)->ingredients[1].type)) return *cur;
      if (it2->isA((*cur)->ingredients[0].type) && it1->isA((*cur)->ingredients[1].type)) return *cur;
    }
  }
  return NULL;
}

bool Item::hasComponents() const { return typeData->hasComponents(); }

ItemCombination* Item::getCombination() const { return typeData->getCombination(); }

// put this inside 'it' container (false if no more room)
Item* Item::putInContainer(Item* it) {
  ItemFeature* cont = it->getFeature(ITEM_FEAT_CONTAINER);
  if (!cont) return NULL;  // it is not a container
  if (it->stack_.size() >= cont->container.size) return nullptr;  // no more room
  Item* item = addToList(it->stack_);
  item->container_ = it;
  item->x = it->x;
  item->y = it->y;
  return item;
}

// remove it from this container (false if not inside)
Item* Item::removeFromContainer(int count) {
  if (!container_ || !helpers::contains(container_->stack_, this)) return NULL;
  Item* item = removeFromList(container_->stack_, count);
  item->container_ = NULL;
  return item;
}

// add to the list, posibly stacking
Item* Item::addToList(std::vector<Item*>& list) {
  if (isStackable()) {
    // if already in the list, increase the count
    for (Item* it : list) {
      if (it->typeData == typeData && (!it->name_ || (name_ && it->name_ == *name_))) {
        it->count_ += count_;
        to_delete_ = count_;
        return it;
      }
    }
  } else if (isSoftStackable()) {
    // if already in the list, add to soft stack
    for (Item* it : list) {
      if (it->typeData == typeData && !it->name_) {
        it->stack_.push_back(this);
        return it;
      }
    }
  }
  // add new item in the list
  list.push_back(this);
  return this;
}
// remove one item, possibly unstacking
Item* Item::removeFromList(std::vector<Item*>& list, int removeCount) {
  if (isStackable() && count_ > removeCount) {
    Item* newItem = Item::getItem(typeData, x, y);
    newItem->count_ = removeCount;
    count_ -= removeCount;
    return newItem;
  } else if (isSoftStackable()) {
    if (stack_.size() > 0) {
      Item* newStackOwner = NULL;
      // this item is the stack owner. rebuild the stack
      if (stack_.size() > removeCount) {
        newStackOwner = stack_.back();
        stack_.pop_back();
        while (stack_.size() > removeCount) {
          newStackOwner->stack_.push_back(stack_.back());
          stack_.pop_back();
        }
      }
      // remove before adding to avoid list reallocation
      helpers::remove(list, this);
      if (newStackOwner) newStackOwner->addToList(list);
      return this;
    } else {
      // this item may be in a stack. find the stack owner
      for (Item* stackOwner : list) {
        if (stackOwner != this && stackOwner->typeData == typeData && helpers::contains(stack_, this)) {
          return stackOwner->removeFromList(list, removeCount);
        }
      }
      // single softStackable item. simply remove it from the list
    }
  } else if (container_ && helpers::contains(list, container_)) {
    // item is inside a container
    removeFromContainer(removeCount);
  }
  helpers::remove<item::Item*>(list, this);
  return this;
}

void Item::computeBottleName() {
  using namespace std::literals::string_literals;
  name_ = {};
  if (stack_.empty()) {
    an_ = true;
    name_ = "empty bottle";
    return;
  }
  Item* liquid = stack_.at(0);
  if (liquid->typeData->name == "water"s) {
    an_ = false;
    name_ = "bottle of water";
  } else if (liquid->typeData->name == "poison"s) {
    an_ = false;
    name_ = "bottle of poison";
  } else if (liquid->typeData->name == "sleep"s) {
    an_ = false;
    name_ = "bottle of soporific";
  } else if (liquid->typeData->name == "antidote"s) {
    an_ = true;
    name_ = "antidote";
  } else if (liquid->typeData->name == "health"s) {
    an_ = false;
    name_ = "health potion";
  } else {
    an_ = liquid->an_;
    name_ = liquid->typeData->name;
  }
}

void Item::render(map::LightMap& lightMap, TCODImage* ground) {
  int conx = (int)(x - gameEngine->xOffset);
  int cony = (int)(y - gameEngine->yOffset);
  if (!IN_RECTANGLE(conx, cony, CON_W, CON_H)) return;
  map::Dungeon* dungeon = gameEngine->dungeon;
  TCODColor lightColor = lightMap.getColor(conx, cony);
  float shadow = dungeon->getShadow(x * 2, y * 2);
  float clouds = dungeon->getCloudCoef(x * 2, y * 2);
  shadow = MIN(shadow, clouds);
  lightColor = lightColor * shadow;
  TCODConsole::root->setChar(conx, cony, ch_);
  TCODConsole::root->setCharForeground(conx, cony, color_ * lightColor);
  if (ground) {
    TCODConsole::root->setCharBackground(conx, cony, ground->getPixel(conx * 2, cony * 2));
  } else {
    TCODConsole::root->setCharBackground(conx, cony, dungeon->getShadedGroundColor(getSubX(), getSubY()));
  }
}

void Item::renderDescription(int x, int y, bool below) {
  int cy = 0;
  descCon->clear();
  descCon->setDefaultForeground(Item::classColor[item_class_]);
  if (name_) {
    if (count_ > 1)
      descCon->print(CON_W / 4, cy++, "%s(%d)", name_->c_str(), count_);
    else
      descCon->print(CON_W / 4, cy++, name_->c_str());
    descCon->setDefaultForeground(ui::guiText);
    descCon->print(CON_W / 4, cy++, typeName_.c_str());
  } else {
    if (count_ > 1)
      descCon->print(CON_W / 4, cy++, "%s(%d)", typeName_.c_str(), count_);
    else
      descCon->print(CON_W / 4, cy++, typeName_.c_str());
  }
  descCon->setDefaultForeground(ui::guiText);
  ItemFeature* feat = getFeature(ITEM_FEAT_FOOD);
  if (feat) descCon->print(CON_W / 4, cy++, "Health:+%d", feat->food.health);
  feat = getFeature(ITEM_FEAT_ATTACK);
  if (feat) {
    static const char* wieldname[] = {NULL, "One hand", "Main hand", "Off hand", "Two hands"};
    if (feat->attack.wield) {
      descCon->print(CON_W / 4, cy++, wieldname[feat->attack.wield]);
    }
    float rate = 1.0f / (cast_delay_ + reload_delay_);
    int dmgPerSec = (int)(damages_ * rate + 0.5f);
    descCon->print(CON_W / 4, cy++, "%d damages/sec", dmgPerSec);
    descCon->print(CON_W / 4, cy++, "Attack rate:%s", getRateName(rate));
    modifier::ItemModifier::renderDescription(descCon, 2, cy, modifiers_);
  }

  /*
          y--;
          if ( y < 0 ) y = 2;
          TCODConsole::root->setDefaultForeground(Item::classColor[itemClass]);
          TCODConsole::root->printEx(x,y,TCOD_BKGND_NONE,TCOD_CENTER,typeName);
  */
  renderDescriptionFrame(x, y, below);
}

const char* Item::getRateName(float rate) const {
  static const char* ratename[] = {"Very slow", "Slow", "Average", "Fast", "Very fast"};
  int rateIdx = 0;
  if (rate <= 0.5f)
    rateIdx = 0;
  else if (rate <= 1.0f)
    rateIdx = 1;
  else if (rate <= 3.0f)
    rateIdx = 2;
  else if (rate <= 5.0f)
    rateIdx = 3;
  else
    rateIdx = 4;
  return ratename[rateIdx];
}

void Item::renderGenericDescription(int x, int y, bool below, bool frame) {
  int cy = 0;
  descCon->clear();
  descCon->setDefaultForeground(Item::classColor[item_class_]);
  if (name_) {
    if (count_ > 1)
      descCon->print(CON_W / 4, cy++, "%s(%d)", name_->c_str(), count_);
    else
      descCon->print(CON_W / 4, cy++, name_->c_str());
    descCon->setDefaultForeground(ui::guiText);
    descCon->print(CON_W / 4, cy++, typeName_.c_str());
  } else {
    if (count_ > 1)
      descCon->print(CON_W / 4, cy++, "%s(%d)", typeName_.c_str(), count_);
    else
      descCon->print(CON_W / 4, cy++, typeName_.c_str());
  }
  descCon->setDefaultForeground(ui::guiText);
  ItemFeature* feat = getFeature(ITEM_FEAT_FOOD);
  if (feat) descCon->print(CON_W / 4, cy++, "Health:+%d", feat->food.health);
  feat = getFeature(ITEM_FEAT_ATTACK);
  if (feat) {
    static const char* wieldname[] = {NULL, "One hand", "Main hand", "Off hand", "Two hands"};
    if (feat->attack.wield) {
      descCon->print(CON_W / 4, cy++, wieldname[feat->attack.wield]);
    }
    float minCast = feat->attack.minCastDelay - item_class_ * MAX_CAST_BONUS;
    float minReload = feat->attack.minReloadDelay - item_class_ * MAX_RELOAD_BONUS;
    float minDamages = 15 * (minCast + minReload) * feat->attack.minDamagesCoef;
    float maxDamages = 15 * (feat->attack.maxCastDelay + feat->attack.maxReloadDelay) * feat->attack.maxDamagesCoef;
    minDamages += minDamages * (int)(item_class_)*0.2f;
    maxDamages += maxDamages * (int)(item_class_)*0.2f;
    minDamages = (int)MIN(1.0f, minDamages);
    maxDamages = (int)MIN(1.0f, maxDamages);

    if (minDamages != maxDamages) {
      descCon->print(CON_W / 4, cy++, "%d-%d damages/hit", (int)minDamages, (int)maxDamages);
    } else {
      descCon->print(CON_W / 4, cy++, "%d damages/hit", (int)minDamages);
    }

    float minRate = 1.0f / (feat->attack.maxCastDelay + feat->attack.maxReloadDelay);
    float maxRate = 1.0f / (minCast + minReload);

    const char* rate1 = getRateName(minRate);
    const char* rate2 = getRateName(maxRate);
    if (rate1 == rate2) {
      descCon->print(CON_W / 4, cy++, "Attack rate:%s", rate1);
    } else {
      descCon->print(CON_W / 4, cy++, "Attack rate:%s-%s", rate1, rate2);
    }
    // ItemModifier::renderDescription(descCon,2,cy,modifiers);
  }

  renderDescriptionFrame(x, y, below, frame);
}

void Item::convertTo(ItemType* newType) {
  // create the new item
  Item* newItem = Item::getItem(newType, x, y);
  newItem->speed = speed;
  newItem->dx = dx;
  newItem->dy = dy;
  if (isA("wall")) newItem->ch_ = ch_;
  if (owner_) {
    if (owner_->isPlayer()) {
      if (as_creature_)
        gameEngine->gui.log.info("%s died.", TheName());
      else {
        if (strncmp(newType->name.c_str(), "rotten", 6) == 0) {
          gameEngine->gui.log.info("%s has rotted.", TheName());
        } else {
          gameEngine->gui.log.info("%s turned into %s.", TheName(), newItem->aName());
        }
      }
    }
    owner_->addToInventory(newItem);
  } else
    gameEngine->dungeon->addItem(newItem);
}

bool Item::age(float elapsed, ItemFeature* feat) {
  if (!feat) feat = typeData->getFeature(ITEM_FEAT_AGE_EFFECT);
  if (feat) {
    life_ -= elapsed;
    if (life_ <= 0.0f) {
      if (feat->ageEffect.type) {
        convertTo(feat->ageEffect.type);
      }
      // destroy this item
      if (!owner_) {
        if (as_creature_) gameEngine->dungeon->removeCreature(as_creature_, false);
      } else {
        if (as_creature_) as_creature_->toDelete = true;
      }
      return false;
    }
  }
  return true;
}

bool Item::update(float elapsed, TCOD_key_t key, TCOD_mouse_t* mouse) {
  map::Dungeon* dungeon = gameEngine->dungeon;
  if (!owner_ && !isOnScreen()) {
    // when not on screen, update only once per second
    cumulated_elapsed_ += elapsed;
    if (cumulated_elapsed_ < 1.0f) return true;
    elapsed = cumulated_elapsed_;
    cumulated_elapsed_ = 0.0f;
  }
  if (speed > 0.0f) {
    float oldx = x;
    float oldy = y;
    x += speed * dx * elapsed;
    y += speed * dy * elapsed;
    if (!IN_RECTANGLE(x, y, dungeon->width, dungeon->height)) {
      x = oldx;
      y = oldy;
      return false;
    }
    if (!dungeon->isCellTransparent(x, y)) {
      // bounce against a wall
      float newx = x;
      float newy = y;
      int cdx = (int)(x - oldx);
      int cdy = (int)(y - oldy);
      x = oldx;
      y = oldy;
      speed *= 0.5f;
      if (cdx == 0) {
        // hit horizontal wall
        dy = -dy;
      } else if (cdy == 0) {
        // hit vertical wall
        dx = -dx;
      } else {
        bool xwalk = dungeon->isCellWalkable(newx, oldy);
        bool ywalk = dungeon->isCellWalkable(oldx, newy);
        if (xwalk && ywalk) {
          // outer corner bounce. detect which side of the cell is hit
          //  ##
          //  ##
          // .
          float fdx = ABS(dx);
          float fdy = ABS(dy);
          if (fdx >= fdy) dy = -dy;
          if (fdy >= fdx) dx = -dx;
        } else if (!xwalk) {
          if (ywalk) {
            // vertical wall bounce
            dx = -dx;
          } else {
            // inner corner bounce
            // ##
            // .#
            dx = -dx;
            dy = -dy;
          }
        } else {
          // horizontal wall bounce
          dy = -dy;
        }
      }
    } else {
      if ((int)x == (int)gameEngine->player.x && (int)y == (int)gameEngine->player.y) {
        ItemFeature* feat = getFeature(ITEM_FEAT_ATTACK);
        if (feat) {
          gameEngine->player.takeDamage(
              TCODRandom::getInstance()->getFloat(feat->attack.minDamagesCoef, feat->attack.maxDamagesCoef));
          x = oldx;
          y = oldy;
          speed = 0;
          return false;
        }
      }
    }
    if ((int)x != (int)oldx || (int)y != (int)oldy) {
      helpers::remove<item::Item*>(dungeon->getCell(oldx, oldy)->items, this);
      dungeon->getCell(x, y)->items.push_back(this);
    }
    duration -= elapsed;
    if (duration < 0.0f) {
      speed = 0.0f;
      if (dungeon->hasRipples(x, y)) {
        gameEngine->startRipple(x, y);
      }
      if (isA("arrow")) {
        return false;
      }
    }
  }
  for (ItemFeature** it = typeData->features.begin(); it != typeData->features.end(); it++) {
    ItemFeature* feat = *it;
    if (feat->id == ITEM_FEAT_AGE_EFFECT) {
      if (!age(elapsed, feat)) return false;
    } else if (feat->id == ITEM_FEAT_ATTACK) {
      switch (phase_) {
        case CAST:
          phase_timer_ -= elapsed;
          if (phase_timer_ <= 0.0f && (feat->attack.flags & WEAPON_PROJECTILE) == 0 && feat->attack.spellCasted) {
            phase_timer_ = reload_delay_;
            if (phase_timer_ > 0.0f)
              phase_ = RELOAD;
            else
              phase_ = IDLE;
            FireBall* fb =
                new FireBall(owner_->x, owner_->y, target_x_, target_y_, FB_STANDARD, feat->attack.spellCasted);
            ((screen::Game*)gameEngine)->addFireball(fb);
            gameEngine->stats.nbSpellStandard++;
          } else {
            if (feat->attack.flags & WEAPON_PROJECTILE) {
              // keep targetting while the mouse button is pressed
              int dx = mouse->cx + gameEngine->xOffset;
              int dy = mouse->cy + gameEngine->yOffset;
              target_x_ = dx;
              target_y_ = dy;
              if (!mouse->lbutton) {
                // fire when mouse button released
                phase_timer_ = MAX(phase_timer_, 0.0f);
                float speed = (cast_delay_ - phase_timer_) / cast_delay_;
                speed = MIN(speed, 1.0f);
                phase_ = RELOAD;
                phase_timer_ = reload_delay_;
                if ((int)target_x_ == (int)owner_->x && (int)target_y_ == (int)owner_->y) return true;
                x = owner_->x;
                y = owner_->y;
                Item* it = owner_->removeFromInventory(this);
                it->dx = target_x_ - x;
                it->dy = target_y_ - y;
                float l = fastInvSqrt(it->dx * it->dx + it->dy * it->dy);
                it->dx *= l;
                it->dy *= l;
                it->x = x;
                it->y = y;
                it->speed = speed * 12;
                it->duration = 1.5f;
                gameEngine->dungeon->addItem(it);
              }
            }
          }
          break;
        case RELOAD:
          phase_timer_ -= elapsed;
          if (phase_timer_ <= 0.0f) {
            phase_ = IDLE;
          }
          break;
        case IDLE:
          if (owner_->isPlayer() && mouse->lbutton_pressed && isEquiped()) {
            phase_timer_ = cast_delay_;
            phase_ = CAST;
            int dx = mouse->cx + gameEngine->xOffset;
            int dy = mouse->cy + gameEngine->yOffset;
            target_x_ = dx;
            target_y_ = dy;
          }
          break;
      }
    } else if (feat->id == ITEM_FEAT_HEAT) {
      heat_timer_ += elapsed;
      if (heat_timer_ > 1.0f) {
        // warm up adjacent items
        heat_timer_ = 0.0f;
        float radius = feat->heat.radius;
        for (int tx = -(int)floor(radius); tx <= (int)ceil(radius); tx++) {
          if ((int)(x) + tx >= 0 && (int)(x) + tx < dungeon->width) {
            int dy = (int)(sqrtf(radius * radius - tx * tx));
            for (int ty = -dy; ty <= dy; ty++) {
              if ((int)(y) + ty >= 0 && (int)(y) + ty < dungeon->height) {
                auto* items = dungeon->getItems((int)(x) + tx, (int)(y) + ty);
                for (Item* it : *items) {
                  // found an adjacent item
                  ItemFeature* fireFeat = it->getFeature(ITEM_FEAT_FIRE_EFFECT);
                  if (fireFeat) {
                    // item is affected by fire
                    it->fire_resistance_ -= feat->heat.intensity;
                  }
                }
                mob::Creature* cr = dungeon->getCreature((int)(x) + tx, (int)(y) + ty);
                if (cr) {
                  cr->takeDamage(feat->heat.intensity);
                  cr->burn = true;
                }
                if (gameEngine->player.x == x + tx && gameEngine->player.y == y + ty) {
                  gameEngine->player.takeDamage(feat->heat.intensity);
                }
              }
            }
          }
        }
      }
    } else if (feat->id == ITEM_FEAT_FIRE_EFFECT && fire_resistance_ <= 0.0f) {
      if (feat->fireEffect.type) {
        convertTo(feat->fireEffect.type);
        // set this item to fire!
        ItemFeature* ignite = feat->fireEffect.type->getFeature(ITEM_FEAT_HEAT);
        if (ignite) {
          float rad = ignite->heat.radius;
          gameEngine->startFireZone((int)(x - rad), (int)(y - 2 * rad), (int)(2 * rad + 1), (int)(3 * rad));
        }
      }
      // destroy this item
      if (!owner_) {
        if (as_creature_) gameEngine->dungeon->removeCreature(as_creature_, false);
      } else {
        if (as_creature_) as_creature_->toDelete = true;
      }
      ItemFeature* ignite = typeData->getFeature(ITEM_FEAT_HEAT);
      if (ignite) {
        // this item stops burning
        float rad = ignite->heat.radius;
        gameEngine->removeFireZone((int)(x - rad), (int)(y - 2 * rad), (int)(2 * rad + 1), (int)(3 * rad));
      }
      map::Cell* cell = dungeon->getCell(x, y);
      if (cell->building) {
        cell->building->collapseRoof();
      }
      return false;
    }
  }
  return true;
}

bool Item::isEquiped() { return (owner_ && (owner_->mainHand == this || owner_->offHand == this)); }

void Item::destroy(int count) {
  Item* newItem = NULL;
  if (owner_) {
    newItem = owner_->removeFromInventory(this, count);
    if (newItem) delete newItem;
  } else if (container_) {
    newItem = removeFromContainer(count);
    if (newItem) delete newItem;
  } else {
    newItem = gameEngine->dungeon->removeItem(this, count, true);
    if (newItem && newItem != this) delete newItem;
  }
}

void Item::use() {
  active_ = true;
  if (hasFeature(ITEM_FEAT_PRODUCES)) {
    float odds = TCODRandom::getInstance()->getFloat(0.0f, 1.0f);
    if (isA("tree")) {
      bool cut = false;
      if (gameEngine->player.mainHand && gameEngine->player.mainHand->isA("cut"))
        cut = true;
      else if (gameEngine->player.offHand && gameEngine->player.offHand->isA("cut"))
        cut = true;
      if (cut) {
        Item* it = produce(odds);
        gameEngine->gui.log.info("You cut %s off %s.", it->aName(), theName());
        it->putInInventory(&gameEngine->player);
      } else {
        if (TCODRandom::getInstance()->getInt(0, 2) == 0) {
          gameEngine->gui.log.info("You kick %s.", theName());
          gameEngine->gui.log.warn("You feel a sharp pain in the foot.");
          mob::Condition* cond = new mob::Condition(mob::CRIPPLED, 10.0f, 0.5f);
          gameEngine->hitFlash();
          gameEngine->player.addCondition(cond);
        } else {
          gameEngine->gui.log.info(
              "Some branches of %s are interesting, but you need something to cut them.", theName());
        }
      }
    } else {
      Item* it = produce(odds);
      gameEngine->gui.log.info("You pick %s.", it->aName());
      it->putInInventory(&gameEngine->player);
    }
  }
  ItemFeature* feat = getFeature(ITEM_FEAT_FOOD);
  if (feat) {
    gameEngine->player.heal(feat->food.health);
  }
  feat = getFeature(ITEM_FEAT_ATTACK);
  if (feat && owner_) {
    if (isEquiped())
      owner_->unwield(this);
    else
      owner_->wield(this);
  }
  feat = getFeature(ITEM_FEAT_CONTAINER);
  if (feat) {
    gameEngine->openCloseLoot(this);
  }
  if (isA("door")) {
    toggle_ = !toggle_;
    gameEngine->gui.log.info("You %s %s", toggle_ ? "open" : "close", theName());
    ch_ = toggle_ ? '/' : '+';
    gameEngine->dungeon->setProperties((int)x, (int)y, toggle_, toggle_);
  }
  if (isDeletedOnUse()) {
    if (count_ > 1)
      count_--;
    else {
      if (!owner_) {
        gameEngine->dungeon->removeItem(this, true);
      } else {
        owner_->removeFromInventory(this, true);
      }
    }
  }
}

Item* Item::putInInventory(mob::Creature* owner, int putCount, const char* verb) {
  if (putCount == 0) putCount = count_;
  Item* it = gameEngine->dungeon->removeItem(this, putCount, false);
  it->owner_ = owner;
  if (it->typeData->onPick) {
    it->name_ = it->typeData->onPick;
  }
  if (verb != NULL) gameEngine->gui.log.info("You %s %s.", verb, it->aName());
  it = owner->addToInventory(it);
  ItemFeature* feat = it->getFeature(ITEM_FEAT_ATTACK);
  if (feat) {
    // auto equip weapon if hand is empty
    if (owner->mainHand == NULL && (feat->attack.wield == WIELD_ONE_HAND || feat->attack.wield == WIELD_MAIN_HAND)) {
      owner->mainHand = it;
    } else if (
        owner->offHand == NULL && (feat->attack.wield == WIELD_ONE_HAND || feat->attack.wield == WIELD_OFF_HAND)) {
      owner->offHand = it;
    } else if (owner->mainHand == NULL && owner->offHand == NULL && feat->attack.wield == WIELD_TWO_HANDS) {
      owner->mainHand = owner->offHand = it;
    }
  }

  if (it->isUsedWhenPicked()) {
    it->use();
  }
  return it;
}

Item* Item::drop(int dropCount) {
  if (!owner_) return this;
  if (dropCount == 0) dropCount = count_;
  mob::Creature* ownerBackup = owner_;  // keep the owner for once the item is removed from inventory
  Item* newItem = owner_->removeFromInventory(this, dropCount);
  newItem->x = ownerBackup->x;
  newItem->y = ownerBackup->y;
  if (as_creature_) {
    // convert item back to creature
    gameEngine->dungeon->addCreature(as_creature_);
  } else {
    gameEngine->dungeon->addItem(newItem);
  }
  gameEngine->gui.log.info(
      "You drop %s %s.",
      newItem->theName(),
      gameEngine->dungeon->hasRipples(newItem->x, newItem->y) ? "in the water" : "on the ground");
  return newItem;
}

void Item::addModifier(modifier::ItemModifierId id, float value) {
  for (modifier::ItemModifier** mod = modifiers_.begin(); mod != modifiers_.end(); mod++) {
    if ((*mod)->id == id) {
      (*mod)->value += value;
      return;
    }
  }
  modifiers_.push(new modifier::ItemModifier(id, value));
}

void Item::renderDescriptionFrame(int x, int y, bool below, bool frame) {
  int cx = 0, cy = 0, cw = CON_W / 2, ch = CON_H / 2;
  bool stop = false;

  // find the right border
  for (cw = CON_W / 2; cw > cx && !stop; cw--) {
    for (int ty = 0; ty < ch; ty++) {
      if (descCon->getChar(cx + cw - 1, ty) != ' ') {
        stop = true;
        break;
      }
    }
  }
  // find the left border
  stop = false;
  for (cx = 0; cx < CON_W / 2 && !stop; cx++, cw--) {
    for (int ty = 0; ty < ch; ty++) {
      if (descCon->getChar(cx, ty) != ' ') {
        stop = true;
        break;
      }
    }
  }
  // find the bottom border
  stop = false;
  for (ch = CON_H / 2; ch > 0 && !stop; ch--) {
    for (int tx = cx; tx < cx + cw; tx++) {
      if (descCon->getChar(tx, cy + ch - 1) != ' ') {
        stop = true;
        break;
      }
    }
  }
  cx -= 2;
  cw += 4;
  ch += 2;
  if (frame) {
    // drawn the frame
    descCon->setDefaultForeground(ui::guiText);
    descCon->putChar(cx, cy, TCOD_CHAR_NW, TCOD_BKGND_NONE);
    descCon->putChar(cx + cw - 1, cy, TCOD_CHAR_NE, TCOD_BKGND_NONE);
    descCon->putChar(cx, cy + ch - 1, TCOD_CHAR_SW, TCOD_BKGND_NONE);
    descCon->putChar(cx + cw - 1, cy + ch - 1, TCOD_CHAR_SE, TCOD_BKGND_NONE);
    for (int tx = cx + 1; tx < cx + cw - 1; tx++) {
      if (descCon->getChar(tx, cy) == ' ') descCon->setChar(tx, cy, TCOD_CHAR_HLINE);
    }
    descCon->hline(cx + 1, cy + ch - 1, cw - 2, TCOD_BKGND_NONE);
    descCon->vline(cx, cy + 1, ch - 2, TCOD_BKGND_NONE);
    descCon->vline(cx + cw - 1, cy + 1, ch - 2, TCOD_BKGND_NONE);
  }
  if (!below) y = y - ch + 1;
  if (x - cw / 2 < 0)
    x = cw / 2;
  else if (x + cw / 2 > CON_W)
    x = CON_W - cw / 2;
  if (y + ch > CON_H) y = CON_H - ch;
  TCODConsole::blit(descCon, cx, cy, cw, ch, TCODConsole::root, x - cw / 2, y, 1.0f, frame ? 0.7f : 0.0f);
}

static auto GetCountedName(const Item& item) -> std::string {
  const int count = item.count_ > 1 ? item.count_ : item.stack_.size() + 1;
  const auto& nameToUse = item.name_.value_or(item.typeName_);
  bool es = (nameToUse[nameToUse.size() - 1] == 's');
  return fmt::format(
      "{:d} {}{}{}{}", count, item.adjective_.value_or(""), item.adjective_ ? " " : "", nameToUse, es ? "es" : "s");
}

static auto GetNameWithPrefix(const Item& item, const char* prefix) -> std::string {
  return fmt::format(
      "{} {}{}{}",
      prefix,
      item.adjective_.value_or(""),
      item.adjective_ ? " " : "",
      item.name_.value_or(item.typeName_));
}

std::string Item::AName() const {
  if (count_ == 1 && (!isSoftStackable() || stack_.size() == 0)) {
    if (name_) {
      return GetNameWithPrefix(*this, an_ ? "An" : "A");
    } else {
      return GetNameWithPrefix(*this, (typeData->flags & ITEM_AN) ? "An" : "A");
    }
  } else {
    return GetCountedName(*this);
  }
}

std::string Item::aName() const {
  if (count_ == 1 && (!isSoftStackable() || stack_.size() == 0)) {
    if (name_) {
      return GetNameWithPrefix(*this, an_ ? "an" : "a");
    } else {
      return GetNameWithPrefix(*this, (typeData->flags & ITEM_AN) ? "An" : "A");
    }
  } else {
    return GetCountedName(*this);
  }
}

std::string Item::TheName() const {
  if (count_ == 1 && (!isSoftStackable() || stack_.size() == 0)) {
    return GetNameWithPrefix(*this, "The");
  } else {
    return GetCountedName(*this);
  }
}

std::string Item::theName() const {
  if (count_ == 1 && (!isSoftStackable() || stack_.size() == 0)) {
    return GetNameWithPrefix(*this, "the");
  } else {
    return GetCountedName(*this);
  }
}

#define ITEM_CHUNK_VERSION 6

bool Item::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != ITEM_CHUNK_VERSION) return false;
  x = zip->getFloat();
  y = zip->getFloat();
  item_class_ = (ItemClass)zip->getInt();
  if (item_class_ < 0 || item_class_ >= NB_ITEM_CLASSES) return false;
  color_ = zip->getColor();
  typeName_ = zip->getString();
  const char* fname = zip->getString();
  if (fname) name_ = fname;
  count_ = zip->getInt();
  ch_ = zip->getInt();
  an_ = zip->getChar() == 1;
  life_ = zip->getFloat();

  int nbItems = zip->getInt();
  bool soft = isSoftStackable();
  while (nbItems > 0) {
    const char* itemTypeName = zip->getString();
    ItemType* itemType = Item::getType(itemTypeName);
    if (!itemType) return false;
    uint32_t itemChunkId, itemChunkVersion;
    saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
    Item* it = Item::getItem(itemType, 0, 0);
    if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
    if (soft)
      stack_.push_back(it);
    else
      it->putInContainer(this);
    nbItems--;
  }

  ItemFeature* feat = getFeature(ITEM_FEAT_ATTACK);
  if (feat) {
    cast_delay_ = zip->getFloat();
    reload_delay_ = zip->getFloat();
    damages_ = zip->getFloat();
  }

  int nbModifiers = zip->getInt();
  while (nbModifiers > 0) {
    modifier::ItemModifierId id = (modifier::ItemModifierId)zip->getInt();
    if (id < 0 || id >= modifier::ITEM_MOD_NUMBER) return false;
    float value = zip->getFloat();
    modifier::ItemModifier* mod = new modifier::ItemModifier(id, value);
    modifiers_.push(mod);
  }
  return true;
}

void Item::saveData(uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(ITEM_CHUNK_ID, ITEM_CHUNK_VERSION);
  zip->putFloat(x);
  zip->putFloat(y);
  zip->putInt(item_class_);
  zip->putColor(&color_);
  zip->putString(typeName_.c_str());
  zip->putString(name_ ? name_->c_str() : nullptr);
  zip->putInt(count_);
  zip->putInt(ch_);
  zip->putChar(an_ ? 1 : 0);
  zip->putFloat(life_);
  // save items inside this item or soft stacks
  zip->putInt(stack_.size());
  for (Item* it : stack_) {
    zip->putString(it->typeData->name.c_str());
    it->saveData(ITEM_CHUNK_ID, zip);
  }
  ItemFeature* feat = getFeature(ITEM_FEAT_ATTACK);
  if (feat) {
    zip->putFloat(cast_delay_);
    zip->putFloat(reload_delay_);
    zip->putFloat(damages_);
  }

  zip->putInt(modifiers_.size());
  for (modifier::ItemModifier** it = modifiers_.begin(); it != modifiers_.end(); it++) {
    zip->putInt((*it)->id);
    zip->putFloat((*it)->value);
  }
}
}  // namespace item

/*
 * Copyright (c) 2009,2010 Jice
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
#include "mob/creature.hpp"

#include <stdio.h>

#include "base/aidirector.hpp"
#include "constants.hpp"
#include "main.hpp"
#include "mob/boss.hpp"
#include "mob/fish.hpp"
#include "mob/friend.hpp"
#include "mob/minion.hpp"

namespace mob {
TCODList<Creature*> Creature::creatureByType[NB_CREATURE_TYPES];

TCODList<ConditionType*> ConditionType::list;

ConditionType* ConditionType::find(const char* name) {
  for (ConditionType** it = list.begin(); it != list.end(); it++) {
    if (strcmp((*it)->name, name) == 0) return *it;
  }
  return NULL;
}

ConditionType* ConditionType::get(ConditionTypeId type) {
  for (ConditionType** it = list.begin(); it != list.end(); it++) {
    if ((*it)->type == type) return *it;
  }
  return NULL;
}

void ConditionType::init() {
  list.push(new ConditionType(STUNNED, "stunned"));
  list.push(new ConditionType(BLEED, "bleed"));
  list.push(new ConditionType(HEAL, "heal"));
  list.push(new ConditionType(POISONED, "poisoned"));
  list.push(new ConditionType(IMMUNE, "immune"));
  list.push(new ConditionType(PARALIZED, "paralized"));
  list.push(new ConditionType(CRIPPLED, "crippled"));
  list.push(new ConditionType(WOUNDED, "wounded"));
}

bool ConditionType::check(Creature* cr) {
  switch (type) {
    case POISONED:
      // a immune creature cannot be poisoned
      return (!cr->hasCondition(IMMUNE));
      break;
    default:
      return true;
      break;
  }
}

Condition::Condition(ConditionTypeId type, float duration, float amount, const char* alias)
    : initialDuration(duration), duration(duration), amount(amount), curAmount(0.0f), alias(alias) {
  this->type = ConditionType::get(type);
}

bool Condition::equals(ConditionTypeId type, const char* name) {
  return this->type->type == type && (name == NULL || (alias && strcmp(alias, name) == 0));
}

// warning : increase CREA_CHUNK_VERSION when changing this

void Condition::save(TCODZip* zip) {
  zip->putInt(type->type);
  zip->putString(alias);
  zip->putFloat(initialDuration);
  zip->putFloat(duration);
  zip->putFloat(amount);
  zip->putFloat(curAmount);
}

void Condition::load(TCODZip* zip) {
  ConditionTypeId typeId = (ConditionTypeId)zip->getInt();
  type = ConditionType::get(typeId);
  alias = zip->getString();
  if (alias) alias = strdup(alias);
  initialDuration = zip->getFloat();
  duration = zip->getFloat();
  amount = zip->getFloat();
  curAmount = zip->getFloat();
}

bool Condition::update(float elapsed) {
  curAmount += amount * elapsed / initialDuration;
  switch (type->type) {
    case POISONED:
    case BLEED: {
      // lose health over time
      if (curAmount > 0) {
        target->takeDamage(curAmount);
        // TODO
        // GameScreen::getInstance()->addBloodStain(target->x,target->y,lostHp);
      }
      curAmount = 0;

    } break;
    case HEAL: {
      // gain health over time
      if (curAmount > 0) {
        target->life_ += curAmount;
        if (target->life_ > target->max_life_) target->life_ = target->max_life_;
      }
      curAmount = 0;
    } break;
    default:
      break;
  }
  duration -= elapsed;
  if (duration <= 0.0f) {
    switch (type->type) {
      case WOUNDED: {
        // wounded decrease the max hp
        target->max_life_ = target->max_life_ / (1.0f - amount);
      } break;
      default:
        break;
    }
    return true;
  }
  return false;
}

void Condition::applyTo(Creature* cr) {
  cr->addCondition(this);
  switch (type->type) {
    case IMMUNE: {
      // the immune condition remove all poisoned conditions
      Condition* cond = NULL;
      do {
        cond = cr->getCondition(POISONED);
        if (cond) {
          cr->conditions_.remove(cond);
          delete cond;
        }
      } while (cond);
    } break;
    case WOUNDED: {
      // wounded decrease the max hp
      const float quantity = amount * cr->max_life_;
      cr->max_life_ -= quantity;
      if (cr->max_life_ < 0) {
        cr->max_life_ = 0;
      }
      cr->life_ = std::min(cr->max_life_, cr->life_);
    } break;
    default:
      break;
  }
}

void Creature::addCondition(Condition* cond) {
  conditions_.push(cond);
  cond->target = this;
}

bool Creature::hasCondition(ConditionTypeId type, const char* alias) { return (getCondition(type, alias) != NULL); }

float Creature::getMaxConditionDuration(ConditionTypeId type, const char* alias) {
  float maxVal = -1E8f;
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    if ((*it)->equals(type, alias) && (*it)->duration > maxVal) maxVal = (*it)->duration;
  }
  return maxVal;
}

float Creature::getMinConditionAmount(ConditionTypeId type, const char* alias) {
  float minVal = 1E8f;
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    if ((*it)->equals(type, alias) && (*it)->amount < minVal) minVal = (*it)->amount;
  }
  return minVal;
}

float Creature::getMaxConditionAmount(ConditionTypeId type, const char* alias) {
  float maxVal = -1E8f;
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    if ((*it)->equals(type, alias) && (*it)->amount > maxVal) maxVal = (*it)->amount;
  }
  return maxVal;
}

Condition* Creature::getCondition(ConditionTypeId type, const char* alias) {
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    if ((*it)->equals(type, alias)) return *it;
  }
  return NULL;
}

Creature* Creature::getCreature(CreatureTypeId id) {
  Creature* ret = NULL;
  switch (id) {
    case CREATURE_DEER:
      ret = new Creature();
      ret->name_ = "deer";
      ret->ch_ = 'd';
      ret->color_ = TCODColor::darkerYellow;
      ret->max_life_ = ret->life_ = 10.0f;
      ret->speed_ = 20.0f;
      ret->flags_ = CREATURE_SAVE;
      ret->current_behavior_ = new HerdBehavior(new AvoidWaterWalkPattern());
      break;
    case CREATURE_FRIEND:
      ret = new Friend();
      break;
    case CREATURE_MINION:
      ret = new Minion();
      break;
    case CREATURE_VILLAGER:
      ret = new Villager();
      break;
    case CREATURE_ARCHER:
      ret = new Archer();
      break;
    case CREATURE_ZEEPOH:
      ret = new Boss();
      break;
    case CREATURE_VILLAGE_HEAD:
      ret = new VillageHead();
      break;
    case CREATURE_FISH:
      ret = new Fish(NULL);
      break;
    default:
      break;
  }
  if (ret) {
    ret->type_ = id;
    creatureByType[id].push(ret);
  }
  return ret;
}

bool Creature::isInRange(int px, int py) {
  const int dx = (int)(px - x_);
  const int dy = (int)(py - y_);
  return (abs(dx) <= fov_range_ && abs(dy) <= fov_range_ && dx * dx + dy * dy <= fov_range_ * fov_range_);
}

bool Creature::isPlayer() { return this == &gameEngine->player; }

void Creature::talk(std::string_view text) {
  talk_text_.text = text;
  talk_text_.delay = gsl::narrow_cast<float>(talk_text_.text.size()) * 0.1f;
  talk_text_.delay = std::max(0.5f, talk_text_.delay);
  // compute text size
  const char* ptr = talk_text_.text.c_str();
  talk_text_.h_ = 1;
  talk_text_.w_ = 0;
  const char* end = strchr(ptr, '\n');
  while (end) {
    const auto len = gsl::narrow<int>(end - ptr);
    if (talk_text_.w_ < len) talk_text_.w_ = len;
    ++talk_text_.h_;
    ptr = end + 1;
    end = strchr(ptr, '\n');
  }
  if (end) {
    const auto len = gsl::narrow<int>(end - ptr);
    if (talk_text_.w_ < len) talk_text_.w_ = len;
  }
}

void Creature::renderTalk() {
  const int console_x = (int)(x_ - gameEngine->xOffset);
  const int console_y = (int)(y_ - gameEngine->yOffset);
  if (!IN_RECTANGLE(console_x, console_y, CON_W, CON_H)) return;  // creature out of console
  talk_text_.x_ = gsl::narrow_cast<float>(console_x);
  talk_text_.y_ = gsl::narrow_cast<float>(console_y - talk_text_.h_);
  if (talk_text_.y_ < 0) talk_text_.y_ = gsl::narrow_cast<float>(console_y + 1);
  gameEngine->packer.addRect(&talk_text_);
  TCODConsole::root->setDefaultBackground(TCODColor::lighterYellow);
  TCODConsole::root->setDefaultForeground(TCODColor::darkGrey);
  TCODConsole::root->printEx(
      (int)talk_text_.x_, (int)talk_text_.y_, TCOD_BKGND_SET, TCOD_CENTER, talk_text_.text.c_str());
}

void Creature::render(map::LightMap& lightMap) {
  static const int penumbraLevel = config.getIntProperty("config.gameplay.penumbraLevel");
  static const int darknessLevel = config.getIntProperty("config.gameplay.darknessLevel");
  static const float fireSpeed = config.getFloatProperty("config.display.fireSpeed");
  static const TCODColor corpseColor = config.getColorProperty("config.display.corpseColor");
  static const std::array<TCODColor, 64> fire_color = []() {
    static constexpr TCODColor lowFire(255, 0, 0);
    static constexpr TCODColor midFire(255, 204, 0);
    static constexpr TCODColor highFire(255, 255, 200);
    std::array<TCODColor, 64> new_array{};
    for (int i = 0; i < 32; ++i) {
      new_array.at(i) = TCODColor::lerp(lowFire, midFire, i / 32.0f);
    }
    for (int i = 32; i < 64; ++i) {
      new_array.at(i) = TCODColor::lerp(midFire, highFire, (i - 32) / 32.0f);
    }
    return new_array;
  }();

  // position on console
  const int console_x = (int)(x_ - gameEngine->xOffset);
  const int console_y = (int)(y_ - gameEngine->yOffset);
  if (!IN_RECTANGLE(console_x, console_y, CON_W, CON_H)) return;  // out of console

  const float playerDist = distance(gameEngine->player);
  const float apparentHeight = height_ / playerDist;
  if (apparentHeight < MIN_VISIBLE_HEIGHT) return;  // too small to see at that distance

  TCODColor color{};
  int displayChar = ch_;
  TCODColor lightColor = lightMap.getColor(console_x, console_y) * 1.5f;
  const map::Dungeon* dungeon = gameEngine->dungeon;
  float shadow = dungeon->getShadow(x_ * 2, y_ * 2);
  const float clouds = dungeon->getCloudCoef(x_ * 2, y_ * 2);
  shadow = std::min(shadow, clouds);
  lightColor = lightColor * shadow;
  if (life_ <= 0) {
    ch_ = '%';
    color = corpseColor * lightColor;
  } else if (burn_) {
    const float fire_x = TCODSystem::getElapsedSeconds() * fireSpeed + noise_offset_;
    const int fire_index = (int)((0.5f + 0.5f * noise1d.get(&fire_x)) * 64.0f);
    color = fire_color.at(fire_index);
    const int r = (int)(color.r * 1.5f * lightColor.r / 255);
    const int g = (int)(color.g * 1.5f * lightColor.g / 255);
    const int b = (int)(color.b * 1.5f * lightColor.b / 255);
    color.r = gsl::narrow<uint8_t>(std::clamp(r, 0, 255));
    color.g = gsl::narrow<uint8_t>(std::clamp(g, 0, 255));
    color.b = gsl::narrow<uint8_t>(std::clamp(b, 0, 255));
  } else {
    color = color_ * lightColor;
  }
  int intensity = color.r + color.g + color.b;
  if (intensity < darknessLevel) return;  // creature not seen
  if (intensity < penumbraLevel) displayChar = '?';
  if (apparentHeight < VISIBLE_HEIGHT) displayChar = '?';  // too small to distinguish
  TCODConsole::root->setChar(console_x, console_y, displayChar);
  TCODConsole::root->setCharForeground(console_x, console_y, color);
}

void Creature::stun(float delay) { walk_timer_ = std::min(-delay, walk_timer_); }

bool Creature::walk(float elapsed) {
  walk_timer_ += elapsed;
  map::TerrainId terrainId = gameEngine->dungeon->getTerrainType((int)x_, (int)y_);
  const float walkTime = map::terrainTypes[terrainId].walkCost / speed_;
  if (walk_timer_ >= 0) {
    walk_timer_ = -walkTime;
    if (path_ && !path_->isEmpty()) {
      int next_x{};
      int next_y{};
      base::GameEngine* game = gameEngine;
      path_->get(0, &next_x, &next_y);
      if ((game->player.x_ != next_x || game->player.y_ != next_y) && !game->dungeon->hasCreature(next_x, next_y)) {
        const int old_x = (int)x_;
        const int old_y = (int)y_;
        int new_x = old_x;
        int new_y = old_y;
        if (path_->walk(&new_x, &new_y, false)) {
          setPos(new_x, new_y);
          game->dungeon->moveCreature(this, old_x, old_y, new_x, new_y);
          if (game->dungeon->hasRipples(new_x, new_y)) {
            gameEngine->startRipple(new_x, new_y);
          }
          return true;
        }
      }
    }
  }
  return false;
}

void Creature::randomWalk(float elapsed) {
  walk_timer_ += elapsed;
  if (walk_timer_ >= 0) {
    walk_timer_ = -1.0f / speed_;
    static constexpr int dir_x[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static constexpr int dir_y[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int d = TCODRandom::getInstance()->getInt(0, 7);
    int count = 8;
    base::GameEngine* game = gameEngine;
    do {
      int new_x = (int)(x_ + dir_x[d]), new_y = (int)(y_ + dir_y[d]);
      if (IN_RECTANGLE(new_x, new_y, game->dungeon->width, game->dungeon->height) &&
          game->dungeon->map->isWalkable(new_x, new_y) && (game->player.x_ != new_x || game->player.y_ != new_y) &&
          !game->dungeon->hasCreature(new_x, new_y)) {
        game->dungeon->moveCreature(this, (int)x_, (int)y_, new_x, new_y);
        x_ = gsl::narrow_cast<decltype(x_)>(new_x);
        y_ = gsl::narrow_cast<decltype(y_)>(new_y);
        return;
      }
      d = (d + 1) % 8;
      count--;
    } while (count > 0);
  }
}

float Creature::getWalkCost(int, int, int xTo, int yTo, void*) const {
  base::GameEngine* game = gameEngine;
  if (!game->dungeon->map->isWalkable(xTo, yTo)) return 0.0f;
  if (ignore_creatures_) return 1.0f;
  if (game->dungeon->hasCreature(xTo, yTo)) return 50.0f;
  if (game->player.x_ == xTo || game->player.y_ == yTo) return 50.0f;
  return 1.0f;
}

void Creature::takeDamage(float amount) {
  current_damage_ += amount;
  int idmg = (int)current_damage_;
  if (idmg > 0) {
    if (life_ > 0 && life_ <= idmg && this != &gameEngine->player) {
      base::AiDirector::instance->killCreature(this);
      gameEngine->stats.creatureDeath[type_]++;
    }
    life_ -= idmg;
    current_damage_ -= idmg;
  }
}

item::Item* Creature::addToInventory(item::Item* item) {
  item = item->addToList(inventory_);
  item->owner_ = this;
  item->x_ = x_;
  item->y_ = y_;
  return item;
}

item::Item* Creature::removeFromInventory(item::Item* item, int count) {
  if (count == 0) count = item->count_;
  item = item->removeFromList(inventory_, count);
  if (item == main_hand_ || item == off_hand_) unwield(item);
  item->owner_ = NULL;
  return item;
}

void Creature::updateConditions(float elapsed) {
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    if ((*it)->update(elapsed)) {
      delete *it;
      it = conditions_.remove(it);
    }
  }
}

bool Creature::update(float elapsed) {
  static float burnDamage = config.getFloatProperty("config.creatures.burnDamage");
  static int distReplace =
      config.getIntProperty("config.aidirector.distReplace") * config.getIntProperty("config.aidirector.distReplace");

  if (life_ <= 0) {
    creatureByType[type_].removeFast(this);
    return false;
  }
  if (talk_text_.delay > 0.0f) {
    talk_text_.delay -= elapsed;
    if (talk_text_.delay < 0.0f) talk_text_.delay = -10.0f;
  } else if (talk_text_.delay < 0.0f) {
    talk_text_.delay += elapsed;
    if (talk_text_.delay > 0.0f) talk_text_.delay = 0.0f;
  }

  updateConditions(elapsed);

  base::GameEngine* game = gameEngine;
  if (isReplacable()) {
    int pdist = (int)squaredDistance(game->player);
    if (pdist > distReplace) {
      base::AiDirector::instance->replace(this);
      return true;
    }
  }

  if (burn_) {
    takeDamage(burnDamage * elapsed);
  }
  // update items in inventory
  inventory_.erase(
      std::remove_if(inventory_.begin(), inventory_.end(), [&elapsed](item::Item* it) { return it->age(elapsed); }),
      inventory_.end());
  // ai
  if (current_behavior_) {
    if (!current_behavior_->update(this, elapsed)) current_behavior_ = NULL;
  }
  return life_ > 0;
}

void Creature::equip(item::Item* it) {
  item::ItemFeature* feat = it->getFeature(item::ITEM_FEAT_ATTACK);
  switch (feat->attack.wield) {
    case item::WIELD_NONE:
      break;
    case item::WIELD_MAIN_HAND:
      if (off_hand_ && off_hand_ == main_hand_) off_hand_ = NULL;  // unequip two hands weapon
      main_hand_ = it;
      break;
    case item::WIELD_ONE_HAND:
      if (!main_hand_)
        main_hand_ = it;
      else if (!off_hand_)
        off_hand_ = it;
      else {
        if (off_hand_ == main_hand_) off_hand_ = NULL;
        main_hand_ = it;
      }
      break;
    case item::WIELD_OFF_HAND:
      if (main_hand_ && off_hand_ == main_hand_) main_hand_ = NULL;  // unequip two hands weapon
      off_hand_ = it;
      break;
    case item::WIELD_TWO_HANDS:
      main_hand_ = off_hand_ = it;
      break;
  }
}

void Creature::unequip(item::Item* it) {
  if (it == main_hand_) main_hand_ = NULL;
  if (it == off_hand_) off_hand_ = NULL;  // might be both for two hands items
}

void Creature::wield(item::Item* it) {
  item::ItemFeature* feat = it->getFeature(item::ITEM_FEAT_ATTACK);
  switch (feat->attack.wield) {
    case item::WIELD_NONE:
      gameEngine->gui.log.warn("You cannot wield %s", it->aName());
      return;
      break;
    case item::WIELD_MAIN_HAND:
      if (main_hand_) unwield(main_hand_);
      break;
    case item::WIELD_OFF_HAND:
      if (off_hand_) unwield(off_hand_);
      break;
    case item::WIELD_ONE_HAND:
      if (main_hand_ && off_hand_) unwield(main_hand_);
      break;
    case item::WIELD_TWO_HANDS:
      if (main_hand_) unwield(main_hand_);
      if (off_hand_) unwield(off_hand_);
      break;
  }
  equip(it);
  if (isPlayer()) {
    gameEngine->gui.log.info("You're wielding %s", it->aName());
  }
}

void Creature::unwield(item::Item* it) {
  unequip(it);
  if (this == &gameEngine->player) {
    gameEngine->gui.log.info("You were wielding %s", it->aName());
  }
}

#define CREA_CHUNK_VERSION 6
void Creature::saveData([[maybe_unused]] uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(CREA_CHUNK_ID, CREA_CHUNK_VERSION);
  zip->putFloat(x_);
  zip->putFloat(y_);
  zip->putFloat(life_);
  zip->putString(name_.c_str());
  // save inventory
  zip->putInt(gsl::narrow<int>(inventory_.size()));
  for (item::Item* it : inventory_) {
    zip->putString(it->typeData->name.c_str());
    it->saveData(ITEM_CHUNK_ID, zip);
  }
  // save conditions
  zip->putInt(conditions_.size());
  for (Condition** it = conditions_.begin(); it != conditions_.end(); it++) {
    (*it)->save(zip);
  }
}

bool Creature::loadData([[maybe_unused]] uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != CREA_CHUNK_VERSION) return false;
  x_ = zip->getFloat();
  y_ = zip->getFloat();
  life_ = zip->getFloat();
  name_ = zip->getString();
  // load inventory
  int nbItems = zip->getInt();
  while (nbItems > 0) {
    const char* itemTypeName = zip->getString();
    item::ItemType* itemType = item::Item::getType(itemTypeName);
    if (!itemType) return false;
    uint32_t itemChunkId, itemChunkVersion;
    saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
    item::Item* it = item::Item::getItem(itemType, 0, 0);
    if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
    addToInventory(it);
    nbItems--;
  }
  // load conditions
  int nbConditions = zip->getInt();
  while (nbConditions > 0) {
    Condition* cond = new Condition();
    cond->target = this;
    cond->load(zip);
    conditions_.push(cond);
    nbConditions--;
  }
  return true;
}
}  // namespace mob

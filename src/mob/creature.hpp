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
#include <memory>
#include <string>
#include <string_view>

#include "base/entity.hpp"
#include "base/noisything.hpp"
#include "base/savegame.hpp"
#include "item.hpp"
#include "mob/behavior.hpp"

namespace screen {
class Game;
}

namespace mob {
class Creature;
}

namespace mob {
enum CreatureTypeId {
  // the cave chapter 1
  CREATURE_FRIEND,
  CREATURE_FISH,
  CREATURE_DEER,
  // pyromancer
  CREATURE_MINION,
  CREATURE_ZEEPOH,
  // screen::TreeBurner
  CREATURE_VILLAGER,
  CREATURE_VILLAGE_HEAD,
  CREATURE_ARCHER,
  NB_CREATURE_TYPES
};

enum CreatureFlags {
  CREATURE_REPLACABLE = 1,  // if too far, put it back near the player
  CREATURE_OFFSCREEN = 2,  // updated even if out of console screen
  CREATURE_SAVE = 4,  // save this creature in savegame
  CREATURE_NOTBLOCK = 8,  // does not block path
  CREATURE_CATCHABLE = 16,  // can catch a creature by clicking on it when adjacent
};

static constexpr auto VISIBLE_HEIGHT = 0.05f;
static constexpr auto MIN_VISIBLE_HEIGHT = 0.02f;

enum ConditionTypeId {
  STUNNED,
  BLEED,
  HEAL,
  POISONED,
  IMMUNE,
  PARALIZED,
  CRIPPLED,
  WOUNDED,
};
class ConditionType {
 public:
  ConditionTypeId type;
  const char* name = nullptr;
  static TCODList<ConditionType*> list;
  static ConditionType* find(const char* name);
  static ConditionType* get(ConditionTypeId type);
  static void init();
  // return true if condition can be applied
  bool check(Creature* cr);

 private:
  ConditionType(ConditionTypeId type, const char* name) : type(type), name(name) {}
};

class Condition {
 public:
  ConditionType* type = nullptr;
  Creature* target = nullptr;
  float initialDuration, duration, amount;  // remaining duration
  float curAmount;
  const char* alias = nullptr;
  Condition() {}
  Condition(ConditionTypeId type, float duration, float amount, const char* alias = NULL);
  // return true if condition finished
  bool update(float elapsed);
  void applyTo(Creature* cr);
  bool equals(ConditionTypeId type, const char* alias = NULL);
  const char* getName() { return alias ? alias : type->name; }
  void save(TCODZip* zip);
  void load(TCODZip* zip);
};

class Creature : public base::DynamicEntity,
                 public ITCODPathCallback,
                 public base::NoisyThing,
                 public base::SaveListener {
 public:
  Creature() = default;

  virtual void onReplace() {}

  // conditions
  void addCondition(Condition* cond);
  bool hasCondition(ConditionTypeId type, const char* alias = NULL);
  Condition* getCondition(ConditionTypeId type, const char* alias = NULL);
  void updateConditions(float elapsed);
  float getMaxConditionDuration(ConditionTypeId type, const char* alias = NULL);
  float getMinConditionAmount(ConditionTypeId type, const char* alias = NULL);
  float getMaxConditionAmount(ConditionTypeId type, const char* alias = NULL);

  // factory
  static Creature* getCreature(CreatureTypeId id);
  static TCODList<Creature*> creatureByType[NB_CREATURE_TYPES];

  virtual bool update(float elapsed);
  virtual void render(map::LightMap& lightMap);
  void renderTalk();
  virtual void takeDamage(float amount);
  virtual void stun(float delay);
  virtual float getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const override;
  void talk(std::string_view text);
  bool isTalking() const noexcept { return talk_text_.delay > 0.0f; }
  bool isInRange(int x, int y);
  bool isPlayer();

  // flags
  bool isReplacable() const noexcept { return (flags_ & CREATURE_REPLACABLE) != 0; }
  bool isUpdatedOffscreen() const noexcept { return (flags_ & CREATURE_OFFSCREEN) != 0; }
  bool mustSave() const noexcept { return (flags_ & CREATURE_SAVE) != 0; }
  bool isBlockingPath() const noexcept { return (flags_ & CREATURE_NOTBLOCK) == 0; }
  bool isCatchable() const noexcept { return (flags_ & CREATURE_CATCHABLE) != 0; }

  // items
  item::Item* addToInventory(item::Item* it);  // in case of stackable items, returned item might be != it
  item::Item* removeFromInventory(item::Item* it, int count = 1);  // same as addToInventory
  auto& getInventory() { return inventory_; }

  void equip(item::Item* it);
  void unequip(item::Item* it);
  // same as equip/unequip but with messages (you're wielding...)
  void wield(item::Item* it);
  void unwield(item::Item* it);
  virtual void initItem() {}  // build as_item_ member

  // SaveListener
  bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) override;
  void saveData(uint32_t chunkId, TCODZip* zip) override;

  CreatureTypeId type_{};
  TCODColor color_{};
  int ch_{};  // character
  float life_{};
  float max_life_{};
  float speed_{};
  float height_{1.0f};  // in meters
  std::unique_ptr<TCODPath> path_{};
  bool ignore_creatures_{};  // walk mode
  bool burn_{};
  int flags_{};
  std::string name_{};
  item::Item* main_hand_{};
  item::Item* off_hand_{};
  item::Item* as_item_{};  // an item corresponding to this creature
  TCODList<Condition*> conditions_{};
  Behavior* current_behavior_{};  // ai
  float fov_range_{};
  bool to_delete_{};

 protected:
  friend class Behavior;
  friend class FollowBehavior;
  friend class HerdBehavior;
  friend class ForestScreen;
  struct TalkText : public base::Rect {
    std::string text{};
    float delay{};
  };
  bool walk(float elapsed);
  void randomWalk(float elapsed);

  std::vector<item::Item*> inventory_{};
  float walk_timer_{};
  float path_timer_{};
  float current_damage_{};
  TalkText talk_text_{};
};
}  // namespace mob

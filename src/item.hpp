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

class Creature;
class ItemType;

// item class, mainly for weapons. higher classes have more modifiers
enum ItemClass {
	ITEM_CLASS_STANDARD,
	ITEM_CLASS_GREEN,
	ITEM_CLASS_ORANGE,
	ITEM_CLASS_RED,
	ITEM_CLASS_SILVER,
	ITEM_CLASS_GOLD,
	NB_ITEM_CLASSES
};

enum ItemFlags {
	ITEM_NOT_WALKABLE   	=   1,
	ITEM_NOT_TRANSPARENT	=   2,
	ITEM_NOT_PICKABLE    	=   4,
	ITEM_STACKABLE       	=   8,
	ITEM_SOFT_STACKABLE  	=  16,
	ITEM_DELETE_ON_USE   	=  32,
	ITEM_USE_WHEN_PICKED	=  64,
	ITEM_CONTAINER			= 128,
	ITEM_ACTIVATE_ON_BUMP	= 256,
	ITEM_AN					= 512,
	ITEM_ABSTRACT			=1024,
	ITEM_BUILD_NOT_BLOCK	=2048,   // building cell that cannot be blocked (door, window)
};

enum WeaponPhase { IDLE, CAST, RELOAD };

// an ingredient for a crafting recipe
struct ItemIngredient {
	ItemType *type; // item type
	int quantity;
	bool optional;
	bool destroy; // consumed by the combination ?
	bool revert; // can be disassembled back ?
};

#define MAX_INGREDIENTS 5

// item recipe
class Item ;
class ItemType;
struct ItemCombination {
	ItemType *resultType;
	int nbResult;
	ItemType *tool;
	int nbIngredients;
	ItemIngredient ingredients[MAX_INGREDIENTS];
	bool isTool(const Item *item) const;
	bool isIngredient(const Item *item) const;
	bool isTool(const ItemType *item) const;
	bool isIngredient(const ItemType *item) const ;
	bool hasTool() const {return tool != NULL;}
	const ItemIngredient * getIngredient(Item *item) const;
};

enum ItemFeatureId {
	ITEM_FEAT_FIRE_EFFECT,  // fire has an effect on item (either transform or destroy)
	ITEM_FEAT_PRODUCES,  // item produces other items when clicked
	ITEM_FEAT_FOOD, // item can be eaten and restore health
	ITEM_FEAT_AGE_EFFECT, // time has an effect on item (either transform or destroy)
	ITEM_FEAT_ATTACK, // when wielded, deal damages or can be thrown
	ITEM_FEAT_LIGHT, // produces light when lying on ground
	ITEM_FEAT_HEAT, // produces heat
	ITEM_FEAT_CONTAINER, // can contain other items
	NB_ITEM_FEATURES
};

// context-menu actions in the inventory screen
enum ItemActionId {
	ITEM_ACTION_TAKE,
	ITEM_ACTION_TAKE_ALL,
	ITEM_ACTION_USE,
	ITEM_ACTION_DROP,
	ITEM_ACTION_DROP_ALL,
	ITEM_ACTION_THROW,
	ITEM_ACTION_DISASSEMBLE,
	ITEM_ACTION_FILL, // bottle when in water
	NB_ITEM_ACTIONS};

enum ItemActionFlag {
	ITEM_ACTION_INVENTORY=1,
	ITEM_ACTION_LOOT=2,
	ITEM_ACTION_ONWATER=4,
};

struct ItemAction {
	const char *name;
	int flags;
	static ItemAction *getFromId(ItemActionId id);
	inline bool onWater() {return (flags & ITEM_ACTION_ONWATER) != 0;}
	inline bool onInventory() {return (flags & ITEM_ACTION_INVENTORY) != 0;}
	inline bool onLoot() {return (flags & ITEM_ACTION_LOOT) != 0;}
};

class ItemType;

struct ItemFireEffect {
	float resistance; // in seconds
	ItemType *type; // if NULL, fire destroys the item
};

struct ItemProduce {
	float delay; // in seconds
	float chance;
	ItemType *type;
};

struct ItemFood {
	int health;
};

struct ItemAgeEffect {
	float delay; // in seconds
	ItemType *type; // if NULL, destroy item
};

// cannot use TCODColor in a union because it has a constructor...
struct ItemColor {
	uint8 r,g,b;
	ItemColor& operator=(const TCODColor&);
};

struct ItemLight {
	float range;
	ItemColor color;
	float patternDelay;
	const char *pattern;
};

enum AttackWieldType {
	WIELD_NONE,
	WIELD_ONE_HAND,
	WIELD_MAIN_HAND,
	WIELD_OFF_HAND,
	WIELD_TWO_HANDS };

enum AttackFlags {
	WEAPON_PROJECTILE = 1, // something that is thrown (stone, shuriken, ...)
};
struct ItemAttack {
	AttackWieldType wield;
	float minCastDelay;
	float maxCastDelay;
	float minReloadDelay;
	float maxReloadDelay;
	float minDamagesCoef;
	float maxDamagesCoef;
	int flags;
	const char *spellCasted;
};

struct ItemHeat {
	float intensity;
	float radius;
};

struct ItemContainer {
	int size;
};

struct ItemFeature {
	ItemFeatureId id;
	union {
		ItemFireEffect fireEffect;
		ItemProduce produce;
		ItemFood food;
		ItemAgeEffect ageEffect;
		ItemAttack attack;
		ItemLight light;
		ItemHeat heat;
		ItemContainer container;
	};
	static ItemFeature *getProduce(float delay, float chance, ItemType *type);
	static ItemFeature *getFireEffect(float resistance, ItemType *type);
	static ItemFeature *getAgeEffect(float delay, ItemType *type);
	static ItemFeature *getFood(int health);
	static ItemFeature *getLight(float range, const TCODColor &color, float patternDelay, const char *pattern);
	static ItemFeature *getAttack(AttackWieldType wield, float minCastDelay, float maxCastDelay,
		float minReloadDelay, float maxReloadDelay, float minDamagesCoef, float maxDamagesCoef,
		int flags, const char *spellCasted=NULL );
	static ItemFeature *getHeat(float intensity, float radius);
	static ItemFeature *getContainer(int size);

};


enum InventoryTabId {
	INV_ALL,
	INV_ARMOR,
	INV_WEAPON,
	INV_FOOD,
	INV_MISC,
	NB_INV_TABS };

// data shared by all item types
struct ItemType {
	InventoryTabId inventoryTab;
	const char *name; // name displayed to the player
	const char *onPick; // name when picked up
	TCODColor color; // color on screen
	int character; // character on screen
	int flags;
	TCODList<ItemType *> inherits;
	TCODList<ItemFeature *>features;
	TCODList<ItemActionId> actions;
	ItemFeature *getFeature(ItemFeatureId id) const;
	bool hasFeature(ItemFeatureId id) const { return getFeature(id) != NULL; }
	bool isA(const ItemType *type) const;
	bool isA(const char *typeName) const;
	bool hasAction(ItemActionId id) const;
	bool hasComponents() const;
	ItemCombination *getCombination() const;
	void computeActions();
	Item *produce(float rng) const; // for items with Produce feature(s)

	bool isIngredient() const; // in any recipe
	bool isTool() const; // in any recipe
};

class Item : public DynamicEntity {
public :
	// factories
	static Item *getItem(const char *type, float x, float y, bool createComponents=true);
	static Item *getItem(const ItemType *type, float x, float y, bool createComponents=true);
	static Item *getRandomWeapon(const char *type,ItemClass itemClass);


	static bool init();
	static ItemType *getType(const char *name);
	void destroy(int count=1);

	virtual ~Item();
	virtual void render(LightMap *lightMap, TCODImage *ground=NULL);
	virtual void renderDescription(int x, int y, bool below=true);
	virtual void renderGenericDescription(int x, int y, bool below=true, bool frame=true);
	virtual bool age(float elapsed, ItemFeature *feat=NULL); // the item gets older
	virtual bool update(float elapsed, TCOD_key_t key, TCOD_mouse_t *mouse);
	ItemFeature *getFeature(ItemFeatureId featureId) { return typeData->getFeature(featureId); }
	bool hasFeature(ItemFeatureId featureId) { return typeData->hasFeature(featureId); }

	virtual Item *putInInventory(Creature *owner, int count=0, const char *verb="pick up"); // move the item from ground to owner's inventory
	virtual void use(); // use the item (depends on the type)
	virtual void use(int dx, int dy) {} // use the item in place (static items)
	virtual Item *drop(int count=0); // move the item from it's owner inventory to the ground
	bool isEquiped();

	// add to the list, posibly stacking
	Item * addToList(TCODList<Item *> *list);
	// remove one item, possibly unstacking
	Item * removeFromList(TCODList<Item *> *list, int count=1, bool fast=true);

    void addModifier(ItemModifierId id, float value);

	bool isA(const ItemType *type) const { return typeData->isA(type); }
	bool isA(const char *typeName) const { return isA(getType(typeName)); }

	// flags checks
	bool isWalkable() const { return (typeData->flags & ITEM_NOT_WALKABLE) == 0; }
	bool isTransparent() const { return (typeData->flags & ITEM_NOT_TRANSPARENT) ==0; }
	bool isPickable() const { return (typeData->flags & ITEM_NOT_PICKABLE) == 0 ; }
	bool isStackable() const { return (typeData->flags & ITEM_STACKABLE ) != 0 && name == NULL; }
	bool isSoftStackable() const { return (typeData->flags & ITEM_SOFT_STACKABLE) != 0 && name == NULL ; }
	bool isDeletedOnUse() const { return typeData->flags & ITEM_DELETE_ON_USE; }
	bool isUsedWhenPicked() const { return typeData->flags & ITEM_USE_WHEN_PICKED; }
	bool isActivatedOnBump() const { return typeData->flags & ITEM_ACTIVATE_ON_BUMP; }

	// craft
	bool isIngredient() const { return typeData->isIngredient(); } // in any recipe
	bool isTool() const { return typeData->isTool(); }  // in any recipe
	Item *produce(float rng) { return typeData->produce(rng); } // for items with Produce feature(s)

	// containers
	Item * putInContainer(Item *it); // put this in 'it' container (NULL if no more room)
	Item * removeFromContainer(int count=1); // remove this from its container (NULL if not inside a container)
	void computeBottleName();

    // returns "A/An <item name>"
    const char *AName() const;
    // returns "a/an <item name>"
    const char *aName() const;
    // returns "The <item name>"
    const char *TheName() const;
    // returns "the <item name>"
    const char *theName() const;

    const char *getRateName(float rate) const;

	virtual bool loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip);
	virtual void saveData(uint32 chunkId, TCODZip *zip);

	static TCODColor classColor[NB_ITEM_CLASSES];

	//crafting
	static TCODList<ItemCombination*> combinations;
	static ItemCombination *getCombination(const Item *it1, const Item *it2);
	bool hasComponents() const;
	void addComponent(Item *component);
	ItemCombination *getCombination() const;


	const ItemType *typeData;
	ItemClass itemClass;
	TCODColor col;
	char *typeName;
	char *name;
	// something that can affect the name of an item created with this component
	// example : an azuran blade => an azuran knife
	char *adjective;
	bool an;
	int count;
	TCODList<ItemModifier *> modifiers;
	Creature *owner; // this is in owner's inventory
	Item *container; // this is inside container
	Creature *asCreature; // a creature corresponding to this item
	float fireResistance;
	int toDelete;
	int ch;
	TCODList<Item *> stack; // for soft stackable items or containers
	TCODList<Item *> components; // for items that can be disassembled
protected :
	friend class ItemFileListener;
	static void addFeature(const char *typeName, ItemFeature *feat);
	static TCODList<ItemType *>types;
	Item(float x,float y,const ItemType &type);
	friend class Dungeon;
	bool active;
	float life; // remaining time before aging effect turn this item into something else
	// attack feature data
	WeaponPhase phase;
	float phaseTimer;
	float castDelay;
	float reloadDelay;
	float damages;
	float cumulatedElapsed;
	int targetx,targety;
	float heatTimer; // time before next heat update (1 per second)
	bool onoff; // for doors, torchs, ... on = open/turned on, off = closed/turned off

	ExtendedLight *light;
	static TCODConsole *descCon; // offscreen console for item description

	void initLight();
	void convertTo(ItemType *type);
	void renderDescriptionFrame(int x, int y, bool below=true, bool frame=true);
	void generateComponents();
};




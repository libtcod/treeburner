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
/*
#include "main.hpp"

Weapon::Weapon(int x,int y,WeaponTypeId2 id) :
	Item(x,y,Weapon::types[id].type), nbSockets(0), phase(IDLE) {
	name=NULL;
	TCODColor wcol=Item::classColor[itemClass];
	if ( light ) {
		switch (itemClass) {
			case ITEM_CLASS_STANDARD : light->color=wcol*0.3;break;
			case ITEM_CLASS_GREEN : light->color=wcol*0.5;break;
			case ITEM_CLASS_ORANGE : light->color=wcol*0.7;break;
			case ITEM_CLASS_RED : light->color=wcol*0.8;break;
			case ITEM_CLASS_SILVER : light->color=wcol;break;
			case ITEM_CLASS_GOLD : light->color=wcol*1.2;break;
			default:break;
		}
	}
}

Weapon::~Weapon() {
	if ( name ) free(name);
}

Item *Weapon::pickup(Creature *owner) {
	Item *newItem = Item::pickup(owner);
	// auto-equip if no weapon wielded
	if (gameEngine->player.mainHand == NULL ) {
		gameEngine->player.mainHand=newItem;
	}
	return newItem;
}

bool Weapon::update(float elapsed, TCOD_key_t key, TCOD_mouse_t *mouse) {
	ItemFeature *feat=
	Item::update(elapsed,key,mouse);
	switch (phase) {
		case CAST :
			timer -= elapsed;
			if ( timer <= 0.0f && (Weapon::types[subType].flags & WEAPON_PROJECTILE) == 0 ) {
				phase=RELOAD;
				timer=reloadDelay;
				fire();
			} else {
				if ( Weapon::types[subType].flags & WEAPON_PROJECTILE ) {
					// keep targetting while the mouse button is pressed
					int dx=mouse->cx+gameEngine->xOffset;
					int dy=mouse->cy+gameEngine->yOffset;
					targetx=dx;
					targety=dy;
					if ( !mouse->lbutton ) {
						// fire when mouse button released
						timer=MAX(timer,0.0f);
						float speed=(castDelay-timer)/castDelay;
						speed=MIN(speed,1.0f);
						phase=RELOAD;
						timer=reloadDelay;
						fire(speed);
					}
				}
			}
			break;
		case RELOAD :
			timer -= elapsed;
			if ( timer <= 0.0f ) {
				phase=IDLE;
			}
			break;
		case IDLE:
			if ( owner == &gameEngine->player && mouse->lbutton && isEquiped() ) {
				timer=castDelay;
				phase=CAST;
				int dx=mouse->cx+gameEngine->xOffset;
				int dy=mouse->cy+gameEngine->yOffset;
				targetx=dx;
				targety=dy;
			}
		break;
	}
	return !active;
}

void Weapon::use() {
	if ( owner ) {
		if ( isEquiped() ) owner->unwield(this);
		else owner->wield(this);
	}
}

void Weapon::use(int dx, int dy) {
	if ( phase == IDLE ) {
		timer=castDelay;
		phase=CAST;
		targetx=dx;
		targety=dy;
	}
}

void Weapon::fire() {
	FireBall *fb=new FireBall(owner->x,owner->y,targetx,targety,FB_STANDARD);
	((Game *)gameEngine)->addFireball(fb);
	gameEngine->stats.nbSpellStandard++;
}

void Weapon::fire(float speed) {
	if ( (int)targetx == (int)owner->x && (int)targety == (int)owner->y ) return;
	x=owner->x;
	y=owner->y;
	Item *it=owner->removeFromInventory(this);
	it->dx = targetx - x;
	it->dy = targety - y;
	float l=fastInvSqrt(it->dx*it->dx+it->dy*it->dy);
	it->dx*=l;
	it->dy*=l;
	it->x = x;
	it->y = y;
	it->speed=speed*12;
	it->duration=1.5f;
	gameEngine->dungeon->addItem(it);
}

void Weapon::renderDescription(int x, int y, bool below) {
	static const char *wieldname[] = {
		NULL, "One hand", "Main hand", "Off hand", "Two hands"
	};
	static const char *ratename[] = {
		"Very slow",
		"Slow",
		"Average",
		"Fast",
		"Very fast"
	};
	int cy=0;
	descCon->clear();
	descCon->setForegroundColor(col);
	if (name) {
		descCon->print(CON_W/4,cy++,name);
		descCon->setForegroundColor(guiText);
	}
	descCon->print(CON_W/4,cy++,typeName);
	descCon->setForegroundColor(guiText);
	if ( wieldname[ Weapon::types[subType].wield ] ) {
		descCon->print(CON_W/4,cy++,wieldname[Weapon::types[subType].wield]);
	}
	float rate=1.0f / (castDelay + reloadDelay);
	int dmgPerSec = (int)(damages *rate + 0.5f);
	descCon->print(CON_W/4,cy++,"%d damages/sec", dmgPerSec);
	int rateIdx=0;
	if ( rate <= 0.5f ) rateIdx = 0;
	else if ( rate <= 1.0f ) rateIdx = 1;
	else if ( rate <= 3.0f ) rateIdx = 2;
	else if ( rate <= 5.0f ) rateIdx = 3;
	else rateIdx = 4;
	descCon->print(CON_W/4,cy++,"Attack rate:%s",ratename[rateIdx]);
	ItemModifier::renderDescription(descCon,2,cy,modifiers);
	renderDescriptionFrame(x,y,below);
}

static TextGenerator *textgen=NULL;

Weapon *Weapon::getRandom(ItemClass itemClass) {
	int typenum=rng->getInt(0,NB_ITEM_WEAPON-1);
	return Weapon::getRandom((WeaponTypeId)typenum,itemClass);
}

Weapon *Weapon::getRandom(ItemTypeId2 id,ItemClass itemClass) {
	if (! textgen ) {
		textgen=new TextGenerator("data/cfg/weapon.txg",rng);
		textgen->parseFile();
	}
	Item * weapon = new Item(-1,-1,id);
	weapon->itemClass=itemClass;
	weapon->col = Item::classColor[itemClass];
	if ( itemClass > ITEM_CLASS_STANDARD ) {
		int goatKey = 6 * gameEngine->player.school.type + id;
		weapon->typeName = strdup( textgen->generate("weapon","${%s}",
			textgen->generators.peek()->rules.get(goatKey)->name ));
	}
	ItemFeature *feat=weapon->getFeature(ITEM_FEAT_ATTACK);
	weapon->castDelay = rng->getFloat(feat->attack.minCastDelay,feat->attack.maxCastDelay);
	weapon->reloadDelay = rng->getFloat(feat->attack.minReloadDelay,feat->attack.maxReloadDelay);
	enum { MOD_RELOAD, MOD_CAST, MOD_MODIFIER };
	for (int i=0; i < itemClass; i++) {
        int modType = rng->getInt(MOD_RELOAD,MOD_MODIFIER);
        switch(modType) {
            case MOD_RELOAD :
                weapon->reloadDelay -= rng->getFloat(0.05f, 0.2f);
                weapon->reloadDelay = MAX(0.1f,weapon->reloadDelay);
            break;
            case MOD_CAST :
                weapon->castDelay -= rng->getFloat(0.05f, 0.2f);
                weapon->castDelay = MAX(0.1f,weapon->reloadDelay);
            break;
            case MOD_MODIFIER :
                ItemModifierId id=(ItemModifierId)0;
                switch (gameEngine->player.school.type ) {
                    case SCHOOL_FIRE :
                        id=(ItemModifierId)rng->getInt(ITEM_MOD_FIRE_BEGIN,ITEM_MOD_FIRE_END);
                    break;
                    case SCHOOL_WATER :
                        id=(ItemModifierId)rng->getInt(ITEM_MOD_WATER_BEGIN,ITEM_MOD_WATER_END);
                    break;
                    default:break;
                }
                weapon->addModifier(id,rng->getFloat(0.1f,0.2f));
            break;
        }
	}
	weapon->damages = 15 * (weapon->reloadDelay + weapon->castDelay ) * rng->getFloat(feat->attack.minDamagesCoef,feat->attack.maxDamagesCoef);
	weapon->damages += weapon->damages * (int)(itemClass)*0.2f; // 20% increase per color level
	weapon->damages = MIN(1.0f,weapon->damages);
    return weapon;
}

#define WEAP_CHUNK_VERSION 1

bool Weapon::loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip) {
	if ( chunkVersion != WEAP_CHUNK_VERSION ) return false;

	// load weapon specific data here

	const char *zipname=zip->getString();
	if ( zipname ) name = strdup(zipname);
	castDelay=zip->getFloat();
	reloadDelay=zip->getFloat();
	damages=zip->getFloat();
	nbSockets=zip->getInt();
	for (int i=0; i < nbSockets; i++) {
		ItemTypeId type=(ItemTypeId)zip->getInt();
		int subType=zip->getInt();
		if (type < 0 || type >= NB_ITEM_TYPES) return false;
		uint32 itemChunkId ,itemChunkVersion;
		saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
		Item *it=Item::getItem(type,subType, 0,0);
		if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
		sockets[i]=it;
	}

	saveGame.loadChunk(&chunkId,&chunkVersion);
	return Item::loadData(chunkId,chunkVersion,zip);
}

void Weapon::saveData(uint32 chunkId, TCODZip *zip) {
	saveGame.saveChunk(WEAP_CHUNK_ID,WEAP_CHUNK_VERSION);

	// save weapon specific data here
	zip->putString(name);
	zip->putFloat(castDelay);
	zip->putFloat(reloadDelay);
	zip->putFloat(damages);
	zip->putInt(nbSockets);
	for (int i=0; i < nbSockets; i++) {
		Item *socketed=sockets[i];
		zip->putInt(socketed->type);
		zip->putInt(socketed->subType);
		socketed->saveData(MISC_CHUNK_ID,zip);
	}

	Item::saveData(ITEM_CHUNK_ID,zip);
}

*/

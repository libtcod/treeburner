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
#include <stdio.h>
#include "main.hpp"

TCODList<Creature *> Creature::creatureByType[NB_CREATURE_TYPES];

TCODList<ConditionType *> ConditionType::list;

ConditionType *ConditionType::find(const char *name) {
	for ( ConditionType **it=list.begin(); it!=list.end(); it++) {
		if (strcmp((*it)->name,name)==0 ) return *it;
	}
	return NULL;
}

ConditionType *ConditionType::get(ConditionTypeId type) {
	for ( ConditionType **it=list.begin(); it!=list.end(); it++) {
		if ((*it)->type==type) return *it;
	}
	return NULL;
}

void ConditionType::init() {
	list.push(new ConditionType(STUNNED,"stunned"));
	list.push(new ConditionType(BLEED,"bleed"));
	list.push(new ConditionType(HEAL,"heal"));
	list.push(new ConditionType(POISONED,"poisoned"));
	list.push(new ConditionType(IMMUNE,"immune"));
	list.push(new ConditionType(PARALIZED,"paralized"));
	list.push(new ConditionType(CRIPPLED,"crippled"));
	list.push(new ConditionType(WOUNDED,"wounded"));
}

bool ConditionType::check(Creature *cr) {
	switch ( type ) {
		case POISONED :
			// a immune creature cannot be poisoned
			return (!cr->hasCondition(IMMUNE));
		break;
		default : return true; break;
	}
}

Condition::Condition(ConditionTypeId type, float duration, float amount, const char *alias)
		: initialDuration(duration), duration(duration), amount(amount),curAmount(0.0f), alias(alias) {
	this->type = ConditionType::get(type);
}

bool Condition::equals(ConditionTypeId type,const char *name) {
	return this->type->type == type && (name == NULL || (alias && strcmp(alias,name) == 0 ));
}

// warning : increase CREA_CHUNK_VERSION when changing this

void Condition::save(TCODZip *zip) {
	zip->putInt(type->type);
	zip->putString(alias);
	zip->putFloat(initialDuration);
	zip->putFloat(duration);
	zip->putFloat(amount);
	zip->putFloat(curAmount);
}

void Condition::load(TCODZip *zip) {
	ConditionTypeId typeId = (ConditionTypeId)zip->getInt();
	type = ConditionType::get(typeId);
	alias=zip->getString();
	if ( alias ) alias=strdup(alias);
	initialDuration=zip->getFloat();
	duration=zip->getFloat();
	amount=zip->getFloat();
	curAmount=zip->getFloat();
}


bool Condition::update(float elapsed) {
	curAmount += amount * elapsed / initialDuration;
	switch(type->type) {
		case POISONED :
		case BLEED : {
			// lose health over time
			if ( curAmount > 0 ) {
				target->takeDamage(curAmount);
				// TODO
				//GameScreen::getInstance()->addBloodStain(target->x,target->y,lostHp);
			}
			curAmount = 0;

		}
		break;
		case HEAL : {
			// gain health over time
			if ( curAmount > 0 ) {
				target->life+=curAmount;
				if ( target->life > target->maxLife ) target->life=target->maxLife;
			}
			curAmount = 0;
		}
		break;
		default:break;
	}
	duration -= elapsed;
	if ( duration <= 0.0f) {
		switch(type->type) {
			case WOUNDED : {
				// wounded decrease the max hp
				target->maxLife = target->maxLife / (1.0f-amount);
			}
			break;
			default:break;
		}
		return true;
	}
	return false;
}

void Condition::applyTo(Creature *cr) {
	cr->addCondition(this);
	switch (type->type) {
		case IMMUNE : {
			// the immune condition remove all poisoned conditions
			Condition *cond=NULL;
			do {
				cond=cr->getCondition(POISONED);
				if ( cond ) {
					cr->conditions.remove(cond);
					delete cond;
				}
			} while (cond);
		}
		break;
		case WOUNDED : {
			// wounded decrease the max hp
			float quantity=amount*cr->maxLife;
			cr->maxLife -= quantity;
			if ( cr->maxLife < 0 ) {
				cr->maxLife=0;
			}
			cr->life = MIN(cr->maxLife,cr->life);
		}
		break;
		default:break;
	}
}



Creature::Creature() : path(NULL),ignoreCreatures(true),burn(false),
	flags(0),mainHand(NULL),offHand(NULL),asItem(NULL),
	fovRange(0),walkTimer(0.0f),pathTimer(0.0f),curDmg(0.0f) {
	name[0]=0;
	talkText.text[0]=0;
	talkText.delay=0.0f;
	height=1.0f;
	toDelete=false;
	currentBehavior=NULL;
}

Creature::~Creature() {
	if ( path ) delete path;
}


void Creature::addCondition(Condition *cond) {
	conditions.push(cond);
	cond->target=this;
}

bool Creature::hasCondition(ConditionTypeId type, const char *alias) {
	return (getCondition(type,alias) != NULL);
}

float Creature::getMaxConditionDuration(ConditionTypeId type,const char *alias) {
	float maxVal=-1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->duration > maxVal) maxVal=(*it)->duration;
	}
	return maxVal;
}

float Creature::getMinConditionAmount(ConditionTypeId type,const char *alias) {
	float minVal=1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->amount < minVal) minVal=(*it)->amount;
	}
	return minVal;
}

float Creature::getMaxConditionAmount(ConditionTypeId type,const char *alias) {
	float maxVal=-1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->amount > maxVal) maxVal=(*it)->amount;
	}
	return maxVal;
}

Condition *Creature::getCondition(ConditionTypeId type, const char *alias) {
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) ) return *it;
	}
	return NULL;
}


Creature *Creature::getCreature(CreatureTypeId id) {
	Creature *ret=NULL;
	switch(id) {
		case CREATURE_DEER :
			 ret=new Creature();
			 strcpy(ret->name,"deer");
			 ret->ch='d';
			 ret->col=TCODColor::darkerYellow;
			 ret->maxLife=ret->life=10.0f;
			 ret->speed=20.0f;
			 ret->flags=CREATURE_SAVE;
			 ret->currentBehavior = new HerdBehavior(new AvoidWaterWalkPattern());
		break;
		case CREATURE_FRIEND :
			ret = new Friend();
		break;
		case CREATURE_MINION :
			ret = new Minion();
		break;
		case CREATURE_VILLAGER :
			ret = new Villager();
		break;
		case CREATURE_ARCHER :
			ret = new Archer();
		break;
		case CREATURE_ZEEPOH :
			ret = new Boss();
		break;
		case CREATURE_VILLAGE_HEAD :
			ret = new VillageHead();
		break;
		case CREATURE_FISH :
			ret = new Fish(NULL);
		break;
		default:break;
	}
	if ( ret ) {
		ret->type=id;
		creatureByType[id].push(ret);
	}
	return ret;
}

bool Creature::isInRange(int px, int py) {
	int dx=(int)(px-x);
	int dy=(int)(py-y);
	return ( ABS(dx) <= fovRange
		&& ABS(dy) <= fovRange
		&& dx*dx + dy*dy <= fovRange*fovRange );
}

bool Creature::isPlayer() {
	return this == &gameEngine->player;
}

void Creature::talk(const char *text) {
	strncpy(talkText.text,(char*)text,CREATURE_TALK_SIZE-1);
	talkText.text[CREATURE_TALK_SIZE-1]=0;
	talkText.delay=strlen(text) * 0.1f;
	talkText.delay=MAX(0.5f,talkText.delay);
	// compute text size
	char *ptr=(char*)text;
	talkText.h=1;
	talkText.w=0;
	char *end=strchr(ptr,'\n');
	while ( end ) {
		int len = end - ptr;
		if ( talkText.w < len ) talkText.w=len;
		talkText.h++;
		ptr=end+1;
		end=strchr(ptr,'\n');
	}
	if ( end ) {
		int len = end - ptr;
		if ( talkText.w < len ) talkText.w=len;
	}
}

void Creature::renderTalk() {
	int conx=(int)(x-gameEngine->xOffset);
	int cony=(int)(y-gameEngine->yOffset);
	if ( !IN_RECTANGLE(conx,cony,CON_W,CON_H) ) return; // creature out of console
	talkText.x=conx;
	talkText.y=cony - talkText.h;
	if ( talkText.y < 0 ) talkText.y = cony+1;
	gameEngine->packer->addRect(&talkText);
	TCODConsole::root->setDefaultBackground(TCODColor::lighterYellow);
	TCODConsole::root->setDefaultForeground(TCODColor::darkGrey);
	TCODConsole::root->printEx((int)talkText.x,(int)talkText.y,TCOD_BKGND_SET,TCOD_CENTER,talkText.text);
}

void Creature::render(LightMap *lightMap) {
	static int penumbraLevel=config.getIntProperty("config.gameplay.penumbraLevel");
	static int darknessLevel=config.getIntProperty("config.gameplay.darknessLevel");
	static float fireSpeed=config.getFloatProperty("config.display.fireSpeed");
	static TCODColor corpseColor=config.getColorProperty("config.display.corpseColor");
	static TCODColor lowFire(255,0,0);
	static TCODColor midFire(255,204,0);
	static TCODColor highFire(255,255,200);
	static TCODColor fire[64];
	static bool fireInit=false;
	if (! fireInit ) {
		for (int i=0; i < 32; i++) {
			fire[i]=TCODColor::lerp(lowFire,midFire,i/32.0f);
		}
		for (int i=32; i < 64; i++) {
			fire[i]=TCODColor::lerp(midFire,highFire,(i-32)/32.0f);
		}
		fireInit=true;
	}

	// position on console
	int conx=(int)(x-gameEngine->xOffset);
	int cony=(int)(y-gameEngine->yOffset);
	if ( !IN_RECTANGLE(conx,cony,CON_W,CON_H) ) return; // out of console

	float playerDist = distance(gameEngine->player);
	float apparentHeight = height / playerDist;
	if ( apparentHeight < MIN_VISIBLE_HEIGHT ) return; // too small to see at that distance

	TCODColor c;
	int displayChar=ch;
	TCODColor lightColor=lightMap->getColor(conx,cony)*1.5f;
	Dungeon *dungeon=gameEngine->dungeon;
	float shadow = dungeon->getShadow(x*2,y*2);
	float clouds = dungeon->getCloudCoef(x*2,y*2);
	shadow = MIN(shadow,clouds);
	lightColor = lightColor * shadow;
	if ( life <= 0 ) {
		ch='%';
		c=corpseColor*lightColor;
	} else if (burn ) {
		float fireX = TCODSystem::getElapsedSeconds() * fireSpeed + noiseOffset;
		int fireIdx = (int)((0.5f+0.5f*noise1d.get(&fireX))*64.0f);
		c=fire[fireIdx];
		int r=(int)(c.r*1.5f*lightColor.r/255);
		int g=(int)(c.g*1.5f*lightColor.g/255);
		int b=(int)(c.b*1.5f*lightColor.b/255);
		c.r=CLAMP(0,255,r);
		c.g=CLAMP(0,255,g);
		c.b=CLAMP(0,255,b);
	} else {
		c=col*lightColor;
	}
	int intensity = c.r+c.g+c.b;
	if (intensity < darknessLevel ) return; // creature not seen
	if (intensity < penumbraLevel ) displayChar='?';
	if ( apparentHeight < VISIBLE_HEIGHT ) displayChar = '?'; // too small to distinguish
	TCODConsole::root->setChar(conx,cony,displayChar);
	TCODConsole::root->setCharForeground(conx,cony,c);
}

void Creature::stun(float delay) {
	walkTimer=MIN(-delay,walkTimer);
}

bool Creature::walk(float elapsed) {
	walkTimer+=elapsed;
	TerrainId terrainId=gameEngine->dungeon->getTerrainType((int)x,(int)y);
	float walkTime = terrainTypes[terrainId].walkCost / speed;
	if ( walkTimer >= 0 ) {
		walkTimer=-walkTime;
		if ( path && ! path->isEmpty()) {
			int newx,newy;
			GameEngine *game=gameEngine;
			path->get(0,&newx,&newy);
			if ( (game->player.x != newx || game->player.y != newy)
				&& !game->dungeon->hasCreature(newx,newy) ) {
				int oldx=(int)x,oldy=(int)y;
				int newx=oldx,newy=oldy;
				if (path->walk(&newx,&newy,false)) {
					setPos(newx,newy);
					game->dungeon->moveCreature(this,oldx,oldy,newx,newy);
					if ( game->dungeon->hasRipples(newx,newy) ) {
						gameEngine->startRipple(newx,newy);
					}
					return true;
				}
			}
		}
	}
	return false;
}

void Creature::randomWalk(float elapsed) {
	walkTimer+=elapsed;
	if ( walkTimer >= 0 ) {
		walkTimer=-1.0f/speed;
		static int dirx[]={-1,0,1,-1,1,-1,0,1};
		static int diry[]={-1,-1,-1,0,0,1,1,1};
		int d=TCODRandom::getInstance()->getInt(0,7);
		int count=8;
		GameEngine *game=gameEngine;
		do {
			int newx=(int)(x+dirx[d]),newy=(int)(y+diry[d]);
			if (IN_RECTANGLE(newx,newy,game->dungeon->width,game->dungeon->height)
				&& game->dungeon->map->isWalkable(newx,newy)
				&& (game->player.x != newx || game->player.y != newy)
				&& !game->dungeon->hasCreature(newx,newy) ) {
				game->dungeon->moveCreature(this,(int)x,(int)y,newx,newy);
				x=newx;y=newy;
				return;
			}
			d = (d+1)%8;
			count --;
		} while (count > 0);
	}
}

float Creature::getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
	GameEngine *game=gameEngine;
	if ( !game->dungeon->map->isWalkable(xTo,yTo)) return 0.0f;
	if ( ignoreCreatures ) return 1.0f;
	if ( game->dungeon->hasCreature(xTo,yTo) ) return 50.0f;
	if ( game->player.x == xTo || game->player.y == yTo ) return 50.0f;
	return 1.0f;
}

void Creature::takeDamage(float amount) {
	curDmg+=amount;
	int idmg=(int)curDmg;
	if (idmg > 0 ) {
		if ( life > 0 && life <= idmg && this != &gameEngine->player) {
			AiDirector::instance->killCreature(this);
			gameEngine->stats.creatureDeath[type]++;
		}
		life-=idmg;
		curDmg-=idmg;
	}
}

Item * Creature::addToInventory(Item *item) {
	item = item->addToList(&inventory);
	item->owner=this;
	item->x=x;
	item->y=y;
	return item;
}

Item * Creature::removeFromInventory(Item *item, int count) {
	if ( count == 0 ) count = item->count;
	item = item->removeFromList(&inventory,count);
	if ( item == mainHand || item == offHand ) unwield(item);
	item->owner=NULL;
	return item;
}

void Creature::updateConditions(float elapsed) {
	for (Condition **it=conditions.begin(); it != conditions.end(); it++) {
		if ((*it)->update(elapsed) ) {
			delete *it;
			it=conditions.remove(it);
		}
	}
}

bool Creature::update(float elapsed) {
	static float burnDamage=config.getFloatProperty("config.creatures.burnDamage");
	static int distReplace=config.getIntProperty("config.aidirector.distReplace")*config.getIntProperty("config.aidirector.distReplace");

	if ( life <= 0 ) {
		creatureByType[type].removeFast(this);
		return false;
	}
	if (talkText.delay > 0.0f) {
		talkText.delay -= elapsed;
		if (talkText.delay < 0.0f) talkText.delay=-10.0f;
	} else if (talkText.delay < 0.0f) {
		talkText.delay += elapsed;
		if (talkText.delay > 0.0f) talkText.delay=0.0f;
	}

	updateConditions(elapsed);

	GameEngine *game=gameEngine;
	if ( isReplacable() ) {
		int pdist=(int)squaredDistance(game->player);
		if ( pdist > distReplace ) {
			AiDirector::instance->replace(this);
			return true;
		}
	}

	if ( burn ) {
		takeDamage(burnDamage*elapsed);
	}
	// update items in inventory
	for ( Item ** it=inventory.begin(); it != inventory.end(); it++) {
		if (!(*it)->age(elapsed)) {
			it=inventory.removeFast(it); // from inventory
		}
	}
	// ai
	if (currentBehavior) {
		if (!currentBehavior->update(this,elapsed))
			currentBehavior=NULL;
	}
	return life > 0;
}

void Creature::equip(Item *it) {
	ItemFeature *feat=it->getFeature(ITEM_FEAT_ATTACK);
	switch ( feat->attack.wield ) {
		case WIELD_NONE : break;
		case WIELD_MAIN_HAND :
			if ( offHand && offHand == mainHand ) offHand=NULL; // unequip two hands weapon
			mainHand=it;
		break;
		case WIELD_ONE_HAND :
			if ( ! mainHand ) mainHand=it;
			else if (! offHand ) offHand = it;
			else {
				if ( offHand == mainHand ) offHand=NULL;
				mainHand=it;
			}
		break;
		case WIELD_OFF_HAND :
			if ( mainHand && offHand == mainHand ) mainHand=NULL; // unequip two hands weapon
			offHand=it;
		break;
		case WIELD_TWO_HANDS :
			mainHand = offHand = it;
		break;
	}
}

void Creature::unequip(Item *it) {
	if ( it == mainHand ) mainHand=NULL;
	if ( it == offHand ) offHand=NULL; // might be both for two hands items
}

void Creature::wield(Item *it) {
	ItemFeature *feat=it->getFeature(ITEM_FEAT_ATTACK);
	switch ( feat->attack.wield ) {
		case WIELD_NONE :
			gameEngine->gui.log.warn("You cannot wield %s",it->aName());
			return;
		break;
		case WIELD_MAIN_HAND :
			if ( mainHand ) unwield(mainHand);
		break;
		case WIELD_OFF_HAND :
			if ( offHand ) unwield(offHand);
		break;
		case WIELD_ONE_HAND :
			if ( mainHand && offHand ) unwield(mainHand);
		break;
		case WIELD_TWO_HANDS :
			if ( mainHand ) unwield(mainHand);
			if ( offHand ) unwield(offHand);
		break;
	}
	equip(it);
	if ( isPlayer() ) {
		gameEngine->gui.log.info("You're wielding %s",it->aName());
	}
}

void Creature::unwield(Item *it) {
	unequip(it);
	if ( this == &gameEngine->player ) {
		gameEngine->gui.log.info("You were wielding %s",it->aName());
	}
}

#define CREA_CHUNK_VERSION 6
void Creature::saveData(uint32 chunkId, TCODZip *zip) {
	saveGame.saveChunk(CREA_CHUNK_ID,CREA_CHUNK_VERSION);
	zip->putFloat(x);
	zip->putFloat(y);
	zip->putFloat(life);
	zip->putString(name);
	// save inventory
	zip->putInt(inventory.size());
	for (Item **it = inventory.begin(); it != inventory.end(); it++) {
		zip->putString((*it)->typeData->name);
		(*it)->saveData(ITEM_CHUNK_ID,zip);
	}
	// save conditions
	zip->putInt(conditions.size());
	for ( Condition **it=conditions.begin(); it != conditions.end(); it++) {
		(*it)->save(zip);
	}

}

bool Creature::loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip) {
	if ( chunkVersion != CREA_CHUNK_VERSION ) return false;
	x=zip->getFloat();
	y=zip->getFloat();
	life=zip->getFloat();
	strcpy(name,zip->getString());
	// load inventory
	int nbItems = zip->getInt();
	while (nbItems > 0 ) {
		const char * itemTypeName=zip->getString();
		ItemType *itemType=Item::getType(itemTypeName);
		if (!itemType) return false;
		uint32 itemChunkId ,itemChunkVersion;
		saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
		Item *it=Item::getItem(itemType, 0,0);
		if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
		addToInventory(it);
		nbItems--;
	}
	// load conditions
	int nbConditions = zip->getInt();
	while (nbConditions > 0 ) {
		Condition *cond= new Condition();
		cond->target=this;
		cond->load(zip);
		conditions.push(cond);
		nbConditions--;
	}
	return true;
}


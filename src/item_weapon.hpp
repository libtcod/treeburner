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


#define MAX_SOCKETS 3

struct WeaponType {
	ItemType type;
	AttackWieldType wield;
	float minCastDelay;
	float maxCastDelay;
	float minReloadDelay;
	float maxReloadDelay;
	float minDamagesCoef;
	float maxDamagesCoef;
	int minSockets;
	int maxSockets;
	int flags;
};

class Creature;

class Weapon : public Item {
public :
	//static Weapon *getRandom(ItemClass itemClass);
	static Weapon *getRandom(ItemTypeId2 id,ItemClass itemClass);

	Weapon(int x,int y, WeaponTypeId2 id);
	~Weapon();

	Item *pickup(Creature *owner);
	bool update(float elapsed, TCOD_key_t key, TCOD_mouse_t *mouse);
	void renderDescription(int x, int y, bool below=true);
	void use();
	void use(int dx, int dy);

	bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip);
	void saveData(uint32_t chunkId, TCODZip *zip);

	char *name;
	float castDelay;
	float reloadDelay;
	float damages;
	float timer;
	int targetx,targety;
	int nbSockets;
	WeaponPhase phase;
	Item *sockets[MAX_SOCKETS];
protected :
	void fire();
	void fire(float speed);
};
*/

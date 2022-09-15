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
class Player;
class LightMap;

class Dungeon :public SaveListener {
public :
    Dungeon(int width, int height); // empty dungeon
	Dungeon(int level, CaveGenerator *caveGen); // bsp / cellular automate dungeon
	virtual ~Dungeon();

	// the final dungeon map
	Cell *cells; // normal resolution data
	SubCell *subcells; // subcell resolution data
	TCODMap *map; // normal resolution for pathfinding
	TCODMap *map2x; // double resolution for fovs
	TCODHeightMap *hmap; // double resolution. ground altitude
	TCODHeightMap *smap; // double resolution. shadow map
	TCODHeightMap *smapBeforeTree; // double resolution. shadow map before trees shadow
	TCODImage *canopy; // double resolution. black = transparent

	// dungeon generation parameters
	//int size;
	//int size2x; // well.. size*2
	int width, height;

	// stair to next level
	int stairx,stairy;
	TCODList <Item *> items;
	TCODList <Creature *> creatures;
	TCODList <Creature *> corpses;
	TCODList <Light *> lights;

	// fov
	void computeFov(int x, int y);
	bool hasLos(int xFrom, int yFrom, int xTo,int yTo, bool ignoreCreatures) const;
	inline bool isCellInFov(float x, float y) { return map->isInFov((int)x,(int)y); }

	// creatures
	bool hasCreature(int x,int y) const;
	Creature *getCreature(int x,int y) const;
	Creature *getCreature(CreatureTypeId id) const;
	Creature *getCreature(const char *name) const;
	void addCreature(Creature *cr);
	void addCorpse(Creature *cr);
	void moveCreature(Creature *cr,int xFrom,int yFrom, int xTo, int yTo);
	void removeCreature(Creature *cr, bool kill=true);
	void renderCreatures(LightMap *lightMap);
	void renderSubcellCreatures(LightMap *lightMap);
	void renderCorpses(LightMap *lightMap);
	void computeSpawnSources();
	void getClosestSpawnSource(float x,float y, int *ssx, int *ssy) const {
		return getClosestSpawnSource((int)x,(int)y,ssx,ssy);
	}
	void getClosestSpawnSource(int x,int y, int *ssx, int *ssy) const;
	void updateCreatures(float elapsed);
	void killCreaturesAtRange(int radius);
	void setPlayerStartingPosition();

	// items
	bool hasItem(int x,int y) const;
	bool hasActivableItem(int x, int y) const;
	bool hasItemType(int x, int y, const char *typeName);
	bool hasItemType(int x, int y, const ItemType *type);
	bool hasItemFlag(int x, int y, int flag);
	//bool hasItemTag(int x, int y, unsigned long long tag);
	TCODList<Item *> *getItems(int x, int y) const;
	Item *getFirstItem(int x, int y) const;
	//Item *getItemTag(int x, int y, unsigned long long tag);
	Item *getItem(int x, int y, const ItemType *type);
	Item *getItem(int x, int y, const char *typeName);
	void addItem(Item *it);
	Item *removeItem(Item *it, int count=1, bool del=true);
	void renderItems(LightMap *lightMap, TCODImage *ground=NULL);
	void updateItems(float elapsed, TCOD_key_t k,TCOD_mouse_t *mouse);
	void computeWalkTransp(int x, int y);

	// lights
	inline void setAmbient(const TCODColor &col) { ambient=col; }
	inline const TCODColor & getAmbient() { return ambient; }
	inline void addLight(Light *light) { lights.push(light); }
	inline void removeLight(Light *light) { lights.removeFast(light); }
	void renderLightsToLightMap(LightMap *lightMap, int *minx = NULL, int *miny= NULL, int *maxx= NULL, int *maxy= NULL, bool clearMap=false);
	void renderLightsToImage(TCODImage *img, int *minx = NULL, int *miny= NULL, int *maxx= NULL, int *maxy= NULL);
	void updateLights(float elapsed);
	void computeOutdoorLight(float lightDir[3], TCODColor lightColor);
	inline void setShadow(int x2, int y2, float val) { subcells[x2+y2*width*2].shadow=val; }
	inline float getShadow(float x2, float y2) const { return getShadow((int)x2,(int)y2); }
	inline float getShadow(int x2, int y2) const { return subcells[x2+y2*width*2].shadow; }
	inline void setShadowHeight(int x2, int y2, float val) { smap->setValue(x2,y2,val); }
	inline float getShadowHeight(float x2, float y2) const { return getShadowHeight((int)x2,(int)y2); }
	inline float getShadowHeight(int x2, int y2) const { return smap->getValue(x2,y2); }
	void smoothShadow();
	void applyShadowMap();
	void saveShadowBeforeTree();
	void restoreShadowBeforeTree();
	inline void updateClouds(float elapsed) { clouds->update(elapsed); }
	inline float getInterpolatedCloudCoef(int x2,int y2) const { return clouds ? clouds->getInterpolatedThickness(x2,y2) : 1.0f;}
	inline float getCloudCoef(float x2,float y2) const { return getCloudCoef((int)x2,(int)y2); }
	inline float getCloudCoef(int x2,int y2) const { return clouds ? clouds->getThickness(x2,y2) : 1.0f;}

	// ground
	inline Cell *getCell(int x, int y) const { return &cells[x+y*width]; }
	inline Cell *getCell(float x, float y) const { return &cells[(int)(x)+(int)(y)*width]; }
	inline SubCell *getSubCell(int x2, int y2) const { return &subcells[x2+y2*width*2]; }
	void setWalkable(int x, int y, bool walkable);
	inline float isCellTransparent(float x, float y) { return map->isTransparent((int)x,(int)y); }
	inline float isCellWalkable(float x, float y) { return map->isWalkable((int)x,(int)y); }
	inline float getWaterCoef(int x2, int y2) const { return getSubCell(x2,y2)->waterCoef; }
	void setProperties(int x, int y, bool transparent, bool walkable);
	inline void setTerrainType(int x, int y,TerrainId id) { cells[x+y*width].terrain=id; setWalkable(x,y,terrainTypes[id].walkable||terrainTypes[id].swimmable);}
	inline TerrainId getTerrainType(int x, int y) const { return cells[x+y*width].terrain;}
	inline TCODColor getGroundColor(int x2, int y2) const { return subcells[x2+y2*width*2].groundColor; }
	TCODColor getShadedGroundColor(int x2, int y2) const;
	void getClosestWalkable(int *x, int *y, bool includingStairs=true, bool includingCreatures=true, bool includingWater=true) const;
	inline bool hasRipples(float x,float y) const { return hasRipples((int)x,(int)y); }
	inline bool hasRipples(int x,int y) const { return terrainTypes[getTerrainType(x,y)].ripples; }
	inline void setGroundColor(int x2, int y2, const TCODColor & col) { subcells[x2+y2*width*2].groundColor=col; }
	inline bool hasCanopy(int x2, int y2) const { return canopy->getPixel(x2,y2).r != 0; }
	inline bool hasWater(int x2, int y2) const { return getWaterCoef(x2,y2) > 0.0f; }

	// player memory
	inline bool getMemory(float x, float y) const {return getMemory((int)x,(int)y); }
	inline bool getMemory(int x, int y) const { return cells[x+y*width].memory; }
	void setMemory(int x, int y);

	// apply blur to ground bitmap
	void finalizeMap(bool roundCorners=true, bool blurGround=true);

	// SaveListener
	bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip);
	void saveData(uint32_t chunkId, TCODZip *zip);
protected :
	int level;
	TCODList<int> spawnSources;
	TCODList<Item *> itemsToAdd;
	bool isUpdatingItems;
	TCODList<Creature *> creaturesToAdd;
	bool isUpdatingCreatures;
	TCODColor ambient; // ambient light
	CloudBox *clouds; // for outdoors

    void initData(CaveGenerator *caveGen);
    void cleanData();
	void getRandomPositionInCorner(int cornerx,int cornery, int *x, int *y);
	void saveMap(int playerX, int playerY);
};

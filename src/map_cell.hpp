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


// Terrain system adapted from Umbrarum Regnum Tech Demo 1
struct TerrainType {
    const char * name; //terrain type name
    TCODColor color; //background colour
    bool walkable; //is it walkable?
    bool swimmable; //is it swimmable?
	bool ripples; // generates ripples
    float walkCost; //cost of moving through it (either walking or swimming)
};


enum TerrainId {
    TERRAIN_SWAMP_DEEP,
    TERRAIN_SWAMP_SHALLOW,
    TERRAIN_MORASS,
    TERRAIN_SCHWINGMOOR,
    TERRAIN_MIRE_LOW,
    TERRAIN_MIRE_RAISED,
    TERRAIN_GRASS_LUSH,
    TERRAIN_GRASS_NORMAL,
    TERRAIN_GRASS_SPARSE,
    TERRAIN_GRASS_WITHERED,
    TERRAIN_GRASS_DRIED,
    TERRAIN_GROUND,
    TERRAIN_DIRT,
    TERRAIN_DIRT_HARD,
    TERRAIN_ROCK_DIRTY,
    TERRAIN_ROCK_BARE,
    TERRAIN_GRANITE_SMOOTH,
    TERRAIN_GRANITE,
    TERRAIN_GRANITE_ROUGH,
    TERRAIN_GROUND_FROZEN,
    TERRAIN_SNOW_SHALLOW,
    TERRAIN_SNOW_NORMAL,
    TERRAIN_SNOW_DEEP,
    TERRAIN_SHALLOW_WATER,
    TERRAIN_DEEP_WATER,
	TERRAIN_WOODEN_FLOOR,
    NB_TERRAINS
};

extern TerrainType terrainTypes[NB_TERRAINS];
class Building;
struct Cell : public Persistant {
	Cell() : nbCreatures(0),hasCorpse(false),memory(false),terrain(TERRAIN_GROUND) {}
	virtual ~Cell() {}
	int nbCreatures;
	TCODList<Item *>items;
	bool hasCorpse;
	// cells already seen by the player
	bool memory;
	TerrainId terrain;
	Building *building; // if cell is inside a building
	
	// SaveListener	
	bool loadData(TCODZip *zip);
	void saveData(TCODZip *zip);
};

struct SubCell : public Persistant {
	SubCell() : shadow(1.0f) {}
	virtual ~SubCell() {}
	TCODColor groundColor;
	// for outdoors, shadow casted by the sun
	float shadowBeforeTree;
	float shadow;
	float waterCoef;

	// SaveListener	
	bool loadData(TCODZip *zip);
	void saveData(TCODZip *zip);
};


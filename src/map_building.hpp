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

enum BuildingMapId {
	BUILDING_NONE,
	BUILDING_FLOOR,
	BUILDING_WALL_N,
	BUILDING_WALL_E,
	BUILDING_WALL_S,
	BUILDING_WALL_W,
	BUILDING_WALL_NW,
	BUILDING_WALL_NE,
	BUILDING_WALL_SE,
	BUILDING_WALL_SW,
	BUILDING_DOOR,
	BUILDING_WINDOW_H,
	BUILDING_WINDOW_V,
	BUILDING_ITEM,
};

class Building : public Rect {
public :
	int doorx,doory; // in building local coordinates
	int *map; // a map of BuildingMapId representing the building

	static Building *generate(int width, int height, int nbRooms, TCODRandom *rng);
	static Building *generateWallsOnly(int width, int height, int nbRooms, TCODRandom *rng);
	void applyTo(Dungeon *dungeon, int dungeonDoorx, int dungeonDoory, bool cityWalls=false);
	void setHuntingHide(Dungeon *dungeon);
	static void buildCityWalls(int x, Dungeon *dungeon);
	void collapseRoof();
protected :
	Building(int w, int h);
	void buildExternalWalls();
	void placeRandomDoor(TCODRandom *rng);
	void placeRandomWindow(TCODRandom *rng);
	bool getFreeFloor(int *fx, int *fy);
	static void setBuildingWallCell(int x, int y,int ysym, int ch, Dungeon *dungeon);
};

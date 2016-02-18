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

#include "main.hpp"

TerrainType terrainTypes[NB_TERRAINS] = {
    {"deep swamp water",TCODColor(44,74,62),false,true,true,5.0f},
    {"shallow swamp water",TCODColor(49,82,68),true,false,true,3.0f},
    {"morass",TCODColor(51,98,49),true,false,false,2.0f},
    {"schwingmoor",TCODColor(68,92,41),true,false,false,1.5f},
    {"mire",TCODColor(69,74,34),true,false,false,1.25f},
    {"raised mire",TCODColor(57,60,28),true,false,false,1.25f},
    {"lush grass",TCODColor(0,144,0),true,false,false,1.1f},
    {"grass",TCODColor(40,117,0),true,false,false,1.0f},
    {"sparse grass",TCODColor(69,125,0),true,false,false,1.0f},
    {"withered grass",TCODColor(110,125,0),true,false,false,1.0f},
    {"dried grass",TCODColor(150,143,92),true,false,false,1.0f},
    {"bare ground",TCODColor(133,115,71),true,false,false,1.0f},
    {"dirt",TCODColor(111,100,73),true,false,false,1.0f},
    {"tough dirt",TCODColor(115,104,83),true,false,false,1.0f},
    {"dirt-covered rock",TCODColor(127,118,98),true,false,false,1.0f},
    {"bare rock",TCODColor(133,126,110),true,false,false,1.0f},
    {"smooth granite floor",TCODColor(105,100,90),true,false,false,1.0f},
    {"granite floor",TCODColor(95,90,80),true,false,false,1.0f},
    {"rough granite floor",TCODColor(85,80,70),true,false,false,1.0f},
    {"frozen ground",TCODColor(163,153,131),true,false,false,1.3f},
    {"shallow snow",TCODColor(218,223,232),true,false,false,1.25f},
    {"snow",TCODColor(229,233,242),true,false,false,1.5f},
    {"deep snow",TCODColor(238,242,248),true,false,false,2.0f},
    {"shallow water",TCODColor(102,153,255),true,false,true,1.7f},
    {"deep water",TCODColor(10,102,255),false,true,true,2.5f},
    {"wooden floor",TCODColor::darkerAmber,true,false,false,1.0f},
};

bool Cell::loadData(TCODZip *zip) {
	memory = zip->getChar()==1;
	terrain = (TerrainId) zip->getInt();
	return true;	
}

void Cell::saveData(TCODZip *zip) {
	zip->putChar(memory ? 1:0);
	zip->putInt(terrain);
}

bool SubCell::loadData(TCODZip *zip) {
	groundColor = zip->getColor();
	shadow = zip->getFloat();
	waterCoef = zip->getFloat();	
	return true;	
}

void SubCell::saveData(TCODZip *zip) {
	zip->putColor(&groundColor);
	zip->putFloat(shadow);
	zip->putFloat(waterCoef);	
}


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

#include "main.hpp"

// dig a rectangular room
void MapCarver::room(TCODMap *map, int x, int y, int w, int h) {
    for (int cx=x; cx < x+w; cx++ ) {
        for (int cy=y; cy < y+h; cy++ ) {
            map->setProperties(cx,cy,true,true);
        }
    }
}

// draw a vertical line
void MapCarver::vline(TCODMap *map,int x, int y1, int y2) {
	int y=y1;
	int dy=(y1>y2?-1:1);
	map->setProperties(x,y,true,true);
	if ( y1 == y2 ) return;
	do {
		y+=dy;
		map->setProperties(x,y,true,true);
		if ( x > 1 ) map->setProperties(x-1,y,true,true);
		else map->setProperties(x+1,y,true,true);
	} while (y!=y2);
}


// draw a vertical line up until we reach an empty space
void MapCarver::vlineUp(TCODMap *map,int x, int y) {
	while (y >= 0 && !map->isTransparent(x,y)) {
		map->setProperties(x,y,true,true);
		if ( x > 1 ) map->setProperties(x-1,y,true,true);
		else map->setProperties(x+1,y,true,true);
		y--;
	}
}

// draw a vertical line down until we reach an empty space
void MapCarver::vlineDown(TCODMap *map,int x, int y) {
	while (y < map->getHeight() && !map->isTransparent(x,y)) {
		map->setProperties(x,y,true,true);
		if ( x > 1 ) map->setProperties(x-1,y,true,true);
		else map->setProperties(x+1,y,true,true);
		y++;
	}
}

// draw a horizontal line
void MapCarver::hline(TCODMap *map,int x1, int y, int x2) {
	int x=x1;
	int dx=(x1>x2?-1:1);
	map->setProperties(x,y,true,true);
	if ( y > 1 ) map->setProperties(x,y-1,true,true);
	else map->setProperties(x,y+1,true,true);
	if ( x1 == x2 ) return;
	do {
		x+=dx;
		map->setProperties(x,y,true,true);
		if ( y > 1 ) map->setProperties(x,y-1,true,true);
		else map->setProperties(x,y+1,true,true);
	} while (x!=x2);
}

// draw a horizontal line left until we reach an empty space
void MapCarver::hlineLeft(TCODMap *map,int x, int y) {
	while (x >= 0 && !map->isTransparent(x,y)) {
		map->setProperties(x,y,true,true);
		if ( y > 1 ) map->setProperties(x,y-1,true,true);
		else map->setProperties(x,y+1,true,true);
		x--;
	}
}

// draw a horizontal line right until we reach an empty space
void MapCarver::hlineRight(TCODMap *map,int x, int y) {
	while (x < map->getWidth() && !map->isTransparent(x,y)) {
		map->setProperties(x,y,true,true);
		if ( y > 1 ) map->setProperties(x,y-1,true,true);
		else map->setProperties(x,y+1,true,true);
		x++;
	}
}


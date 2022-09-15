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

// cells value :
// 1 : wall
// 0 : ground

CellularAutomata::CellularAutomata(int w, int h,int per) : w(w),h(h),minx(0),miny(0),maxx(w-1),maxy(h-1) {
	data = new uint8_t[w*h];
	randomize(per);
}

CellularAutomata::CellularAutomata(CellularAutomata *c1) : w(c1->w),h(c1->h),minx(0),miny(0),maxx(w-1),maxy(h-1) {
	data = new uint8_t[w*h];
	int px,py;
	for (px = 0; px < w; px++) {
		for (py = 0; py < h; py++) {
			int offset=px+py*w;
			data[offset] = c1->data[offset];
		}
	}
}

CellularAutomata::CellularAutomata(TCODMap *map) : w(map->getWidth()),h(map->getHeight()),minx(0),miny(0),maxx(w-1),maxy(h-1) {
	data = new uint8_t[w*h];
	int px,py;
	for (px = 0; px < w; px++) {
		for (py = 0; py < h; py++) {
			if ( map->isWalkable(px,py) ) data[px+py*w] = 0 ;
			else data[px+py*w] = 1;
		}
	}
}

void CellularAutomata::seal() {
	for (int px = 0; px < w; px++) {
		data[px]=1;
		data[px+(h-1)*w]=1;
	}
	for (int py = 0; py < h; py++) {
		data[py*w]=1;
		data[w-1+py*w]=1;
	}
}

void CellularAutomata::apply(TCODMap *map) {
	int px,py;
	for (px = minx; px <= maxx; px++) {
		for (py = miny; py <= maxx; py++) {
			if ( data[px+py*w] ) map->setProperties(px,py,false,false);
			else map->setProperties(px,py,true,true);
		}
	}
}

CellularAutomata::~CellularAutomata(){
	delete [] data;
}

void CellularAutomata::randomize(int per) {
	int px,py;
	for (px = minx; px <= maxx; px++) {
		for (py = miny; py <= maxx; py++) {
			if ( rng->getInt(0,100) < per ) data[px+py*w] = 1;
			else data[px+py*w] = 0;
		}
	}
	for (px = minx; px <= maxx; px++) {
		data[px]=1;
		data[px+w*(h-1)]=1;
	}
	for (py = miny; py <= maxx; py++) {
		data[py*w]=1;
		data[w-1+w*py]=1;
	}
}

void CellularAutomata::generate(CAFunc func, int nbLoops, void *userData) {
	uint8_t *data2=new uint8_t[w*h];
	memcpy(data2,data,sizeof(uint8_t)*w*h);
	for (int l=0; l < nbLoops; l++) {
		for (int px = minx+1; px < maxx; px++) {
			for (int py = miny+1; py < maxy; py++) {
				if ( (this->*func)(px,py,userData) ) data2[px+py*w]=1;
				else data2[px+py*w]=0;
			}
		}
		memcpy(data,data2,sizeof(uint8_t)*w*h);
	}
	delete [] data2;
}

// number of walls around x,y
int CellularAutomata::count(int x, int y, int range) {
	int c=0;
	int px,py;
	int pminx,pmaxx,pminy,pmaxy;
	pminx=x-range;
	pminy=y-range;
	pmaxx=x+range;
	pmaxy=y+range;
	// count in map walls
	for (px=pminx; px <= pmaxx; px ++) {
		for (py=pminy; py <= pmaxy; py++) {
			if ( IN_RECTANGLE(px,py,w,h ) ) {
				if (px!=x || py!=y) c+=data[px+py*w];
			} else c++;
		}
	}
	return c;
}

void CellularAutomata::connect() {
	// find an empty cell
	int i=0;
	while (i < w*h && data[i]) i++;
	if ( i==w*h ) return; // no empty cell
	while (true) {
		// floodfill from this empty cell with value 2
		TCODList<int> cells;
		cells.push(i);
		while ( ! cells.isEmpty() ) {
			int j=cells.pop();
			data[j]=2;
			// west cell
			if ( (j % w) > 0 && data[j-1]==0 ) cells.push(j-1);
			// east cell
			if ( (j % w) < w-1 && data[j+1]==0 ) cells.push(j+1);
			// north cell
			if ( j >= w && data[j-w]==0 ) cells.push(j-w);
			// south cell
			if ( j < w*(h-1) && data[j+w]==0 ) cells.push(j+w);
			// north-west cell
			if ( (j % w) > 0 && j >= w && data[j-w-1]==0 ) cells.push(j-w-1);
			// south-west cell
			if ( j < w*(h-1) && (j % w) > 0 && data[j+w-1]==0 ) cells.push(j+w-1);
			// north-east cell
			if ( (j % w) < w-1 && j >= w && data[j-w+1]==0 ) cells.push(j-w+1);
			// south-east cell
			if ( j < w*(h-1) && (j % w) < w-1 && data[j+w+1]==0 ) cells.push(j+w+1);
		}
		while (i < w*h && data[i]) i++;
		if ( i==w*h ) break; // no empty cell
		// find the nearest 2 cell
		int mindist=99999;
		int bx=0,by=0;
		int cx=i%w;
		int cy=i/w;
		for (int x=0; x < w; x++) {
			for (int y=0; y < h; y++) {
				if ( data[x+y*w]==2 ) {
					int dist=SQRDIST(x,y,cx,cy);
					if ( dist < mindist ) {
						mindist=dist;
						bx=x;
						by=y;
					}
				}
			}
		}
		// link cx,cy to bx,by
		while ( data[cx+cy*w] != 2 && cx > bx ) {
			data[cx+cy*w]=2; cx--;
		}
		while ( data[cx+cy*w] != 2 && cx < bx ) {
			data[cx+cy*w]=2; cx++;
		}
		while ( data[cx+cy*w] != 2 && cy < by ) {
			data[cx+cy*w]=2; cy++;
		}
		while ( data[cx+cy*w] != 2 && cy > by ) {
			data[cx+cy*w]=2; cy--;
		}
	}
	// replace all 2 by 0
	i=0;
	while (i < w*h) {
		if (data[i]==2 ) data[i]=0;
		i++;
	}
}

bool CellularAutomata::CAFunc_cave(int x, int y, void *userData) {
	return count(x,y,1)>=5 || count(x,y,2) <= 2;
}

bool CellularAutomata::CAFunc_cave2(int x, int y, void *userData) {
	return count(x,y,1)>=5;
}

bool CellularAutomata::CAFunc_removeInnerWalls(int x, int y, void *userData) {
	return data[x+y*w] && count(x,y,2) < 24;
}

bool CellularAutomata::CAFunc_roundCorners(int x, int y, void *userData) {
	int cnt=count(x,y,1);
	if ( cnt == 5 ) return 1;
	if ( cnt == 3 ) return 0;
	return data[x+y*w];
}

bool CellularAutomata::CAFunc_cleanIsolatedWalls(int x, int y, void *userData) {
	return data[x+y*w] && count(x,y,1) >= 2;
}

bool CellularAutomata::CAFunc_dig(int x, int y, void *userData) {
	return ( data[x+y*w] // empty cells stays empty
			&& count(x,y,1) == 8);// rocks adjacent to empty gets empty
}

void CellularAutomata::setRange(int minx,int miny, int maxx,int maxy) {
	this->minx=MAX(0,minx);
	this->miny=MAX(0,miny);
	this->maxx=MIN(w-1,maxx);
	this->maxy=MIN(h-1,maxy);
}

CellularAutomata::CellularAutomata(CellularAutomata *c1, CellularAutomata *c2, float morphCoef) {
	w=c1->w;
	h=c1->h;
	minx=miny=0;
	maxx=w-1;maxy=h-1;
	data = new uint8_t[w*h];
	memset(data,0,w*h);
	for (int x=0; x < w; x ++) {
		for (int y=0; y < h; y++) {
			data[x+y*w] = (uint8_t)(morphCoef * (c1->data[x+y*w]*8)
						+ (1.0-morphCoef)*(c2->data[x+y*w]*8));
		}
	}
	uint8_t *data2=new uint8_t[w*h];
	memset(data2,0,w*h);
	for (int x=0; x < w; x ++) {
		for (int y=0; y < h; y++) {
			if ( ((int)data[x+y*w] )* 5 + count(x,y,1)/2 >= 40 ) data2[x+y*w]=1;
		}
	}
	memcpy(data,data2,w*h);
	delete[] data2;
}

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

class CellularAutomata {
public :
	typedef bool (CellularAutomata::*CAFunc)(int x, int y, void *userData);

	CellularAutomata(int w, int h,int per);
	CellularAutomata(TCODMap *map);
	CellularAutomata(CellularAutomata *c1);
	CellularAutomata(CellularAutomata *c1, CellularAutomata *c2, float morphCoef);
	virtual ~CellularAutomata();
	// per % of cells are empty
	void randomize(int per);
	void generate(CAFunc func, int nbLoops, void *userData=NULL);
	// number of active cells at given range
	int count(int x, int y, int range);
	void apply(TCODMap *map);
	// connect isolated room with corridors
	void connect();
	// make sure there are walls on map borders
	void seal();
	void setRange(int minx,int miny, int maxx,int maxy);
	//CA functions
	bool CAFunc_cave(int x, int y, void *userData);
	bool CAFunc_cave2(int x, int y, void *userData);
	bool CAFunc_dig(int x, int y, void *userData);
	bool CAFunc_roundCorners(int x, int y, void *userData);
	bool CAFunc_removeInnerWalls(int x, int y, void *userData);
	bool CAFunc_cleanIsolatedWalls(int x, int y, void *userData);
private :
	int w,h;
	int minx,miny,maxx,maxy;
	uint8_t *data;
};

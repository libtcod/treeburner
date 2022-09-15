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

// a position in the world
class Entity {
public :
	enum Direction { NONE=0,
		UP, DOWN,
		NORTH, SOUTH, EAST, WEST,
		NE, NW, SE, SW };

	float x,y;

	Entity() : x(0),y(0) {}
	Entity(int px,int py) : x(px),y(py) {}
	Entity(float px,float py) : x(px),y(py) {}
	// get subcell coordinates
	int getSubX() const { return (int)(x*2); }
	int getSubY() const { return (int)(y*2); }
	void setPos(int x, int y) { this->x=x; this->y=y; }
	void setPos(float x, float y) { this->x=x; this->y=y; }
	Entity &addDir(Direction d) {
		static int xdirs[11] = { 0, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1};
		static int ydirs[11] = { 0, 0, 0, -1, 1, 0, 0, -1, -1, 1, 1};
		x += xdirs[d];
		y += ydirs[d];
		return *this;
	}
	static Direction movementToDir(int xFrom, int yFrom, int xTo, int yTo) {
		static Direction dirs[3][3]={
			{NW,NORTH,NE},
			{WEST,NONE,EAST},
			{SW,SOUTH,SE}
		};
		return dirs[yTo-yFrom+1][xTo-xFrom+1];
	}
	float squaredDistance(const Entity &p) const {
		return (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y);
	}
	bool isOnScreen() const;
	float distance(const Entity &p) const;
	float fastInvDistance(const Entity &p) const {return fastInvSqrt(squaredDistance(p));}
	static float fastInvSqrt(float n);

};

// a rectangular zone in the world
class Rect : public Entity {
public :
	int w,h;

	Rect(): Entity(),w(0),h(0){}
	Rect(int x,int y,int pw, int ph) : Entity((float)x,(float)y),w(pw),h(ph) {}
	Rect(float x,float y,int pw, int ph) : Entity(x,y),w(pw),h(ph) {}
	Rect(Rect *r) : Entity(r->x,r->y),w(r->w),h(r->h) {}
	bool pointInside(float px, float py) const {
		return (px >= x && py >= y && px < x+w && py < y+h);
	}
	bool pointInside(const Entity & pt) const {
		return pointInside(pt.x,pt.y);
	}
	bool isIntersecting(const Rect &r) const {
		return !(r.x > x+w || r.x+r.w < x || r.y > y+h || r.y+r.h < y);
	}
	// smallest rectangle containing this and r
	void merge(const Rect &r) {
		float minx = MIN(x,r.x);
		float maxx = MAX(x+w,r.x+r.w);
		float miny = MIN(y,r.y);
		float maxy = MAX(y+h,r.y+r.h);
		x = minx;
		w = (int)(maxx-minx);
		y = miny;
		h = (int)(maxy-miny);
	}
	// intersection of this and r
	void intersect(const Rect &r) {
		float minx = MAX(x,r.x);
		float maxx = MIN(x+w,r.x+r.w);
		float miny = MAX(y,r.y);
		float maxy = MIN(y+h,r.y+r.h);
		x=minx;
		y=miny;
		w=(int)(maxx-minx);
		h=(int)(maxy-miny);
	}

};

// entity with dynamic pos and speed
class DynamicEntity : public Entity {
public :
	float dx,dy; // movement direction (unit vector)
	float duration,speed; // movement duration in second, speed in cells/sec
	DynamicEntity() : speed(0.0f) {}
};

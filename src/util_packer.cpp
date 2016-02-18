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

#include <stdio.h>
#include "main.hpp"

//#define PACKER_DBG

Packer::Packer(int x, int y, int w, int h)
	: Rect(x,y,w,h),minWidth(1),minHeight(1),
	leftPadding(0),rightPadding(0),topPadding(0),bottomPadding(0) {
	clear();
}

void Packer::setMinimumSize(int minWidth, int minHeight) {
	this->minWidth=minWidth;
	this->minHeight=minHeight;
}

void Packer::setPadding(int leftPadding,int rightPadding,int topPadding,int bottomPadding) {
	this->leftPadding=leftPadding;
	this->rightPadding=rightPadding;
	this->topPadding=topPadding;
	this->bottomPadding=bottomPadding;
}

void Packer::clear() {
	rects.clear();
	empty.clear();
	Rect r((int)x,(int)y,w,h);
	empty.push(r);
}

bool Packer::findEmptyPlace(Rect *rect) {
	float bestdist=w*w*h*h;
	float bestdx=0,bestdy=0;
	bool found=false;

	for (Rect *r=empty.begin(); r != empty.end(); r++) {
		if ( rect->x >= r->x && rect->y >= r->y && rect->x+rect->w <= r->x+r->w && rect->y+rect->h <= r->y+r->h ) {
			return true; // rect is inside an empty rect
		}
		// check if rect can fit inside r
		if (rect->w > r->w ) continue;
		if (rect->h > r->h ) continue;
		// get the minimum movement to put rect inside r
		float xmove=0,ymove=0;
		if ( rect->x < r->x ) xmove = r->x-rect->x;
		else if ( rect->x + rect->w > r->x + r->w ) xmove = r->x+r->w-rect->x-rect->w;
		if ( rect->y < r->y ) ymove = r->y-rect->y;
		else if ( rect->y + rect->h > r->y + r->h ) ymove = r->y+r->h-rect->y-rect->h;
		float dist=xmove*xmove+ymove*ymove;
		if ( dist < bestdist ) {
			bestdist=dist;
			bestdx=xmove;
			bestdy=ymove;
			found=true;
		}
	}
	rect->x += bestdx;
	rect->y += bestdy;
	return found;
}

bool Packer::addRect(Rect *prect, bool fill) {
	Rect rect=*prect;
	rect.x-=leftPadding;
	rect.y-=topPadding;
	rect.w+=leftPadding+rightPadding;
	rect.h+=topPadding+bottomPadding;
	// move the rectangle if its not inside the packer
	if (rect.x < x) rect.x = x;
	else if ( rect.x+rect.w > x + w ) rect.x = x + w - rect.w;
	if (rect.y < y) rect.y = y;
	else if ( rect.y+rect.h > y + h ) rect.y = y + h - rect.h;

	// move rect inside an empty rectangle
#ifdef PACKER_DBG
/*
	char **txt=(char **)(((char *)prect)+6*sizeof(int));
	if ( *txt == NULL ) printf ("hero :");
	else {
		printf ("%s :",*txt);
	}
*/
	printf ("%dx%d-%dx%d =>",(int)rect.x,(int)rect.y,rect.w,rect.h);
#endif
	if (!findEmptyPlace(&rect)) return false; // no place
#ifdef PACKER_DBG
	if ( rect.x != prect->x || rect.y != prect->y ) printf ("%dx%d-%dx%d\n",(int)rect.x,(int)rect.y,rect.w,rect.h);
	else printf (" /\n");
#endif
	prect->x = rect.x+leftPadding;
	prect->y = rect.y+topPadding;
	if (fill) addRectInternal(prect,false);
	return true;
}

void Packer::merge(TCODList<Rect> &list) {
	for (Rect *r1=list.begin(); r1 != list.end(); r1++) {
		for (Rect *r2=r1+1; r2 != list.end(); r2++) {
			if ( r1->x == r2->x && r1->y == r2->y && r1->w == r2->w && r1->h == r2->h ) {
				// r1 and r2 identical
				r2=list.removeFast(r2);
				continue; 
			} else if ( r2->x >= r1->x && r2->y >= r1->y && r2->x+r2->w <= r1->x+r1->w && r2->y+r2->h <= r1->y+r1->h ) {
				// r2 inside r1
				r2=list.removeFast(r2);
				continue;
			} else if ( r2->x == r1->x && r2->x+r2->w == r1->x+r1->w 
				&& (r2->y == r1->y + r1->h || r2->y+r2->h == r1->y )) {
				// vertical merge
				r1->h += r2->h;
				if ( r2->y < r1->y ) {
					r1->y=r2->y;
				}
				r2 = list.removeFast(r2);
				continue;
			} else if ( r2->y == r1->y && r2->y+r2->h == r1->y+r1->h 
				&& (r2->x == r1->x + r1->w || r2->x+r2->w == r1->x ) ){
				// horizontal merge
				r1->w += r2->w;
				if ( r2->x < r1->x ) {
					r1->x=r2->x;
				}
				r2 = list.removeFast(r2);
				continue;
			}
		}
	}
}

void Packer::addRectInternal(Rect *prect, bool duplicate) {
	Rect rect = *prect;
	/*
	if ( duplicate ) rect=new Rect(*prect);
	else rect=prect;
	*/
	rects.push(rect);
	TCODList<Rect> toAdd;
	for (Rect *r=empty.begin(); r != empty.end(); r++) {
		if ( r->isIntersecting(rect) ) {
			if ( ! ( prect->x <= r->x && prect->y <= r->y && prect->x+prect->w >= r->x+r->w && prect->y+prect->h >= r->y+r->h ) ) {
				// empty rectangle not completely covered. split it
				if ( prect->x+prect->w < r->x+r->w ) {
					// right part
					Rect newr(prect->x+prect->w,r->y,(int)(r->x+r->w-prect->x-prect->w),r->h);
					toAdd.push(newr);
				}
				if ( prect->y+prect->h < r->y+r->h ) {
					// bottom part
					Rect newr(r->x,prect->y+prect->h,r->w,(int)(r->y+r->h-prect->y-prect->h));
					toAdd.push(newr);
				}
				if ( prect->x > r->x ) {
					// left part
					Rect newr(r->x,r->y,(int)(prect->x-r->x),r->h);
					toAdd.push(newr);
				}
				if ( prect->y > r->y ) {
					// top part
					Rect newr(r->x,r->y,r->w,(int)(prect->y-r->y));
					toAdd.push(newr);
				}
			}
			r=empty.remove(r);
		}
	}
	for (Rect *r1=toAdd.begin(); r1 != toAdd.end(); r1++) {
		// check size
		if ( r1->w < minWidth || r1->h < minHeight ) {
			r1=toAdd.remove(r1); // too small
			continue;
		}
		for (Rect *r2=toAdd.begin(); r2 != toAdd.end(); r2++) {
			if ( r2 == r1 ) continue;
			if ( r1->x == r2->x && r1->y == r2->y && r1->w == r2->w && r1->h == r2->h ) {
				// r1 and r2 identical
				if ( r1 < r2 ) {
					r1=toAdd.remove(r1);
					break; // keep only the last
				}
			} else if ( r1->x >= r2->x && r1->y >= r2->y && r1->x+r1->w <= r2->x+r2->w && r1->y+r1->h <= r2->y+r2->h ) {
				// r1 inside r2
				r1=toAdd.remove(r1);
				break;
			}
		}
	}
	// add remaining rectangles to the free rectangles list
	for (Rect *r1=toAdd.begin(); r1 != toAdd.end(); r1++) {
		empty.push(*r1);
	}

}

void Packer::addForbiddenZone(const Rect rect) {
#ifdef PACKER_DBG
	printf ("%dx%d-%dx%d => forbidden\n",(int)rect.x,(int)rect.y,rect.w,rect.h);
#endif
	addRectInternal((Rect *)(&rect),true);
}

/*
void Packer::pack() {
#ifdef PACKER_DBG
	printf ("\nEmpty:\n");
	for (Rect *r=empty.begin(); r != empty.end(); r++) {
		printf ("%dx%d-%dx%d ",r->x,r->y,r->w,r->h);
	}
	printf ("\n");
#endif
}
*/

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

#include <assert.h>
#include <math.h>
#include "main.hpp"

// range below which fishes try to get away from each other
#define SHOAL_CLOSE_RANGE 2.0f
// range below which fishes try to get closer from each other
#define SHOAL_FAR_RANGE 5.0f
// range below which fishes get away from scare points
#define SHOAL_SCARE_RANGE 6.0f
// maximum fish speed while not scared
#define MAX_FISH_SPEED 5.0f

// dummy height indicating that a cell is not water
#define NO_WATER -1000.0f
// how much wave energy is lost per second
#define DAMPING_COEF 1.3f
// wave height below which the water is considered still
#define ACTIVE_THRESHOLD 0.02f
// height on the triggering ripple
#define RIPPLE_TRIGGER 3.0f
// how many times ripples are updated per second
#define RIPPLE_FPS 10

RippleManager::RippleManager(Dungeon *dungeon) : dungeon(dungeon) {
}

void RippleManager::init() {
	bool visited[dungeon->width][dungeon->height];
	memset(visited,0,sizeof(bool)*dungeon->width*dungeon->height);
	// first time : compute water zones
	for (int dy=0; dy < dungeon->height; dy++) {
		for (int dx=0; dx < dungeon->width; dx++) {
			if (!visited[dx][dy]) {
				if ( dungeon->hasRipples(dx,dy) ) {
					// new water zone. floodfill it
					TCODList<int> cells;
					cells.push(dx+dy*dungeon->width);
					int minx=dx;
					int miny=dy;
					int maxx=dx;
					int maxy=dy;
					while ( cells.size() > 0 ) {
						int off=cells.pop();
						int x=off%dungeon->width;
						int y=off/dungeon->width;
						visited[x][y]=true;
						if ( x > 0 && ! visited[x-1][y] && dungeon->hasRipples(x-1,y)) {
							cells.push(off-1);
							if ( minx > x-1 ) minx=x-1;
						}
						if ( x < dungeon->width-1 && ! visited[x+1][y] && dungeon->hasRipples(x+1,y)) {
							cells.push(off+1);
							if ( maxx < x+1 ) maxx=x+1;
						}
						if ( y > 0 && ! visited[x][y-1] && dungeon->hasRipples(x,y-1)) {
							cells.push(off-dungeon->width);
							if ( miny > y-1 ) miny=y-1;
						}
						if ( y < dungeon->height-1 && ! visited[x][y+1] && dungeon->hasRipples(x,y+1)) {
							cells.push(off+dungeon->width);
							if ( maxy < y+1 ) maxy=y+1;
						}
					}
					if ( maxx-minx > 5 && maxy-miny > 5 ) {
						// create a new zone
						WaterZone *zone=new WaterZone();
						zone->rect.x=minx;
						zone->rect.y=miny;
						zone->rect.w=maxx-minx+1;
						zone->rect.h=maxy-miny+1;
						zone->cumulatedElapsed=0.0f;
						zone->isActive=false;
						zone->data = new float[zone->rect.w * zone->rect.h * 4];
						zone->oldData = new float[zone->rect.w * zone->rect.h * 4];
						zone->shoal=NULL;
						zones.push(zone);
						// set water height to 0.0, not water cells to -1000
						// water height uses subcell rez
						for ( int zx=0; zx < zone->rect.w*2; zx++ ) {
							int zdx2=(int)(zx+zone->rect.x*2);
							for ( int zy=0; zy < zone->rect.h*2; zy++ ) {
								int zdy2=(int)(zy+zone->rect.y*2);
								float value = dungeon->hasWater(zdx2,zdy2) ? 0.0f : NO_WATER;
								zone->data[zx+zy*zone->rect.w*2] = value;
							}
						}
						memcpy(zone->oldData,zone->data,sizeof(float)*zone->rect.w * zone->rect.h * 4);
						int nbFish = TCODRandom::getInstance()->getInt(zone->rect.w * zone->rect.h/80,zone->rect.w * zone->rect.h/20);
						if ( nbFish > 0 ) {
							zone->shoal=new Shoal();
							while ( nbFish > 0 ) {
								Fish *fish=new Fish(zone);
								zone->shoal->list.push(fish);
								// find a random place in the lake
								int x2=TCODRandom::getInstance()->getInt(0,zone->rect.w-1);
								int y2=TCODRandom::getInstance()->getInt(0,zone->rect.h-1);
								while ( zone->data[x2+y2*zone->rect.w*2] == NO_WATER ) {
									x2++;
									if ( x2 == zone->rect.w*2 ) {
										x2=0;
										y2++;
										if ( y2 == zone->rect.h*2 ) y2=0;
									}
								}
								fish->setPos(x2*0.5f+zone->rect.x,y2*0.5f+zone->rect.y);
								dungeon->addCreature(fish);
								assert(dungeon->hasWater(fish->getSubX(),fish->getSubY()));
								nbFish--;
							}
						}
					}
				}
			} else {
				visited[dx][dy]=true;
			}
		}
	}
}

void RippleManager::startRipple(int dungeonx, int dungeony, float height) {
	if ( zones.size() == 0 ) init();
	if ( height == 0.0f ) height=RIPPLE_TRIGGER;
	// look for the water zone
	for (WaterZone **it=zones.begin(); it!=zones.end(); it++) {
		WaterZone *zone=*it;
		if ( zone->rect.pointInside(dungeonx,dungeony) ) {
			int zx2=(int)(dungeonx-zone->rect.x)*2;
			int zy2=(int)(dungeony-zone->rect.y)*2;
			int off=zx2+zy2*zone->rect.w*2;
			if ( zone->data[off] == NO_WATER ) continue; // not the right zone
			zone->data[off] = -height;
			zone->isActive=true;
			if (zone->shoal) {
				zone->shoal->scare.push(new ScarePoint(dungeonx,dungeony));
			}
			break;
		}
	}
}

bool RippleManager::updateRipples(float elapsed) {
	// compute visible part of the dungeon
	Rect visibleZone;
	visibleZone.x=gameEngine->xOffset;
	visibleZone.y=gameEngine->yOffset;
	visibleZone.w=CON_W;
	visibleZone.h=CON_H;
	bool updated=false;
	for (WaterZone **it=zones.begin(); it!=zones.end(); it++) {
		WaterZone *zone=*it;
		zone->cumulatedElapsed+=elapsed;
		if ( zone->isActive ) {
			// update the ripples
			if ( zone->cumulatedElapsed > 1.0f/RIPPLE_FPS && zone->rect.isIntersecting(visibleZone) ) {
				zone->cumulatedElapsed=0.0f;
				// swap grids
				float *tmp=zone->data;
				zone->data=zone->oldData;
				zone->oldData=tmp;
				zone->isActive=false;
				for (int zx2=1; zx2 < zone->rect.w*2-1; zx2++) {
					for (int zy2=1; zy2 < zone->rect.h*2-1; zy2++) {
						int off = zx2 + zy2 * zone->rect.w*2;
						if (zone->data[off] != NO_WATER) {
							float sum=0.0f;
							int count=0;
							// wave smoothing + spreading
							if ( zone->oldData[off-1] != NO_WATER ) {
								sum += zone->oldData[off-1];
								count++;
							}
							if ( zone->oldData[off+1] != NO_WATER ) {
								sum += zone->oldData[off+1];
								count++;
							}
							if ( zone->oldData[off-zone->rect.w*2] != NO_WATER ) {
								sum += zone->oldData[off-zone->rect.w*2];
								count++;
							}
							if ( zone->oldData[off+zone->rect.w*2] != NO_WATER ) {
								sum += zone->oldData[off+zone->rect.w*2];
								count++;
							}
							sum = sum * 2 / count;
							zone->data[off] = sum - zone->data[off];
							// damping
							zone->data[off] *= 1.0f-DAMPING_COEF/RIPPLE_FPS;
							if ( ABS(zone->data[off]) > ACTIVE_THRESHOLD ) {
								zone->isActive=true;
								updated=true;
							}
						}
					}
				}
			}
		}
		// update the fish shoal
		Shoal *shoal=zone->shoal;
		if ( shoal ) {
			for (Fish **f1=shoal->list.begin(); f1 != shoal->list.end(); f1++) {
				Fish *fish1=*f1;
				// some of the fishes of the shoal may be out of screen. skip them
				if (!fish1->updated) continue;
				for (Fish **f2=shoal->list.begin(); f2 != shoal->list.end(); f2++) {
					Fish *fish2=*f2;
					// fish-fish interaction
					// TODO can be optimized with fastInvSqrt
					if ( fish1 != fish2 ) {
						float dx=fish2->x - fish1->x;
						float dy=fish2->y - fish1->y;
						float dist=sqrtf(dx*dx+dy*dy);
						if (dist <= 1E-4f) {
						} else if ( dist < SHOAL_CLOSE_RANGE ) {
							// get away from other fish
							fish1->dx -= elapsed*5.0f * dx / dist;
							fish1->dy -= elapsed*5.0f * dy / dist;
						} else if ( dist < SHOAL_FAR_RANGE ) {
							// get closer to other fish
							fish1->dx += elapsed*1.2f * dx / dist;
							fish1->dy += elapsed*1.2f * dy / dist;
						}
					}
				}
				fish1->dx=CLAMP(-MAX_FISH_SPEED,MAX_FISH_SPEED,fish1->dx);
				fish1->dy=CLAMP(-MAX_FISH_SPEED,MAX_FISH_SPEED,fish1->dy);
				// fish-scare interaction
				// TODO can be optimized with fastInvSqrt
				for (ScarePoint **spit=shoal->scare.begin(); spit != shoal->scare.end(); spit++) {
					float dx = (*spit)->x - fish1->x;
					float dy = (*spit)->y - fish1->y;
					float dist=sqrtf(dx*dx+dy*dy);
					if ( dist > 1E-4f && dist < SHOAL_SCARE_RANGE ) {
						float coef=(SHOAL_SCARE_RANGE-dist)/SHOAL_SCARE_RANGE;
						fish1->dx -= elapsed*MAX_FISH_SPEED*10*coef*dx/dist;
						fish1->dy -= elapsed*MAX_FISH_SPEED*10*coef*dy/dist;
					}
				}
				fish1->dx=CLAMP(-MAX_FISH_SPEED*2,MAX_FISH_SPEED*2,fish1->dx);
				fish1->dy=CLAMP(-MAX_FISH_SPEED*2,MAX_FISH_SPEED*2,fish1->dy);

				fish1->slide();
				assert(gameEngine->dungeon->hasWater(fish1->getSubX(),fish1->getSubY()));
			}
			for (ScarePoint **spit=shoal->scare.begin(); spit != shoal->scare.end(); spit++) {
				(*spit)->life -= elapsed;
				if ( (*spit)->life <= 0.0f ) {
					delete (*spit);
					spit = shoal->scare.remove(spit);
				}
			}
		}
	}

	return updated;
}

void RippleManager::renderRipples(TCODImage *ground) {
	if ( zones.size() == 0 ) init();
	// compute visible part of the dungeon
	Rect visibleZone;
	visibleZone.x=gameEngine->xOffset;
	visibleZone.y=gameEngine->yOffset;
	visibleZone.w=CON_W;
	visibleZone.h=CON_H;
	float elCoef=TCODSystem::getElapsedSeconds()*2.0f;
	for (WaterZone **it=zones.begin(); it!=zones.end(); it++) {
		WaterZone *zone=*it;
		if ( zone->rect.isIntersecting(visibleZone) ) {
			Rect z=zone->rect;
			// remove borders
			z.x++;
			z.y++;
			z.w-=2;
			z.h-=2;
			// intersect zone with viewport (dungeon coords)
			z.intersect(visibleZone);
			// part to render (zone coordinates)
			int minx=(int)(z.x-zone->rect.x);
			int miny=(int)(z.y-zone->rect.y);
			int maxx=minx+z.w-1;
			int maxy=miny+z.h-1;
			int dungeon2groundx = (int)(zone->rect.x - visibleZone.x)*2;
			int dungeon2groundy = (int)(zone->rect.y - visibleZone.y)*2;
			for ( int zx2 = minx*2; zx2 < maxx*2; zx2++ ) {
				int dungeonx2=(int)(zx2+zone->rect.x*2);
				int groundx = dungeon2groundx+zx2;
				for ( int zy2 = miny*2; zy2 < maxy*2; zy2++ ) {
					int dungeony2=(int)(zy2+zone->rect.y*2);
					int groundy = (int)(dungeon2groundy+zy2);
					if ( !dungeon->hasCanopy(dungeonx2,dungeony2)
						&& getData(**it,zx2,zy2) != NO_WATER
						) {
						float xOffset=(getData(**it,zx2-1,zy2)-getData(**it,zx2+1,zy2));
						float yOffset=(getData(**it,zx2,zy2-1)-getData(**it,zx2,zy2+1));
						float f[3]={zx2,zy2,elCoef};
						xOffset+=noise3d.get(f)*0.3f;
						if ( ABS(xOffset) < 250 && ABS(yOffset) < 250 ) {
							TCODColor col=ground->getPixel(groundx+(int)(xOffset*2),groundy+(int)(yOffset*2));
							col = col + TCODColor::white*xOffset*0.1f;
							ground->putPixel(groundx,groundy,col);
						}
					}
				}
			}
		}
	}
}

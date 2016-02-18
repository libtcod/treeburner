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
#include <math.h>
#include "main.hpp"

// returns a value between 0.5 and 1.2 
// 50% chances between 0.5 and 1.0 (clouds), 50% chances between 1.0 and 1.2 (clear sky)
static inline float noiseFunc(float *f) {
	/*
	float ret = 0.5f * (1.0f + noise2d.getFbm(f,4.0f)); // 0.0  - 1.0
	ret = 1.2f - 0.3f * ret; // 0.8 - 1.2
	if ( ret < 1.0f ) ret *= 0.75f + 2.5f * (ret-0.9f); // 0.5 - 1.0
	*/
	float ret=noise2d.getFbm(f,4.0f);
	if ( ret < 0.0f ) ret = 1.0f+ret*0.5f; // 0.5 - 1.0
	else ret =1.0f+0.2f*ret; // 1.0 - 1.2
	return ret;
}

CloudBox::CloudBox(int width, int height) : width(width),height(height),xOffset(0.0f),xTotalOffset(0.0f) {
	data = new float[width*height];
	highOctaveNoise = new float[width*height];
	float f[2];
	float *val=data;
	// we want high octave noise to wrap on x coordinates
	// => use a cylinder in 3d noise space
	float f2[3];
	float *hoval=highOctaveNoise;
    for (int y=0; y < height; y++) {
    	if ( y % 40 == 0 ) gameEngine->displayProgress(0.1f + (float)y/height*0.3f);
        f[1] = (6.0f*y) / height;
		f2[2]=f[1]*15.0f;
		for ( int x=0; x < width; x++) {
    	    f[0] = (6.0f*x) / width;
			float angle=x * 3.14159f*2/width;
			f2[0]=15.0f * cosf(angle);
			f2[1]=15.0f * sinf(angle);
            *val = noiseFunc(f);
            val++;
			*hoval=0.3f*noise3d.getFbm(f2,8.0f);
			hoval++;
	    }
	}
	x0=0;
	static TCODColor up[] = {
		TCODColor::white,
		TCODColor::white,
		TCODColor::sky,
		TCODColor::sky,
	};
	static int upKeys[] = { 0, 110, 130, 255 };
	TCODColor::genMap(cloudColorMap,4,up,upKeys);
}

CloudBox::~CloudBox() {
	delete [] data;
}

float CloudBox::getData(float *pdata,int x, int y) {
	int realX = (x + x0)%width;
	return pdata[realX+y*width]; 
}

float CloudBox::getInterpolatedData(float *pdata,int x, int y) {
	int realX = (x + x0)%width;
	int maxX = (x0-1+width)%width;
    float fx = realX + xOffset;
    int ix=realX;
    int iy=y;
    int ix1 = ix == maxX ? maxX : (ix+1)%width;
    float fdx = fx - ix;
    float v1 = pdata[ix+iy*width];
    float v2 = pdata[ix1+iy*width];
    float vx1 = ((1.0f - fdx) * v1 + fdx * v2);
    return vx1;
}

float CloudBox::getThickness(int x, int y) { 
	return getData(data,x,y);
}

float CloudBox::getNoisierThickness(int x, int y) {
	return getInterpolatedData(data,x,y)+getInterpolatedData(highOctaveNoise,x,y);
}

float CloudBox::getInterpolatedThickness(int x, int y) {
	return getInterpolatedData(data,x,y);
}

TCODColor CloudBox::getColor(float thickness, int x, int y) {
	static float idxMul=255/1.5f;
	int idx= (int)((getNoisierThickness(x,y)-0.2)*idxMul);
	return cloudColorMap[ idx ];
}

void CloudBox::update(float elapsed) {
    xTotalOffset += elapsed;
    xOffset += elapsed;
    if ( xOffset >= 1.0f ) {
        int colsToTranslate=(int)xOffset;
        xOffset -= colsToTranslate;
		// x0 is the cloud map column corresponding to the map column 0
		// do a virtual translation of the cloud table (only increment x0)
		int oldx0 = x0;
		x0 = (x0 + colsToTranslate) % width;
        // compute the new columns (between old x0 and x0-1)
        float f[2];
        float cdx = (int)xTotalOffset ;
		int x = oldx0;
		float noiseX = ((x-x0+width) % width)+cdx;
		while ( x != x0 ) {
            f[0] = (6.0f*noiseX) / width;
            for (int y=0; y < height; y++) {
                f[1] = (6.0f*y) / height;
                data[x+y*width] = noiseFunc(f);
            }
			x++; if ( x == width ) x=0;
			noiseX += 1.0f;
			colsToTranslate--;
        }
    }
}

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

// background with MULTIPLY op, opaque foreground
void blitTransparent(const TCODConsole *src,int x, int y, int w, int h, TCODConsole *dst, int xd, int yd) {
	if (w == 0) w = src->getWidth();
	if (h == 0) h = src->getHeight();
	for (int ix=x; ix < x+w; ix++ ) {
		for (int iy=y; iy < y+h; iy++ ) {
			int xdest=xd+ix-x;
			int ydest=yd+iy-y;
			int ch=src->getChar(ix,iy);
			TCODColor bk=src->getCharBackground(ix,iy);
			dst->setCharBackground(xdest,ydest,bk,TCOD_BKGND_MULTIPLY);
			if ( ch != ' ' && ch != 0 ) {
				TCODColor fore=src->getCharForeground(ix,iy);
				dst->setCharForeground(xdest,ydest,fore);
				dst->setChar(xdest,ydest,ch);
			} else {
				int destch=dst->getChar(xdest,ydest);
				if ( destch != ' ' && destch != 0 ) {
					TCODColor fore=dst->getCharForeground(xdest,ydest);
					dst->setCharForeground(xdest,ydest,fore*bk);
				}
			}
		}
	}
}

// transparent background & foreground with independant alphas
void blitSemiTransparent(const TCODConsole *src,int x, int y, int w, int h, TCODConsole *dst, int xd, int yd, float bgalpha, float fgalpha) {
	if (w == 0) w = src->getWidth();
	if (h == 0) h = src->getHeight();
	for (int ix=x; ix < x+w; ix++ ) {
		for (int iy=y; iy < y+h; iy++ ) {
			int xdest=xd+ix-x;
			int ydest=yd+iy-y;
			int ch=src->getChar(ix,iy);
			TCODColor bk=src->getCharBackground(ix,iy);
			TCODColor bk2=dst->getCharBackground(xdest,ydest);
			dst->setCharBackground(xdest,ydest,TCODColor::lerp(bk2,bk,bgalpha));
			int destch=dst->getChar(xdest,ydest);
			if ( ch != ' ' && ch != 0 ) {
				TCODColor fore=src->getCharForeground(ix,iy);
				TCODColor fore2=(destch == ' '|| destch==0 ? dst->getCharBackground(xdest,ydest) : dst->getCharForeground(xdest,ydest));
				dst->setCharForeground(xdest,ydest,TCODColor::lerp(fore2,fore,fgalpha));
				dst->setChar(xdest,ydest,ch);
			} else if ( destch != ' ' && destch != 0 ) {
				TCODColor fore=dst->getCharForeground(xdest,ydest);
				dst->setCharForeground(xdest,ydest,TCODColor::lerp(fore,bk,bgalpha));
			}
		}
	}
}

// multiply colors with coef
void darken(int x0, int y0, int w, int h, float coef) {
	rect(TCODConsole::root,x0,y0,w,h,TCODColor::black,coef);
}

void rect(TCODConsole *con, int x0, int y0, int w, int h, const TCODColor &col, float alpha) {
	int minx=x0 >= 0 ? x0 : 0;
	int maxx=x0+w <= con->getWidth() ? x0+w : con->getWidth();
	int miny=y0 >= 0 ? y0 : 0;
	int maxy=y0+h <= con->getHeight() ? y0+h : con->getWidth();
	for (int x=minx; x < maxx; x++ ) {
		for (int y=miny; y< maxy; y++ ) {
			int c=con->getChar(x,y);
			if ( c != 0 && c != ' ') {
				TCODColor f=con->getCharForeground(x,y);
				f = TCODColor::lerp(f,col,alpha);
				con->setCharForeground(x,y,f);
			}
			TCODColor b=con->getCharBackground(x,y);
			b = TCODColor::lerp(b,col,alpha);
			con->setCharBackground(x,y,b);
		}
	}
}

// add white * coef to foreground and background
void lighten(int x0, int y0, int w, int h, float coef) {
	rect(TCODConsole::root,x0,y0,w,h,TCODColor::white,coef);
}

// subcell over subcell
static int asciiToFlag[] = {
	1, // TCOD_CHAR_SUBP_NW
	2, // TCOD_CHAR_SUBP_NE
	3, // TCOD_CHAR_SUBP_N
	8, // TCOD_CHAR_SUBP_SE
	9, // TCOD_CHAR_SUBP_DIAG
	10, // TCOD_CHAR_SUBP_E
	4, // TCOD_CHAR_SUBP_SW
};

TCODColor getSubcellColor(TCODConsole *con, int x2, int y2) {
	TCODColor dstBack = con->getCharBackground(x2/2,y2/2);
	int dstChar = con->getChar(x2/2,y2/2);
	// get the flag for the existing cell on root console
	int bkflag=asciiToFlag[dstChar - TCOD_CHAR_SUBP_NW ];
	int xflag = (x2 & 1);
	int yflag = (y2 & 1);
	// get the flag for the current subcell
	int credflag = (1+3*yflag) * (xflag+1);
	if ( (credflag & bkflag) != 0 ) {
		// the color for this subcell on root console
		// is foreground, not background
		dstBack = con->getCharForeground(x2/2,y2/2);
	}
	return dstBack;
}

// transparent blit2x, updates the content of img !
void transpBlit2x(TCODImage *img, int xsrc, int ysrc, int wsrc, int hsrc, TCODConsole *con, int xdst, int ydst, float alpha) {
	for (int x=xsrc; x < xsrc+wsrc; x++ ) {
		int x2dst = xdst*2 + x-xsrc;
		for (int y=ysrc;y <ysrc+hsrc; y++) {
			int y2dst = ydst*2 + y-ysrc;
			TCODColor srccol=img->getPixel(x,y);
			TCODColor dstBack = con->getCharBackground(x2dst/2,y2dst/2);
			int dstChar = con->getChar(x2dst/2,y2dst/2);
			if ( dstChar >= TCOD_CHAR_SUBP_NW && dstChar <= TCOD_CHAR_SUBP_SW ) {
				// merge two subcell chars...
				dstBack=getSubcellColor(con,x2dst,y2dst);
			}
			// subcell over standard cell
			srccol = TCODColor::lerp(dstBack,srccol,alpha);
			img->putPixel(x,y,srccol);
		}
	}
	img->blit2x(con,xdst,ydst,xsrc,ysrc,wsrc,hsrc);
}

void transpRect2x(TCODConsole *con, int x, int y, int w2, int h2, const TCODColor &col, const TCODColor &keyCol, float alpha) {
	static TCODImage *img=NULL;
	if ( img == NULL ) {
		img=new TCODImage(CON_W,CON_H);
	}
//	int w = ( (w2&1) == 0 ) ? w2 : w2+1;
	for (int px=0; px < w2; px ++ ) {
		for (int py=0; py < h2; py ++ ) {
			img->putPixel(px,py,col);
		}
	}
/*
	if ( w2 & 1 ) {
		for (int py=0; py < h2; py ++ ) {
			img->putPixel(w2,py,getSubcellColor(con,x*2+w2,y*2+py));
		}
	}
*/
	transpBlit2x(img,0,0,w2,h2,con,x,y,alpha);
}


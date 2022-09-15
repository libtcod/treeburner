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

// sub-cell image & console blitting functions

// background with MULTIPLY op, opaque foreground
void blitTransparent(const TCODConsole *src,int x, int y, int w, int h, TCODConsole *dst, int xd, int yd);
// transparent background & foreground with independant alphas
void blitSemiTransparent(const TCODConsole *src,int x, int y, int w, int h, TCODConsole *dst, int xd, int yd, float bgalpha, float fgalpha);
// multiply colors with coef
void darken(int x0, int y0, int w, int h, float coef);
// add white * coef to foreground and background
void lighten(int x0, int y0, int w, int h, float coef);

// draw a transparent rectangle
void rect(TCODConsole *con, int x, int y, int w, int h, const TCODColor &col, float alpha);
// transparent blit2x, updates the content of img !
void transpBlit2x(TCODImage *img, int xsrc, int ysrc, int wsrc, int hsrc, TCODConsole *con, int xdst, int ydst, float alpha);
// transparent rectangle blit2x with color key
void transpRect2x(TCODConsole *con, int x, int y, int w2, int h2, const TCODColor &col, const TCODColor &keyCol, float alpha);

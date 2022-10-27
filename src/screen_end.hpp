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
#include <SDL_surface.h>

class EndScreen : public Screen{
public :
	EndScreen(const char *txt,float fadeLvl=0.0f, bool stats=false);
	void render();
	bool update(float elapsed, TCOD_key_t k,TCOD_mouse_t mouse);
	virtual ~EndScreen() {}
	void renderText(int x,int y, int w, const char *txt);
	void onFontChange();
protected :
	const char *txt = nullptr;
	const char *version = nullptr;
	TCOD_alignment_t alignment;

	float noiseZ;
	bool stats;
	TCODImage *img = nullptr;
	void onInitialise() override;
	void onActivate() override;
};

class PaperScreen : public EndScreen, public ITCODSDLRenderer {
public :
	PaperScreen(const char *txgfile, const char  *titlegen, const char *textgen, int chapter);
	void render();
	void render(void *sdlSurface);
	bool update(float elapsed, TCOD_key_t k,TCOD_mouse_t mouse);
	virtual ~PaperScreen() {}
	void onFontChange();
protected :
	const char *title = nullptr;
	const char *txgfile = nullptr;
	const char *titlegen = nullptr;
	const char *textgen = nullptr;
	TCODImage *tcodpix = nullptr;
	static TCODImage *paper;
	static int paperHeight;
	SDL_Surface *pix = nullptr;
	float scrolltimer;
	int chapter;
	int pixx,pixy; // picture position (pixels)
	int pixw,pixh; // picture size (pixels)
	int overlaph,offseth; // if text use more than one screen
	void onInitialise() override;
	void onActivate() override;
	void onDeactivate() override;
};

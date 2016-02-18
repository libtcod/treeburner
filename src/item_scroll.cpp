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
/*
#include "main.hpp"

ScrollType scrollTypes[] = {
	{ { NB_ITEMS, INV_MISC,"scroll", TCODColor(255,127,127), '#', {4,TCODColor(76,38,38),1.0f,"56789876"}, false, ITEM_DELETE_ON_USE|ITEM_STACKABLE, NB_ITEM_TYPES } },
};

Scroll::Scroll(float x,float y) : Item(x,y,scrollTypes[0].type), selected(0), initDelay(1.0f) {
	type=ITEM_SCROLL;
	subType=ITEM_SCROLL_BASE;
}

Item * Scroll::pickup(Creature *owner) {
	Item *newItem = Item::pickup(owner);
	newItem->use();
	gameEngine->pauseGame();
	return newItem;
}

bool Scroll::update(float elapsed, TCOD_key_t key, TCOD_mouse_t *mouse) {
	Item::update(elapsed,key,mouse);
	if ( active ) {
		initDelay -= elapsed;
		if (initDelay > 0) mouse->lbutton_pressed=false;
		return PowerupGraph::instance->update(elapsed,key,mouse);
	}
	return true;
}

void Scroll::render(LightMap *lightMap) {

	if (! active) {
		Item::render(lightMap);
	} else {
		PowerupGraph::instance->render();
	}
}
*/

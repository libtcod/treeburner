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

#define NB_WEAPONS 5

EquipScreen::EquipScreen() : Screen(0), pauseOn(false) {
}

void EquipScreen::activate() {
	// generate a random list of items according to the selected school
	for (int i=0; i < NB_WEAPONS; i++ ) {
		ItemClass wcol = ITEM_CLASS_STANDARD;
		int p = rng->getInt(1,100);
		if ( p > 90 ) wcol = ITEM_CLASS_ORANGE;
		else if ( p > 60 ) wcol = ITEM_CLASS_GREEN;
		weapons.push(Weapon::getRandom( wcol ));
	}
}

void EquipScreen::deactivate() {
	weapons.clearAndDelete();
}

void EquipScreen::render() {
	background.blit2x(TCODConsole::root,0,0);
	int y=10,i=1;
	TCODConsole::root->setForegroundColor(TCODColor::white);
	TCODConsole::root->print(10,7,"Choose a weapon :");
	for (Weapon **w=weapons.begin(); w!= weapons.end(); w++) {
		TCODConsole::root->setForegroundColor((*w)->col);
		TCODConsole::root->print(10,y++,"%d %s",i++,(*w)->name);
	}
	// render items under the mouse cursor
	if ( mousey >= 10 && mousey < 10+NB_WEAPONS ) {
		Weapon *w=weapons.get(mousey-10);
		w->renderDescription(mousex,mousey);
	}
}
bool EquipScreen::update(float elapsed, TCOD_key_t k,TCOD_mouse_t mouse) {

	mousex=mouse.cx;
	mousey=mouse.cy;
	if ( mousey >= 10 && mousey < 10+NB_WEAPONS && mouse.lbutton_pressed ) {
		Weapon *w=weapons.get(mousey-10);
		w->pickup(&gameEngine->player);
		weapons.remove(w);
		return false;
	}
	return true;

}



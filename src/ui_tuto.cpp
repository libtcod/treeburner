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

#define FADE_TIME 0.5f
#define BLINK_TIME 3.0f
Tutorial::Tutorial() : Screen(255) {
	blinkDelay=BLINK_TIME;
	id = TUTO_NONE;
	memset(alreadyStarted,0,sizeof(alreadyStarted));
	for (TutorialPageId id = (TutorialPageId)0; id < TUTO_NB_PAGES; id = (TutorialPageId)(id+1)) {
		pages[id].con = NULL;
		pages[id].inMenu = true;
		pages[id].x=-1;
		pages[id].y=-1;
		pages[id].delay=10.0f;
	}
	pages[TUTO_MOVE].name = "Basic controls";
	pages[TUTO_RUN].name="Run and sprint";
	renderMenu = false;
	lastPage = TUTO_MOVE;
	selectedItem=0;
	saveGame.registerListener(TUTO_CHUNK_ID,PHASE_START,this);
}

Tutorial::~Tutorial() {
	saveGame.unregisterListener(this);
}

void Tutorial::createPage(TutorialPageId id) {
	switch (id) {
		case TUTO_MOVE : pages[TUTO_MOVE].con=createMovePage(); break;
		case TUTO_RUN : pages[TUTO_RUN].con=createSprintPage(); break;
		case TUTO_FOOD : pages[TUTO_FOOD].con=createFoodPage(); break;
		case TUTO_HIDE_SEEK : pages[TUTO_HIDE_SEEK].con=createHideSeekPage(); break;
		case TUTO_ITEMS : pages[TUTO_ITEMS].con=createItemsPage(); break;
		case TUTO_INVENTORY : pages[TUTO_INVENTORY].con=createInventoryPage(); break;
		case TUTO_CROUCH : pages[TUTO_CROUCH].con=createCrouchPage(); break;
		case TUTO_INVENTORY2 : pages[TUTO_INVENTORY2].con=createInventory2Page(); break;
		case TUTO_FIREBALL : pages[TUTO_FIREBALL].con=createFireballPage(); break;
		default: break;
	}
}

TCODConsole *Tutorial::createBasePage(const char *name, int w, int h) {
	TCODConsole *con = new TCODConsole(w,h);
	con->setDefaultForeground(guiText);
	con->setDefaultBackground(guiBackground);
	con->printFrame(0,0,w,h,true,TCOD_BKGND_SET,name);
	con->setChar(w-1,0,'x');
	return con;
}

TCODConsole *Tutorial::createMovePage() {
	pages[TUTO_MOVE].delay = 40.0f;
	TCODConsole *con = createBasePage(pages[TUTO_MOVE].name,50,22);
	con->print(2,2,
		"                %c== Movements ==%c             \n\n"
		"Movements support several keyboard layouts.\n"
		"In 4 keys layouts, press simultaneously two\n"
		"keys to move in diagonals\n\n"
		"FPS layout   Arrows        Numpad        vi\n\n"
		"%c   W           %c           7 8 9        Y K U\n"
		" A S D       %c %c %c         4   6        H   L\n"
		"                           1 2 3        B J N%c\n\n"
		"              %c== Miscellaneous ==%c             \n\n"
		"%cSPACE%c           : pause/resume game\n"
		"%cPRINTSCREEN%c     : take screenshot\n"
		"%cPGUP%c/%cPGDOWN%c     : change font size\n"
		"%cALT-ENTER%c       : fullscreen/windowed",
		TCOD_COLCTRL_1,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,
		TCOD_CHAR_ARROW_N,TCOD_CHAR_ARROW_W,TCOD_CHAR_ARROW_S,TCOD_CHAR_ARROW_E,
		TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_1,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
	return con;
}

TCODConsole *Tutorial::createFoodPage() {
	pages[TUTO_FOOD].delay = 25.0f;
	pages[TUTO_FOOD].name = "Movements";
	TCODConsole *con = createBasePage(pages[TUTO_FOOD].name,50,15);
	con->print(2,2,
		"Use arrows to move around. Press simultaneously\n"
		"two keys to move in diagonals\n\n"
		"%c       %c    \n"
		"     %c %c %c  %c\n\n"
		"You can sprint by pressing %cSHIFT%c.\n\n"
		"Press ? to see other movement key layouts.",
		TCOD_COLCTRL_2,
		TCOD_CHAR_ARROW_N,TCOD_CHAR_ARROW_W,TCOD_CHAR_ARROW_S,TCOD_CHAR_ARROW_E,
		TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
	pages[TUTO_FOOD].x=1;
	pages[TUTO_FOOD].y=1;
	return con;
}

TCODConsole *Tutorial::createHideSeekPage() {
	pages[TUTO_HIDE_SEEK].delay = 25.0f;
	pages[TUTO_HIDE_SEEK].name = "Hiding";
	TCODConsole *con = createBasePage(pages[TUTO_HIDE_SEEK].name,50,15);
	con->print(2,2,
		"Use %cCTRL%c to crouch. When crouching, you're\n"
		"more difficult to see. The stealth bar shows\n"
		"how visible you are. The higher it is, the more\n"
		"chance you have to go unnoticed by creatures\n"
		"in your field of view.\n\n"
		"Move slowly and stay in shadows to keep your\n"
		"stealth bar low...\n\n"
		"Now, find some hiding spot, quickly !",
		TCOD_COLCTRL_2,	TCOD_COLCTRL_STOP
	);
	pages[TUTO_HIDE_SEEK].x=1;
	pages[TUTO_HIDE_SEEK].y=1;
	return con;
}

TCODConsole *Tutorial::createSprintPage() {
	pages[TUTO_RUN].delay=20.0f;
	TCODConsole *con = createBasePage(pages[TUTO_RUN].name,50,20);
	con->print(2,2,
	"If you hold a movement key pressed, your\n"
	"character will start to run.\n\n"
	"While running, you're not as attentive\n"
	"to your surroundings. Your %cfield of view\n"
	"range decreases%c and your perception gets\n"
	"very low. You have to stop and wait\n"
	"to recover your breath to get it at full\n"
	"range.\n\n"
	"While running, you can %csprint by pressing\n"
	"SHIFT%c. The longer you press, the faster you go\n"
	"but if you go too fast, you will be exhausted\n"
	"and you will have to wait before being able to\n"
	"sprint again."
	, TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
	TCOD_COLCTRL_2,TCOD_COLCTRL_STOP);
	return con;
}

TCODConsole *Tutorial::createInventoryPage() {
	pages[TUTO_INVENTORY].delay=20.0f;
	pages[TUTO_INVENTORY].name = "Inventory";
	TCODConsole *con = createBasePage(pages[TUTO_INVENTORY].name,39,20);
	con->print(2,2,
	"This is your inventory. Right-click\n"
	"on an item to see available actions.\n"
	"Left-click to use the %cdefault\n"
	"action%c.\n\n"
	"%cDrop or throw %c an item by dragging\n"
	"it out of the inventory window.\n\n"
	"%cCombine%c items by dragging one on\n"
	"another.\n"
	, TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	, TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	, TCOD_COLCTRL_2,TCOD_COLCTRL_STOP);
	pages[TUTO_INVENTORY].x=41;
	pages[TUTO_INVENTORY].y=11;
	return con;
}

TCODConsole *Tutorial::createPage(TutorialPageId id,const char *title, const char *content, ...) {
	// analyze content
	va_list ap;
	va_start (ap,content);
	char buf[1024];
	vsprintf(buf,content,ap);
	va_end(ap);
	pages[id].name=title;
	int h=TCODConsole::root->getHeightRect(0,0,46,0,buf);
	TCODConsole *con=createBasePage(title,50,h+4);
	con->printRect(2,2,46,0,buf);
	return con;
}

TCODConsole *Tutorial::createInventory2Page() {
	return createPage(TUTO_INVENTORY2,"Inventory",
		"Press %ci%c to open your inventory. You can examine your belongings by hovering the mouse over them.\n\n"
		"Hold the %cright mouse button%c on an item to display a %ccontext menu%c with available actions for this item."
		"The first action of the context menu is the %cdefault action%c, triggered when you %cleft click%c on the item.\n\n"
		"You can %cdrop%c an item by selecting the drop action or %cdragging%c the item on your character on the map.\n\n"
		"You can %cthrow%c an item by selecting the throw action and clicking on the target, or %cdragging%c the item on the map."
		"You can cancel the throwing by right clicking.\n\n"
		"You can %ccombine%c two items by %cdragging%c the first one on the other. If the items can be combined, the result will be displayed under the mouse cursor.",
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
}

TCODConsole *Tutorial::createFireballPage() {
	return createPage(TUTO_FIREBALL,"Fireballs",
		"Press either %cleft or right mouse button%c to cast a standard fireball.\n\n"
		"%cHold left mouse button%c to cast an %cincandescent%c variation of the spell.\n"
		"%cHold right mouse button%c to cast a %csparks%c variation of the spell.\n\n"
		"Variations might interact with each other. Feel free to experiment.",
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
}

TCODConsole *Tutorial::createCrouchPage() {
	return createPage(TUTO_CROUCH,"Crouch",
		"Hold %cCTRL to crouch%c. When crouching, a vertical bar indicates your stealth level.\n\n"
		"Stay in shadows and move slowly to increase your stealth level.\n\n"
		"When crouching and standing still, you're more aware of your surroundings and your field of view range increases.",
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
}

TCODConsole *Tutorial::createItemsPage() {
	return createPage(TUTO_ITEMS,"Items",
		"Items are shown as character on the map.\n\n"
		"%cHover your mouse%c over an item to see its name. Hold %cALT%c to see a more detailed description.\n\n"
		"You can %cpickup%c an item by %cclicking on it%c if it's next to you, or %cwalk on it%c.\n\n"
		"You can interact with non pickable items like trees and doors by clicking on them.",
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP,
		TCOD_COLCTRL_2,TCOD_COLCTRL_STOP
	);
}
void Tutorial::init() {
	enableMenuPage(TUTO_MOVE);
	enableMenuPage(TUTO_RUN);
	enableMenuPage(TUTO_CROUCH);
	enableMenuPage(TUTO_ITEMS);
	enableMenuPage(TUTO_INVENTORY2);
	pages[TUTO_FOOD].inMenu=false;
	pages[TUTO_HIDE_SEEK].inMenu=false;
	pages[TUTO_INVENTORY].inMenu=false;
}

void Tutorial::openMenu() {
	if ( gameEngine->gui.inventory.getActive() ) lastPage=TUTO_INVENTORY;
	else {
		renderMenu=true;
		menu.clear();
		for (TutorialPageId i=(TutorialPageId)1; i < TUTO_NB_PAGES; i = (TutorialPageId)(i+1)) {
			if ( alreadyStarted[i] && pages[i].inMenu ) menu.push(i);
		}
	}
	startTuto(lastPage);

	gameEngine->pauseGame();
}

void Tutorial::closeMenu() {
	renderMenu=false;
	closeLiveTuto();
}

void Tutorial::render() {
	TCODColor fore=guiText;
	if ( blinkDelay > 0.0 && ((int)(blinkDelay * 10) & 1) == 1 ) fore=guiDisabledText;
	TCODConsole::root->setDefaultForeground(fore);
	TCODConsole::root->print(CON_W-10,CON_H-3,"? Help");

	if ( renderMenu ) {
		static TCODConsole menuCon(20,20);
		int nbItems=menu.size();
		menuCon.setDefaultBackground(guiBackground);
		menuCon.setDefaultForeground(guiText);
		menuCon.printFrame(0,0,20,nbItems+4,true,TCOD_BKGND_SET,"Help menu");
		int y=2, itemNum=0;
		for (TutorialPageId *it=menu.begin(); it != menu.end(); it++, itemNum++) {
			if (*it == id) {
				menuCon.setDefaultForeground(TCODColor::white);
				menuCon.setDefaultBackground(guiHighlightedBackground);
			}
			else {
				menuCon.setDefaultForeground(guiText);
				menuCon.setDefaultBackground(guiBackground);
			}
			menuCon.rect(2,y,16,1,false,TCOD_BKGND_SET);
			menuCon.print(2,y++,pages[*it].name);
		}
		if ( fadeOutDelay > 0.0f ) {
			float coef = fadeOutDelay/FADE_TIME;
			blitSemiTransparent(&menuCon,0,0,0,nbItems+4,TCODConsole::root,0,2,coef*0.8f,coef);
		} else {
			blitSemiTransparent(&menuCon,0,0,0,nbItems+4,TCODConsole::root,0,2,0.8f,1.0f);
		}
	}
	if ( id != TUTO_NONE ) {
		if ( fadeOutDelay > 0.0f ) {
			float coef=(FADE_TIME-fadeOutDelay)/FADE_TIME;
			int curx = (int)(x + (CON_W-10-x) * coef);
			int cury = (int)(y + (CON_H-3-y) * coef);
			int curw = (int)(w + (10-w) * coef);
			int curh = (int)(h + (1-h) * coef);
			TCODConsole::root->setDefaultForeground(guiText);
			TCODConsole::root->printFrame(curx,cury,curw,curh,false,TCOD_BKGND_NONE,NULL);
		} else {
			if ( pages[id].x == -1 || renderMenu)
				blitSemiTransparent(pages[id].con,0,0,0,0,TCODConsole::root,CON_W-10-pages[id].con->getWidth(),2,0.8f,1.0f);
			else
				blitSemiTransparent(pages[id].con,0,0,0,0,TCODConsole::root,pages[id].x,pages[id].y,0.8f,1.0f);
		}
	}
}

bool Tutorial::update(float elapsed, TCOD_key_t k,TCOD_mouse_t mouse) {
	if ( blinkDelay > 0.0f ) blinkDelay -= elapsed;
	if ( id != TUTO_NONE ) {
		if (! gameEngine->isGamePaused()) tutoElapsed += elapsed;
		if ( fadeOutDelay > 0.0f ) {
			fadeOutDelay -= elapsed;
			if ( fadeOutDelay <= 0.0f) {
				id = TUTO_NONE;
				blinkDelay=BLINK_TIME;
				renderMenu=false;
			}
		} else {

			if ( mouse.cx >= 2 && mouse.cx <= 18 && mouse.cy >= 4 && mouse.cy < 4+menu.size() && (mouse.dx != 0 || mouse.dy != 0 )) {
				selectedItem = mouse.cy-4;
				id=menu.get(selectedItem);
				setNewPage(id);
			}
			if ( mouse.cx == x + w - 1 && mouse.cy == y ) {
				pages[id].con->setCharForeground(w-1,0,TCODColor::lighterYellow);
			} else {
				pages[id].con->setCharForeground(w-1,0,TCODColor::lightGrey);
			}
			if ( (mouse.lbutton_pressed && mouse.cx == x + w - 1 && mouse.cy == y )
				|| (!k.pressed && (k.c == '?' || k.c ==' ' || k.vk == TCODK_ESCAPE )) || tutoElapsed > pages[id].delay ) {
				if (renderMenu) gameEngine->gui.setMode(GUI_NONE);
				else if ( k.c != '?') closeLiveTuto();
				else gameEngine->gui.setMode(GUI_TUTORIAL);
			}
			if ( renderMenu && menu.size() > 1 && fadeOutDelay == 0.0f ) {
				bool up=false,down=false,left=false,right=false;
				Player::getMoveKey(k,&up,&down,&left,&right);
				if ( up || left ) {
					selectedItem--;
					if ( selectedItem < 0 ) {
						selectedItem = menu.size()-1;
					}
					tutoElapsed=0.0f;
				} else if ( down || right ) {
					selectedItem = (selectedItem+1)%menu.size();
					tutoElapsed=0.0f;
				}
				id=menu.get(selectedItem);
				setNewPage(id);
			}
		}
	} else if ( k.c == '?' && ! k.pressed ) {
		gameEngine->gui.setMode(GUI_TUTORIAL);
	}
	return true;
}

void Tutorial::enableMenuPage(TutorialPageId id) {
	if (! pages[id].con ) createPage(id);
	alreadyStarted[id]=true;
}

void Tutorial::disableMenuPage(TutorialPageId id) {
	alreadyStarted[id]=false;
	pages[id].inMenu=false;

}
void Tutorial::setNewPage(TutorialPageId newId) {
	enableMenuPage(newId);
	if ( pages[newId].x == -1 ) {
		x = CON_W-10-pages[newId].con->getWidth();
		y = 2;
	} else {
		x=pages[newId].x;
		y=pages[newId].y;
	}
	w = pages[newId].con->getWidth();
	h = pages[newId].con->getHeight();
	if ( pages[newId].inMenu ) lastPage=newId;
	id=newId;
}

void Tutorial::startLiveTuto(TutorialPageId newId) {
	if ( alreadyStarted[newId] ) return;
	if ( id != TUTO_NONE ) {
		toStart.push(newId);
	} else {
		startTuto(newId);
	}
}

void Tutorial::startTuto(TutorialPageId newId) {
	tutoElapsed=0.0f;
	fadeOutDelay=0.0f;
	setNewPage(newId);
	alreadyStarted[newId]=true;
}

void Tutorial::closeLiveTuto() {
	if ( id != TUTO_NONE ) {
		fadeOutDelay=FADE_TIME;
		if ( renderMenu ) gameEngine->resumeGame();
	}
}

void Tutorial::onInitialise() {
}

#define TUTO_CHUNK_VERSION 4
void Tutorial::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(TUTO_CHUNK_ID,TUTO_CHUNK_VERSION);
	zip->putData(sizeof(bool)*sizeof(alreadyStarted),alreadyStarted);
	zip->putInt(lastPage);
	zip->putInt(selectedItem);
}

bool Tutorial::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != TUTO_CHUNK_VERSION ) return false;
	zip->getData(sizeof(bool)*sizeof(alreadyStarted),alreadyStarted);
	lastPage=(TutorialPageId)zip->getInt();
	selectedItem=zip->getInt();
	return true;
}

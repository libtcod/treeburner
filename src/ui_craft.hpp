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

class Craft  : public Dialog, public UIListener, public Scrollable {
public :
	Craft();
	void initialize(Creature *owner, bool soft=false);
	void render();
	bool update(float elapsed, TCOD_key_t &k, TCOD_mouse_t &mouse);

	// UIListener
	bool onWidgetEvent(Widget *widget, EWidgetEvent event);

	// scrollable
	int getScrollTotalSize();
	const char *getScrollText(int idx);
	void getScrollColor(int idx, TCODColor *fore, TCODColor *back);
protected :
	TCODList<Item *> items;
	int selectedItem;
	int selectedIngredient;
	bool selectedTool;
	Scroller *scroller;
	Button clear;
	Button create;
	Creature *owner;
	bool isDragging, isDraggingStart;
	int dragx,dragy, dragStartX, dragStartY;
	Item *dragItem;
	Item *tool;
	TCODList<Item *> ingredients;
	TCODList<ItemCombination *> recipes;
	Item *result;
	ItemCombination *recipe;

	void detectItem(TCOD_mouse_t &mouse);
	void computeResult();
	void computeRecipes();
};

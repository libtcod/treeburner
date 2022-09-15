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
class Objective {
public :
	const char *title;
	const char *description;
	const char *enableScript;
	const char *successScript;
	TCODList<const char *> steps;

	Objective(	const char *title,
		const char *description,
		const char *enableScript = NULL,
		const char *successScript = NULL);
	Script *onEnable;
	Script *onSuccess;
};

class Objectives  : public Dialog, public SaveListener, public Scrollable {
public :
	Objectives();
	TCODList<Objective *> sleeping;
	TCODList<Objective *> active;
	TCODList<Objective *> success;
	TCODList<Objective *> failed;

	TCODList<Objective *> toActivate;
	TCODList<Objective *> toSuccess;
	TCODList<Objective *> toFailure;

	void setOnOff(bool onoff) { showWindow = onoff; }
	void render();
	// check conditions to enable new objectives or finish existing ones
	bool update(float elapsed, TCOD_key_t &k, TCOD_mouse_t &mouse);

	void addObjective(Objective *obj);
	void activateCurrent();
	void activateObjective(const char *title);
	// add a new message in the current objective log
	void addStep(const char *msg, Objective *obj=NULL);
	void closeCurrent(bool success);
	// objective whose script is executed
	Objective *currentObjective;

	// SaveListener
	bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip);
	void saveData(uint32_t chunkId, TCODZip *zip);

    // scrollable
	int getScrollTotalSize();
	const char *getScrollText(int idx);
	void getScrollColor(int idx, TCODColor *fore, TCODColor *back);
protected :
	float timer;
	bool showWindow, firstObj, wasShowingWindow;
	int selected;
	Tabs guiTabs;
	TCODList<Objective *> *currentList;
	Scroller *scroller;
	bool executeObjScript(Objective *obj, Script *script);
};

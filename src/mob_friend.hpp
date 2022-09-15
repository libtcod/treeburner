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

enum FriendAiMode {
	AI_CATCH_ME,
	AI_HIDE_AND_SEEK,
};

class Friend : public Creature {
public :
	Friend();
	bool update(float elapsed);
	float getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;

	// SaveListener
	bool loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip);
	void saveData(uint32 chunkId, TCODZip *zip);

private :
	TextGenerator *talkGenerator;
	float timer;
	bool startPhrase, foodTuto, foodObj;

	// catch me vars
	float lostDelay;

	int awayCount;
	int caught;

	// hide & seek vars
	int counter;
	bool see, tutoPause;
	float sight;

	bool updateCatchMe(float elapsed);
	bool updateHideAndSeek(float elapsed);
	bool updateFollow(float elapsed);
	float getWalkCostCatchMe(int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;
	float getWalkCostHideAndSeek(int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;
	float getWalkCostFollow(int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;
};


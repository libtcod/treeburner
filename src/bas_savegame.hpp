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

#define SAVEGAME_MAGIC_NUMBER 0xFD051E4F

#define GAME_CHUNK_ID 0x47414D45
#define TUTO_CHUNK_ID 0x5455544F
#define CREA_CHUNK_ID 0x43524541
#define DUNG_CHUNK_ID 0x44554E47
#define PLAY_CHUNK_ID 0x504C4159
#define FRIE_CHUNK_ID 0x46524945
#define ITEM_CHUNK_ID 0x4954454D
#define FOOD_CHUNK_ID 0x464F4F44
#define WEAP_CHUNK_ID 0x57454150
#define ARMO_CHUNK_ID 0x41524D4F
#define MISC_CHUNK_ID 0x4D495343
#define HIST_CHUNK_ID 0x48495354
#define CHA1_CHUNK_ID 0x43484131
#define OBJE_CHUNK_ID 0x4F424A45

// an object saved inside a chunk managed by another object
class Persistant {
public :
	virtual bool loadData(TCODZip *zip) = 0;
	virtual void saveData(TCODZip *zip) = 0;
};


// a listener handles saving/loading of an independant chunk of the savegame file
class SaveListener {
public :
	virtual bool loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip) = 0;
	virtual void saveData(uint32 chunkId, TCODZip *zip) = 0;
};

enum SavePhase {
	// only loads what is required for background map generation
	PHASE_INIT,
	// loads the rest
	PHASE_START,
};

class SaveGame : public SaveListener {
public :
	SaveGame();
	virtual ~SaveGame() {}
	uint32 seed; // seed to initialise the random number generator
	int chapter;

	void init(); // reset data when starting a new game
	void clear(); // delete current zip
	void registerListener(uint32 chunkId, SavePhase phase, SaveListener *listener);
	void unregisterListener(SaveListener *listener);
	bool load(SavePhase phase);
	void save();

	// load/save the GAME chunk
	bool loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip);
	void saveData(uint32 chunkId, TCODZip *zip);

	static bool isBigEndian();
	static const char *getChunkId(uint32 chunkId);
	// called when a listener starts to load a new chunk
	void loadChunk(uint32 *chunkId, uint32 *chunkVersion);
	// called when a listener starts to save a new chunk
	void saveChunk(uint32 chunkId, uint32 chunkVersion);

protected :
	struct SaveListenerRecord {
		SaveListenerRecord(uint32 chunkId,SavePhase phase, SaveListener *listener) :
			chunkId(chunkId),phase(phase),listener(listener) {}
		uint32 chunkId;
		SavePhase phase;
		SaveListener *listener;
	};

	// currently loading/saving savegame data and index
	TCODZip *zip, *idx;
	TCODList <SaveListenerRecord *> listeners;
	// used to compute chunk lengths
	TCODList <uint32> startPos;
	// size of the chunks currently saved
	TCODList <uint32> sizes;
	uint32 nbChunks;

};


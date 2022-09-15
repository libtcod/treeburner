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

bool SaveGame::isBigEndian() {
    union {
        uint32_t i;
        char c[4];
    } test = {0x01020304};

    return test.c[0] == 1;
}

const char *SaveGame::getChunkId(uint32_t chunkId) {
	static char s[5];
	union { char c[4];uint32_t i; } un;
	un.i=chunkId;
	if ( isBigEndian() ) {
		s[0]=un.c[0];
		s[1]=un.c[1];
		s[2]=un.c[2];
		s[3]=un.c[3];
		s[4]=0;
	} else {
		s[0]=un.c[3];
		s[1]=un.c[2];
		s[2]=un.c[1];
		s[3]=un.c[0];
		s[4]=0;
	}
	return s;
}

SaveGame::SaveGame() {
	zip = idx = NULL;
	registerListener(GAME_CHUNK_ID,PHASE_INIT,this);
}

void SaveGame::init() {
	seed=(uint32_t)time(NULL);
	// swap high/low bits
	seed = ((seed & 0xFFFF0000)>>16) | ((seed&0xFFFF)<<16);
	chapter=0;
	zip = idx = NULL;
}

void SaveGame::registerListener(uint32_t chunkId, SavePhase phase, SaveListener *listener) {
	listeners.push(new SaveListenerRecord(chunkId,phase,listener));
}

void SaveGame::unregisterListener(SaveListener *listener) {
	for (SaveListenerRecord **it=listeners.begin(); it != listeners.end(); it++) {
		if ( (*it)->listener == listener ) {
			listeners.remove(it);
			break;
		}
	}
}

void SaveGame::clear() {
	if ( zip ) {
		delete zip;
		zip=NULL;
	}
	if ( idx ) {
		delete idx;
		idx=NULL;
	}
}

bool SaveGame::load(SavePhase phase) {
	zip = new TCODZip();
	if (zip->loadFromFile("data/sav/savegame.dat") == 0 ) {
		clear();
		return false;
	}
	idx = new TCODZip();
	if ( idx->loadFromFile("data/sav/savegame.idx") == 0 ) {
		clear();
		return false;
	}
	nbChunks=idx->getInt();
	uint32_t magic = zip->getInt();
	if ( magic != SAVEGAME_MAGIC_NUMBER ) {
		clear();
		return false;
	}
	bool skipToEnd=false;
	while (!skipToEnd && zip->getRemainingBytes() > sizeof(uint32_t)*2) {
		uint32_t chunkId=(uint32_t)zip->getInt();
		uint32_t chunkVersion = (uint32_t) zip->getInt();
		uint32_t chunkSize = (uint32_t)idx->getInt();
		bool listener=false;
		for (SaveListenerRecord **it=listeners.begin(); it != listeners.end(); it++) {
			if ( (*it)->chunkId == chunkId && (*it)->phase == phase ) {
				if ((*it)->listener->loadData(chunkId,chunkVersion,zip)) {
					listener=true;
					if ( phase == PHASE_INIT ) skipToEnd=true;
					break;
				} else {
					fprintf (stderr,"fatal : could not load chunk %s, version %u size %u\n",
						getChunkId(chunkId),chunkVersion,chunkSize);
					delete zip;
					zip=NULL;
					return false;
				}
			}
		}
		/*
		DBG(("%s chunk %s v%u -offset %d size %u\n",
			listener ? "load" : "skip",
			getChunkId(chunkId),chunkVersion,zip->getRemainingBytes(),chunkSize));
			fflush(stdout);
		*/
		if (! listener) {
			zip->skipBytes(chunkSize);
		}
	}
	delete zip;
	zip=NULL;
	return true;
}

void SaveGame::loadChunk(uint32_t *chunkId, uint32_t *chunkVersion) {
	if ( nbChunks == 0 ) {
		clear();
		fprintf(stderr,"error : unexpected end of file...");
		exit(1);
	}
	*chunkId = zip->getInt();
	*chunkVersion = zip->getInt();
	idx->getInt(); // pop chunk size
}

void SaveGame::saveChunk(uint32_t chunkId, uint32_t chunkVersion) {
	if ( startPos.size() > 0 ) {
		sizes.push(zip->getCurrentBytes()-startPos.pop());
	//	DBG(("size : %d\n",sizes.peek()));
	}
	//DBG(("save chunk %s current offset %d ",getChunkId(chunkId),zip->getCurrentBytes()));

	zip->putInt(chunkId);
	zip->putInt(chunkVersion);
	startPos.push(zip->getCurrentBytes());
}

void SaveGame::save() {
	if ( listeners.size() <= 1 ) return;
	clear();
	zip = new TCODZip();
	zip->putInt(SAVEGAME_MAGIC_NUMBER);
	for (SaveListenerRecord **it=listeners.begin(); it != listeners.end(); it++) {
		(*it)->listener->saveData((*it)->chunkId,zip);
	}
	sizes.push(zip->getCurrentBytes()-startPos.pop());
	//DBG(("size : %d\n",sizes.peek()));
	zip->saveToFile("data/sav/savegame.dat");
	idx=new TCODZip();
	idx->putInt(sizes.size());
	for (uint32_t *offset=sizes.begin(); offset!=sizes.end(); offset++) {
		idx->putInt(*offset);
	}
	idx->saveToFile("data/sav/savegame.idx");
}

#define GAME_CHUNK_VERSION 1
void SaveGame::saveData(uint32_t chunkId, TCODZip *zip) {
	saveChunk(chunkId,GAME_CHUNK_VERSION);
	zip->putInt(seed);
	zip->putInt(chapter);
}

bool SaveGame::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion == GAME_CHUNK_VERSION ) {
		seed=zip->getInt();
		chapter=zip->getInt();
		return true;
	}
	return false;
}

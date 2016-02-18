#include <stdio.h>
#include "main.hpp"

Sound sound;

Sound::Sound() : on(false),possible(true), updateCalled(false) {
}

//initializes sound
void Sound::initialize (void) {
#ifndef NO_SOUND
    result = FMOD_System_Create(&fmodsystem);
    if (result == FMOD_OK) result = FMOD_System_Init(fmodsystem, 20, FMOD_INIT_NORMAL, 0);
    if (result != FMOD_OK) {
		possible = false;
		printf ("Warning : could not initialize sound system : %s",FMOD_ErrorString(result));
	}
#else
	possible=false;
#endif
}

//sets the actual playing sound's volume
void Sound::setVolume (float v) {
#ifndef NO_SOUND
	if (possible && on) {
		FMOD_Channel_SetVolume(channel,v);
	}
#endif
}

//loads a soundfile
void Sound::load (const char * filename) {
#ifndef NO_SOUND
	currentSound = (char *)filename;
	if (possible) {
		result = FMOD_System_CreateStream(fmodsystem, currentSound, FMOD_DEFAULT, 0, &snd);
		if (result != FMOD_OK) {
			printf ("Warning : could not load sound %s : %s",filename,FMOD_ErrorString(result));
			possible = false;
		}
	}
#endif
}

//frees the sound object
void Sound::unload (void) {
#ifndef NO_SOUND
	if (possible) {
		result = FMOD_Sound_Release(snd);
	}
#endif
}

//plays a sound (no argument to leave pause as dafault)
void Sound::play() {
#ifndef NO_SOUND
	if (possible) {
		result = FMOD_System_PlaySound(fmodsystem, FMOD_CHANNEL_FREE, snd, false, &channel);
		on=true;
		//FMOD_Channel_SetMode(channel,FMOD_LOOP_NORMAL);
	}
#endif
}

void Sound::playLoop() {
#ifndef NO_SOUND
	if (possible) {
		play();
		FMOD_Channel_SetMode(channel,FMOD_LOOP_NORMAL);
	}
#endif
}

//pause or unpause the sound
void Sound::setPause (bool pause) {
#ifndef NO_SOUND
	if (possible) {
		FMOD_Channel_SetPaused(channel,pause);
	}
#endif
}

void Sound::update() {
#ifndef NO_SOUND
	if (possible && ! updateCalled ) {
		FMOD_System_Update(fmodsystem);
		updateCalled=true;
	}
#endif
}

void Sound::endFrame() {
	updateCalled=false;
}

//toggle pause on and off
void Sound::togglePause (void) {
#ifndef NO_SOUND
   	if (possible) {
		FMOD_BOOL p;
		FMOD_Channel_GetPaused(channel,&p);
		FMOD_Channel_SetPaused(channel,!p);
	}
#endif
}

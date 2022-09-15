class Sound {
private:
    bool on;
    bool possible;
    char * currentSound; //currently played sound
	bool updateCalled; // update called this frame
#ifndef NO_SOUND
	FMOD_RESULT result;
	FMOD_SYSTEM *fmodsystem;
	FMOD_SOUND *snd;
	FMOD_CHANNEL *channel;
#endif
public:
    Sound ();
	void initialize ();

	//sound control
	void setVolume(float v);
	void load(const char * filename);
	void unload();
	void play();
	void playLoop();
	void update();
	void endFrame();

    //setters
    void setPause(bool pause);

    //toggles
    void togglePause(void);
};



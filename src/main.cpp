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

#include <stdio.h>
#include <time.h>

#include "screen_end.hpp"
#include "screen_forest.hpp"
#include "screen_game.hpp"
#include "screen_mainmenu.hpp"
#include "screen_treeBurner.hpp"
#include "util_powerup.hpp"

TCODNoise noise1d(1);
TCODNoise noise2d(2);
TCODNoise noise3d(3);
TCODRandom* rng = nullptr;
bool mouseControl = false;
bool newGame = false;
bas::SaveGame saveGame;
bas::UserPref userPref;
UmbraEngine engine("./data/cfg/umbra.txt", UMBRA_REGISTER_ALL);
TCODImage background("./data/img/background.png");
TCODParser config;

HDRColor getHDRColorProperty(const TCODParser& parser, const char* name) {
  TCODList<float> l(parser.getListProperty(name, TCOD_TYPE_FLOAT));
  return HDRColor(l.get(0), l.get(1), l.get(2));
}

class ModuleFactory : public UmbraModuleFactory {
 public:
  UmbraModule* createModule(const char* name) {
    // the cave modules
    if (strcmp(name, "mainMenu") == 0)
      return new MainMenu();
    else if (strcmp(name, "chapter1") == 0)
      return new ForestScreen();
    else if (strcmp(name, "chapter1Story") == 0)
      return new PaperScreen("data/cfg/chapter1.txg", "title", "text", 1);
    // pyromancer modules
    else if (strcmp(name, "pyroTitle") == 0)
      return new EndScreen(
          "There was a time when you were one of the most brilliant students"
          " in the pyromancy school of Doryen. Most of your power was concentrated in a single artefact :"
          " an amulet. Yeah, yeah. Amulets are soo old school, but your master never allowed you"
          " to create one of those fancy pyromancer rings. The old bag...\n\n"
          "Anyway, a jealous student named Zeepoh managed to take the amulet away from you."
          " He is said to hide deep in a dungeon, protected by a horde of minions."
          " God knows how he managed to learn this summoning spell. He really sucked at magic when he"
          " was still attending courses.\n\n"
          "The thing is, you have this date with Alena tomorrow. Not only is she your pyromancy master's daughter,"
          " but she's also hot like hell, and it would be very bad if you couldn't use your"
          " amulet to impress her...\n\nLooks like you'll have to dive in the dungeon and kick some arses to get"
          " your stuff back...",
          1.0f,
          false);
    else if (strcmp(name, "pyroGame") == 0)
      return new Game();
    else if (strcmp(name, "pyroVictory") == 0)
      return new EndScreen(
          "Congratulations, you won!\n\nMore important, you'll be able to show off with Alena tomorrow ! But this is "
          "another story...");
    else if (strcmp(name, "pyroGameOver") == 0)
      return new EndScreen(
          "You're dead...\n\nEven worse, Zeepoh will probably go with Alena now that you turned into a pile of "
          "ashes...");
    // treeBurner modules
    else if (strcmp(name, "treeBurnerTitle") == 0)
      return new EndScreen(
          TCODRandom::getInstance()->getInt(0, 1)
              ? "Hate. Unending, undying hate. That's all you can feel. "
                "The deepest loathing imaginable, nothing more than an intense desire to kill them all. "
                "You will find them, and they will die. Their homes will burn, and their flesh will "
                "sizzle under the crackling flames. Every last trace of their existence will be removed "
                "from this world."
              : "A deep, undying hatred pulses through your veins. You feel the darkness running through you, "
                "and all you can think about is killing them, wiping their existence from this world. Kill them. "
                "Kill them all.",
          1.0f,
          false);
    else if (strcmp(name, "treeBurnerVictory") == 0)
      return new EndScreen(
          TCODRandom::getInstance()->getInt(0, 1)
              ? "You wake up, drenched in a cold sweat. The cave is dark, and the still underground lake "
                "lays at your feet. Ilia is laying next to you, still fast asleep. She doesn't even stir. "
                "Good. The last thing you wanted to do was awaken her.\n\n"
                "Staring at her peaceful face, delicately framed by locks of her golden hair, you start to "
                "feel a little better. The dream was horrible, made even more terrifying by your revolting "
                "actions, and the sick joy you felt, using the destructive power of the arts of pyromancy.\n\n"
                "The campfire still burns quietly at your side. You must have only slept for a short time. "
                "It'd be a good idea to get some more rest. Long days lay ahead of you. Who knows how far "
                "and how deep you'll have to descend into the eternal darkness before you find Aidan... "
                "or what's left of him."
              : "You awaken. The darkness of the cave surrounds you, with only the dim glow of the dying "
                "campfire to be seen, casting a glow on the still underground lake by your feet. The water "
                "is still, except for the occasional ripple caused by the plinking drops of water rolling "
                "down the ceiling.\n\n"
                "Ilia is still sleeping by your side, and the look of her serene face calms you. Her delicate "
                "features are neatly framed by the locks of her hair, painting an image of peace in your mind. "
                "You need every bit of help you can get, to wipe the terrible memory of the dream from your mind.\n\n"
                "The surge of power you felt, and the sick joy of killing all those helpless women and children "
                "leaves you feeling drained and exhausted. You know that you should probably go back to sleep, "
                "but fear of the nightmare recurring keeps your eyes open wide.\n\n"
                "The long journey still lays ahead of you, and you have no idea how far and how deep you'll "
                "need to travel before finding your lost friend. Aidan needs your help, and you must find him. "
                "Or at least, what's left of him...",
          1.0f,
          false);
    else if (strcmp(name, "treeBurnerGame") == 0)
      return new TreeBurner();
    else if (strcmp(name, "treeBurnerGameOver") == 0)
      return new EndScreen(
          TCODRandom::getInstance()->getInt(0, 1)
              ? "You bolt upright, screaming. The sound of your voice echoes in the darkness of the cave. "
                "You can see nothing, but you hear Ilia stirring. Her hand instinctively reaches for her "
                "Azuran dagger, and she turns to you.\n\n"
                "\"Are you alright?\"\n"
                "You shake your head. \"It was just a dream, but it was awful. I was this powerful pyromancer, "
                "and I was using my powers in such terrible ways. Killing people, by the dozens. Women... kids... "
                "all the same.\"\n"
                "\"Don't worry about it. Like you said, it was just a dream. Go back to sleep, we have a long "
                "day ahead of us.\"\n"
                "\"Yeah, I suppose. Sorry for waking you, but it just felt so real.\" You lay back down, and pull "
                "the blanket snug around you. Only the wind whistling through the cave and the steady plink of "
                "water can be heard. You focus on the sound, unable to get back to sleep. All you can think about "
                "is the dream. That terrible nightmare still leaves you shaking..."
              : "You awaken to the sound of your own terrified screams. The darkness of the cave is complete, "
                "and you can't even see your hands in front of your face. You can't see, but you can hear Ilia "
                "bolting upright, and the sound of her drawing her Azuran dagger echoes loudly in the expansive "
                "cavern.\n\n"
                "\"What's wrong?\" Her voice is tense, and you feel terrible for waking her and startling her so "
                "severely.\n"
                "\"Nothing. Don't worry about it, it was just a nightmare.\"\n"
                "She breathes a sigh of relief. \"Silly boy, don't let a dream cause you so much stress.\"\n"
                "\"I know, but it just seemed so real. I was this powerful pyromancer, and I was killing everything "
                "in my sight. Women... and children... everyone.\"\n"
                "She laughs. \"You? A mighty pyromancer? It must have been a dream. I've yet to see you produce "
                "more than a candle's flicker of light, much less a massive fireball. Go back to sleep, we have a "
                "long way to go before we find Aidan.\"\n"
                "\"You're right. Sorry for waking you, it's just... I don't know.\" You lay back down, and close "
                "your eyes, but your heart is still pounding rapidly in your chest. You know that sleep won't come "
                "easily. Not after the dream...",
          1.0f,
          false);
    else if (strcmp(name, "treeBurnerCredits") == 0)
      return new EndScreen(
          "Code - Jice\n\n\n"
          "Words - Tim Pruett\n\n\n\n\n"
          "libtcod 1.5.1wip\n\n\n"
          "umbra 10.11wip",
          1.0f,
          true);
    else
      return NULL;
  }
};

int main(int argc, char* argv[]) {
  // read main configuration file
  config.run("data/cfg/config.txt", NULL);
  ConditionType::init();
  TextGenerator::setGlobalFunction("NUMBER_TO_LETTER", new NumberToLetterFunc());

  // load user preferences (mouse control mode, ...)
  userPref.load();
  Powerup::init();

  threadPool = new ThreadPool();

  // initialise random number generator
  if (!saveGame.load(bas::PHASE_INIT)) {
    newGame = true;
    saveGame.init();
    if (config.getBoolProperty("config.debug") && argc == 2) {
      // use a user-defined seed for RNG
      saveGame.seed = (uint32_t)atoi(argv[1]);
    }
  }
  if (config.getBoolProperty("config.debug")) {
    printf("Random seed : %d\n", saveGame.seed);
  }
  userPref.nbLaunches++;
  rng = new TCODRandom(saveGame.seed, TCOD_RNG_CMWC);

  engine.loadModuleConfiguration("data/cfg/modules.cfg", new ModuleFactory());

  sound.initialize();
  if (engine.initialise(TCOD_RENDERER_SDL2)) {
    engine.run();
    // saveGame.save();
    userPref.save();
    return 0;
  }

  return 1;
}

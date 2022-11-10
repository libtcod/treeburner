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
#include "mob/friend.hpp"

#include <stdio.h>

#include "main.hpp"

namespace mob {
#define SECURE_DIST 12
#define SECURE_COEF 3.0f
#define NB_CAUGHT 25

Friend::Friend() : Creature(), timer(0.0f), startPhrase(false), lostDelay(-5.0f) {
  name_ = "Aidan";
  util::TextGenerator::addGlobalValue("FRIEND_NAME", name_.c_str());
  ch_ = '@';
  color_ = TCODColor(210, 210, 255);
  life_ = 100;
  speed_ = 12.0f;
  type_ = CREATURE_FRIEND;
  talkGenerator = new util::TextGenerator("data/cfg/dialog_chap1.txg");
  awayCount = 0;
  talk_text_.delay = -2.0f;
  caught = 0;
  height_ = 1.2f;
  see = tutoPause = false;
  foodTuto = foodObj = false;
  flags_ = CREATURE_OFFSCREEN | CREATURE_SAVE;
}

bool Friend::update(float elapsed) {
  if (!current_behavior_) {
    FollowBehavior* behavior = new FollowBehavior(new AvoidWaterWalkPattern());
    behavior->setLeader(&gameEngine->player);
    current_behavior_ = behavior;
  }
  timer += elapsed;
  if (!foodTuto && timer > 3.0f) {
    foodTuto = true;
    if (userPref.nbLaunches == 1) {
      gameEngine->gui.tutorial.startLiveTuto(ui::TUTO_FOOD);
      timer = -30.0f;
    } else
      timer = 0.0f;
  } else if (
      foodTuto && timer > 4.0f && talk_text_.delay == 0.0f && !startPhrase &&
      gameEngine->player.isInRange((int)x_, (int)y_)) {
    talk(talkGenerator->generate("friend", "${HUNGRY}"));
    startPhrase = true;
    timer = 0.0f;
  } else if (startPhrase && foodTuto && timer > 5.0f && !foodObj) {
    foodObj = true;
    ui::Objective* obj = new ui::Objective(
        "Find some food",
        "You have been playing with your friend the whole afternoon and you're quite hungry. Try to find some food.",
        NULL,
        "local currentBest=getBestCreatureFoodHP('player') "
        "if best == nil then best = 0 end "
        "if currentBest > best then "
        "   local ruleName='${FOUND_FOOD_' .. getBestCreatureFoodName('player') .. '}'"
        "   creatureTalk('Uban',textGen('data/cfg/dialog_chap1.txg','friend',ruleName) )"
        "   if currentBest >= 10 then closeObjective(true) end "
        "   addObjectiveStep(textGen('data/cfg/dialog_chap1.txg','objsteps',ruleName)) "
        "   best=currentBest "
        "end ");
    gameEngine->gui.objectives.addObjective(obj);
  }
  return Creature::update(elapsed);
}

/*
bool Friend::updateHideAndSeek(float elapsed) {
        if ( ! startPhrase && timer > 3.0f ) {
                talk(talkGenerator->generate("friend","${HIDE_AND_SEEK}"));
                startPhrase=true;
                counter=0;
                timer=0.0f;
        } else if ( startPhrase && timer-3 > counter && counter < 10 ) {
                if ( tutoPause && ! gameEngine->isGamePaused() ) {
                        tutoPause=false;
                        timer=3.0f;
                }
                talk(talkGenerator->generate("friend"," ${NUMBER_TO_LETTER(%d)} ! ",10-counter));
                counter++;
                if ( counter == 3 ) {
                        tutoPause=true;
                        gameEngine->pauseGame();
                        gameEngine->gui.tutorial->startLiveTuto(ui::TUTO_HIDE_SEEK);
                }
        } else if ( timer > 10.0f ) {
                Player *player=&gameEngine->player;
                int pdist=(int)distance(*player);
                map::Dungeon *dungeon=gameEngine->dungeon;
                standDelay+=elapsed;
                if ( ( pdist > 2*SECURE_DIST || standDelay > 10.0f ) && (! path || path->isEmpty()) ) {
                                // go near the player
                                int destx = (int)(player->x +
TCODRandom::getInstance()->getInt(-SECURE_DIST*2,SECURE_DIST*2)); int desty = (int)(player->y +
TCODRandom::getInstance()->getInt(-SECURE_DIST*2,SECURE_DIST*2)); destx=std::clamp(destx,0,dungeon->size-1);
                                desty=std::clamp(desty,0,dungeon->size-1);
                                dungeon->getClosestWalkable(&destx,&desty,true,true);
                                if (! path) {
                                        path=new TCODPath(dungeon->size,dungeon->size,this,NULL);
                                }
                                path->compute((int)x,(int)y,destx,desty);
                                pathTimer=0.0f;
                } else {
                        if (walk(elapsed)) {
                                standDelay=0.0f;
                        }
                        bool inFov = dungeon->map->isInFov((int)x,(int)y);
                        if ( walk_timer_ == 0.0f && inFov ) {
                                float sightTest=TCODRandom::getInstance()->getFloat(0.5f,0.9f);
                                if ( sightTest < player->stealth ) {
                                        player->stealth += 2 * (player->stealth - sightTest) * elapsed;
                                        player->stealth=std::min(1.0f,player->stealth);
                                        if ( !see && player->stealth >= 1.0f ) {
                                                talk(talkGenerator->generate("friend","${HS_SPOTTED}"));
                                                see=true;
                                        }
                                }
                        }
                        if (! inFov && see ) {
                                see=false;
                                standDelay=0.0f;
                        }
                        if (! see && talk_text_.delay == 0.0f && standDelay > 5.0f ) {
                                talk(talkGenerator->generate("friend","${HS_TEASE}"));
                        }
                }
        }
        return true;
}

bool Friend::updateCatchMe(float elapsed) {
        map::Dungeon *dungeon=gameEngine->dungeon;
        bool inFov = dungeon->map->isInFov((int)x,(int)y);
        Player *player=&gameEngine->player;

        pathTimer+=elapsed;
        standDelay+=elapsed;
        if ( talk_text_.delay == 0.0f && ! startPhrase ) {
                talk(talkGenerator->generate("friend","${CATCH_ME}"));
                startPhrase=true;
                gameEngine->gui.tutorial->startLiveTuto(TUTO_CATCH_ME);
        } else {
                if (!inFov) {
                        lostDelay += elapsed;
                        if (talk_text_.delay == 0.0f) {
                                if ( awayCount > 4 ) {
                                        talk(talkGenerator->generate("friend","${CATCH_AWAY}"));
                                        awayCount=-5;
                                } else if ( lostDelay > 0.0f ) {
                                        talk(talkGenerator->generate("friend","${CATCH_LOST}"));
                                        lostDelay=-10.0f;
                                }
                        }
                } else {
                        lostDelay = -5.0f;
                }
                if (talk_text_.delay == 0.0f && standDelay > 10.0f ) {
                        talk(talkGenerator->generate("friend","${CATCH_COMEON}"));
                        standDelay=0.0f;
                }
        }
        if( caught >= NB_CAUGHT ) {
                talk(talkGenerator->generate("friend","${CATCH_CAUGHT}"));
                aiMode = AI_FOLLOW;
                timer=0.0f;
                startPhrase=false;
                return true;
        }
        // friend can see the player. adapt his distance
        int pdist=(int)distance(*player);
        if ( startPhrase && timer > 4.0f ) {
                if ( pdist < 2 && inFov ) caught++;
                if ( caught < NB_CAUGHT && talk_text_.delay == 0.0f && pdist < 4 ) {
                        talk(talkGenerator->generate("friend","${CATCH_ALMOST}"));
                }
        }
        if ( ( (inFov && pdist < SECURE_DIST) || pdist > 2*SECURE_DIST )
                && (! path || path->isEmpty()) ) {
                        // stay away from player
                        int destx,desty;
                        float dx = x - player->x;
                        float dy = y - player->y;
                        if ( pdist > 0 ) {
                                if ( pdist < SECURE_DIST ) {
                                        // get away from player
                                        destx = (int)(x + dx*SECURE_DIST/pdist);
                                        desty = (int)(y + dy*SECURE_DIST/pdist);
                                        awayCount=0;
                                } else {
                                        // get closer to player
                                        destx = (int)(player->x + dx * SECURE_DIST/pdist);
                                        desty = (int)(player->y + dy * SECURE_DIST/pdist);
                                        awayCount++;
                                }
                        } else {
                                destx=TCODRandom::getInstance()->getInt((int)x-20,(int)x+20);
                                desty=TCODRandom::getInstance()->getInt((int)y-20,(int)y+20);
                        }
                        destx=std::clamp(destx,0,dungeon->size-1);
                        desty=std::clamp(desty,0,dungeon->size-1);
                        dungeon->getClosestWalkable(&destx,&desty,true,true);
                        if (! path) {
                                path=new TCODPath(dungeon->size,dungeon->size,this,NULL);
                        }
                        path->compute((int)x,(int)y,destx,desty);
                        pathTimer=0.0f;
        } else {
                if (walk(elapsed)) standDelay=0.0f;
        }
        return true;
}

float Friend::getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
        switch(aiMode) {
                case AI_CATCH_ME : return getWalkCostCatchMe(xFrom,yFrom,xTo,yTo,userData); break;
                case AI_HIDE_AND_SEEK : return getWalkCostHideAndSeek(xFrom,yFrom,xTo,yTo,userData); break;
                case AI_FOLLOW : return getWalkCostFollow(xFrom,yFrom,xTo,yTo,userData); break;
        }
        return 1.0f;
}

float Friend::getWalkCostCatchMe(int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
        map::Dungeon *dungeon=gameEngine->dungeon;
        if ( !dungeon->map->isWalkable(xTo,yTo)) return 0.0f;
        if ( ignore_creatures_ ) return map::terrainTypes[dungeon->getTerrainType(xTo,yTo)].walkCost;
        Player *player=&gameEngine->player;
        float pdist=squaredDistance(*player);
        if ( pdist < 16.0f ) return 1.0f + SECURE_COEF*(SECURE_DIST - pdist);
        return 1.0f;
}

float Friend::getWalkCostHideAndSeek(int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
        map::Dungeon *dungeon=gameEngine->dungeon;
        if ( !dungeon->map->isWalkable(xTo,yTo)) return 0.0f;
        if ( ignore_creatures_ ) return map::terrainTypes[dungeon->getTerrainType(xTo,yTo)].walkCost;
        Player *player=&gameEngine->player;
        float pdist=squaredDistance(*player);
        if ( pdist < 16.0f ) return 1.0f + SECURE_COEF*(SECURE_DIST - pdist);
        return 1.0f;
}
*/
float Friend::getWalkCost(int xFrom, int yFrom, int xTo, int yTo, void* userData) const {
  map::Dungeon* dungeon = gameEngine->dungeon;
  if (!dungeon->map->isWalkable(xTo, yTo)) return 0.0f;
  float cost = map::terrainTypes[dungeon->getTerrainType(xTo, yTo)].walkCost;
  if (dungeon->hasWater(xTo * 2, yTo * 2)) cost *= 3;  // try to avoid getting wet!
  return cost;
}

#define FRIE_CHUNK_VERSION 3
void Friend::saveData(uint32_t chunkId, TCODZip* zip) {
  Creature::saveData(chunkId, zip);
  // friend specific data
  saveGame.saveChunk(FRIE_CHUNK_ID, FRIE_CHUNK_VERSION);
  zip->putChar(startPhrase ? 1 : 0);
  zip->putChar(foodTuto ? 1 : 0);
  zip->putChar(foodObj ? 1 : 0);
  // zip->putInt(aiMode);
}

bool Friend::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (!Creature::loadData(chunkId, chunkVersion, zip)) return false;

  saveGame.loadChunk(&chunkId, &chunkVersion);
  if (chunkId != FRIE_CHUNK_ID) return false;
  if (chunkVersion != FRIE_CHUNK_VERSION) return false;
  // load friend specific data
  startPhrase = (zip->getChar() == 1);
  foodTuto = (zip->getChar() == 1);
  foodObj = (zip->getChar() == 1);
  // aiMode=(FriendAiMode)zip->getInt();
  return true;
}
}  // namespace mob

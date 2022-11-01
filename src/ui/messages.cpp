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
#include "ui/messages.hpp"

#include <fmt/core.h>
#include <stdio.h>

#include "constants.hpp"
#include "main.hpp"
#include "util/subcell.hpp"

namespace ui {
#define NB_LINES 10
#define HISTORY_SIZE 256
#define LOG_WIDTH (CON_W - 12)

static TCODColor sevColor[NB_SEVERITIES];

Logger::Logger() {
  con = new TCODConsole(CON_W - 12, CON_H);
  con->setDefaultBackground(ui::guiBackground);
  con->clear();
  sevColor[DEBUG] = config.getColorProperty("config.display.debugColor");
  sevColor[INFO] = config.getColorProperty("config.display.infoColor");
  sevColor[WARN] = config.getColorProperty("config.display.warnColor");
  sevColor[CRITICAL] = config.getColorProperty("config.display.criticalColor");
  nbActive = 0;
  flags = ui::DIALOG_MAXIMIZABLE | ui::DIALOG_MULTIPOS | ui::DIALOG_DRAGGABLE;
  // focus=drag=false;
  // offset=0;
  possiblePos.push(new UmbraRect(0, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(0, 0, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(12, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));

  possiblePos.push(new UmbraRect(0, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(0, 0, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(12, 0, LOG_WIDTH, NB_LINES + 1));

  possiblePos.push(new UmbraRect(12, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(12, 0, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(0, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));

  possiblePos.push(new UmbraRect(12, CON_H - NB_LINES - 1, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(12, 0, LOG_WIDTH, NB_LINES + 1));
  possiblePos.push(new UmbraRect(0, 0, LOG_WIDTH, NB_LINES + 1));
  minimizedRect = *possiblePos.get(0);
  minimizedRect.x = userPref.logx;
  minimizedRect.y = userPref.logy;
  maximizedRect = minimizedRect;
  maximizedRect.y = 0;
  maximizedRect.w = LOG_WIDTH;
  maximizedRect.h = CON_H;
  setMinimized();
  titleBarAlpha = 0.0f;
  lookOn = false;
  saveGame.registerListener(HIST_CHUNK_ID, base::PHASE_START, this);
  scroller = new ui::Scroller(this, LOG_WIDTH, CON_H - 1, true);
}

int Logger::getScrollTotalSize() { return messages.size(); }

const std::string& Logger::getScrollText(int idx) { return messages.at(idx).txt; }

void Logger::getScrollColor(int idx, TCODColor* fore, TCODColor* back) {
  *fore = sevColor[messages.at(idx).severity];
  *back = ui::guiBackground;
}

void Logger::render() {
  // render messages
  int y = 0;
  if (!isMinimized) {
    scroller->render(con, 0, 1);
    scroller->renderScrollbar(con, 0, 1);
    util::blitSemiTransparent(
        con, 0, 0, maximizedRect.w, maximizedRect.h, TCODConsole::root, maximizedRect.x, maximizedRect.y, 0.8f, 1.0f);
    /*
    int nbMessages = messages.size();
    int nbDisplayed = MIN(CON_H-1,nbMessages-offset);
    if ( nbMessages > 0 ) {
            y = CON_H-nbDisplayed;
            util::blitTransparent(con,0,0,maximizedRect.w,maximizedRect.h,TCODConsole::root,maximizedRect.x,maximizedRect.y);
            // scrollbar
            if ( nbDisplayed < nbMessages ) {
                    int firstDisplayed = nbMessages - offset - nbDisplayed;
                    int start = ((CON_H-1) * firstDisplayed)/nbMessages;
                    int end = (CON_H * (firstDisplayed + nbDisplayed))/nbMessages;
                    end=MIN(CON_H-1,end);
                    if ( start > 0 ) util::darken(maximizedRect.x+LOG_WIDTH-1,0,1,start,0.5f);
                    if ( end+1 < CON_H ) util::darken(maximizedRect.x+LOG_WIDTH-1,end+1,1,CON_H-1-end,0.5f);
                    util::lighten(maximizedRect.x+LOG_WIDTH-2,start,2,end-start+1,focus || drag ? 0.5f : 0.25f);
            }
    }
    */
  } else if (titleBarAlpha > 0.0f) {
    util::blitSemiTransparent(
        con, 0, 0, rect.w, rect.h, TCODConsole::root, rect.x, rect.y, 0.8f * titleBarAlpha, titleBarAlpha);
  } else if (nbActive > 0) {
    int ry = 0, dy = 0, count = 0;
    auto it = messages.end();
    if (rect.y == 0) {
      y = 0;
      count = MIN(rect.h, nbActive);
      it = messages.end() - 1;
      dy = -1;
      ry = 0;
    } else {
      y = MAX(0, rect.h - nbActive);
      ry = rect.y + y;
      count = rect.h - y;
      dy = 1;
      it = messages.end() - count;
    }
    for (; count > 0; it += dy, y++, ry++, count--) {
      float timer = it->timer;
      if (timer >= 0.5f)
        timer = 1.0f;
      else
        timer = timer * 2;
      util::blitSemiTransparent(con, 0, y, LOG_WIDTH, 1, TCODConsole::root, rect.x, ry, 0.8f * timer, timer);
    }
  }
  if (titleBarAlpha > 0.0f) renderFrame(titleBarAlpha, "Message log");
}

bool Logger::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) {
  static float messageLife = config.getFloatProperty("config.display.messageLife");

  // update messages
  while (messages.size() > HISTORY_SIZE) messages.pop_front();

  if (!gameEngine->isGamePaused()) {
    for (auto it = messages.end() - nbActive; it != messages.end(); ++it) {
      it->timer -= elapsed / messageLife;
      if (it->timer < 0.0f) {
        nbActive--;
      }
    }
  }
  if (!k.pressed && (toupper(k.c) == 'M' || (!isMinimized && k.c == ' ') || (!isMinimized && k.vk == TCODK_ESCAPE)) &&
      messages.size() > 0) {
    if (isMinimized)
      setMaximized();
    else
      setMinimized();
  }
  if (!isMinimized) {
    if (!gameEngine->isGamePaused()) {
      gameEngine->pauseGame();
      scroller->initDrag();
    }
  }
  if (k.vk == TCODK_ALT || k.lalt) lookOn = k.pressed;

  if (!isMinimized) {
    scroller->update(elapsed, k, mouse, maximizedRect.x, maximizedRect.y + 1);
  }
  con->clear();

  if (!isMinimized || (rect.mouseHover && !lookOn && !gameEngine->isGamePaused())) {
    titleBarAlpha += elapsed;
    titleBarAlpha = MIN(1.0f, titleBarAlpha);
  } else if (!isDragging) {
    titleBarAlpha -= elapsed;
    titleBarAlpha = MAX(0.0f, titleBarAlpha);
  }

  int y = 0, count = 0, dy = 0;

  if (!isMinimized) {
    // y = MAX(1,rect.h-messages.size()-offset);
    // count=rect.h-y;
  } else if (titleBarAlpha > 0.0f) {
    if (rect.y == 0) {
      y = 1;
      count = MIN(rect.h, messages.size());
    } else {
      y = MAX(1, rect.h - messages.size());
      count = rect.h - y;
    }
  } else {
    if (rect.y == 0) {
      y = 0;
      count = MIN(rect.h, nbActive);
    } else {
      y = MAX(0, rect.h - nbActive);
      count = rect.h - y;
    }
  }
  auto it = messages.end();
  if (isMinimized && rect.y == 0) {
    it = messages.end() - 1;
    dy = -1;
  } else {
    it = messages.end() - count;
    // if ( ! isMinimized ) it -= offset;
    dy = 1;
  }
  for (; count > 0; it += dy, y++, count--) {
    con->setDefaultForeground(sevColor[it->severity]);
    con->printEx(rect.w / 2, y, TCOD_BKGND_NONE, TCOD_CENTER, it->txt.c_str());
  }
  return true;
}

void Logger::addMessage(MessageSeverity severity, std::string msg_in) {
  char* msg = msg_in.data();
  while (strlen(msg) >= LOG_WIDTH - 2) {
    char* ptr = msg + LOG_WIDTH - 2;
    while (!isspace(*ptr) && ptr > msg) ptr--;
    if (ptr == msg) ptr = msg + LOG_WIDTH - 2;
    char backup = *ptr;
    *ptr = 0;
    messages.emplace_back(Message{1.0f, std::string(msg), severity});
    *ptr = backup;
    msg = ptr;
    while (isspace(*msg)) msg++;
    nbActive++;
  }
  messages.emplace_back(Message{1.0f, std::string(msg), severity});
  nbActive++;
}

#define HIST_CHUNK_VERSION 2
void Logger::saveData(uint32_t chunkId, TCODZip* zip) {
  saveGame.saveChunk(HIST_CHUNK_ID, HIST_CHUNK_VERSION);
  zip->putChar(isMinimized ? 1 : 0);
  scroller->save(zip);
  zip->putInt(messages.size());
  for (int i = 0; i < messages.size(); i++) {
    Message& msg = messages.at(i);
    zip->putFloat(msg.timer);
    zip->putString(msg.txt.c_str());
    zip->putInt(msg.severity);
  }
}

bool Logger::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) {
  if (chunkVersion != HIST_CHUNK_VERSION) return false;
  isMinimized = zip->getChar() == 1;
  scroller->load(zip);
  int nbMessages = zip->getInt();
  while (nbMessages > 0) {
    nbMessages--;
    messages.emplace_back(Message{zip->getFloat(), zip->getString(), static_cast<MessageSeverity>(zip->getInt())});
  }
  if (isMinimized)
    setMinimized();
  else
    setMaximized();
  return true;
}

void Logger::setPos(int x, int y) {
  ui::MultiPosDialog::setPos(x, y);
  userPref.logx = x;
  userPref.logy = y;
  maximizedRect = minimizedRect;
  maximizedRect.y = 0;
  maximizedRect.h = CON_H;
}
}  // namespace ui

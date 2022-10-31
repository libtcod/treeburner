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
#pragma once
#include <fmt/printf.h>
#include <stdarg.h>

#include <deque>
#include <libtcod.hpp>

#include "savegame.hpp"
#include "ui_dialog.hpp"

enum MessageSeverity { DEBUG, INFO, WARN, CRITICAL, NB_SEVERITIES };

class Logger : public MultiPosDialog, public bas::SaveListener, public Scrollable {
 public:
  Logger();
  template <typename S, typename... Ts>
  void debug(const S& fmt, const Ts&... args) {
    addMessage(DEBUG, fmt::sprintf(fmt, args...));
  }
  template <typename S, typename... Ts>
  void info(const S& fmt, const Ts&... args) {
    addMessage(INFO, fmt::sprintf(fmt, args...));
  }
  template <typename S, typename... Ts>
  void warn(const S& fmt, const Ts&... args) {
    addMessage(WARN, fmt::sprintf(fmt, args...));
  }
  template <typename S, typename... Ts>
  void critical(const S& fmt, const Ts&... args) {
    addMessage(CRITICAL, fmt::sprintf(fmt, args...));
  }
  void render() override;
  bool update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) override;
  void setPos(int x, int y) override;

  // SaveListener
  bool loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip* zip) override;
  void saveData(uint32_t chunkId, TCODZip* zip) override;

  // scrollable
  int getScrollTotalSize() override;
  const std::string& getScrollText(int idx) override;
  void getScrollColor(int idx, TCODColor* fore, TCODColor* back) override;

 protected:
  struct Message {
    float timer{};
    std::string txt{};
    MessageSeverity severity{};
  };

  int nbActive;
  Scroller* scroller;
  bool lookOn;
  float titleBarAlpha;

  void addMessage(MessageSeverity severity, std::string msg);
  std::deque<Message> messages{};
};

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
#pragma once
#include <libtcod.hpp>
#include <umbra/umbra.hpp>

class Screen : public UmbraModule {
 public:
  Screen(float fadeLvl) : UmbraModule{}, fadeLvl{fadeLvl} {}
  Screen(const char* name, float fadeLvl) : UmbraModule{name}, fadeLvl{fadeLvl} {}
  void render() override = 0;
  virtual bool update(float elapsed, TCOD_key_t k, TCOD_mouse_t mouse) = 0;
  void keyboard(TCOD_key_t& key) override { this->key = key; }
  void mouse(TCOD_mouse_t& ms) override { this->ms = ms; }
  bool update(void) override;

  void setFadeIn(int lengthInMilli, TCODColor col = TCODColor::black);  // set fade lengths in milliseconds
  void setFadeOut(int lengthInMilli, TCODColor col = TCODColor::black);  // set fade lengths in milliseconds

 protected:
  float timefix{};  // remove activate execution time for smooth animation
  float fadeLvl{};
  int fadeInLength{};
  TCODColor fadeInColor{};
  int fadeOutLength{};
  TCODColor fadeOutColor{};

  enum { FADE_UP, FADE_DOWN, FADE_OFF, FADE_NONE } fade{FADE_UP};
  TCOD_key_t key{};
  TCOD_mouse_t ms{};
  void onInitialise() override { UmbraModule::onInitialise(); }
  void prepareImage(TCODImage* img) const;
  TCODImage* loadChapterPicture(bool big = false);
  void onActivate() override {
    fadeLvl = 0;
    fade = FADE_UP;
  }
};

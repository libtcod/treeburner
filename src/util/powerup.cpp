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
#include "util/powerup.hpp"

#include <stdio.h>

#include "constants.hpp"
#include "main.hpp"

namespace util {
TCODList<Powerup*> Powerup::list;
TCODImage* PowerupGraph::img_[PW_NB] = {NULL};
const char* PowerupGraph::img_name_[PW_NB] = {
    "data/img/fireball.png",
    "data/img/burst.png",
    "data/img/speed.png",
    "data/img/bounce.png",
    "data/img/incandescence.png",
};
PowerupGraph* PowerupGraph::instance = NULL;

// cheap data initialization, but well.. 7DRL style...
void Powerup::init() {
  Powerup* fb_lvl1 =
      new Powerup(PW_FB, 1, "Fireball 1", "Spell powerup", "Fireball damages increased to 10 and range 1");
  list.push(fb_lvl1);
  fb_lvl1->enabled_ = true;
  Powerup* fb_lvl2 =
      new Powerup(PW_FB, 2, "Fireball 2", "Spell powerup", "Fireball damages increased to 15 and range 2", fb_lvl1);
  list.push(fb_lvl2);
  Powerup* fb_lvl3 =
      new Powerup(PW_FB, 3, "Fireball 3", "Spell powerup", "Fireball damages increased to 20 and range 3", fb_lvl2);
  list.push(fb_lvl3);
  Powerup* fb_lvl4 =
      new Powerup(PW_FB, 4, "Fireball 4", "Spell powerup", "Fireball damages increased to 25 and range 4", fb_lvl3);
  list.push(fb_lvl4);
  Powerup* fb_lvl5 =
      new Powerup(PW_FB, 5, "Fireball 5", "Spell powerup", "Fireball damages increased to 30 and range 5", fb_lvl4);
  list.push(fb_lvl5);
  Powerup* fb_burst1 = new Powerup(
      PW_BURST,
      1,
      "Live embers",
      "Spell variation",
      "Hold right mouse button to cast a special fireball that projects deadly embers when it explodes",
      fb_lvl1);
  list.push(fb_burst1);
  Powerup* fb_burst2 =
      new Powerup(PW_BURST, 2, "Live embers 2", "Spell powerup", "Increase embers speed and number", fb_burst1);
  list.push(fb_burst2);
  Powerup* fb_sparkleThrough =
      new Powerup(PW_SPARKLE_THROUGH, 0, "Fast embers", "Spell powerup", "Live embers go through creatures", fb_burst2);
  list.push(fb_sparkleThrough);
  Powerup* fb_burst3 =
      new Powerup(PW_BURST, 3, "Live embers 3", "Spell powerup", "Increase embers speed and number", fb_burst2);
  list.push(fb_burst3);
  Powerup* fb_sparkleBounce = new Powerup(
      PW_SPARKLE_BOUNCE, 0, "Bouncing embers", "Spell powerup", "Live embers bounce against walls", fb_burst3);
  list.push(fb_sparkleBounce);
  Powerup* fb_burst4 =
      new Powerup(PW_BURST, 4, "Live embers 4", "Spell powerup", "Increase embers speed and number", fb_burst3);
  list.push(fb_burst4);
  Powerup* fb_burst5 =
      new Powerup(PW_BURST, 5, "Live embers 5", "Spell powerup", "Increase embers speed and number", fb_burst4);
  list.push(fb_burst5);
  Powerup* fb_incan1 = new Powerup(
      PW_INCAN,
      1,
      "Incandescence 1",
      "Spell variation",
      "Hold shift+left mouse button to cast a fire cloud which sets fire to any creature around",
      fb_lvl1);
  list.push(fb_incan1);
  Powerup* fb_incan2 = new Powerup(
      PW_INCAN, 2, "Incandescence 2", "Spell powerup", "Increases the fire cloud size and life time", fb_incan1);
  list.push(fb_incan2);
  Powerup* fb_incan3 = new Powerup(
      PW_INCAN, 3, "Incandescence 3", "Spell powerup", "Increases the fire cloud size and life time", fb_incan2);
  list.push(fb_incan3);
  Powerup* fb_incan4 = new Powerup(
      PW_INCAN, 4, "Incandescence 4", "Spell powerup", "Increases the fire cloud size and life time", fb_incan3);
  list.push(fb_incan4);
  Powerup* fb_incan5 = new Powerup(
      PW_INCAN, 5, "Incandescence 5", "Spell powerup", "Increases the fire cloud size and life time", fb_incan4);
  list.push(fb_incan5);

  new PowerupGraph();  // create singleton
}

Powerup::Powerup(
    PowerupId id, int level, const char* name, const char* type, const char* description, Powerup* prerequisite)
    : id_(id), name_(name), level_(level), type_(type), description_(description), prerequisite_(prerequisite) {
  enabled_ = false;
}

void Powerup::apply() {
  enabled_ = true;
  if (id_ == PW_FB) {
    FireBall::damage += 3;
    FireBall::range += 0.7f;
  } else if (id_ == PW_BURST) {
    if (level_ == 1) {
      FireBall::sparkle = true;
      FireBall::sparkleSpeed = 1.0f;
      FireBall::nbSparkles = 4;
    } else {
      FireBall::sparkleSpeed += 0.8f;
      FireBall::nbSparkles += 2;
    }
  } else if (id_ == PW_INCAN) {
    if (level_ == 1) {
      FireBall::incandescence = true;
      FireBall::incanRange = 2.0f;
      FireBall::incanLife = 2.0f;
    } else {
      FireBall::incanRange += 0.8f;
      FireBall::incanLife += 2.0f;
    }
  } else if (id_ == PW_SPARKLE_THROUGH) {
    FireBall::sparkleThrough = true;
  } else if (id_ == PW_SPARKLE_BOUNCE) {
    FireBall::sparkleBounce = true;
  }
}

void Powerup::getAvailable(TCODList<Powerup*>* l) {
  for (Powerup** it = list.begin(); it != list.end(); it++) {
    if (!(*it)->enabled_ && ((*it)->prerequisite_ == NULL || (*it)->prerequisite_->enabled_)) {
      l->push(*it);
    }
  }
}

PowerupGraph::PowerupGraph() {
  instance = this;
  need_refresh_ = true;
}

void PowerupGraph::setFontSize(int fontSize) {
  static bool firstTime = true;
  // load or reload powerup icons
  // unused ascii code / font bitmap part
  static int asciiRanges[] = {1, 8, 11, 15, 19, 23, 127, 175, 181, 184, 207, 216, 233, 255};
  int range = 1;
  int ascii = 2;
  int fontx = 1;
  int fonty = 9;
  for (int typ = 0; typ < PW_NB; typ++) {
    // load image
    if (!firstTime) delete img_[typ];
    img_[typ] = new TCODImage(img_name_[typ]);
    int iw, ih;
    img_[typ]->getSize(&iw, &ih);
    icon_width_ = 1;
    while (icon_width_ * fontSize <= iw) icon_width_++;
    icon_width_--;
    icon_height_ = 1;
    while (icon_height_ * fontSize <= ih) icon_height_++;
    icon_height_--;
    iw = icon_width_ * fontSize;
    ih = icon_height_ * fontSize;
    // resize them according to font size
    img_[typ]->scale(iw, ih);
    for (int x = 0; x < icon_width_; x++) {
      for (int y = 0; y < icon_height_; y++) {
        // map them to some unused ascii codes
        TCODConsole::mapAsciiCodeToFont(ascii, fontx, fonty);
        TCODSystem::updateChar(ascii, fontx, fonty, img_[typ], x * fontSize, y * fontSize);
        fontx++;
        if (fontx == 32) {
          fontx = 0;
          fonty++;
        }
        ascii++;
        if (ascii == asciiRanges[range]) {
          ascii = asciiRanges[range + 1];
          range += 2;
        }
      }
    }
  }
  selected = NULL;
  need_refresh_ = true;
  firstTime = false;
}

typedef struct _PwGraphNode {
  Powerup* pw;
  struct _PwGraphNode* prerequisite;
  int x, y, w, pos, nbSons;
  PowerupId getRequiredId();
} PwGraphNode;

PowerupId PwGraphNode::getRequiredId() {
  Powerup* req = pw->prerequisite_;
  PowerupId id = req->id_;
  while (id == pw->id_) {
    req = req->prerequisite_;
    if (!req) return (PowerupId)(-1);
    id = req->id_;
  }
  return id;
}

void PowerupGraph::render() {
  static TCODConsole pwscreen(CON_W - 20, CON_H - 20);
  static PwGraphNode pw[PW_NB];
  if (need_refresh_) {
    need_refresh_ = false;
    int graphHeight = 0;
    pwscreen.setDefaultBackground(TCODColor::darkerRed);
    pwscreen.rect(0, 0, CON_W - 20, CON_H - 20, true, TCOD_BKGND_SET);
    for (int x = 0; x < CON_W - 20; x++) {
      pwscreen.setCharForeground(x, 0, TCODColor::lightRed);
      pwscreen.setChar(x, 0, TCOD_CHAR_HLINE);
      pwscreen.setCharForeground(x, CON_H - 21, TCODColor::lightRed);
      pwscreen.setChar(x, CON_H - 21, TCOD_CHAR_HLINE);
    }
    for (PowerupId id = PW_FIRST; id < PW_NB; id = (PowerupId)(id + 1)) {
      pw[id].pw = NULL;
      // get the first available powerup for this type
      for (Powerup** it = Powerup::list.begin(); it != Powerup::list.end(); it++) {
        if ((*it)->id_ == id && !(*it)->enabled_) {
          pw[id].pw = *it;
          break;
        }
      }
      if (pw[id].pw == NULL) {
        // no powerup available. get the last level
        for (Powerup** it = Powerup::list.begin(); it != Powerup::list.end(); it++) {
          if ((*it)->id_ == id) {
            pw[id].pw = *it;
          }
        }
      }
    }
    // build tree
    pw[0].w = 0;
    pw[0].nbSons = 0;
    // compute dependance tree (requires_ / nbSons fields)
    for (int i = 1; i < PW_NB; i++) {
      PwGraphNode* current = &pw[i];
      current->prerequisite = NULL;
      current->w = 0;
      current->pos = 0;
      current->nbSons = 0;
      int nbRequired = 0;
      PowerupId id = current->getRequiredId();
      for (int j = 0; j < PW_NB; j++) {
        if (pw[j].pw->id_ == id)
          current->prerequisite = &pw[j];
        else if (pw[j].getRequiredId() == id) {
          nbRequired++;
          if (j < i) current->pos++;
        }
      }
      current->prerequisite->nbSons = nbRequired;
    }
    // compute nodes size (w field)
    for (int i = PW_NB - 1; i >= 0; i--) {
      PwGraphNode* current = &pw[i];
      if (current->nbSons == 0)
        current->w = icon_width_;
      else {
        for (int j = 0; j < PW_NB; j++) {
          if (pw[j].prerequisite == current) {
            if (current->w > 0) current->w++;
            current->w += pw[j].w;
          }
        }
      }
    }
    // compute nodes position (x & y fields)
    pw[0].x = CON_W / 2 - 10;
    pw[0].y = 0;
    for (int i = 1; i < PW_NB; i++) {
      PwGraphNode* current = &pw[i];
      current->x = current->prerequisite->x - current->prerequisite->w / 2;
      int pos = current->pos;
      while (pos > 0) {
        for (int j = 0; j < i; j++) {
          if (pw[j].prerequisite == current->prerequisite) current->x += pw[j].w + 1;
        }
        pos--;
      }
      current->x += icon_width_ / 2;
      current->y = current->prerequisite->y - 1 - icon_height_;
      if (-current->y > graphHeight) graphHeight = -current->y;
    }
    // position the graph vertically
    graphHeight += icon_height_;
    for (int i = 0; i < PW_NB; i++) {
      pw[i].y += graphHeight;
    }
    static int asciiRanges[] = {1, 8, 11, 15, 19, 23, 127, 175, 181, 184, 207, 216, 233, 255};
    int range = 1;
    int ascii = 2;
    for (int i = 0; i < PW_NB; i++) {
      // draw icon i
      PwGraphNode* current = &pw[i];
      int startx = current->x - icon_width_ / 2;
      int starty = current->y - icon_height_ / 2;
      for (int ix = startx; ix < startx + icon_width_; ix++) {
        for (int iy = starty; iy < starty + icon_height_; iy++) {
          pwscreen.setChar(ix, iy, ascii);
          ascii++;
          if (ascii == asciiRanges[range]) {
            ascii = asciiRanges[range + 1];
            range += 2;
          }
        }
      }
      pwscreen.setDefaultForeground(TCODColor::lightOrange);
      if (current->prerequisite) {
        int liney = current->y - icon_height_ / 2 + icon_height_;
        if (current->prerequisite->nbSons > 1) {
          if (current->pos == 0) {
            pwscreen.putChar(current->x, liney, TCOD_CHAR_SW, TCOD_BKGND_NONE);

          } else if (current->pos == current->prerequisite->nbSons - 1) {
            pwscreen.putChar(current->x, liney, TCOD_CHAR_SE, TCOD_BKGND_NONE);
          } else {
            pwscreen.putChar(current->x, liney, TCOD_CHAR_TEEN, TCOD_BKGND_NONE);
          }
        } else
          pwscreen.putChar(current->x, liney, TCOD_CHAR_VLINE, TCOD_BKGND_NONE);
      }
      if (current->nbSons > 1) {
        // not the same as -iconWidth for odd icon widths
        int linew = current->w - icon_width_ / 2 - icon_width_ / 2;
        int startx = current->x - linew / 2;
        int liney = current->y - icon_height_ / 2 - 1;
        pwscreen.hline(startx, liney, linew, TCOD_BKGND_NONE);
        pwscreen.putChar(current->x, liney, TCOD_CHAR_TEES, TCOD_BKGND_NONE);
      }
    }
  }
  selected = NULL;
  for (int i = 0; i < PW_NB; i++) {
    TCODColor col;
    PwGraphNode* current = &pw[i];
    // check if mouse cursor in this icon
    int startx = current->x - icon_width_ / 2;
    int starty = current->y - icon_height_ / 2;
    bool active = IN_RECTANGLE(mouse_cx_ - startx, mouse_cy_ - starty, icon_width_, icon_height_);
    if (active) selected = current->pw;
    // set color
    if (current->pw->enabled_)
      col = TCODColor::orange;
    else if (current->pw->prerequisite_->enabled_)
      col = active ? TCODColor::orange : TCODColor::red;
    else
      col = active ? TCODColor::lightGrey : TCODColor::grey;
    // draw icon i
    for (int ix = startx; ix < startx + icon_width_; ix++) {
      for (int iy = starty; iy < starty + icon_height_; iy++) {
        pwscreen.setCharForeground(ix, iy, col);
      }
    }
  }
  // draw details on selected powerup
  int y = pw[0].y + icon_height_;
  pwscreen.rect(0, y, CON_W - 20, CON_H - 21 - y, true, TCOD_BKGND_NONE);
  if (selected) {
    TCODColor col;
    if (!selected->enabled_ && !selected->prerequisite_->enabled_) {
      col = TCODColor::lightGrey;
    } else {
      col = TCODColor::lightOrange;
    }
    pwscreen.setDefaultForeground(col);
    pwscreen.printEx(CON_W / 2 - 10, y++, TCOD_BKGND_NONE, TCOD_CENTER, selected->name_);
    y++;
    pwscreen.print(CON_W / 2 - 30, y++, selected->type_);
    if (!selected->enabled_ && !selected->prerequisite_->enabled_) {
      pwscreen.setDefaultForeground(TCODColor::red);
      pwscreen.print(CON_W / 2 - 30, y++, "Requires_ %s", selected->prerequisite_->name_);
      pwscreen.setDefaultForeground(col);
    }
    y++;
    pwscreen.printRect(CON_W / 2 - 30, y++, 40, 0, selected->description_);
  }
  // gameEngine->pauseScreen=&pwscreen;
}

bool PowerupGraph::update(float elapsed, TCOD_key_t key, TCOD_mouse_t* mouse) {
  mouse_cx_ = mouse->cx - 10;
  mouse_cy_ = mouse->cy - 5;
  if (selected && mouse->lbutton_pressed && !selected->enabled_ && selected->prerequisite_->enabled_) {
    selected->apply();
    mouse->lbutton_pressed = false;  // do not fire when selecting a powerup
    gameEngine->resumeGame();
    need_refresh_ = true;
    return false;
  }
  return true;
}
}  // namespace util

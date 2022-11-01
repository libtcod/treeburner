
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
#include "ui_dialog.hpp"

#include "main.hpp"
#include "util/subcell.hpp"

void Widget::sendEvent(EWidgetEvent event) {
  for (UIListener** it = listeners.begin(); it != listeners.end(); it++) {
    if ((*it)->onWidgetEvent(this, event)) break;
  }
}

Button::Button() : label(NULL) {}
Button::Button(const char* label, int x, int y) : Widget(x, y) {
  if (label) {
    this->label = strdup(label);
    setSize(strlen(label), 1);
  } else
    setSize(0, 0);
}
Button::~Button() {
  if (label) free(label);
}

void Button::setLabel(const char* label) {
  if (this->label) free(this->label);
  if (label) {
    this->label = strdup(label);
    setSize(strlen(label), 1);
  } else {
    setSize(0, 0);
    this->label = NULL;
  }
}

void Button::render(TCODConsole* con) {
  con->setDefaultBackground(mouseHover && !pressed ? guiHighlightedBackground : guiBackground);
  con->setDefaultForeground(pressed ? guiHighlightedText : guiText);
  con->printEx(x_, y_, TCOD_BKGND_SET, TCOD_LEFT, label);
}

void Button::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty) {
  int mx = mouse.cx - x_ - rectx;
  int my = mouse.cy - y_ - recty;
  mouseHover = pressed = false;
  if (mx >= 0 && mx < w_ && my >= 0 && my < h_) {
    mouseHover = true;
    pressed = mouse.lbutton;
    if (mouse.lbutton_pressed) {
      sendEvent(WE_BUTTON_CLICKED);
    }
  }
}

Tabs::Tabs() {
  curTab = 0;
  mouseTab = -1;
}

Tabs::~Tabs() {
  for (const char** it = labels.begin(); it != labels.end(); it++) {
    free((void*)*it);
  }
}

int Tabs::addTab(const char* label) {
  labels.push(strdup(label));
  tabpos.push(0);
  tablen.push(strlen(label));
  return labels.size() - 1;
}

void Tabs::setLabel(int id, const char* label) {
  const char* oldLabel = labels.get(id);
  if (oldLabel && strcmp(oldLabel, label) == 0) return;
  if (oldLabel) free((void*)oldLabel);
  labels.set(strdup(label), id);
  tablen.set(strlen(label), id);
}

void Tabs::render(TCODConsole* con, int rectx, int recty) {
  int tx = 2 + rectx;
  for (int i = 0; i < labels.size(); i++) {
    if (i > 0) {
      con->setDefaultForeground(guiText);
      con->print(tx++, recty + 1, "\xB3");
    }
    tabpos.set(tx, i);
    con->setDefaultBackground(mouseTab == i && curTab != i ? guiHighlightedBackground : guiBackground);
    con->setDefaultForeground(TCODColor::white);
    const char* label = labels.get(i);
    con->printEx(tx++, recty + 1, TCOD_BKGND_SET, TCOD_LEFT, "%c", label[0]);
    con->setDefaultForeground(curTab == i ? guiHighlightedText : guiText);
    con->printEx(tx, recty + 1, TCOD_BKGND_SET, TCOD_LEFT, &label[1]);
    tx += tablen.get(i) - 1;
  }
}
void Tabs::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty) {
  mouseTab = -1;
  int mx = mouse.cx - rectx;
  int my = mouse.cy - recty;
  if (my == 1) {
    for (int i = 0; i < labels.size(); i++) {
      if (mx >= tabpos.get(i) && mx < tabpos.get(i) + tablen.get(i)) {
        mouseTab = i;
        if (mouse.lbutton) curTab = mouseTab;
        break;
      }
    }
  }
}

Scroller::Scroller(Scrollable* scrollable, int width, int height, bool inverted)
    : inverted(inverted), width(width), height(height), scrollable(scrollable) {
  scrollFocus = scrollDrag = false;
  scrollOffset = 0;
}

void Scroller::render(TCODConsole* con, int x, int y) {
  int nbMessages = scrollable->getScrollTotalSize();
  int ypos = inverted ? MAX(0, height - nbMessages - scrollOffset) : 0;
  int count = inverted ? height - ypos : MIN(height, nbMessages - scrollOffset);
  int idx = nbMessages - count - scrollOffset;
  for (; count > 0; idx++, ypos++, count--) {
    TCODColor fore, back;
    scrollable->getScrollColor(idx, &fore, &back);
    con->setDefaultForeground(fore);
    con->setDefaultBackground(back);
    con->printEx(width / 2, y + ypos, TCOD_BKGND_SET, TCOD_CENTER, scrollable->getScrollText(idx).c_str());
  }
}
void Scroller::renderScrollbar(TCODConsole* con, int x, int y) {
  int nbMessages = scrollable->getScrollTotalSize();
  int nbDisplayed = MIN(height, nbMessages - scrollOffset);
  if (nbMessages > 0 && nbDisplayed < nbMessages) {
    // scrollbar
    int firstDisplayed = nbMessages - scrollOffset - nbDisplayed;
    int start = ((height - 1) * firstDisplayed) / nbMessages;
    int end = ((height - 1) * (firstDisplayed + nbDisplayed)) / nbMessages;
    end = MIN(height - 1, end);
    con->setDefaultBackground(scrollFocus || scrollDrag ? guiText : guiText * 0.8f);
    con->rect(x + width - 2, y + start, 2, end - start + 1, true, TCOD_BKGND_SET);
  }
}

void Scroller::update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty) {
  int nbMessages = scrollable->getScrollTotalSize();
  int nbDisplayed = MIN(height, nbMessages - scrollOffset);
  int firstDisplayed = nbMessages - scrollOffset - nbDisplayed;
  // scrollbar focus
  if (nbDisplayed < nbMessages) {
    bool up = false, down = false, left = false, right = false;
    mob::Player::getMoveKey(k, &up, &down, &left, &right);
    if ((up || left) && scrollOffset < nbMessages - height) {
      scrollOffset += 5;
      scrollOffset = MIN(nbMessages - height, scrollOffset);
    } else if ((down || right) && scrollOffset > 0) {
      scrollOffset -= MIN(5, scrollOffset);
    }
    int start = ((height - 1) * firstDisplayed) / nbMessages;
    int end = ((height - 1) * (firstDisplayed + nbDisplayed)) / nbMessages;
    scrollFocus =
        (mouse.cx >= rectx + width - 2 && mouse.cx < rectx + width && mouse.cy >= recty + start &&
         mouse.cy <= recty + end);
    if (!scrollDrag && scrollFocus && mouse.lbutton) {
      scrollDrag = true;
      scrollStartDrag = mouse.cy;
      scrollStartOffset = start;
    } else if (scrollDrag && mouse.lbutton) {
      int delta = mouse.cy - scrollStartDrag;
      int newStart = scrollStartOffset + delta;
      int newFirst = newStart * nbMessages / height;
      newFirst = MAX(0, newFirst);
      newFirst = MIN(nbMessages - height, newFirst);
      scrollOffset = nbMessages - nbDisplayed - newFirst;
    } else if (scrollDrag && !mouse.lbutton) {
      scrollDrag = false;
    }
  }
}

void Scroller::save(TCODZip* zip) { zip->putInt(scrollOffset); }

void Scroller::load(TCODZip* zip) { scrollOffset = zip->getInt(); }

bool Dialog::update() {
  static float timeScale = config.getFloatProperty("config.gameplay.timeScale");
  float elapsed = TCODSystem::getLastFrameLength() * timeScale;
  if (isMaximizable() && minimiseButton.mouseDown && !waitRelease) {
    if (isMinimized)
      setMaximized();
    else
      setMinimized();
    waitRelease = true;
  } else if (!minimiseButton.mouseDown)
    waitRelease = false;
  if (isClosable() && closeButton.mouseDown) {
    return false;
  }
  if (!UmbraWidget::update()) return false;
  internalUpdate();
  return update(elapsed, key_, ms_);
}

void Dialog::onActivate() {
  UmbraWidget::onActivate();
  if (isModal()) gameEngine->pauseGame();
}

void Dialog::onDeactivate() {
  UmbraWidget::onDeactivate();
  if (isModal()) gameEngine->resumeGame();
}

void Dialog::setMaximized() {
  // if draggable, save position
  if (isDraggable()) {
    minimizedRect.x = rect.x;
    minimizedRect.y = rect.y;
  }
  rect = maximizedRect;
  minimiseButton.set(rect.w - 2, 0);
  isMinimized = false;
  canDrag = false;
  if (gameEngine && !gameEngine->isGamePaused()) gameEngine->pauseGame();
  if (gameEngine) gameEngine->gui.setMode(GUI_MAXIMIZED);
}

void Dialog::setMinimized() {
  rect = minimizedRect;
  minimiseButton.set(rect.w - 2, 0);
  isMinimized = true;
  canDrag = isDraggable() || isMultiPos();
  if (isDraggable() || isMultiPos()) {
    setDragZone(0, 0, minimizedRect.w - 3, 1);
  }
  if (gameEngine && gameEngine->isGamePaused()) gameEngine->resumeGame();
  if (gameEngine) gameEngine->gui.mode = GUI_NONE;
}

void Dialog::renderFrame(float alpha, const char* title) {
  con->setDefaultBackground(guiBackground);
  con->setDefaultForeground(guiText);
  con->rect(0, 0, rect.w, 1, true, TCOD_BKGND_SET);
  con->printEx(rect.w / 2, 0, TCOD_BKGND_NONE, TCOD_CENTER, title);
  if (isMinimized && (isDraggable() || isMultiPos())) {
    // draw dragging handle
    int l = strlen(title);
    for (int x = 0; x < rect.w / 2 - l / 2 - 1; x++) con->putChar(x, 0, TCOD_CHAR_BLOCK2, TCOD_BKGND_NONE);
    for (int x = rect.w / 2 + l / 2 + 1 + (l & 1); x < rect.w; x++)
      con->putChar(x, 0, TCOD_CHAR_BLOCK2, TCOD_BKGND_NONE);
  }
  if (isAnyClosable()) {
    con->setDefaultForeground(closeButton.mouseHover ? guiHighlightedText : guiText);
    con->putChar(closeButton.x, closeButton.y, 'x', TCOD_BKGND_NONE);
  }
  if (isMaximizable()) {
    con->setDefaultForeground(minimiseButton.mouseHover ? guiHighlightedText : guiText);
    con->putChar(
        minimiseButton.x, minimiseButton.y, isMinimized ? TCOD_CHAR_ARROW2_N : TCOD_CHAR_ARROW2_S, TCOD_BKGND_NONE);
  }

  if (alpha < 1.0f)
    util::blitSemiTransparent(con, 0, 0, rect.w, 1, TCODConsole::root, rect.x, rect.y, alpha, alpha);
  else
    TCODConsole::blit(con, 0, 0, rect.w, 1, TCODConsole::root, rect.x, rect.y);
}

void Dialog::internalUpdate() {
  if (isModal() && !gameEngine->isGamePaused()) {
    gameEngine->pauseGame();
  }
}

void MultiPosDialog::renderFrame(float alpha, const char* title) {
  if (isMultiPos() && isDragging && isMinimized) {
    renderTargetFrame();
  }
  Dialog::renderFrame(alpha, title);
}

void MultiPosDialog::internalUpdate() {
  Dialog::internalUpdate();
  if (isMultiPos() && isDragging && isMinimized) {
    targetx = rect.x;
    targety = rect.y;
    if (!isDraggable()) {
      // cancel actual dragging
      rect.x = minimizedRect.x;
      rect.y = minimizedRect.y;
    }
    // find nearest position
    int dist = 10000, best = 0;
    for (int i = 0; i < possiblePos.size(); i++) {
      UmbraRect curRect = *possiblePos.get(i);
      int curDist = (curRect.x - targetx) * (curRect.x - targetx) + (curRect.y - targety) * (curRect.y - targety);
      if (curDist < dist) {
        dist = curDist;
        best = i;
      }
    }
    targetx = possiblePos.get(best)->x;
    targety = possiblePos.get(best)->y;
  }
}

void MultiPosDialog::onDragEnd() {
  minimizedRect.x = targetx;
  minimizedRect.y = targety;
  setPos(targetx, targety);
  // alter the dialogs of the set
  int posnum = 0;
  int bestdist = 100000;
  int bestpos = 0;
  // find the best set position
  for (posnum = 0; posnum < possiblePos.size(); posnum++) {
    UmbraRect curRect = *possiblePos.get(posnum);
    if (rect.x == curRect.x && rect.y == curRect.y) {
      int dist = 0;
      for (MultiPosDialog** it = set.begin(); it != set.end(); it++) {
        if (*it != this) {
          UmbraRect* prect = (*it)->possiblePos.get(posnum);
          int dx = prect->x - (*it)->rect.x;
          int dy = prect->y - (*it)->rect.y;
          dist += dx * dx + dy * dy;
        }
      }
      if (dist < bestdist) {
        bestdist = dist;
        bestpos = posnum;
      }
    }
  }
  for (MultiPosDialog** it = set.begin(); it != set.end(); it++) {
    if (*it != this) {
      UmbraRect* bestp = (*it)->possiblePos.get(bestpos);
      (*it)->rect = (*it)->minimizedRect = *bestp;
      (*it)->setPos(bestp->x, bestp->y);
    }
  }
}

void MultiPosDialog::renderTargetFrame() {
  TCODConsole::root->setDefaultForeground(guiText);
  TCODConsole::root->printFrame(targetx, targety, rect.w, rect.h, false, TCOD_BKGND_NONE, NULL);
}

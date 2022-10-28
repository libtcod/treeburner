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

enum EDialogFlag {
  DIALOG_MAXIMIZABLE = 1,  // can be minimized/maximized
  DIALOG_DRAGGABLE = 2,  // can be dragged at any position
  DIALOG_CLOSABLE = 4,  // has a close button
  DIALOG_CLOSABLE_NODISABLE = 8,  // has a close button, but it doesn't shutdown the module
  DIALOG_ANY_CLOSABLE = 12,
  DIALOG_MULTIPOS = 16,  // can be dragged at specific positions
  DIALOG_MODAL = 32,  // pause the game when displayed
};

enum EWidgetEvent {
  WE_BUTTON_CLICKED,
};

class UIListener;

class Widget {
 public:
  Widget() : x(0), y(0), w(0), h(0) {}
  Widget(int x, int y) : x(x), y(y) {}
  Widget(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
  void setPos(int x, int y) {
    this->x = x;
    this->y = y;
  }
  void setSize(int w, int h) {
    this->w = w;
    this->h = h;
  }
  void addListener(UIListener* listener) { listeners.push(listener); }
  void removeListener(UIListener* listener) { listeners.removeFast(listener); }
  int x, y, w, h;

 protected:
  TCODList<UIListener*> listeners;
  void sendEvent(EWidgetEvent event);
};

class UIListener {
 public:
  // return true to stop the event propagation
  virtual bool onWidgetEvent(Widget* widget, EWidgetEvent event) = 0;
};

class Button : public Widget {
 public:
  Button();
  Button(const char* label, int x, int y);
  ~Button();

  void setLabel(const char* label);
  void render(TCODConsole* con);
  void update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty);
  bool mouseHover;
  bool pressed;

 protected:
  char* label;
};

// render tabs selector
class Tabs {
 public:
  Tabs();
  ~Tabs();
  int addTab(const char* label);  // returns tab id
  void render(TCODConsole* con, int rectx, int recty);
  void setLabel(int id, const char* label);
  const char* getLabel(int id) { return labels.get(id); }
  void update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty);
  int curTab;  // tab currently selected
  int mouseTab;  // tab under mouse cursor
 protected:
  TCODList<const char*> labels;
  TCODList<int> tabpos;
  TCODList<int> tablen;
};

// render a scrollable box
class Scrollable {
 public:
  virtual int getScrollTotalSize() = 0;
  virtual const char* getScrollText(int idx) = 0;
  virtual void getScrollColor(int idx, TCODColor* fore, TCODColor* back) = 0;
};
class Scroller {
 public:
  Scroller(Scrollable* scrollable, int width, int height, bool inverted = false);
  void render(TCODConsole* con, int x, int y);
  void renderScrollbar(TCODConsole* con, int x, int y);
  void update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse, int rectx, int recty);
  void initDrag() { scrollFocus = scrollDrag = false; }
  void setInverted(bool inv) { inverted = inv; }
  void save(TCODZip* zip);
  void load(TCODZip* zip);
  int getOffset() { return scrollOffset; }

 protected:
  bool inverted;
  int width, height;
  Scrollable* scrollable;
  bool scrollFocus, scrollDrag;
  int scrollOffset, scrollStartDrag, scrollStartOffset;
};

class Dialog : public UmbraWidget {
 public:
  Dialog() : flags(0), isMinimized(false), waitRelease(false) {}
  void keyboard(TCOD_key_t& key) {
    this->key = key;
    UmbraWidget::keyboard(key);
  }
  void mouse(TCOD_mouse_t& ms) {
    this->ms = ms;
    UmbraWidget::mouse(ms);
  }
  bool update(void);
  virtual bool update(float elapsed, TCOD_key_t& k, TCOD_mouse_t& mouse) = 0;
  void setMaximized();
  void setMinimized();
  bool isMaximizable() { return (flags & DIALOG_MAXIMIZABLE) != 0; }
  bool isDraggable() { return (flags & DIALOG_DRAGGABLE) != 0; }
  bool isClosable() { return (flags & DIALOG_CLOSABLE) != 0; }
  bool isClosableNoDisable() { return (flags & DIALOG_CLOSABLE_NODISABLE) != 0; }
  bool isAnyClosable() { return (flags & DIALOG_ANY_CLOSABLE) != 0; }
  bool isMultiPos() { return (flags & DIALOG_MULTIPOS) != 0; }
  bool isModal() { return (flags & DIALOG_MODAL) != 0; }
  void onActivate() override;
  virtual void setPos(int x, int y) { rect.setPos(x, y); }
  void onDeactivate() override;

 protected:
  int flags;
  TCOD_key_t key;
  TCOD_mouse_t ms;
  TCODConsole* con;
  bool isMinimized;
  bool waitRelease;
  UmbraRect maximizedRect;  // pos on screen when maximized
  UmbraRect minimizedRect;  // pos when minimized

  virtual void internalUpdate();
  virtual void renderFrame(float alpha, const char* title);
};

class MultiPosDialog : public Dialog {
 public:
  TCODList<MultiPosDialog*> set;  // set of several dependant dialogs
 protected:
  TCODList<UmbraRect*> possiblePos;  // list of possible pos when minimized
  int targetx, targety;
  void renderTargetFrame();  // render next pos when dragging
  void internalUpdate();
  void renderFrame(float alpha, const char* title);
  void onDragEnd();
};

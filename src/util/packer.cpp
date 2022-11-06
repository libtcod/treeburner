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
#include "util/packer.hpp"

#include <stdio.h>

namespace util {
// #define PACKER_DBG

Packer::Packer(int x, int y, int w, int h)
    : base::Rect(x, y, w, h),
      minWidth(1),
      minHeight(1),
      leftPadding(0),
      rightPadding(0),
      topPadding(0),
      bottomPadding(0) {
  clear();
}

void Packer::setMinimumSize(int minWidth, int minHeight) {
  this->minWidth = minWidth;
  this->minHeight = minHeight;
}

void Packer::setPadding(int leftPadding, int rightPadding, int topPadding, int bottomPadding) {
  this->leftPadding = leftPadding;
  this->rightPadding = rightPadding;
  this->topPadding = topPadding;
  this->bottomPadding = bottomPadding;
}

void Packer::clear() {
  rects.clear();
  empty.clear();
  base::Rect r((int)x_, (int)y_, w_, h_);
  empty.push(r);
}

bool Packer::findEmptyPlace(base::Rect* rect) {
  float bestdist = w_ * w_ * h_ * h_;
  float bestdx = 0, bestdy = 0;
  bool found = false;

  for (base::Rect* r = empty.begin(); r != empty.end(); r++) {
    if (rect->x_ >= r->x_ && rect->y_ >= r->y_ && rect->x_ + rect->w_ <= r->x_ + r->w_ &&
        rect->y_ + rect->h_ <= r->y_ + r->h_) {
      return true;  // rect is inside an empty rect
    }
    // check if rect can fit inside r
    if (rect->w_ > r->w_) continue;
    if (rect->h_ > r->h_) continue;
    // get the minimum movement to put rect inside r
    float xmove = 0, ymove = 0;
    if (rect->x_ < r->x_)
      xmove = r->x_ - rect->x_;
    else if (rect->x_ + rect->w_ > r->x_ + r->w_)
      xmove = r->x_ + r->w_ - rect->x_ - rect->w_;
    if (rect->y_ < r->y_)
      ymove = r->y_ - rect->y_;
    else if (rect->y_ + rect->h_ > r->y_ + r->h_)
      ymove = r->y_ + r->h_ - rect->y_ - rect->h_;
    float dist = xmove * xmove + ymove * ymove;
    if (dist < bestdist) {
      bestdist = dist;
      bestdx = xmove;
      bestdy = ymove;
      found = true;
    }
  }
  rect->x_ += bestdx;
  rect->y_ += bestdy;
  return found;
}

bool Packer::addRect(base::Rect* prect, bool fill) {
  base::Rect rect = *prect;
  rect.x_ -= leftPadding;
  rect.y_ -= topPadding;
  rect.w_ += leftPadding + rightPadding;
  rect.h_ += topPadding + bottomPadding;
  // move the rectangle if its not inside the packer
  if (rect.x_ < x_)
    rect.x_ = x_;
  else if (rect.x_ + rect.w_ > x_ + w_)
    rect.x_ = x_ + w_ - rect.w_;
  if (rect.y_ < y_)
    rect.y_ = y_;
  else if (rect.y_ + rect.h_ > y_ + h_)
    rect.y_ = y_ + h_ - rect.h_;

    // move rect inside an empty rectangle
#ifdef PACKER_DBG
  /*
          char **txt=(char **)(((char *)prect)+6*sizeof(int));
          if ( *txt == NULL ) printf ("hero :");
          else {
                  printf ("%s :",*txt);
          }
  */
  printf("%dx%d-%dx%d =>", (int)rect.x, (int)rect.y, rect.w, rect.h);
#endif
  if (!findEmptyPlace(&rect)) return false;  // no place
#ifdef PACKER_DBG
  if (rect.x != prect->x || rect.y != prect->y)
    printf("%dx%d-%dx%d\n", (int)rect.x, (int)rect.y, rect.w, rect.h);
  else
    printf(" /\n");
#endif
  prect->x_ = rect.x_ + leftPadding;
  prect->y_ = rect.y_ + topPadding;
  if (fill) addRectInternal(prect, false);
  return true;
}

void Packer::merge(TCODList<base::Rect>& list) {
  for (base::Rect* r1 = list.begin(); r1 != list.end(); r1++) {
    for (base::Rect* r2 = r1 + 1; r2 != list.end(); r2++) {
      if (r1->x_ == r2->x_ && r1->y_ == r2->y_ && r1->w_ == r2->w_ && r1->h_ == r2->h_) {
        // r1 and r2 identical
        r2 = list.removeFast(r2);
        continue;
      } else if (
          r2->x_ >= r1->x_ && r2->y_ >= r1->y_ && r2->x_ + r2->w_ <= r1->x_ + r1->w_ &&
          r2->y_ + r2->h_ <= r1->y_ + r1->h_) {
        // r2 inside r1
        r2 = list.removeFast(r2);
        continue;
      } else if (
          r2->x_ == r1->x_ && r2->x_ + r2->w_ == r1->x_ + r1->w_ &&
          (r2->y_ == r1->y_ + r1->h_ || r2->y_ + r2->h_ == r1->y_)) {
        // vertical merge
        r1->h_ += r2->h_;
        if (r2->y_ < r1->y_) {
          r1->y_ = r2->y_;
        }
        r2 = list.removeFast(r2);
        continue;
      } else if (
          r2->y_ == r1->y_ && r2->y_ + r2->h_ == r1->y_ + r1->h_ &&
          (r2->x_ == r1->x_ + r1->w_ || r2->x_ + r2->w_ == r1->x_)) {
        // horizontal merge
        r1->w_ += r2->w_;
        if (r2->x_ < r1->x_) {
          r1->x_ = r2->x_;
        }
        r2 = list.removeFast(r2);
        continue;
      }
    }
  }
}

void Packer::addRectInternal(base::Rect* prect, bool duplicate) {
  base::Rect rect = *prect;
  /*
  if ( duplicate ) rect=new Rect(*prect);
  else rect=prect;
  */
  rects.push(rect);
  TCODList<base::Rect> toAdd;
  for (base::Rect* r = empty.begin(); r != empty.end(); r++) {
    if (r->isIntersecting(rect)) {
      if (!(prect->x_ <= r->x_ && prect->y_ <= r->y_ && prect->x_ + prect->w_ >= r->x_ + r->w_ &&
            prect->y_ + prect->h_ >= r->y_ + r->h_)) {
        // empty rectangle not completely covered. split it
        if (prect->x_ + prect->w_ < r->x_ + r->w_) {
          // right part
          base::Rect newr(prect->x_ + prect->w_, r->y_, (int)(r->x_ + r->w_ - prect->x_ - prect->w_), r->h_);
          toAdd.push(newr);
        }
        if (prect->y_ + prect->h_ < r->y_ + r->h_) {
          // bottom part
          base::Rect newr(r->x_, prect->y_ + prect->h_, r->w_, (int)(r->y_ + r->h_ - prect->y_ - prect->h_));
          toAdd.push(newr);
        }
        if (prect->x_ > r->x_) {
          // left part
          base::Rect newr(r->x_, r->y_, (int)(prect->x_ - r->x_), r->h_);
          toAdd.push(newr);
        }
        if (prect->y_ > r->y_) {
          // top part
          base::Rect newr(r->x_, r->y_, r->w_, (int)(prect->y_ - r->y_));
          toAdd.push(newr);
        }
      }
      r = empty.remove(r);
    }
  }
  for (base::Rect* r1 = toAdd.begin(); r1 != toAdd.end(); r1++) {
    // check size
    if (r1->w_ < minWidth || r1->h_ < minHeight) {
      r1 = toAdd.remove(r1);  // too small
      continue;
    }
    for (base::Rect* r2 = toAdd.begin(); r2 != toAdd.end(); r2++) {
      if (r2 == r1) continue;
      if (r1->x_ == r2->x_ && r1->y_ == r2->y_ && r1->w_ == r2->w_ && r1->h_ == r2->h_) {
        // r1 and r2 identical
        if (r1 < r2) {
          r1 = toAdd.remove(r1);
          break;  // keep only the last
        }
      } else if (
          r1->x_ >= r2->x_ && r1->y_ >= r2->y_ && r1->x_ + r1->w_ <= r2->x_ + r2->w_ &&
          r1->y_ + r1->h_ <= r2->y_ + r2->h_) {
        // r1 inside r2
        r1 = toAdd.remove(r1);
        break;
      }
    }
  }
  // add remaining rectangles to the free rectangles list
  for (base::Rect* r1 = toAdd.begin(); r1 != toAdd.end(); r1++) {
    empty.push(*r1);
  }
}

void Packer::addForbiddenZone(const base::Rect rect) {
#ifdef PACKER_DBG
  printf("%dx%d-%dx%d => forbidden\n", (int)rect.x, (int)rect.y, rect.w, rect.h);
#endif
  addRectInternal((base::Rect*)(&rect), true);
}

/*
void Packer::pack() {
#ifdef PACKER_DBG
        printf ("\nEmpty:\n");
        for (Rect *r=empty.begin(); r != empty.end(); r++) {
                printf ("%dx%d-%dx%d ",r->x,r->y,r->w,r->h);
        }
        printf ("\n");
#endif
}
*/
}  // namespace util

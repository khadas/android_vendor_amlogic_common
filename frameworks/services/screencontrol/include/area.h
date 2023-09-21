/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_SCREENCONTROL_RECT_H_
#define ANDROID_SCREENCONTROL_RECT_H_


#include "size.h"

namespace android {

// Helper struct for area to replace gfx::Area usage from original code.
// Only partial functions of gfx::Area is implemented here.
class Area {
 public:
  Area() : x_(0), y_(0),right_(0),bottom_(0), size_(0, 0) {}
  Area(int width, int height) : x_(0), y_(0),right_(width),bottom_(height),size_(width, height) {}
  Area(int x, int y, int width, int height)
      : x_(x), y_(y), right_(width - x),bottom_(height - y),size_(width, height) {}
  explicit Area(const Size& size) : x_(0), y_(0), size_(size) {}

  int x() const { return x_; }
  void set_x(int x) { x_ = x; }

  int y() const { return y_; }
  void set_y(int y) { y_ = y; }

  int width() const { return size_.width(); }
  void set_width(int width) { size_.set_width(width); }

  int height() const { return size_.height(); }
  void set_height(int height) { size_.set_height(height); }



  const Size& size() const { return size_; }
  void set_size(const Size& size) {
    set_width(size.width());
    set_height(size.height());
  }

  constexpr int right() const { return  right_; }
  void set_right(int right) { right_ = right; }
  constexpr int bottom() const { return bottom_; }
  void set_bottom(int bottom) { bottom_ = bottom; }

  void SetRect(int x, int y, int width, int height) {
    set_x(x);
    set_y(y);
    set_width(width);
    set_height(height);
  }

  // Returns true if the area of the rectangle is zero.
  bool IsEmpty() const { return size_.IsEmpty(); }

  // Returns true if this rectangle contains the specified rectangle.
  bool Contains(const Area& area) const {
    return (area.x() >= x() && area.right() <= right() && area.y() >= y() &&
            area.bottom() <= bottom());
  }


 private:
  int x_;
  int y_;
  int right_;
  int bottom_;
  Size size_;
};

inline bool operator==(const Area& lhs, const Area& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.size() == rhs.size();
}

inline bool operator!=(const Area& lhs, const Area& rhs) {
  return !(lhs == rhs);
}

}  // namespace android

#endif  // ANDROID_SCREENCONTROL_RECT_H_

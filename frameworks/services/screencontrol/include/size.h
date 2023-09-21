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

#ifndef ANDROID_SCREENCONTROL_SIZE_H_
#define ANDROID_SCREENCONTROL_SIZE_H_

#include <string>


namespace android {

// Helper struct for size to replace gfx::size usage from original code.
// Only partial functions of gfx::size is implemented here.
struct Size {
 public:
  Size() : width_(0), height_(0) {}
  Size(int width, int height)
      : width_(width < 0 ? 0 : width), height_(height < 0 ? 0 : height) {}

  constexpr int width() const { return width_; }
  constexpr int height() const { return height_; }

  void set_width(int width) { width_ = width < 0 ? 0 : width; }
  void set_height(int height) { height_ = height < 0 ? 0 : height; }

  void SetSize(int width, int height) {
    set_width(width);
    set_height(height);
  }

  bool SizeChanged(int width, int height) {
    if (width_ != width || height_ != height)
        return true;
    else
        return false;
  }

  bool IsEmpty() const { return !width() || !height(); }


  Size& operator=(const Size& ps) {
    set_width(ps.width());
    set_height(ps.height());
    return *this;
  }

 private:
  int width_;
  int height_;
};

inline bool operator==(const Size& lhs, const Size& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

inline bool operator!=(const Size& lhs, const Size& rhs) {
  return !(lhs == rhs);
}

}  // namespace android

#endif  // ANDROID_SCREENCONTROL_SIZE_H_

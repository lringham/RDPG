//
//  imgui_color_gradient.h
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.
//   Twitter / Github: @galloscript

#pragma once

#include "imgui.h"

#include <list>

struct ImGradientMark {
  float color[4];
  float position; // 0 to 1
};

class ImGradient {
public:
  ImGradient();
  ~ImGradient();

  void getColorAt(float position, float *color) const;
  void addMark(float position, ImColor const color);
  void removeMark(ImGradientMark *mark);
  void refreshCache();
  std::list<ImGradientMark *> &getMarks() { return m_marks; }
  int gradientID() { return ID; }

private:
  int ID = 0;
  void computeColorAt(float position, float *color) const;
  std::list<ImGradientMark *> m_marks;
  float m_cachedValues[256 * 3];
};

namespace ImGui {
bool GradientButton(ImGradient *gradient);

bool GradientEditor(ImGradient *gradient, ImGradientMark *&draggingMark,
                    ImGradientMark *&selectedMark);

} // namespace ImGui

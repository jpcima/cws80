#pragma once
#include "ui/cws80_ui_controller.h"
#include <memory>

namespace cws80 {

class GraphicsDevice;

//
class UIView {
 public:
  UIView(UIController &ctl, GraphicsDevice &gdev);
  ~UIView();

  void draw(bool interactible);

 private:
  struct Impl;
  std::unique_ptr<Impl> P;
};

}  // namespace cws80

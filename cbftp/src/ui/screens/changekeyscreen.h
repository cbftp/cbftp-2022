#pragma once

#define SHORTESTKEY 4

#include "../../core/pointer.h"

#include "../uiwindow.h"
#include "../menuselectoption.h"

class MenuSelectOptionElement;

class ChangeKeyScreen : public UIWindow {
public:
  ChangeKeyScreen(Ui *);
  ~ChangeKeyScreen();
  void initialize(unsigned int, unsigned int);
  void update();
  void redraw();
  bool keyPressed(unsigned int);
  std::string getLegendText() const;
  std::string getInfoLabel() const;
private:
  std::string currentlegendtext;
  std::string defaultlegendtext;
  bool active;
  bool mismatch;
  bool oldmismatch;
  bool tooshort;
  Pointer<MenuSelectOptionElement> activeelement;
  MenuSelectOption mso;
  std::string operation;
};

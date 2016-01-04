#pragma once

#include <string>

#include "textinputfield.h"
#include "menuselectoptionelement.h"
#include "resizableelement.h"

class MenuSelectOptionTextField : public ResizableElement {
private:
  TextInputField textfield;
  std::string extralegend;
public:
  MenuSelectOptionTextField();
  MenuSelectOptionTextField(std::string, int, int, std::string, std::string, int, int, bool);
  std::string getContentText() const;
  bool activate();
  void inputChar(int);
  int cursorPosition() const;
  std::string getData() const;
  void clear();
  void setText(const std::string &);
  std::string getLegendText() const;
  void setExtraLegendText(const std::string &);
  unsigned int wantedWidth() const;
  void setMaxWidth(unsigned int);
};

#pragma once

#include <map>
#include <string>

#include "../../pointer.h"

#include "../uiwindow.h"
#include "../menuselectoption.h"

class Engine;
class TransferJob;

class AllTransferJobsScreen : public UIWindow {
public:
  AllTransferJobsScreen(Ui *);
  void initialize(unsigned int, unsigned int);
  void redraw();
  void update();
  bool keyPressed(unsigned int);
  std::string getLegendText() const;
  std::string getInfoLabel() const;
  std::string getInfoText() const;
  static void addJobTableHeader(unsigned int, MenuSelectOption &, std::string);
  static void addJobDetails(unsigned int, MenuSelectOption &, Pointer<TransferJob>);
private:
  static void addJobTableRow(unsigned int, MenuSelectOption &, unsigned int, bool, std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string);
  MenuSelectOption table;
  Engine * engine;
  bool hascontents;
};

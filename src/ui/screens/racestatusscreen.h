#pragma once

#include <list>
#include <map>

#include "../../pointer.h"

#include "../uiwindow.h"
#include "../menuselectoption.h"

class Race;

class RaceStatusScreen : public UIWindow {
public:
  RaceStatusScreen(Ui *);
  ~RaceStatusScreen();
  void initialize(unsigned int, unsigned int, std::string);
  void redraw();
  void update();
  void command(std::string, std::string);
  void keyPressed(unsigned int);
  std::string getLegendText() const;
  std::string getInfoLabel() const;
private:
  char getFileChar(bool, bool, bool, bool) const;
  Pointer<Race> race;
  bool smalldirs;
  bool awaitingremovesite;
  bool awaitingabort;
  bool awaitingdelete;
  unsigned int currnumsubpaths;
  unsigned int currguessedsize;
  unsigned int longestsubpath;
  std::string release;
  std::list<std::string> subpaths;
  MenuSelectOption mso;
  std::map<std::string, int> filetagpos;
  std::map<std::string, std::string> filenametags;
  std::string removesite;
  std::string defaultlegendtext;
  std::string finishedlegendtext;
  bool finished;
};

#pragma once

#include <list>
#include <utility>
#include <string>

#include "browsescreensub.h"

#include "../uiwindow.h"
#include "../uifilelist.h"
#include "../menuselectoption.h"
#include "../menuselectoptiontextfield.h"
#include "../../path.h"
#include "../../rawbuffer.h"

class SiteLogic;
class Site;
class FileList;
class Ui;
class UIFile;
class BrowseScreenAction;

enum class ConfirmAction {
  NONE,
  DELETE,
  WIPE
};

class BrowseScreenSite : public BrowseScreenSub {
public:
  BrowseScreenSite(Ui *, const std::string &);
  ~BrowseScreenSite();
  BrowseScreenType type() const override;
  void redraw(unsigned int, unsigned int, unsigned int) override;
  void update() override;
  void command(const std::string & command, const std::string & arg) override;
  BrowseScreenAction keyPressed(unsigned int) override;
  std::string getLegendText() const override;
  std::string getInfoLabel() const override;
  std::string getInfoText() const override;
  void setFocus(bool) override;
  void tick(int) override;
  std::string siteName() const;
  FileList * fileList() const;
  UIFile * selectedFile() const;
  UIFileList * getUIFileList() override;
  std::list<UIFile *> getSelectedUIFiles() const;
private:
  Ui * ui;
  unsigned int row;
  unsigned int col;
  unsigned int coloffset;
  MenuSelectOption table;
  unsigned int currentviewspan;
  bool virgin;
  bool resort;
  mutable int tickcount;
  std::list<BrowseScreenRequest> requests;
  bool gotomode;
  bool gotomodefirst;
  int gotomodeticker;
  bool filtermodeinput;
  std::string gotomodestring;
  UIFileList::SortMethod sortmethod;
  std::shared_ptr<SiteLogic> sitelogic;
  std::shared_ptr<Site> site;
  UIFileList list;
  mutable int spinnerpos;
  FileList * filelist;
  bool withinraceskiplistreach;
  Path closestracesectionpath;
  std::string closestracesection;
  std::string separatortext;
  std::list<std::pair<Path, std::string> > selectionhistory;
  bool focus;
  MenuSelectOptionTextField filterfield;
  int temphighlightline;
  RawBuffer cwdrawbuffer;
  bool softselecting;
  LastInfo lastinfo;
  std::string lastinfotarget;
  ConfirmAction confirmaction;
  void refreshFilelist();
  void disableGotoMode();
  void clearSoftSelects();
  bool handleReadyRequests();
  void loadFileListFromRequest();
};

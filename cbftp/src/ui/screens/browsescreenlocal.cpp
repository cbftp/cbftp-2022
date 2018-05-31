#include "browsescreenlocal.h"

#include "../../core/tickpoke.h"
#include "../../localstorage.h"
#include "../../localfilelist.h"
#include "../../globalcontext.h"
#include "../../skiplist.h"
#include "../../util.h"

#include "../ui.h"
#include "../menuselectoptiontextbutton.h"
#include "../resizableelement.h"
#include "../termint.h"
#include "../menuselectadjustableline.h"
#include "../misc.h"

#include "browsescreensite.h"
#include "browsescreenaction.h"

BrowseScreenLocal::BrowseScreenLocal(Ui * ui) : ui(ui), currentviewspan(0),
    focus(true), changedsort(false), cwdfailed(false), deletesuccess(false),
    deletefailed(false), tickcount(0),
    resort(false), sortmethod(0), gotomode(false), gotomodefirst(false),
    gotomodeticker(0), filtermodeinput(false), temphighlightline(-1),
    deleting(false)
{
  gotoPath(global->getLocalStorage()->getDownloadPath());
}

BrowseScreenLocal::~BrowseScreenLocal() {

}

BrowseScreenType BrowseScreenLocal::type() const {
  return BROWSESCREEN_LOCAL;
}

void BrowseScreenLocal::redraw(unsigned int row, unsigned int col, unsigned int coloffset) {
  row = row - (filtermodeinput ? 2 : 0);
  this->row = row;
  this->col = col;
  this->coloffset = coloffset;

  if (resort == true) {
    sort();
  }
  unsigned int position = list.currentCursorPosition();
  unsigned int listsize = list.size();
  adaptViewSpan(currentviewspan, row, position, listsize);

  table.reset();
  const std::vector<UIFile *> * uilist = list.getSortedList();
  for (unsigned int i = 0; i + currentviewspan < uilist->size() && i < row; i++) {
    unsigned int listi = i + currentviewspan;
    UIFile * uifile = (*uilist)[listi];
    bool selected = uifile == list.cursoredFile();
    bool isdir = uifile->isDirectory();
    bool allowed = global->getSkipList()->check((list.getPath() / uifile->getName()).toString(), isdir, false).action != SKIPLIST_DENY;
    std::string prepchar = " ";
    if (isdir) {
      if (allowed) {
        prepchar = "#";
      }
      else {
        prepchar = "S";
      }
    }
    else if (!allowed) {
      prepchar = "s";
    }
    else if (uifile->isLink()){
      prepchar = "L";
    }
    std::string owner = uifile->getOwner() + "/" + uifile->getGroup();
    BrowseScreenSite::addFileDetails(table, coloffset, i, prepchar, uifile->getName(), uifile->getSizeRepr(), uifile->getLastModified(), owner, true, selected);
  }
  table.adjustLines(col - 3);
  table.checkPointer();
  if (temphighlightline != -1) {
    Pointer<MenuSelectAdjustableLine> highlightline = table.getAdjustableLineOnRow(temphighlightline);
    if (!!highlightline) {
      std::pair<unsigned int, unsigned int> minmaxcol = highlightline->getMinMaxCol();
      for (unsigned int i = minmaxcol.first; i <= minmaxcol.second; i++) {
        ui->printChar(temphighlightline, i, ' ', true);
      }
    }
  }
  for (unsigned int i = 0; i < table.size(); i++) {
    Pointer<ResizableElement> re = table.getElement(i);
    bool highlight = false;
    if (table.getSelectionPointer() == i || (int)re->getRow() == temphighlightline) {
      highlight = true;
    }
    if (re->isVisible()) {
      ui->printStr(re->getRow(), re->getCol(), re->getLabelText(), highlight && focus);
    }
  }
  printSlider(ui, row, coloffset + col - 1, listsize, currentviewspan);

  if (listsize == 0) {
    ui->printStr(0, coloffset + 3, "(empty directory)");
  }
  if (filtermodeinput) {
    std::string oldtext = filterfield.getData();
    filterfield = MenuSelectOptionTextField("filter", row + 1, 1, "", oldtext,
        col - 20, 512, false);
    ui->showCursor();
  }
  update();
}

void BrowseScreenLocal::update() {
  if (table.size()) {
    Pointer<ResizableElement> re = table.getElement(table.getLastSelectionPointer());
    ui->printStr(re->getRow(), re->getCol(), re->getLabelText());
    re = table.getElement(table.getSelectionPointer());
    ui->printStr(re->getRow(), re->getCol(), re->getLabelText(), focus);
  }
  if (filtermodeinput) {
    std::string pretag = "[Filter]: ";
    ui->printStr(filterfield.getRow(), coloffset + filterfield.getCol(), pretag + filterfield.getContentText());
    ui->moveCursor(filterfield.getRow(), coloffset + filterfield.getCol() + pretag.length() + filterfield.cursorPosition());
  }
}

void BrowseScreenLocal::command(const std::string & command, const std::string & arg) {
  if (command == "yes" && deleting) {
    if (LocalStorage::deleteRecursive(actiontarget)) {
      deletesuccess = true;
    }
    else {
      deletefailed = true;
    }
    tickcount = 0;
    gotoPath(list.getPath());
    ui->setInfo();
  }
  deleting = false;
  ui->redraw();
}

BrowseScreenAction BrowseScreenLocal::keyPressed(unsigned int ch) {
  if (temphighlightline != -1) {
    temphighlightline = -1;
    ui->redraw();
    if (ch == '-') {
      return BrowseScreenAction();
    }
  }
  bool update = false;
  bool success = false;
  unsigned int pagerows = (unsigned int) row * 0.6;
  if (filtermodeinput) {
    if ((ch >= 32 && ch <= 126) || ch == KEY_BACKSPACE || ch == 8 || ch == 127 ||
        ch == KEY_RIGHT || ch == KEY_LEFT || ch == KEY_DC || ch == KEY_HOME ||
        ch == KEY_END) {
      filterfield.inputChar(ch);
      ui->update();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    }
    else if (ch == 10) {
      std::string filter = filterfield.getData();
      if (filter.length()) {
        list.setFilters(util::split(filter));
        resort = true;
      }
      filtermodeinput = false;
      ui->hideCursor();
      ui->redraw();
      ui->setLegend();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    }
    else if (ch == 27) {
      if (filterfield.getData() != "") {
        filterfield.clear();
        ui->update();
      }
      else {
        filtermodeinput = false;
        ui->hideCursor();
        ui->redraw();
        ui->setLegend();
      }
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    }
  }
  if (gotomode) {
    if (gotomodefirst) {
      gotomodefirst = false;
    }
    if (ch >= 32 && ch <= 126) {
      gotomodeticker = 0;
      gotomodestring += toupper(ch);
      unsigned int gotomodelength = gotomodestring.length();
      const std::vector<UIFile *> * sortedlist = list.getSortedList();
      for (unsigned int i = 0; i < sortedlist->size(); i++) {
        if ((*sortedlist)[i] == NULL) continue;
        std::string name = (*sortedlist)[i]->getName();
        if (name.length() >= gotomodelength) {
          std::string substr = name.substr(0, gotomodelength);
          for (unsigned int j = 0; j < gotomodelength; j++) {
            substr[j] = toupper(substr[j]);
          }
          if (substr == gotomodestring) {
            list.setCursorPosition(i);
            break;
          }
        }
      }
      ui->redraw();
      return BrowseScreenAction(BROWSESCREENACTION_CAUGHT);
    }
    else {
      disableGotoMode();
    }
    if (ch == 27) {
      return BrowseScreenAction();
    }
  }
  switch (ch) {
    case 27: // esc
      ui->returnToLast();
      break;
    case 'c':
      return BrowseScreenAction(BROWSESCREENACTION_CLOSE);
    case 'W':
    case KEY_DC: {
      UIFile * cursoredfile = list.cursoredFile();
      if (cursoredfile == NULL) {
        break;
      }
      actiontarget = filelist->getPath() / cursoredfile->getName();
      deleting = true;
      LocalPathInfo pathinfo = LocalStorage::getPathInfo(actiontarget);
      std::string dirs = util::int2Str(pathinfo.getNumDirs());
      std::string files = util::int2Str(pathinfo.getNumFiles());
      std::string size = util::parseSize(pathinfo.getSize());
      std::string confirmation = "Do you really want to delete " + cursoredfile->getName() + "?";
      if (pathinfo.getDepth() > 0) {
        confirmation += " It contains " + size + " in " + files + "f/" + dirs + "d and is ";
      }
      if (pathinfo.getDepth() == 1) {
        confirmation += "1 level deep.";
      }
      else if (pathinfo.getDepth() > 1) {
        confirmation += util::int2Str(pathinfo.getDepth()) + " levels deep.";
      }
      if (pathinfo.getNumDirs() >= 10 || pathinfo.getNumFiles() >= 100 || pathinfo.getSize() >= 100000000000 || pathinfo.getDepth() > 2) {
        ui->goStrongConfirmation(confirmation);
      }
      else {
        ui->goConfirmation(confirmation);
      }
      break;
    }
    case 'q':
      gotomode = true;
      gotomodefirst = true;
      gotomodeticker = 0;
      gotomodestring = "";
      global->getTickPoke()->startPoke(this, "BrowseScreenLocal", 50, 0);
      ui->update();
      ui->setLegend();
      break;
    case 'f':
      if (list.hasFilters()) {
        resort = true;
        list.unsetFilters();
      }
      else {
        filtermodeinput = true;
      }
      ui->redraw();
      ui->setLegend();
      break;
    case KEY_LEFT:
    case 8:
    case 127:
    case KEY_BACKSPACE:
    {
      //go up one directory level, or return if at top already
      const Path & oldpath = list.getPath();
      if (oldpath == "/") {
        return BrowseScreenAction(BROWSESCREENACTION_CLOSE);
      }
      gotoPath(oldpath.dirName());
      ui->setInfo();
      ui->redraw();
      break;
    }
    case KEY_F(5):
    {
      gotoPath(list.getPath());
      ui->setInfo();
      ui->redraw();
      break;
    }
    case KEY_DOWN:
      //go down and highlight next item (if not at bottom already)
      update = list.goNext();
      if (list.currentCursorPosition() >= currentviewspan + row) {
        ui->redraw();
      }
      else if (update) {
        table.goDown();
        ui->update();
      }
      break;
    case KEY_UP:
      //go up and highlight previous item (if not at top already)
      update = list.goPrevious();
      if (list.currentCursorPosition() < currentviewspan) {
        ui->redraw();
      }
      else if (update) {
        table.goUp();
        ui->update();
      }
      break;
    case KEY_NPAGE:
      for (unsigned int i = 0; i < pagerows; i++) {
        success = list.goNext();
        if (!success) {
          break;
        }
        else if (!update) {
          update = true;
        }
        table.goDown();
      }
      if (update) {
        ui->redraw();
      }
      break;
    case KEY_PPAGE:
      for (unsigned int i = 0; i < pagerows; i++) {
        success = list.goPrevious();
        if (!success) {
          break;
        }
        else if (!update) {
          update = true;
        }
        table.goUp();
      }
      if (update) {
        ui->redraw();
      }
      break;
    case KEY_RIGHT:
    case 10:
    {
      UIFile * cursoredfile = list.cursoredFile();
      if (cursoredfile == NULL) {
        break;
      }
      bool isdir = cursoredfile->isDirectory();
      bool islink = cursoredfile->isLink();
      if (isdir || islink) {
        const Path & oldpath = list.getPath();
        Path targetpath;
        if (islink) {
          Path target = cursoredfile->getLinkTarget();
          if (target.isAbsolute()) {
            targetpath = target;
          }
          else {
            targetpath = oldpath / target;
          }
        }
        else {
          targetpath = oldpath / cursoredfile->getName();
        }
        gotoPath(targetpath);
        ui->setInfo();
        ui->redraw();
      }
      break;
    }
    case KEY_HOME:
      while (list.goPrevious()) {
        table.goUp();
        if (!update) {
          update = true;
        }
      }
      if (update) {
        ui->redraw();
      }
      break;
    case KEY_END:
      while (list.goNext()) {
        table.goDown();
        if (!update) {
          update = true;
        }
      }
      if (update) {
        ui->redraw();
      }
      break;
    case 's':
      sortmethod++;
      resort = true;
      changedsort = true;
      tickcount = 0;
      ui->redraw();
      ui->setInfo();
      break;
    case 'S':
      sortmethod = 0;
      resort = true;
      changedsort = true;
      tickcount = 0;
      ui->redraw();
      ui->setInfo();
      break;
    case 'v':
      //view selected file, do nothing if a directory is selected
      if (list.cursoredFile() != NULL && !list.cursoredFile()->isDirectory()) {
        ui->goViewFile(filelist->getPath(), list.cursoredFile()->getName());
      }
      break;
    case '-':
      if (list.cursoredFile() == NULL) {
        break;
      }
      temphighlightline = table.getElement(table.getSelectionPointer())->getRow();
      ui->redraw();
      break;
  }
  return BrowseScreenAction();
}

std::string BrowseScreenLocal::getLegendText() const {
  if (gotomode) {
    return "[Any] Go to first matching entry name - [Esc] Cancel";
  }
  if (filtermodeinput) {
    return "[Any] Enter space separated filters. Valid operators are !, *, ?. Must match all negative filters and at least one positive if given. Case insensitive. - [Esc] Cancel";
  }
  return "[Up/Down] Navigate - [Enter/Right] open dir - [s]ort - [Backspace/Left] return - [Esc] Cancel - [c]lose - [q]uick jump - Toggle [f]ilter";
}

std::string BrowseScreenLocal::getInfoLabel() const {
  return "LOCAL BROWSING";
}

std::string BrowseScreenLocal::getInfoText() const {
  std::string text = list.getPath().toString();
  if (changedsort) {
    if (tickcount++ < 8) {
      text = "Sort method: " + list.getSortMethod();
      return text;
    }
    else {
      changedsort = false;
    }
  }
  else if (cwdfailed) {
    if (tickcount++ < 8) {
      text = "CWD failed: " + targetpath.toString();
      return text;
    }
    else {
      cwdfailed = false;
    }
  }
  else if (deletesuccess) {
    if (tickcount++ < 8) {
      text = "Delete successful: " + actiontarget.toString();
      return text;
    }
    else {
      deletesuccess = false;
    }
  }
  else if (deletefailed) {
    if (tickcount++ < 8) {
      text = "Delete failed: " + actiontarget.toString();
      return text;
    }
    else {
      deletefailed = false;
    }
  }
  if (list.hasFilters() || list.hasUnique()) {
    if (list.hasFilters()) {
      text += std::string("  FILTER: ") + filterfield.getData();
    }
    if (list.hasUnique()) {
      text += "  UNIQUES";
    }
    text += "  " + util::int2Str(list.filteredSizeFiles()) + "/" + util::int2Str(list.sizeFiles()) + "f " +
        util::int2Str(list.filteredSizeDirs()) + "/" + util::int2Str(list.sizeDirs()) + "d";
    text += std::string("  ") + util::parseSize(list.getFilteredTotalSize()) + "/" +
        util::parseSize(list.getTotalSize());
  }
  else {
    text += "  " + util::int2Str(list.sizeFiles()) + "f " + util::int2Str(list.sizeDirs()) + "d";
    text += std::string("  ") + util::parseSize(list.getTotalSize());
  }
  return text;
}

void BrowseScreenLocal::setFocus(bool focus) {
  this->focus = focus;
  if (!focus) {
    disableGotoMode();
    filtermodeinput = false;
  }
}

void BrowseScreenLocal::tick(int) {
  if (gotomode && !gotomodefirst) {
    if (gotomodeticker++ >= 20) {
      disableGotoMode();
    }
  }
}

UIFile * BrowseScreenLocal::selectedFile() const {
  return list.cursoredFile();
}

void BrowseScreenLocal::sort() {
  switch (sortmethod % 9) {
    case 0:
      list.sortCombined();
      break;
    case 1:
      list.sortName(true);
      break;
    case 2:
      list.sortName(false);
      break;
    case 3:
      list.sortSize(false);
      break;
    case 4:
      list.sortSize(true);
      break;
    case 5:
      list.sortTime(false);
      break;
    case 6:
      list.sortTime(true);
      break;
    case 7:
      list.sortOwner(true);
      break;
    case 8:
      list.sortOwner(false);
      break;
  }
  resort = false;
}

void BrowseScreenLocal::disableGotoMode() {
  if (gotomode) {
    gotomode = false;
    global->getTickPoke()->stopPoke(this, 0);
    ui->setLegend();
  }
}

void BrowseScreenLocal::gotoPath(const Path & path) {
  targetpath = path;
  Pointer<LocalFileList> filelist = global->getLocalStorage()->getLocalFileList(path);
  if (!filelist) {
    cwdfailed = true;
    tickcount = 0;
    return;
  }
  this->filelist = filelist;
  if (list.cursoredFile() != NULL) {
    selectionhistory.push_front(std::pair<Path, std::string>(list.getPath(), list.cursoredFile()->getName()));
  }
  else {
    currentviewspan = 0;
  }
  unsigned int position = 0;
  std::list<std::string> filters;
  std::set<std::string> uniques;
  if (list.getPath() == filelist->getPath()) {
    position = list.currentCursorPosition();
    filters = list.getFilters();
    uniques = list.getUniques();
  }
  list.parse(filelist);
  if (filters.size()) {
    list.setFilters(filters);
  }
  sort();
  for (std::list<std::pair<Path, std::string> >::iterator it = selectionhistory.begin(); it != selectionhistory.end(); it++) {
    if (it->first == path) {
      bool selected = list.selectFileName(it->second);
      selectionhistory.erase(it);
      if (selected) {
        return;
      }
      break;
    }
  }
  if (position) {
    list.setCursorPosition(position);
  }
}

Pointer<LocalFileList> BrowseScreenLocal::fileList() const {
  return filelist;
}

UIFileList * BrowseScreenLocal::getUIFileList() {
  return &list;
}

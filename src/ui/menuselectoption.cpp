#include "menuselectoption.h"

#include "menuselectoptionelement.h"
#include "menuselectoptiontextfield.h"
#include "menuselectoptionnumarrow.h"
#include "menuselectoptioncheckbox.h"
#include "menuselectoptiontextbutton.h"
#include "menuselectoptiontextarrow.h"
#include "menuselectadjustableline.h"

MenuSelectOption::MenuSelectOption() {
  pointer = 0;
  lastpointer = 0;
}

MenuSelectOption::~MenuSelectOption() {

}

bool MenuSelectOption::goDown() {
  if (!options.size()) return false;
  unsigned int ccol = options[pointer]->getCol();
  unsigned int crow = options[pointer]->getRow();
  unsigned int closestelem;
  bool movefound = false;
  unsigned int closest = -1;
  for (unsigned int i = 0; i < options.size(); i++) {
    if (!options[i]->visible() || !options[i]->isSelectable()) {
      continue;
    }
    unsigned int row = options[i]->getRow();
    if (row > crow && options[i]->getCol() == ccol) {
      if (row < closest || closest == (unsigned int)-1) {
        closest = row;
        closestelem = i;
        movefound = true;
      }
    }
  }
  if (movefound) {
    lastpointer = pointer;
    pointer = closestelem;
    return true;
  }
  if (leavedown) {
    lastpointer = pointer;
    focus = false;
    return true;
  }
  return false;
}

bool MenuSelectOption::goUp() {
  if (!options.size()) return false;
  unsigned int ccol = options[pointer]->getCol();
  unsigned int crow = options[pointer]->getRow();
  unsigned int closestelem;
  bool movefound = false;
  unsigned int closest = -1;
  for (unsigned int i = 0; i < options.size(); i++) {
    if (!options[i]->visible() || !options[i]->isSelectable()) {
      continue;
    }
    unsigned int row = options[i]->getRow();
    if (row < crow && options[i]->getCol() == ccol) {
      if (row > closest || closest == (unsigned int)-1) {
        closest = row;
        closestelem = i;
        movefound = true;
      }
    }
  }
  if (movefound) {
    lastpointer = pointer;
    pointer = closestelem;
    return true;
  }
  if (leaveup) {
    lastpointer = pointer;
    focus = false;
    return true;
  }
  return false;
}

bool MenuSelectOption::goRight() {
  if (!options.size()) return false;
  unsigned int ccol = options[pointer]->getCol();
  unsigned int crow = options[pointer]->getRow();
  unsigned int closestelem;
  bool movefound = false;
  unsigned int closest = -1;
  for (unsigned int i = 0; i < options.size(); i++) {
    if (!options[i]->visible() || !options[i]->isSelectable()) {
      continue;
    }
    unsigned int col = options[i]->getCol();
    if (col > ccol && options[i]->getRow() == crow) {
      if (col < closest || closest == (unsigned int)-1) {
        closest = col;
        closestelem = i;
        movefound = true;
      }
    }
  }
  if (movefound) {
    lastpointer = pointer;
    pointer = closestelem;
    return true;
  }
  if (leaveright) {
    lastpointer = pointer;
    focus = false;
    return true;
  }
  return false;
}

bool MenuSelectOption::goLeft() {
  if (!options.size()) return false;
  unsigned int ccol = options[pointer]->getCol();
  unsigned int crow = options[pointer]->getRow();
  unsigned int closestelem;
  bool movefound = false;
  unsigned int closest = -1;
  for (unsigned int i = 0; i < options.size(); i++) {
    if (!options[i]->visible() || !options[i]->isSelectable()) {
      continue;
    }
    unsigned int col = options[i]->getCol();
    if (col < ccol && options[i]->getRow() == crow) {
      if (col > closest || closest == (unsigned int)-1) {
        closest = col;
        closestelem = i;
        movefound = true;
      }
    }
  }
  if (movefound) {
    lastpointer = pointer;
    pointer = closestelem;
    return true;
  }
  if (leaveleft) {
    lastpointer = pointer;
    focus = false;
    return true;
  }
  return false;
}

bool MenuSelectOption::goNext() {
  if (!options.size()) return false;
  unsigned int temppointer = pointer;
  while (pointer < options.size() - 1) {
    pointer++;
    if (options[pointer]->isSelectable()) {
      lastpointer = temppointer;
      return true;
    }
  }
  return false;
}

bool MenuSelectOption::goPrevious() {
  if (!options.size()) return false;
  unsigned int temppointer = pointer;
  while (pointer > 0) {
    pointer--;
    if (options[pointer]->isSelectable()) {
      lastpointer = temppointer;
      return true;
    }
  }
  return false;
}

Pointer<MenuSelectOptionTextField> MenuSelectOption::addStringField(int row, int col, std::string identifier, std::string label, std::string starttext, bool secret) {
  return addStringField(row, col, identifier, label, starttext, secret, 32, 32);
}

Pointer<MenuSelectOptionTextField> MenuSelectOption::addStringField(int row, int col, std::string identifier, std::string label, std::string starttext, bool secret, int maxlen) {
  return addStringField(row, col, identifier, label, starttext, secret, maxlen, maxlen);
}

Pointer<MenuSelectOptionTextField> MenuSelectOption::addStringField(int row, int col, std::string identifier, std::string label, std::string starttext, bool secret, int visiblelen, int maxlen) {
  Pointer<MenuSelectOptionTextField> msotf(makePointer<MenuSelectOptionTextField>(identifier, row, col, label, starttext, visiblelen, maxlen, secret));
  options.push_back(msotf);
  return msotf;
}

Pointer<MenuSelectOptionTextArrow> MenuSelectOption::addTextArrow(int row, int col, std::string identifier, std::string label) {
  Pointer<MenuSelectOptionTextArrow> msota(makePointer<MenuSelectOptionTextArrow>(identifier, row, col, label));
  options.push_back(msota);
  return msota;
}

Pointer<MenuSelectOptionNumArrow> MenuSelectOption::addIntArrow(int row, int col, std::string identifier, std::string label, int startval, int min, int max) {
  Pointer<MenuSelectOptionNumArrow> msona(makePointer<MenuSelectOptionNumArrow>(identifier, row, col, label, startval, min, max));
  options.push_back(msona);
  return msona;
}

Pointer<MenuSelectOptionCheckBox> MenuSelectOption::addCheckBox(int row, int col, std::string identifier, std::string label, bool startval) {
  Pointer<MenuSelectOptionCheckBox> msocb(makePointer<MenuSelectOptionCheckBox>(identifier, row, col, label, startval));
  options.push_back(msocb);
  return msocb;
}

Pointer<MenuSelectOptionTextButton> MenuSelectOption::addTextButton(int row, int col, std::string identifier, std::string label) {
  Pointer<MenuSelectOptionTextButton> msotb(makePointer<MenuSelectOptionTextButton>(identifier, row, col, label, true));
  options.push_back(msotb);
  return msotb;
}

Pointer<MenuSelectOptionTextButton> MenuSelectOption::addTextButtonNoContent(int row, int col, std::string identifier, std::string label) {
  Pointer<MenuSelectOptionTextButton> msotb(makePointer<MenuSelectOptionTextButton>(identifier, row, col, label, false));
  options.push_back(msotb);
  return msotb;
}

Pointer<MenuSelectAdjustableLine> MenuSelectOption::addAdjustableLine() {
  Pointer<MenuSelectAdjustableLine> msal(makePointer<MenuSelectAdjustableLine>());
  adjustablelines.push_back(msal);
  return msal;
}

Pointer<MenuSelectAdjustableLine> MenuSelectOption::addAdjustableLineBefore(Pointer<MenuSelectAdjustableLine> before) {
  Pointer<MenuSelectAdjustableLine> msal(makePointer<MenuSelectAdjustableLine>());
  std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    if (*it == before) {
      adjustablelines.insert(it, msal);
      return msal;
    }
  }
  adjustablelines.push_back(msal);
  return msal;
}

Pointer<MenuSelectOptionElement> MenuSelectOption::getElement(unsigned int i) const {
  if (i >= size()) {
    return Pointer<MenuSelectOptionElement>();
  }
  return options[i];
}

Pointer<MenuSelectOptionElement> MenuSelectOption::getElement(std::string identifier) const {
  std::vector<Pointer<MenuSelectOptionElement> >::const_iterator it;
  for (it = options.begin(); it != options.end(); it++) {
    if ((*it)->getIdentifier() == identifier) {
      return *it;
    }
  }
  return Pointer<MenuSelectOptionElement>();
}

unsigned int MenuSelectOption::getLastSelectionPointer() const {
  return lastpointer;
}

unsigned int MenuSelectOption::getSelectionPointer() const {
  return pointer;
}

bool MenuSelectOption::activateSelected() {
  return getElement(pointer)->activate();
}

void MenuSelectOption::clear() {
  options.clear();
  adjustablelines.clear();
}

void MenuSelectOption::reset() {
  clear();
  pointer = 0;
  lastpointer = 0;
  focus = false;
}

void MenuSelectOption::enterFocusFrom(int dir) {
  focus = true;
  if (dir == 2) { // bottom
    pointer = size() - 1;
  }
  else {
    pointer = 0;
  }
  lastpointer = pointer;
  checkPointer();
}

unsigned int MenuSelectOption::size() const {
  return options.size();
}

unsigned int MenuSelectOption::linesSize() const {
  return adjustablelines.size();
}

void MenuSelectOption::adjustLines(unsigned int linesize) {
  if (!adjustablelines.size()) {
    return;
  }
  unsigned int elementcount = adjustablelines[0]->size();
  if (!elementcount) {
    return;
  }
  std::vector<int> maxwantedwidths;
  std::vector<int> maxwidths;
  maxwantedwidths.resize(elementcount); // int is initialized to 0
  maxwidths.resize(elementcount);
  int shortspaces = 0;
  for (std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    for (unsigned int i = 0; i < elementcount; i++) {
      Pointer<ResizableElement> re = (*it)->getElement(i);
      int wantedwidth = re->wantedWidth();
      if (wantedwidth > maxwantedwidths[i]) {
        maxwantedwidths[i] = wantedwidth;
      }
      if (it == adjustablelines.begin() && re->shortSpacing() && i + 1 != elementcount) {
        shortspaces++;
      }
    }
  }
  unsigned int totalwantedwidth = (maxwantedwidths.size() - 1) * RESIZE_SPACING -
      shortspaces * (RESIZE_SPACING - RESIZE_SPACING_SHORT);
  for (unsigned int i = 0; i < maxwantedwidths.size(); i++) {
    totalwantedwidth += maxwantedwidths[i];
    maxwidths[i] = maxwantedwidths[i];
  }
  while (totalwantedwidth != linesize) {
    if (totalwantedwidth < linesize) {
      for (unsigned int i = 0; i < elementcount; i++) {
        Pointer<ResizableElement> elem = adjustablelines[0]->getElement(i);
        if (elem->isExpandable()) {
          unsigned int expansion = linesize - totalwantedwidth;
          maxwidths[i] += expansion;
          totalwantedwidth += expansion;
          break;
        }
      }
      break;
    }
    else if (totalwantedwidth > linesize) {
      int leastimportant = -1;
      int leastimportantprio = 0;
      for (unsigned int i = 0; i < elementcount; i++) {
        if (!adjustablelines[0]->getElement(i)->isVisible()) {
          continue;
        }
        int prio = adjustablelines[0]->getElement(i)->priority();
        if (prio < leastimportantprio || leastimportant < 0) {
          leastimportantprio = prio;
          leastimportant = i;
        }
      }
      Pointer<ResizableElement> leastimportantelem = adjustablelines[0]->getElement(leastimportant);
      int spacing = leastimportantelem->shortSpacing() ? RESIZE_SPACING_SHORT : RESIZE_SPACING;
      unsigned int maxsaving = maxwantedwidths[leastimportant] + spacing;
      unsigned int resizemethod = leastimportantelem->resizeMethod();
      switch (resizemethod) {
        case RESIZE_REMOVE:
          leastimportantelem->setVisible(false);
          totalwantedwidth -= maxsaving;
          break;
        case RESIZE_WITHDOTS:
        case RESIZE_CUTEND:
        case RESIZE_WITHLAST3:
          if (totalwantedwidth - maxwantedwidths[leastimportant] < linesize) {
            int reduction = totalwantedwidth - linesize;
            maxwidths[leastimportant] = maxwantedwidths[leastimportant] - reduction;
            totalwantedwidth -= reduction;
          }
          else {
            leastimportantelem->setVisible(false);
            totalwantedwidth -= maxsaving;
          }
          break;
      }
    }
  }
  int startpos = adjustablelines[0]->getElement(0)->getCol();
  for (std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    int elementpos = startpos;
    for (unsigned int i = 0; i < elementcount; i++) {
      Pointer<ResizableElement> elem = (*it)->getElement(i);
      if (adjustablelines[0]->getElement(i)->isVisible()) {
        elem->setMaxWidth(maxwidths[i]);
        elem->setPosition(elem->getRow(), elementpos);
        int spacing = elem->shortSpacing() ? RESIZE_SPACING_SHORT : RESIZE_SPACING;
        elementpos += maxwidths[i] + spacing;
      }
      else {
        elem->setVisible(false);
      }
    }
  }
}

void MenuSelectOption::checkPointer() {
  if (pointer >= size()) {
    pointer = size() - 1;
  }
  if (size() == 0) {
    pointer = 0;
  }
  else {
    while ((!options[pointer]->visible() || !options[pointer]->isSelectable()) && pointer > 0) pointer--;
    while ((!options[pointer]->visible() || !options[pointer]->isSelectable()) && pointer < size() - 1) pointer++;
  }
  lastpointer = pointer;
}

std::vector<Pointer<MenuSelectAdjustableLine> >::iterator MenuSelectOption::linesBegin() {
  return adjustablelines.begin();
}

std::vector<Pointer<MenuSelectAdjustableLine> >::iterator MenuSelectOption::linesEnd() {
  return adjustablelines.end();
}

Pointer<MenuSelectAdjustableLine> MenuSelectOption::getAdjustableLine(Pointer<MenuSelectOptionElement> msoe) const {
  std::vector<Pointer<MenuSelectAdjustableLine> >::const_iterator it;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    for (unsigned int i = 0; i < (*it)->size(); i++) {
      if ((*it)->getElement(i) == msoe) {
        return *it;
      }
    }
  }
  return Pointer<MenuSelectAdjustableLine>();
}

void MenuSelectOption::removeAdjustableLine(Pointer<MenuSelectAdjustableLine> msal) {
  std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    if (*it == msal) {
      for (unsigned int i = 0; i < (*it)->size(); i++) {
        removeElement(msal->getElement(i));
      }
      adjustablelines.erase(it);
      return;
    }
  }
}

void MenuSelectOption::removeElement(Pointer<MenuSelectOptionElement> msoe) {
  std::vector<Pointer<MenuSelectOptionElement> >::iterator it;
  for (it = options.begin(); it != options.end(); it++) {
    if (*it == msoe) {
      options.erase(it);
      return;
    }
  }
}

void MenuSelectOption::setPointer(Pointer<MenuSelectOptionElement> set) {
  for (unsigned int i = 0; i < options.size(); i++) {
    Pointer<MenuSelectOptionElement> msoe = options[i];
    if (msoe == set) {
      pointer = i;
      return;
    }
  }
}

bool MenuSelectOption::swapLineWithNext(Pointer<MenuSelectAdjustableLine> msal) {
  std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    if (*it == msal) {
      if (it + 1 != adjustablelines.end()) {
        Pointer<MenuSelectAdjustableLine> swap = *(it + 1);
        *(it + 1) = msal;
        *it = swap;
        return true;
      }
      return false;
    }
  }
  return false;
}

bool MenuSelectOption::swapLineWithPrevious(Pointer<MenuSelectAdjustableLine> msal) {
  std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    if (*it == msal) {
      if (it != adjustablelines.begin()) {
        Pointer<MenuSelectAdjustableLine> swap = *(it - 1);
        *(it - 1) = msal;
        *it = swap;
        return true;
      }
      return false;
    }
  }
  return false;
}

int MenuSelectOption::getLineIndex(Pointer<MenuSelectAdjustableLine> msal) {
  std::vector<Pointer<MenuSelectAdjustableLine> >::iterator it;
  int index = 0;
  for (it = adjustablelines.begin(); it != adjustablelines.end(); it++) {
    if (*it == msal) {
      return index;
    }
    index++;
  }
  return -1;
}

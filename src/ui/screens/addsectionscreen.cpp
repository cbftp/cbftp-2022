#include "addsectionscreen.h"

#include "../../globalcontext.h"
#include "../../site.h"
#include "../../sitemanager.h"
#include "../../settingsloadersaver.h"
#include "../../path.h"

#include "../ui.h"
#include "../menuselectoptiontextfield.h"
#include "../menuselectoptionelement.h"

AddSectionScreen::AddSectionScreen(Ui * ui) {
  this->ui = ui;
}

AddSectionScreen::~AddSectionScreen() {

}

void AddSectionScreen::initialize(unsigned int row, unsigned int col, std::string site, const Path & path) {
  defaultlegendtext = "[Enter] Modify - [Down] Next option - [Up] Previous option - [d]one - [c]ancel";
  currentlegendtext = defaultlegendtext;
  active = false;
  unsigned int y = 1;
  unsigned int x = 1;
  modsite = global->getSiteManager()->getSite(site);
  mso.clear();
  mso.addStringField(y++, x, "name", "Name:", "", false);
  mso.addStringField(y++, x, "path", "Path:", path.toString(), false, 64);
  init(row, col);
}

void AddSectionScreen::redraw() {
  ui->erase();
  bool highlight;
  for (unsigned int i = 0; i < mso.size(); i++) {
    std::shared_ptr<MenuSelectOptionElement> msoe = mso.getElement(i);
    highlight = false;
    if (mso.getSelectionPointer() == i) {
      highlight = true;
    }
    ui->printStr(msoe->getRow(), msoe->getCol(), msoe->getLabelText(), highlight);
    ui->printStr(msoe->getRow(), msoe->getCol() + msoe->getLabelText().length() + 1, msoe->getContentText());
  }
}

void AddSectionScreen::update() {
  std::shared_ptr<MenuSelectOptionElement> msoe = mso.getElement(mso.getLastSelectionPointer());
  ui->printStr(msoe->getRow(), msoe->getCol(), msoe->getLabelText());
  ui->printStr(msoe->getRow(), msoe->getCol() + msoe->getLabelText().length() + 1, msoe->getContentText());
  msoe = mso.getElement(mso.getSelectionPointer());
  ui->printStr(msoe->getRow(), msoe->getCol(), msoe->getLabelText(), true);
  ui->printStr(msoe->getRow(), msoe->getCol() + msoe->getLabelText().length() + 1, msoe->getContentText());
  if (active && msoe->cursorPosition() >= 0) {
    ui->showCursor();
    ui->moveCursor(msoe->getRow(), msoe->getCol() + msoe->getLabelText().length() + 1 + msoe->cursorPosition());
  }
  else {
    ui->hideCursor();
  }
}

bool AddSectionScreen::keyPressed(unsigned int ch) {
  if (active) {
    if (ch == 10) {
      activeelement->deactivate();
      active = false;
      currentlegendtext = defaultlegendtext;
      ui->setLegend();
      ui->update();
      return true;
    }
    activeelement->inputChar(ch);
    ui->update();
    return true;
  }
  bool activation;
  switch(ch) {
    case KEY_UP:
      mso.goUp();
      ui->update();
      return true;
    case KEY_DOWN:
      mso.goDown();
      ui->update();
      return true;
    case 10:

      activation = mso.getElement(mso.getSelectionPointer())->activate();
      if (!activation) {
        ui->update();
        return true;
      }
      active = true;
      activeelement = mso.getElement(mso.getSelectionPointer());
      currentlegendtext = activeelement->getLegendText();
      ui->setLegend();
      ui->update();
      return true;
    case 27: // esc
    case 'c':
      ui->returnToLast();
      return true;
    case 'd':
      std::shared_ptr<MenuSelectOptionTextField> field1 = std::static_pointer_cast<MenuSelectOptionTextField>(mso.getElement(0));
      std::shared_ptr<MenuSelectOptionTextField> field2 = std::static_pointer_cast<MenuSelectOptionTextField>(mso.getElement(1));
      std::string name = field1->getData();
      std::string path = field2->getData();
      modsite->addSection(name, path);
      global->getSettingsLoaderSaver()->saveSettings();
      ui->returnToLast();
      return true;
  }
  return false;
}

std::string AddSectionScreen::getLegendText() const {
  return currentlegendtext;
}

std::string AddSectionScreen::getInfoLabel() const {
  return "ADD SECTION";
}

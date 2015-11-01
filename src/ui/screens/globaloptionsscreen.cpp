#include "globaloptionsscreen.h"

#include "../../globalcontext.h"
#include "../../remotecommandhandler.h"
#include "../../sitemanager.h"
#include "../../site.h"
#include "../../iomanager.h"
#include "../../localstorage.h"
#include "../../util.h"
#include "../../settingsloadersaver.h"

#include "../ui.h"
#include "../focusablearea.h"
#include "../menuselectoptioncheckbox.h"
#include "../menuselectoptiontextfield.h"
#include "../menuselectoptionnumarrow.h"
#include "../menuselectoptiontextarrow.h"
#include "../menuselectoptiontextbutton.h"
#include "../menuselectoptionelement.h"

extern GlobalContext * global;

GlobalOptionsScreen::GlobalOptionsScreen(Ui * ui) {
  this->ui = ui;
}

GlobalOptionsScreen::~GlobalOptionsScreen() {

}

void GlobalOptionsScreen::initialize(unsigned int row, unsigned int col) {
  defaultlegendtext = "[Enter] Modify - [Down] Next option - [Up] Previous option - [d]one - [c]ancel";
  currentlegendtext = defaultlegendtext;
  active = false;
  unsigned int y = 1;
  unsigned int x = 1;
  rch = global->getRemoteCommandHandler();
  sm = global->getSiteManager();
  ls = global->getLocalStorage();
  mso.reset();
  defaultinterface = mso.addTextArrow(y++, x, "defaultinterface", "Default network interface:");
  defaultinterface->addOption("Unspecified", 0);
  interfacemap[0] = "";
  std::list<std::pair<std::string, std::string> > interfaces = global->getIOManager()->listInterfaces();
  int interfaceid = 1;
  bool hasdefault = global->getIOManager()->hasDefaultInterface();
  std::string defaultinterfacename;
  if (hasdefault) {
    defaultinterfacename = global->getIOManager()->getDefaultInterface();
  }
  for (std::list<std::pair<std::string, std::string> >::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
    if (it->first == "lo") {
      continue;
    }
    interfacemap[interfaceid] = it->first;
    defaultinterface->addOption(it->first + ", " + it->second, interfaceid++);
    if (hasdefault && it->first == defaultinterfacename) {
      defaultinterface->setOption(interfaceid - 1);
    }
  }
  y++;
  mso.addCheckBox(y++, x, "udpenable", "Enable remote commands:", rch->isEnabled());
  mso.addStringField(y++, x, "udpport", "Remote command UDP Port:", util::int2Str(rch->getUDPPort()), false, 5);
  mso.addStringField(y++, x, "udppass", "Remote command password:", rch->getPassword(), true);
  y++;
  mso.addCheckBox(y++, x, "legend", "Show legend bar:", ui->legendEnabled());
  y++;
  mso.addStringField(y++, x, "defuser", "Default site username:", sm->getDefaultUserName(), false);
  mso.addStringField(y++, x, "defpass", "Default site password:", sm->getDefaultPassword(), true);
  mso.addIntArrow(y++, x, "deflogins", "Default site login slots:", sm->getDefaultMaxLogins(), 0, 99);
  mso.addIntArrow(y++, x, "defmaxup", "Default site upload slots:", sm->getDefaultMaxUp(), 0, 99);
  mso.addIntArrow(y++, x, "defmaxdn", "Default site download slots:", sm->getDefaultMaxDown(), 0, 99);
  mso.addCheckBox(y++, x, "defsslconn", "Default site AUTH SSL:", sm->getDefaultSSL());
  Pointer<MenuSelectOptionTextArrow> sslfxp = mso.addTextArrow(y++, x, "sslfxp", "Default SSL transfers:");
  sslfxp->addOption("Always off", SITE_SSL_ALWAYS_OFF);
  sslfxp->addOption("Prefer off", SITE_SSL_PREFER_OFF);
  sslfxp->addOption("Prefer on", SITE_SSL_PREFER_ON);
  sslfxp->addOption("Always on", SITE_SSL_ALWAYS_ON);
  sslfxp->setOption(sm->getDefaultSSLTransferPolicy());
  mso.addStringField(y++, x, "defidletime", "Default site max idle time (s):", util::int2Str(sm->getDefaultMaxIdleTime()), false);
  y++;
  mso.addIntArrow(y++, x, "globalrank", "Global site rank:", sm->getGlobalRank(), 1, SITE_RANK_MAX);
  mso.addIntArrow(y++, x, "globalranktolerance", "Global site rank tolerance:", sm->getGlobalRankTolerance(), 1, SITE_RANK_MAX);
  y++;
  mso.addStringField(y++, x, "dlpath", "Download path:", ls->getDownloadPath(), false, 128, 128);
  y++;
  mso.addTextButtonNoContent(y++, x, "skiplist", "Configure skiplist...");
  mso.addTextButtonNoContent(y++, x, "proxy", "Configure proxy settings...");
  mso.addTextButtonNoContent(y++, x, "fileviewer", "Configure file viewing...");
  mso.addTextButtonNoContent(y++, x, "changekey", "Change encryption key...");
  init(row, col);
}

void GlobalOptionsScreen::redraw() {
  ui->erase();
  bool highlight;
  for (unsigned int i = 0; i < mso.size(); i++) {
    Pointer<MenuSelectOptionElement> msoe = mso.getElement(i);
    highlight = false;
    if (mso.getSelectionPointer() == i) {
      highlight = true;
    }
    ui->printStr(msoe->getRow(), msoe->getCol(), msoe->getLabelText(), highlight);
    ui->printStr(msoe->getRow(), msoe->getCol() + msoe->getLabelText().length() + 1, msoe->getContentText());
  }
}

void GlobalOptionsScreen::update() {
  Pointer<MenuSelectOptionElement> msoe = mso.getElement(mso.getLastSelectionPointer());
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

void GlobalOptionsScreen::keyPressed(unsigned int ch) {
  if (active) {
    if (ch == 10) {
      activeelement->deactivate();
      active = false;
      currentlegendtext = defaultlegendtext;
      ui->update();
      ui->setLegend();
      return;
    }
    activeelement->inputChar(ch);
    ui->update();
    return;
  }
  bool activation;
  Pointer<MenuSelectOptionElement> msoe;
  switch(ch) {
    case KEY_UP:
      mso.goUp();
      ui->update();
      break;
    case KEY_DOWN:
      mso.goDown();
      ui->update();
      break;
    case 10:
      msoe = mso.getElement(mso.getSelectionPointer());
      if (msoe->getIdentifier() == "skiplist") {
        ui->goSkiplist();
        return;
      }
      if (msoe->getIdentifier() == "changekey") {
        ui->goChangeKey();
        return;
      }
      if (msoe->getIdentifier() == "proxy") {
        ui->goProxy();
        return;
      }
      if (msoe->getIdentifier() == "fileviewer") {
        ui->goFileViewerSettings();
        return;
      }
      activation = msoe->activate();
      if (!activation) {
        ui->update();
        break;
      }
      active = true;
      activeelement = msoe;
      currentlegendtext = activeelement->getLegendText();
      ui->setLegend();
      ui->update();
      break;
    case 27: // esc
    case 'c':
      global->getSettingsLoaderSaver()->saveSettings();
      ui->returnToLast();
      break;
    case 'd':
      bool udpenable = false;
      for(unsigned int i = 0; i < mso.size(); i++) {
        Pointer<MenuSelectOptionElement> msoe = mso.getElement(i);
        std::string identifier = msoe->getIdentifier();
        if (identifier == "defaultinterface") {
          std::string interface = interfacemap[msoe.get<MenuSelectOptionTextArrow>()->getData()];
          global->getIOManager()->setDefaultInterface(interface);
        }
        if (identifier == "udpenable") {
          udpenable = msoe.get<MenuSelectOptionCheckBox>()->getData();
          if (rch->isEnabled() && !udpenable) {
            rch->setEnabled(false);
          }
        }
        else if (identifier == "udpport") {
          rch->setPort(util::str2Int(msoe.get<MenuSelectOptionTextField>()->getData()));
        }
        else if (identifier == "udppass") {
          rch->setPassword(msoe.get<MenuSelectOptionTextField>()->getData());
        }
        else if (identifier == "defuser") {
          sm->setDefaultUserName(msoe.get<MenuSelectOptionTextField>()->getData());
        }
        else if (identifier == "defpass") {
          sm->setDefaultPassword(msoe.get<MenuSelectOptionTextField>()->getData());
        }
        else if (identifier == "deflogins") {
          sm->setDefaultMaxLogins(msoe.get<MenuSelectOptionNumArrow>()->getData());
        }
        else if (identifier == "defmaxup") {
          sm->setDefaultMaxUp(msoe.get<MenuSelectOptionNumArrow>()->getData());
        }
        else if (identifier == "defmaxdn") {
          sm->setDefaultMaxDown(msoe.get<MenuSelectOptionNumArrow>()->getData());
        }
        else if (identifier == "defsslconn") {
          sm->setDefaultSSL(msoe.get<MenuSelectOptionCheckBox>()->getData());
        }
        else if (identifier == "sslfxp") {
          sm->setDefaultSSLTransferPolicy(msoe.get<MenuSelectOptionTextArrow>()->getData());
        }
        else if (identifier == "defidletime") {
          sm->setDefaultMaxIdleTime(util::str2Int(msoe.get<MenuSelectOptionTextField>()->getData()));
        }
        else if (identifier == "globalrank") {
          sm->setGlobalRank(msoe.get<MenuSelectOptionNumArrow>()->getData());
        }
        else if (identifier == "globalranktolerance") {
          sm->setGlobalRankTolerance(msoe.get<MenuSelectOptionNumArrow>()->getData());
        }
        else if (identifier == "legend") {
          ui->showLegend(msoe.get<MenuSelectOptionCheckBox>()->getData());
        }
        else if (identifier == "dlpath") {
          ls->setDownloadPath(msoe.get<MenuSelectOptionTextField>()->getData());
        }
      }
      rch->setEnabled(udpenable);
      global->getSettingsLoaderSaver()->saveSettings();
      ui->returnToLast();
      break;
  }
}

std::string GlobalOptionsScreen::getLegendText() const {
  return currentlegendtext;
}

std::string GlobalOptionsScreen::getInfoLabel() const {
  return "GLOBAL SETTINGS";
}

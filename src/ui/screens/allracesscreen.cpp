#include "allracesscreen.h"

#include "../ui.h"
#include "../menuselectadjustableline.h"
#include "../menuselectoptionelement.h"
#include "../resizableelement.h"
#include "../menuselectoptiontextbutton.h"

#include "../../race.h"
#include "../../engine.h"
#include "../../globalcontext.h"
#include "../../util.h"

extern GlobalContext * global;

AllRacesScreen::AllRacesScreen(Ui * ui) {
  this->ui = ui;
}

void AllRacesScreen::initialize(unsigned int row, unsigned int col) {
  autoupdate = true;
  hascontents = false;
  engine = global->getEngine();
  table.reset();
  table.enterFocusFrom(0);
  init(row, col);
}

void AllRacesScreen::redraw() {
  ui->erase();
  int y = 0;
  table.clear();
  addRaceTableHeader(y, table, "RELEASE");
  y++;
  for (std::list<Pointer<Race> >::const_iterator it = --engine->getRacesEnd(); it != --engine->getRacesBegin(); it--) {
    addRaceDetails(y++, table, *it);
  }
  table.checkPointer();
  hascontents = table.linesSize() > 1;
  table.adjustLines(col - 3);
  bool highlight;
  for (unsigned int i = 0; i < table.size(); i++) {
    Pointer<ResizableElement> re = table.getElement(i);
    highlight = false;
    if (table.getSelectionPointer() == i && hascontents) {
      highlight = true;
    }
    if (re->isVisible()) {
      ui->printStr(re->getRow(), re->getCol(), re->getLabelText(), highlight);
    }
  }
}

void AllRacesScreen::update() {
  redraw();
}

void AllRacesScreen::keyPressed(unsigned int ch) {
  switch (ch) {
    case KEY_UP:
      if (hascontents && table.goUp()) {
        ui->update();
      }
      break;
    case KEY_DOWN:
      if (hascontents && table.goDown()) {
        ui->update();
      }
      break;
    case 'c':
    case 27: // esc
      ui->returnToLast();
      break;
    case 10:
      if (hascontents) {
        Pointer<MenuSelectOptionTextButton> msotb =
            table.getElement(table.getSelectionPointer());
        ui->goRaceStatus(msotb->getId());
      }
      break;
  }
}

std::string AllRacesScreen::getLegendText() const {
  return "[Esc/c] Return - [Enter] Details - [Up/Down] Navigate";
}

std::string AllRacesScreen::getInfoLabel() const {
  return "ALL RACES";
}

std::string AllRacesScreen::getInfoText() const {
  return "Active: " + util::int2Str(engine->currentRaces()) + "  Total: " + util::int2Str(engine->allRaces());
}

void AllRacesScreen::addRaceTableHeader(unsigned int y, MenuSelectOption & mso, std::string release) {
  addRaceTableRow(y, mso, -1, false, "STARTED", "USE", "SECTION", release, "SIZE", "WORST", "AVG", "BEST", "STATUS", "DONE", "SITES");
}

void AllRacesScreen::addRaceTableRow(unsigned int y, MenuSelectOption & mso, unsigned int id, bool selectable,
    std::string timestamp, std::string timespent, std::string section, std::string release,
    std::string size, std::string worst, std::string avg, std::string best, std::string status, std::string done,
    std::string sites)
{
  Pointer<MenuSelectAdjustableLine> msal = mso.addAdjustableLine();
  Pointer<MenuSelectOptionTextButton> msotb;

  msotb = mso.addTextButtonNoContent(y, 1, "timestamp", timestamp);
  msotb->setSelectable(false);
  msal->addElement(msotb, 7, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "timespent", timespent);
  msotb->setSelectable(false);
  msal->addElement(msotb, 8, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "section", section);
  msotb->setSelectable(false);
  msal->addElement(msotb, 3, RESIZE_REMOVE);

  msotb = mso.addTextButton(y, 1, "release", release);
  if (!selectable) {
    msotb->setSelectable(false);
  }
  msotb->setId(id);
  msal->addElement(msotb, 12, RESIZE_CUTEND, true);

  msotb = mso.addTextButtonNoContent(y, 1, "size", size);
  msotb->setSelectable(false);
  msal->addElement(msotb, 10, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "worst", worst);
  msotb->setSelectable(false);
  msal->addElement(msotb, 2, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "avg", avg);
  msotb->setSelectable(false);
  msal->addElement(msotb, 5, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "best", best);
  msotb->setSelectable(false);
  msal->addElement(msotb, 4, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "status", status);
  msotb->setSelectable(false);
  msal->addElement(msotb, 11, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "done", done);
  msotb->setSelectable(false);
  msal->addElement(msotb, 6, RESIZE_REMOVE);

  msotb = mso.addTextButtonNoContent(y, 1, "sites", sites);
  msotb->setSelectable(false);
  msal->addElement(msotb, 1, RESIZE_WITHDOTS);
}

void AllRacesScreen::addRaceDetails(unsigned int y, MenuSelectOption & mso, Pointer<Race> race) {
  std::string done = util::int2Str(race->numSitesDone()) + "/" + util::int2Str(race->numSites());
  std::string timespent = util::simpleTimeFormat(race->getTimeSpent());
  std::string status;
  std::string worst = util::int2Str(race->getWorstCompletionPercentage()) + "%";
  std::string avg = util::int2Str(race->getAverageCompletionPercentage()) + "%";
  std::string best = util::int2Str(race->getBestCompletionPercentage()) + "%";
  std::string size = util::parseSize(race->estimatedTotalSize());
  switch (race->getStatus()) {
    case RACE_STATUS_RUNNING:
      status = "running";
      break;
    case RACE_STATUS_DONE:
      status = "done";
      break;
    case RACE_STATUS_ABORTED:
      status = "aborted";
      break;
    case RACE_STATUS_TIMEOUT:
      status = "timeout";
      break;
  }
  addRaceTableRow(y, mso, race->getId(), true, race->getTimeStamp(), timespent, race->getSection(),
                  race->getName(), size, worst, avg, best, status, done,
                  race->getSiteListText());
}

#pragma once

#include <string>
#include <list>
#include <vector>

#include "ncurseswrap.h"
#include "uicommand.h"

#include "../core/threading.h"
#include "../core/semaphore.h"
#include "../core/eventreceiver.h"
#include "../core/blockingqueue.h"
#include "../core/pointer.h"
#include "../uibase.h"
#include "../settingsloadersaver.h"

class UIWindow;
class InfoWindow;
class LegendWindow;
class FileList;
class UIFileList;
class Site;
class DataFileHandler;
class TransferStatus;
class Path;
class SkipList;
class RawBuffer;
class LoginScreen;
class NewKeyScreen;
class MainScreen;
class ConfirmationScreen;
class EditSiteScreen;
class SiteStatusScreen;
class RawDataScreen;
class RawCommandScreen;
class BrowseScreen;
class AddSectionScreen;
class NewRaceScreen;
class RaceStatusScreen;
class GlobalOptionsScreen;
class SkipListScreen;
class ChangeKeyScreen;
class EventLogScreen;
class ProxyOptionsScreen;
class EditProxyScreen;
class ViewFileScreen;
class NukeScreen;
class FileViewerSettingsScreen;
class ScoreBoardScreen;
class SelectSitesScreen;
class TransfersScreen;
class TransferJobStatusScreen;
class AllRacesScreen;
class AllTransferJobsScreen;
class TransferStatusScreen;
class TransfersFilterScreen;
class InfoScreen;
class SelectJobsScreen;
class MakeDirScreen;

class LegendPrinterKeybinds;
struct TransferFilteringParameters;

enum LegendMode {
  LEGEND_DISABLED = 123,
  LEGEND_SCROLLING = 124,
  LEGEND_STATIC = 125
};

class Ui : public EventReceiver, public UIBase, public SettingsAdder {
private:
  Thread<Ui> thread;
  BlockingQueue<UICommand> uiqueue;
  WINDOW * main;
  WINDOW * info;
  WINDOW * legend;
  std::vector<Pointer<UIWindow> > mainwindows;
  Pointer<UIWindow> topwindow;
  Pointer<InfoWindow> infowindow;
  Pointer<LegendWindow> legendwindow;
  Pointer<LoginScreen> loginscreen;
  Pointer<NewKeyScreen> newkeyscreen;
  Pointer<MainScreen> mainscreen;
  Pointer<ConfirmationScreen> confirmationscreen;
  Pointer<EditSiteScreen> editsitescreen;
  Pointer<SiteStatusScreen> sitestatusscreen;
  Pointer<RawDataScreen> rawdatascreen;
  Pointer<RawCommandScreen> rawcommandscreen;
  Pointer<BrowseScreen> browsescreen;
  Pointer<AddSectionScreen> addsectionscreen;
  Pointer<NewRaceScreen> newracescreen;
  Pointer<RaceStatusScreen> racestatusscreen;
  Pointer<GlobalOptionsScreen> globaloptionsscreen;
  Pointer<SkipListScreen> skiplistscreen;
  Pointer<ChangeKeyScreen> changekeyscreen;
  Pointer<EventLogScreen> eventlogscreen;
  Pointer<ProxyOptionsScreen> proxyoptionsscreen;
  Pointer<EditProxyScreen> editproxyscreen;
  Pointer<ViewFileScreen> viewfilescreen;
  Pointer<NukeScreen> nukescreen;
  Pointer<FileViewerSettingsScreen> fileviewersettingsscreen;
  Pointer<ScoreBoardScreen> scoreboardscreen;
  Pointer<SelectSitesScreen> selectsitesscreen;
  Pointer<TransfersScreen> transfersscreen;
  Pointer<TransferJobStatusScreen> transferjobstatusscreen;
  Pointer<AllRacesScreen> allracesscreen;
  Pointer<AllTransferJobsScreen> alltransferjobsscreen;
  Pointer<TransferStatusScreen> transferstatusscreen;
  Pointer<TransfersFilterScreen> transfersfilterscreen;
  Pointer<InfoScreen> infoscreen;
  Pointer<SelectJobsScreen> selectjobsscreen;
  Pointer<MakeDirScreen> makedirscreen;
  Pointer<LegendPrinterKeybinds> legendprinterkeybinds;
  int mainrow;
  int maincol;
  int col;
  int row;
  int ticker;
  bool haspushed;
  bool pushused;
  bool initret;
  bool legendenabled;
  bool infoenabled;
  bool dead;
  LegendMode legendmode;
  bool split;
  bool fullscreentoggle;
  std::string eventtext;
  Semaphore eventcomplete;
  std::list<Pointer<UIWindow> > history;
  void FDData(int);
  void refreshAll();
  void initIntern();
  void enableInfo();
  void disableInfo();
  void enableLegend();
  void disableLegend();
  void redrawAll();
  void switchToWindow(Pointer<UIWindow>);
  void switchToWindow(Pointer<UIWindow>, bool);
  void tick(int);
  void globalKeyBinds(int);
  void switchToLast();
  void setLegendText(const std::string & legendtext);
public:
  Ui();
  ~Ui();
  void run();
  bool init();
  void backendPush();
  void terminalSizeChanged();
  void signal(int, int);
  void kill();
  void resizeTerm();
  void readConfiguration();
  void writeState();
  LegendMode legendMode() const;
  void setLegendMode(LegendMode);
  void returnToLast();
  void update();
  void setLegend();
  void addTempLegendTransferJob(unsigned int id);
  void addTempLegendSpreadJob(unsigned int id);
  void setInfo();
  void setSplit(bool);
  void redraw();
  void erase();
  void erase(WINDOW *);
  void showCursor();
  void hideCursor();
  void moveCursor(unsigned int, unsigned int);
  void highlight(bool);
  void highlight(WINDOW *, bool);
  void printStr(unsigned int, unsigned int, const std::string &);
  void printStr(unsigned int, unsigned int, const std::basic_string<unsigned int> &);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::string &);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::basic_string<unsigned int> &);
  void printStr(unsigned int, unsigned int, const std::string &, bool);
  void printStr(unsigned int, unsigned int, const std::basic_string<unsigned int> &, bool);
  void printStr(unsigned int, unsigned int, const std::string &, unsigned int);
  void printStr(unsigned int, unsigned int, const std::basic_string<unsigned int> &, unsigned int);
  void printStr(unsigned int, unsigned int, const std::string &, unsigned int, bool);
  void printStr(unsigned int, unsigned int, const std::basic_string<unsigned int> &, unsigned int, bool);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::string &, unsigned int, bool);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::basic_string<unsigned int> &, unsigned int, bool);
  void printStr(unsigned int, unsigned int, const std::string &, unsigned int, bool, bool);
  void printStr(unsigned int, unsigned int, const std::basic_string<unsigned int> &, unsigned int, bool, bool);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::string &, unsigned int, bool, bool);
  void printStr(WINDOW *, unsigned int, unsigned int, const std::basic_string<unsigned int> &, unsigned int, bool, bool);
  void printChar(unsigned int, unsigned int, unsigned int);
  void printChar(WINDOW *, unsigned int, unsigned int, unsigned int);
  void printChar(unsigned int, unsigned int, unsigned int, bool);
  void printChar(WINDOW *, unsigned int, unsigned int, unsigned int, bool);
  void goRawCommand(const std::string &);
  void goRawCommand(const std::string &, const Path &);
  void goRawCommand(const std::string &, const Path &, const std::string &);
  void goConfirmation(const std::string & message);
  void goStrongConfirmation(const std::string & message);
  void goNuke(const std::string & site, const std::list<std::pair<std::string, bool> > & items, const Path & path);
  void goViewFile(const std::string &, const std::string &, FileList *);
  void goViewFile(const Path &, const std::string &);
  void goAddSection(const std::string &, const Path &);
  void goNewRace(const std::string & site, const std::list<std::string> & sections, const std::list<std::pair<std::string, bool> > & items);
  void goSelectSites(const std::string &, std::list<Pointer<Site> >, std::list<Pointer<Site> >);
  void goSelectSitesFrom(const std::string &, std::list<Pointer<Site> >, std::list<Pointer<Site> >);
  void goSelectSpreadJobs();
  void goSelectTransferJobs();
  void goSkiplist();
  void goSkiplist(SkipList *);
  void goChangeKey();
  void goProxy();
  void goFileViewerSettings();
  void goSiteStatus(const std::string &);
  void goRaceStatus(unsigned int);
  void goTransferJobStatus(unsigned int);
  void goTransferStatus(Pointer<TransferStatus>);
  void goGlobalOptions();
  void goEventLog();
  void goScoreBoard();
  void goTransfers();
  void goTransfersFilterSite(const std::string &);
  void goTransfersFilterSpreadJob(const std::string &);
  void goTransfersFilterTransferJob(const std::string &);
  void returnTransferFilters(const TransferFilteringParameters &);
  void goTransfersFiltering(const TransferFilteringParameters &);
  void goEditSite(const std::string &);
  void goAddSite();
  void goBrowse(const std::string &);
  void goBrowseSplit(const std::string &);
  void goBrowseLocal();
  void goContinueBrowsing();
  void goAddProxy();
  void goEditProxy(const std::string &);
  void goRawData(const std::string &);
  void goRawDataJump(const std::string &, int);
  void goRawBuffer(RawBuffer * rawbuffer, const std::string & label, const std::string & infotext);
  void goAllRaces();
  void goAllTransferJobs();
  void goInfo();
  void goMakeDir(const std::string & site, UIFileList & filelist);
  void returnSelectItems(const std::string &);
  void key(const std::string &);
  void newKey(const std::string &);
  void confirmYes();
  void confirmNo();
  void returnNuke(const std::list<int> & reqids);
  void returnRaceStatus(unsigned int);
  void returnMakeDir(const std::string & dirname);
  void loadSettings(Pointer<DataFileHandler>);
  void saveSettings(Pointer<DataFileHandler>);
  void notify();
  WINDOW * getLegendWindow() const;
};


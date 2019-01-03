#pragma once

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/eventreceiver.h"

enum class PrioType;
enum class SitePriority;
class CommandOwner;
class Race;
class TransferJob;
class SiteTransferJob;
class SiteRace;
class FileList;
class File;
class ScoreBoard;
class SiteLogic;
class PendingTransfer;
class Site;
class PreparedRace;
class Path;
class SkipList;
class ScoreBoardElement;

class Engine : public EventReceiver {
public:
  Engine();
  ~Engine();
  std::shared_ptr<Race> newRace(const std::string &, const std::string &, const std::list<std::string> &);
  std::shared_ptr<Race> newRace(const std::string &, const std::string &);
  bool prepareRace(const std::string &, const std::string &, const std::list<std::string> &);
  bool prepareRace(const std::string &, const std::string &);
  std::shared_ptr<Race> newDistribute(const std::string &, const std::string &, const std::list<std::string> &);
  std::shared_ptr<Race> newDistribute(const std::string &, const std::string &);
  void startPreparedRace(unsigned int);
  void deletePreparedRace(unsigned int);
  void startLatestPreparedRace();
  void toggleStartNextPreparedRace();
  unsigned int newTransferJobDownload(const std::string &, FileList *, const std::string &, const Path &);
  unsigned int newTransferJobDownload(const std::string &, FileList *, const std::string &, const Path &, const std::string &);
  unsigned int newTransferJobDownload(const std::string &, const Path &, const std::string &, const Path &, const std::string &);
  unsigned int newTransferJobUpload(const Path &, const std::string &, const std::string &, FileList *);
  unsigned int newTransferJobUpload(const Path &, const std::string &, const std::string &, FileList *, const std::string &);
  unsigned int newTransferJobUpload(const Path &, const std::string &, const std::string &, const Path &, const std::string &);
  unsigned int newTransferJobFXP(const std::string &, FileList *, const std::string &, FileList *, const std::string &);
  unsigned int newTransferJobFXP(const std::string &, FileList *, const std::string &, const std::string &, FileList *, const std::string &);
  unsigned int newTransferJobFXP(const std::string &, const Path &, const std::string &, const std::string &, const Path &, const std::string &);
  void removeSiteFromRace(std::shared_ptr<Race> &, const std::string &);
  void removeSiteFromRaceDeleteFiles(std::shared_ptr<Race> &, const std::string &, bool);
  void abortRace(std::shared_ptr<Race> &);
  void resetRace(std::shared_ptr<Race> &, bool);
  void deleteOnAllSites(std::shared_ptr<Race> &);
  void deleteOnSites(std::shared_ptr<Race> &, std::list<std::shared_ptr<Site> >);
  void deleteOnSites(std::shared_ptr<Race> &, std::list<std::shared_ptr<Site> >, bool);
  void abortTransferJob(std::shared_ptr<TransferJob> &);
  void jobFileListRefreshed(SiteLogic *, CommandOwner *, FileList *);
  bool transferJobActionRequest(std::shared_ptr<SiteTransferJob> &);
  void raceActionRequest();
  void setPreparedRaceExpiryTime(int);
  void clearSkipListCaches();
  unsigned int preparedRaces() const;
  unsigned int currentRaces() const;
  unsigned int allRaces() const;
  unsigned int currentTransferJobs() const;
  unsigned int allTransferJobs() const;
  std::shared_ptr<Race> getRace(unsigned int) const;
  std::shared_ptr<Race> getRace(const std::string &) const;
  std::shared_ptr<TransferJob> getTransferJob(unsigned int) const;
  std::list<std::shared_ptr<PreparedRace> >::const_iterator getPreparedRacesBegin() const;
  std::list<std::shared_ptr<PreparedRace> >::const_iterator getPreparedRacesEnd() const;
  std::list<std::shared_ptr<Race> >::const_iterator getRacesBegin() const;
  std::list<std::shared_ptr<Race> >::const_iterator getRacesEnd() const;
  std::list<std::shared_ptr<TransferJob> >::const_iterator getTransferJobsBegin() const;
  std::list<std::shared_ptr<TransferJob> >::const_iterator getTransferJobsEnd() const;
  void tick(int);
  void addSiteToRace(std::shared_ptr<Race> &, const std::string &);
  std::shared_ptr<ScoreBoard> getScoreBoard() const;
  int getMaxPointsRaceTotal() const;
  int getMaxPointsFileSize() const;
  int getMaxPointsAvgSpeed() const;
  int getMaxPointsPriority() const;
  int getMaxPointsPercentageOwned() const;
  int getMaxPointsLowProgress() const;
  int getPriorityPoints(SitePriority priority) const;
  int getSpeedPoints(int) const;
  int getPreparedRaceExpiryTime() const;
  bool getNextPreparedRaceStarterEnabled() const;
  int getNextPreparedRaceStarterTimeRemaining() const;
 private:
  std::shared_ptr<Race> newSpreadJob(int, const std::string &, const std::string &, const std::list<std::string> &);
  std::shared_ptr<Race> newSpreadJob(int, const std::string &, const std::string &);
  void estimateRaceSizes();
  void estimateRaceSize(const std::shared_ptr<Race> &);
  void estimateRaceSize(const std::shared_ptr<Race> &, bool);
  void reportCurrentSize(const SkipList &, const SkipList &, SiteRace *, FileList *, bool final);
  void addToScoreBoard(FileList * fl, SiteRace * sr, SiteLogic * slp);
  void addToScoreBoardForPair(const std::shared_ptr<SiteLogic> & sls,
      const std::shared_ptr<Site> & ss, SiteRace * srs,
      FileList * fls, const std::shared_ptr<SiteLogic> & sld, const std::shared_ptr<Site> & ds,
      SiteRace * srd, FileList * fld, const SkipList & dstskip,
      const SkipList & secskip,
      const std::shared_ptr<Race> & race, const Path & subpath, SitePriority priority,
      bool racemode);
  void updateScoreBoard();
  void refreshScoreBoard();
  void issueOptimalTransfers();
  void setSpeedScale();
  unsigned short calculateScore(PrioType priotype, unsigned long long int filesize, const std::shared_ptr<Race> &, FileList *, SiteRace *, FileList *, SiteRace *, int, SitePriority priority, bool) const;
  unsigned short calculateScore(ScoreBoardElement * sbe) const;
  void checkIfRaceComplete(SiteLogic *, std::shared_ptr<Race> &);
  void raceComplete(std::shared_ptr<Race>);
  void transferJobComplete(std::shared_ptr<TransferJob>);
  void issueGlobalComplete(const std::shared_ptr<Race> & race);
  void refreshPendingTransferList(std::shared_ptr<TransferJob> &);
  void checkStartPoke();
  void addPendingTransfer(std::list<PendingTransfer> &, PendingTransfer &);
  std::shared_ptr<Race> getCurrentRace(const std::string &) const;
  void preSeedPotentialData(std::shared_ptr<Race> &);
  bool raceTransferPossible(const std::shared_ptr<SiteLogic> &, const std::shared_ptr<SiteLogic> &, std::shared_ptr<Race> &) const;
  void wipeFromScoreBoard(SiteRace * sr);
  bool waitingInScoreBoard(const std::shared_ptr<Race> & race) const;
  bool transferExpectedSoon(ScoreBoardElement * sbe) const;
  std::list<std::shared_ptr<Race> > allraces;
  std::list<std::shared_ptr<Race> > currentraces;
  std::list<std::shared_ptr<PreparedRace> > preparedraces;
  std::list<std::shared_ptr<TransferJob>  > alltransferjobs;
  std::list<std::shared_ptr<TransferJob> > currenttransferjobs;
  std::shared_ptr<ScoreBoard> scoreboard;
  std::unordered_map<std::shared_ptr<TransferJob>, std::list<PendingTransfer> > pendingtransfers;
  int maxavgspeed;
  bool pokeregistered;
  unsigned int dropped;
  unsigned int nextid;
  int maxpointsfilesize;
  int maxpointsavgspeed;
  int maxpointspriority;
  int maxpointspercentageowned;
  int maxpointslowprogress;
  int preparedraceexpirytime;
  bool startnextprepared;
  int nextpreparedtimeremaining;
  std::unordered_map<FileList *, std::pair<SiteRace *, std::shared_ptr<SiteLogic>>> spreadjobfilelistschanged;
  bool forcescoreboard;
};

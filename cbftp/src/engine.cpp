#include "engine.h"

#include <cstdlib>

#include "core/workmanager.h"
#include "core/tickpoke.h"
#include "scoreboard.h"
#include "scoreboardelement.h"
#include "globalcontext.h"
#include "sitelogic.h"
#include "site.h"
#include "filelist.h"
#include "file.h"
#include "path.h"
#include "sitelogicmanager.h"
#include "transfermanager.h"
#include "race.h"
#include "siterace.h"
#include "skiplist.h"
#include "eventlog.h"
#include "sitemanager.h"
#include "transferjob.h"
#include "pendingtransfer.h"
#include "localstorage.h"
#include "localfilelist.h"
#include "transferstatus.h"
#include "util.h"
#include "preparedrace.h"

#define SPREAD 0
#define POKEINTERVAL 1000
#define MAXCHECKSTIMEOUT 60
#define STATICTIMEFORCOMPLETION 5000
#define DIROBSERVETIME 20000
#define SFVDIROBSERVETIME 5000

Engine::Engine() :
  scoreboard(makePointer<ScoreBoard>()),
  maxavgspeed(1024),
  pokeregistered(false),
  nextid(0),
  maxpointsfilesize(2000),
  maxpointsavgspeed(3000),
  maxpointspriority(2500),
  maxpointspercentageowned(2000),
  maxpointslowprogress(2000),
  preparedraceexpirytime(120)
{
}

Engine::~Engine() {

}

Pointer<Race> Engine::newSpreadJob(int profile, const std::string & release, const std::string & section, const std::list<std::string> & sites) {
  Pointer<Race> race;
  bool append = false;
  for (std::list<Pointer<Race> >::iterator it = allraces.begin(); it != allraces.end(); it++) {
    if ((*it)->getName() == release && (*it)->getSection() == section) {
      race = *it;
      append = true;
      break;
    }
  }
  if (!global->getSkipList()->isAllowed(release, true, false)) {
    global->getEventLog()->log("Engine", "Race skipped due to skiplist match: " + release);
    return Pointer<Race>();
  }
  if (release.find("/") != std::string::npos) {
    global->getEventLog()->log("Engine", "Race skipped due to invalid target: " + release);
    return Pointer<Race>();
  }
  if (!race) {
    race = makePointer<Race>(nextid++, static_cast<SpreadProfile>(profile), release, section);
  }
  std::list<std::string> addsites;
  std::list<Pointer<SiteLogic> > addsiteslogics;
  for (std::list<std::string>::const_iterator it = sites.begin(); it != sites.end(); it++) {
    const Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(*it);
    if (!sl) {
      global->getEventLog()->log("Engine", "Trying to race a nonexisting site: " + *it);
      continue;
    }
    if (sl->getSite()->getDisabled()) {
      global->getEventLog()->log("Engine", "Skipping disabled site: " + *it);
      continue;
    }
    if (!sl->getSite()->hasSection(section)) {
      global->getEventLog()->log("Engine", "Trying to use an undefined section: " +
          section + " on " + *it);
      continue;
    }
    if (checkBannedGroup(section, sl->getSite(), race->getGroup())) {
      continue;
    }
    bool add = true;
    for (std::list<std::string>::iterator it2 = addsites.begin(); it2 != addsites.end(); it2++) {
      if (*it == *it2) {
        add = false;
        break;
      }
    }
    if (add && append) {
      if (race->getSiteRace(*it) != NULL) {
        continue;
      }
    }
    if (add) {
      addsites.push_back(*it);
      addsiteslogics.push_back(sl);
    }
  }
  if (addsites.size() < 2 && !append) {
    global->getEventLog()->log("Engine", "Ignoring attempt to race " + release + " in "
        + section + " on less than 2 sites.");
    return Pointer<Race>();
  }
  if (addsites.size() > 0 || append) {
    checkStartPoke();
    if (profile == SPREAD_PREPARE) {
      global->getEventLog()->log("Engine", "Preparing race: " + section + "/" + release +
                " on " + util::int2Str((int)addsites.size()) + " sites.");
      preparedraces.push_back(makePointer<PreparedRace>(race->getId(), release, section, addsites, preparedraceexpirytime));
      for (std::list<Pointer<SiteLogic> >::const_iterator ait = addsiteslogics.begin(); ait != addsiteslogics.end(); ait++) {
        (*ait)->activateAll();
      }
      return Pointer<Race>();
    }
    bool readdtocurrent = true;
    if (append) {
      for (std::list<Pointer<Race> >::iterator it = currentraces.begin(); it != currentraces.end(); it++) {
        if (*it == race) {
          readdtocurrent = false;
          break;
        }
      }
      if (readdtocurrent) {
        global->getEventLog()->log("Engine", "Reactivating race: " + section + "/" + release);
        race->setUndone();
        for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator it = race->begin(); it != race->end(); it++) {
          it->second->activateAll();
        }
      }
    }
    for (std::list<std::string>::iterator it = addsites.begin(); it != addsites.end(); it++) {
      addSiteToRace(race, *it);
    }
    if (!append) {
      currentraces.push_back(race);
      allraces.push_back(race);
      dropped = 0;
      global->getEventLog()->log("Engine", "Starting race: " + section + "/" + release +
          " on " + util::int2Str((int)addsites.size()) + " sites.");
    }
    else if (addsites.size()) {
      if (readdtocurrent) {
        currentraces.push_back(race);
      }
      global->getEventLog()->log("Engine", "Appending to race: " + section + "/" + release +
          " with " + util::int2Str((int)addsites.size()) + " site" + (addsites.size() > 1 ? "s" : "") + ".");
    }
    setSpeedScale();
    preSeedPotentialData(race);
  }
  return race;
}

Pointer<Race> Engine::newSpreadJob(int profile, const std::string & release, const std::string & section) {
  std::list<std::string> sites;
  for (std::vector<Pointer<Site> >::const_iterator it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); it++) {
    if ((*it)->hasSection(section) && !(*it)->getDisabled()) {
      sites.push_back((*it)->getName());
    }
  }
  return newSpreadJob(profile, release, section, sites);
}

Pointer<Race> Engine::newRace(const std::string & release, const std::string & section, const std::list<std::string> & sites) {
  return newSpreadJob(SPREAD_RACE, release, section, sites);
}

Pointer<Race> Engine::newRace(const std::string & release, const std::string & section) {
  return newSpreadJob(SPREAD_RACE, release, section);
}

Pointer<Race> Engine::newDistribute(const std::string & release, const std::string & section, const std::list<std::string> & sites) {
  return newSpreadJob(SPREAD_DISTRIBUTE, release, section, sites);
}

void Engine::prepareRace(const std::string & release, const std::string & section, const std::list<std::string> & sites) {
  newSpreadJob(SPREAD_PREPARE, release, section, sites);
}

void Engine::prepareRace(const std::string & release, const std::string & section) {
  newSpreadJob(SPREAD_PREPARE, release, section);
}

void Engine::startPreparedRace(unsigned int id) {
  for (std::list<Pointer<PreparedRace> >::iterator it = preparedraces.begin(); it != preparedraces.end(); it++) {
    if ((*it)->getId() == id) {
      newRace((*it)->getRelease(), (*it)->getSection(), (*it)->getSites());
      preparedraces.erase(it);
      return;
    }
  }
}

void Engine::deletePreparedRace(unsigned int id) {
  for (std::list<Pointer<PreparedRace> >::iterator it = preparedraces.begin(); it != preparedraces.end(); it++) {
    if ((*it)->getId() == id) {
      preparedraces.erase(it);
      return;
    }
  }
}

void Engine::startLatestPreparedRace() {
  if (preparedraces.size()) {
    Pointer<PreparedRace> preparedrace = preparedraces.back();
    preparedraces.pop_back();
    newRace(preparedrace->getRelease(), preparedrace->getSection(), preparedrace->getSites());
  }
}

void Engine::newTransferJobDownload(const std::string & site, const std::string & file, FileList * filelist, const Path & path) {
  newTransferJobDownload(site, file, filelist, path, file);
}

void Engine::newTransferJobUpload(const Path & path, const std::string & site, const std::string & file, FileList * filelist) {
  newTransferJobUpload(path, file, site, file, filelist);
}

void Engine::newTransferJobFXP(const std::string & srcsite, FileList * srcfilelist, const std::string & dstsite, FileList * dstfilelist, const std::string & file) {
  newTransferJobFXP(srcsite, file, srcfilelist, dstsite, file, dstfilelist);
}

void Engine::newTransferJobDownload(const std::string & site, const std::string & srcfile, FileList * filelist, const Path & path, const std::string & dstfile) {
  const Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(site);
  Pointer<TransferJob> tj = makePointer<TransferJob>(nextid++, sl, srcfile, filelist, path, dstfile);
  alltransferjobs.push_back(tj);
  currenttransferjobs.push_back(tj);
  global->getEventLog()->log("Engine", "Starting download job: " + srcfile +
            " from " + site);
  sl->addTransferJob(tj);
  checkStartPoke();
}

void Engine::newTransferJobUpload(const Path & path, const std::string & srcfile, const std::string & site, const std::string & dstfile, FileList * filelist) {
  const Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(site);
  Pointer<TransferJob> tj = makePointer<TransferJob>(nextid++, path, srcfile, sl, dstfile, filelist);
  alltransferjobs.push_back(tj);
  currenttransferjobs.push_back(tj);
  global->getEventLog()->log("Engine", "Starting upload job: " + srcfile +
            " to " + site);
  sl->addTransferJob(tj);
  checkStartPoke();
}

void Engine::newTransferJobFXP(const std::string & srcsite, const std::string & srcfile, FileList * srcfilelist, const std::string & dstsite, const std::string & dstfile, FileList * dstfilelist) {
  const Pointer<SiteLogic> slsrc = global->getSiteLogicManager()->getSiteLogic(srcsite);
  const Pointer<SiteLogic> sldst = global->getSiteLogicManager()->getSiteLogic(dstsite);
  Pointer<TransferJob> tj = makePointer<TransferJob>(nextid++, slsrc, srcfile, srcfilelist, sldst, dstfile, dstfilelist);
  alltransferjobs.push_back(tj);
  currenttransferjobs.push_back(tj);
  global->getEventLog()->log("Engine", "Starting FXP job: " + srcfile +
            " - " + srcsite + " -> " + dstsite);
  slsrc->addTransferJob(tj);
  sldst->addTransferJob(tj);
  checkStartPoke();
}

void Engine::removeSiteFromRace(Pointer<Race> & race, const std::string & site) {
  if (!!race) {
    SiteRace * sr = race->getSiteRace(site);
    if (sr != NULL) {
      const Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(site);
      sl->abortRace(race->getId());
      race->removeSite(sr);
    }
  }
}

void Engine::abortRace(Pointer<Race> & race) {
  if (!!race) {
    race->abort();
    for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator it = race->begin(); it != race->end(); it++) {
      it->second->abortRace(race->getId());
    }
    currentraces.remove(race);
    global->getEventLog()->log("Engine", "Race aborted: " + race->getName());
  }
}

void Engine::resetRace(Pointer<Race> & race) {
  if (!!race) {
    race->reset();
    for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator it = race->begin(); it != race->end(); it++) {
      it->first->reset();
      it->second->activateAll();
    }
    bool current = false;
    for (std::list<Pointer<Race> >::iterator it = currentraces.begin(); it != currentraces.end(); it++) {
      if (*it == race) {
        current = true;
        break;
      }
    }
    if (!current) {
      currentraces.push_back(race);
    }
    checkStartPoke();
    global->getEventLog()->log("Engine", "Race reset: " + race->getName());
  }
}

void Engine::deleteOnAllSites(Pointer<Race> & race) {
  std::list<Pointer<Site> > sites;
  for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator it = race->begin(); it != race->end(); it++) {
    sites.push_back(it->second->getSite());
  }
  deleteOnSites(race, sites);
}

void Engine::deleteOnSites(Pointer<Race> & race, std::list<Pointer<Site> > delsites) {
  if (!!race) {
    if (race->getStatus() == RACE_STATUS_RUNNING) {
      abortRace(race);
    }
    std::string sites;
    for (std::list<Pointer<Site> >::const_iterator it = delsites.begin(); it != delsites.end(); it++) {
      if (!*it) {
        continue;
      }
      Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic((*it)->getName());
      if (!sl) {
        continue;
      }
      SiteRace * sr = sl->getRace(race->getName());
      if (!sr) {
        global->getEventLog()->log("Engine", "Site " + (*it)->getName() + " is not in race: " + race->getName());
        continue;
      }
      const Path & path = sr->getPath();
      sl->requestDelete(path, true, false);
      sites += (*it)->getName() + ",";
    }
    if (sites.length() > 0) {
      sites = sites.substr(0, sites.length() - 1);
      global->getEventLog()->log("Engine", "Attempting delete of " + race->getName() + " on: " + sites);
    }
  }
}

void Engine::abortTransferJob(Pointer<TransferJob> & tj) {
  tj->abort();
  currenttransferjobs.remove(tj);
  global->getEventLog()->log("Engine", "Transfer job aborted: " + tj->getSrcFileName());
}

void Engine::raceFileListRefreshed(SiteLogic * sls, SiteRace * sr) {
  Pointer<Race> race = sr->getRace();
  if (currentraces.size() > 0) {
    if (!global->getWorkManager()->overload()) {
      estimateRaceSize(race);
      checkIfRaceComplete(sls, race);
      filelistUpdated();
    }
    else {
      ++dropped;
    }
  }
}

void Engine::filelistUpdated() {
  if (currentraces.size() > 0) {
    if (!global->getWorkManager()->overload()) {
      refreshScoreBoard();
      issueOptimalTransfers();
    }
    else {
      ++dropped;
    }
  }
}

bool Engine::transferJobActionRequest(Pointer<TransferJob> & tj) {
  std::map<Pointer<TransferJob>, std::list<PendingTransfer> >::iterator it = pendingtransfers.find(tj);
  if (it == pendingtransfers.end()) {
    pendingtransfers[tj] = std::list<PendingTransfer>();
    it = pendingtransfers.find(tj);
  }
  if (!tj->isInitialized()) {
    tj->setInitialized();
  }
  refreshPendingTransferList(tj);
  if (it->second.size() == 0) {
    tj->refreshOrAlmostDone();
    return true;
  }
  tj->clearRefreshLists();
  PendingTransfer pt = it->second.front();
  switch (pt.type()) {
    case PENDINGTRANSFER_DOWNLOAD:
    {
      if (!pt.getSrc()->downloadSlotAvailable()) return false;
      Pointer<TransferStatus> ts = global->getTransferManager()->suggestDownload(pt.getSrcFileName(),
          pt.getSrc(), pt.getSrcFileList(), pt.getLocalFileList());
      tj->addTransfer(ts);
      break;
    }
    case PENDINGTRANSFER_UPLOAD:
    {
      if (!pt.getDst()->uploadSlotAvailable()) return false;
      Pointer<TransferStatus> ts = global->getTransferManager()->suggestUpload(pt.getSrcFileName(),
          pt.getLocalFileList(), pt.getDst(), pt.getDstFileList());
      tj->addTransfer(ts);
      break;
    }
    case PENDINGTRANSFER_FXP:
    {
      if (!pt.getSrc()->downloadSlotAvailable()) return false;
      if (!pt.getDst()->uploadSlotAvailable()) return false;
      if (pt.getDst() == pt.getSrc() && pt.getDst()->slotsAvailable() < 2) {
        pt.getDst()->haveConnected(2);
        return false;
      }
      Pointer<TransferStatus> ts = global->getTransferManager()->suggestTransfer(pt.getSrcFileName(),
          pt.getSrc(), pt.getSrcFileList(), pt.getDst(), pt.getDstFileList());
      tj->addTransfer(ts);
      break;
    }
  }
  it->second.pop_front();
  return true;
}

void Engine::estimateRaceSizes() {
  for (std::list<Pointer<Race> >::iterator itr = currentraces.begin(); itr != currentraces.end(); itr++) {
    estimateRaceSize(*itr, true);
  }
}

void Engine::estimateRaceSize(const Pointer<Race> & race) {
  estimateRaceSize(race, false);
}

void Engine::estimateRaceSize(const Pointer<Race> & race, bool forceupdate) {
  for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator its = race->begin(); its != race->end(); its++) {
    SiteRace * srs = its->first;
    std::map<std::string, FileList *>::const_iterator itfl;
    for (itfl = srs->fileListsBegin(); itfl != srs->fileListsEnd(); itfl++) {
      FileList * fls = itfl->second;
      if (!forceupdate && srs->sizeEstimated(fls)) {
        continue;
      }
      reportCurrentSize(srs, fls, false);
      if (fls->hasSFV()) {
        if (srs->getSFVObservedTime(fls) > SFVDIROBSERVETIME) {
          reportCurrentSize(srs, fls, true);
        }
      }
      else {
        if (srs->getObservedTime(fls) > DIROBSERVETIME) {
          reportCurrentSize(srs, fls, true);
        }
      }
    }
  }
}

void Engine::reportCurrentSize(SiteRace * srs, FileList * fls, bool final) {
  std::set<std::string> uniques;
  std::map<std::string, File *>::const_iterator itf;
  std::string subpath = srs->getSubPathForFileList(fls);
  for (itf = fls->begin(); itf != fls->end(); itf++) {
    File * file = itf->second;
    bool isdir = file->isDirectory();
    if (isdir) {
      continue;
    }
    std::string filename = file->getName();
    size_t lastdotpos = filename.rfind(".");
    if (lastdotpos != std::string::npos && lastdotpos < filename.length() - 4) {
      int offsetdot = 4;
      if (file->getSize() == 0 && lastdotpos > 0 && lastdotpos == filename.length() - 8 &&
          filename.substr(lastdotpos) == ".missing") { // special hack for some zipscripts
        offsetdot = -1;
      }
      filename = filename.substr(0, lastdotpos + offsetdot);
    }
    Path prepend = subpath;
    if (!global->getSkipList()->isAllowed((prepend / filename).toString(), isdir)) {
      continue;
    }
    uniques.insert(filename);
  }
  srs->reportSize(fls, uniques, final);
}

void Engine::refreshScoreBoard() {
  scoreboard->wipe();
  for (std::list<Pointer<Race> >::iterator itr = currentraces.begin(); itr != currentraces.end(); itr++) {
    Pointer<Race> race = *itr;
    bool racemode = race->getProfile() == SPREAD_RACE;
    for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator its = race->begin(); its != race->end(); its++) {
      SiteRace * srs = its->first;
      const Pointer<SiteLogic> & sls = its->second;
      if (!sls->getCurrLogins() || !sls->getSite()->getAllowDownload()) continue;
      for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator itd = race->begin(); itd != race->end(); itd++) {
        SiteRace * srd = itd->first;
        const Pointer<SiteLogic> & sld = itd->second;
        if (!raceTransferPossible(sls, sld, race)) continue;
        if (!sld->getCurrLogins()) continue;
        int avgspeed = sls->getSite()->getAverageSpeed(sld->getSite()->getName());
        if (avgspeed > maxavgspeed) {
          avgspeed = maxavgspeed;
        }
        int prioritypoints = getPriorityPoints(sld->getSite()->getPriority());
        for (std::map<std::string, FileList *>::const_iterator itfls = srs->fileListsBegin(); itfls != srs->fileListsEnd(); itfls++) {
          if (itfls->first.length() > 0 && !global->getSkipList()->isAllowed(itfls->first, true)) continue;
          FileList * fls = itfls->second;
          FileList * fld = srd->getFileListForPath(itfls->first);
          if (fld != NULL) {
            if (fld->getState() == FILELIST_UNKNOWN || fld->getState() == FILELIST_EXISTS) continue;
            std::map<std::string, File *>::const_iterator itf;
            for (itf = fls->begin(); itf != fls->end(); itf++) {
              File * f = itf->second;
              const bool isdir = f->isDirectory();
              const Path prepend = itfls->first;
              if (!global->getSkipList()->isAllowed((prepend / itf->first).toString(), isdir)) {
                continue;
              }
              const std::string filename = f->getName();
              if (fld->contains(filename) || f->isDirectory() || f->getSize() == 0) continue;
              if (race->hasFailedTransfer(f, fld)) continue;
              bool prio = false;
              unsigned short score = calculateScore(f, race, fls, srs, fld, srd, avgspeed, &prio, prioritypoints, racemode);
              scoreboard->add(filename, score, prio, sls, fls, sld, fld, race);
              race->resetUpdateCheckCounter();
            }
          }
          else {
            srd->addSubDirectory(itfls->first);
          }
        }
      }
    }
  }
  scoreboard->sort();
}

void Engine::refreshPendingTransferList(Pointer<TransferJob> & tj) {
  std::map<Pointer<TransferJob>, std::list<PendingTransfer> >::iterator it = pendingtransfers.find(tj);
  if (it == pendingtransfers.end()) {
    pendingtransfers[tj] = std::list<PendingTransfer>();
  }
  it = pendingtransfers.find(tj);
  std::list<PendingTransfer> & list = it->second;
  list.clear();
  tj->clearExisting();
  switch (tj->getType()) {
    case TRANSFERJOB_DOWNLOAD: {
      std::map<std::string, FileList *>::const_iterator it2;
      for (it2 = tj->srcFileListsBegin(); it2 != tj->srcFileListsEnd(); it2++) {
        FileList * srclist = it2->second;
        Pointer<LocalFileList> dstlist = tj->findLocalFileList(it2->first);
        for (std::map<std::string, File *>::iterator srcit = srclist->begin(); srcit != srclist->end(); srcit++) {
          if (!srcit->second->isDirectory() && srcit->second->getSize() > 0) {
            std::map<std::string, LocalFile>::const_iterator dstit;
            if (!dstlist) {
              dstlist = tj->wantedLocalDstList(it2->first);
            }
            dstit = dstlist->find(srcit->first);
            if (tj->hasFailedTransfer((Path(dstlist->getPath()) / srcit->first).toString())) {
              continue;
            }
            const Path subpath = it2->first;
            if (!dstlist || dstit == dstlist->end() || dstit->second.getSize() == 0) {
              PendingTransfer p(tj->getSrc(), srclist, srcit->first, dstlist, srcit->first);
              addPendingTransfer(list, p);
              tj->addPendingTransfer(subpath / srcit->first, srcit->second->getSize());
            }
            else {
              tj->targetExists(subpath / srcit->first);
            }
          }
        }
      }
      break;
    }
    case TRANSFERJOB_DOWNLOAD_FILE:
      if (tj->getSrcFileList()->getFile(tj->getSrcFileName())->getSize() > 0) {
        if (tj->hasFailedTransfer((tj->getLocalPath() / tj->getDstFileName()).toString())) {
          break;
        }
        Pointer<LocalFileList> dstlist = tj->getLocalFileList();
        std::map<std::string, LocalFile>::const_iterator dstit;
        if (!!dstlist) {
          dstit = dstlist->find(tj->getDstFileName());
        }
        if (!dstlist || dstit == dstlist->end() || dstit->second.getSize() == 0) {
          PendingTransfer p(tj->getSrc(), tj->getSrcFileList(),
              tj->getSrcFileName(), dstlist, tj->getDstFileName());
          addPendingTransfer(list, p);
          tj->addPendingTransfer(tj->getSrcFileName(),
              tj->getSrcFileList()->getFile(tj->getSrcFileName())->getSize());
        }
        else {
          tj->targetExists(tj->getSrcFileName());
        }
      }
      break;
    case TRANSFERJOB_UPLOAD: {
      std::map<std::string, Pointer<LocalFileList> >::const_iterator lit;
      for (lit = tj->localFileListsBegin(); lit != tj->localFileListsEnd(); lit++) {
        FileList * dstlist = tj->findDstList(lit->first);
        if (dstlist == NULL || dstlist->getState() == FILELIST_UNKNOWN || dstlist->getState() == FILELIST_EXISTS) {
          continue;
        }
        for (std::map<std::string, LocalFile>::const_iterator lfit = lit->second->begin(); lfit != lit->second->end(); lfit++) {
          if (!lfit->second.isDirectory() && lfit->second.getSize() > 0) {
            std::string filename = lfit->first;
            const Path subpath = lit->first;
            if (dstlist->getFile(filename) == NULL) {
              if (tj->hasFailedTransfer((Path(dstlist->getPath()) / filename).toString())) {
                continue;
              }
              PendingTransfer p(lit->second, filename, tj->getDst(), dstlist, filename);
              addPendingTransfer(list, p);
              tj->addPendingTransfer(subpath / filename, lfit->second.getSize());
            }
            else {
              tj->targetExists(subpath / filename);
            }
          }
        }
      }
      break;
    }
    case TRANSFERJOB_UPLOAD_FILE: {
      if (tj->hasFailedTransfer((Path(tj->getDstFileList()->getPath()) / tj->getDstFileName()).toString())) {
        break;
      }
      Pointer<LocalFileList> srclist = tj->getLocalFileList();
      std::map<std::string, LocalFile>::const_iterator srcit;
      if (!!srclist) {
        srcit = srclist->find(tj->getSrcFileName());
      }
      if (!!srclist && srcit != srclist->end() && srcit->second.getSize() > 0 &&
          tj->getDstFileList()->getFile(tj->getDstFileName()) == NULL)
      {
        PendingTransfer p(srclist, tj->getSrcFileName(), tj->getDst(), tj->getDstFileList(), tj->getDstFileName());
        addPendingTransfer(list, p);
        tj->addPendingTransfer(tj->getSrcFileName(), srcit->second.getSize());
      }
      else {
        tj->targetExists(tj->getSrcFileName());
      }
      break;
    }
    case TRANSFERJOB_FXP: {
      std::map<std::string, FileList *>::const_iterator it2;
      for (it2 = tj->srcFileListsBegin(); it2 != tj->srcFileListsEnd(); it2++) {
        FileList * srclist = it2->second;
        FileList * dstlist = tj->findDstList(it2->first);
        if (dstlist == NULL || dstlist->getState() == FILELIST_UNKNOWN || dstlist->getState() == FILELIST_EXISTS) {
          continue;
        }
        for (std::map<std::string, File *>::iterator srcit = srclist->begin(); srcit != srclist->end(); srcit++) {
          if (!srcit->second->isDirectory() && srcit->second->getSize() > 0) {
            std::string filename = srcit->first;
            const Path subpath = it2->first;
            if (dstlist->getFile(filename) == NULL) {
              if (tj->hasFailedTransfer((Path(dstlist->getPath()) / filename).toString())) {
                continue;
              }
              PendingTransfer p(tj->getSrc(), srclist, filename, tj->getDst(), dstlist, filename);
              addPendingTransfer(list, p);

              tj->addPendingTransfer(subpath / filename, srcit->second->getSize());
            }
            else {
              tj->targetExists(subpath / filename);
            }
          }
        }
      }
      break;
    }
    case TRANSFERJOB_FXP_FILE:
      if (tj->hasFailedTransfer((Path(tj->getDstFileList()->getPath()) / tj->getDstFileName()).toString())) {
        break;
      }
      if (tj->getSrcFileList()->getFile(tj->getSrcFileName())->getSize() > 0) {
        if (tj->getDstFileList()->getFile(tj->getDstFileName()) == NULL) {
          PendingTransfer p(tj->getSrc(), tj->getSrcFileList(),
                        tj->getSrcFileName(), tj->getDst(), tj->getDstFileList(), tj->getDstFileName());
          addPendingTransfer(list, p);
          tj->addPendingTransfer(tj->getSrcFileName(),
              tj->getSrcFileList()->getFile(tj->getSrcFileName())->getSize());
        }
        else {
          tj->targetExists(tj->getSrcFileName());
        }
      }
      break;
  }
}

void Engine::addPendingTransfer(std::list<PendingTransfer> & list, PendingTransfer & p) {
  std::string extension = File::getExtension(p.getSrcFileName());
  if (extension == "sfv" || extension == "nfo") {
    list.push_front(p); // sfv and nfo files have top priority
  }
  else {
    list.push_back(p);
  }
}

void Engine::issueOptimalTransfers() {
  std::vector<ScoreBoardElement *>::const_iterator it;
  ScoreBoardElement * sbe;
  Pointer<SiteLogic> sls;
  Pointer<SiteLogic> sld;
  std::string filename;
  Pointer<Race> race;
  for (it = scoreboard->begin(); it != scoreboard->end(); it++) {
    sbe = *it;
    sls = sbe->getSource();
    sld = sbe->getDestination();
    race = sbe->getRace();
    filename = sbe->fileName();
    //potentiality handling
    if (!sbe->isPrioritized()) { // priority files shouldn't affect the potential tracking
      sls->pushPotential(sbe->getScore(), filename, sld);
    }
    if (!sls->downloadSlotAvailable()) continue;
    if (!sld->uploadSlotAvailable()) continue;
    if (!sls->potentialCheck(sbe->getScore())) continue;
    Pointer<TransferStatus> ts =
      global->getTransferManager()->suggestTransfer(filename, sls,
        sbe->getSourceFileList(), sld, sbe->getDestinationFileList());
    race->addTransfer(ts);
  }
}

void Engine::checkIfRaceComplete(SiteLogic * sls, Pointer<Race> & race) {
  SiteRace * srs = sls->getRace(race->getName());
  if (!srs->isDone()) {
    bool unfinisheddirs = false;
    bool emptydirs = false;
    int completedlists = 0;
    std::list<std::string> subpaths = race->getSubPaths();
    for (std::list<std::string>::iterator itsp = subpaths.begin(); itsp != subpaths.end(); itsp++) {
      FileList * spfl = srs->getFileListForPath(*itsp);
      if (spfl != NULL && spfl->getState() == FILELIST_LISTED) {
        if (!race->sizeEstimated(*itsp)) {
          if (spfl->getSize() > 0) {
            unfinisheddirs = true;
            continue;
          }
          emptydirs = true;
        }
        else if (spfl->getNumUploadedFiles() >= race->estimatedSize(*itsp) &&
          spfl->timeSinceLastChanged() > STATICTIMEFORCOMPLETION &&
          sls->getCurrLogins() > sls->getCurrUp() + sls->getCurrDown() &&
          !spfl->hasFilesUploading())
        {
          completedlists++;
          if (!srs->isSubPathComplete(spfl)) {
            srs->subPathComplete(spfl);
          }
        }
        else {
          unfinisheddirs = true;
          continue;
        }
      }
      else {
        unfinisheddirs = true;
        continue;
      }
    }
    if (completedlists > 0 && !unfinisheddirs) {
      if (emptydirs) {
        race->reportSemiDone(srs);
      }
      else {
        int uploadslotcount = 0;
        for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator it = race->begin(); it != race->end(); it++) {
          if (it->second.get() != sls && !it->first->isDone()) {
            uploadslotcount += it->second->getSite()->getMaxUp();
          }
        }
        sls->raceLocalComplete(srs, uploadslotcount);
        global->getEventLog()->log("Engine", "Race " + race->getName() + " completed on " +
            sls->getSite()->getName());
      }
      if (race->isDone()) {
        raceComplete(race);
      }
    }
  }
}

void Engine::raceComplete(Pointer<Race> race) {
  issueGlobalComplete(race);
  for (std::list<Pointer<Race> >::iterator it = currentraces.begin(); it != currentraces.end(); it++) {
    if ((*it) == race) {
      currentraces.erase(it);
      break;
    }
  }
  refreshScoreBoard();
  global->getEventLog()->log("Engine", "Race globally completed: " + race->getName());
  if (dropped) {
    global->getEventLog()->log("Engine", "Scoreboard refreshes dropped since race start: " + util::int2Str(dropped));
  }
  return;
}

void Engine::transferJobComplete(Pointer<TransferJob> tj) {

  for (std::list<Pointer<TransferJob> >::iterator it = currenttransferjobs.begin(); it != currenttransferjobs.end(); it++) {
    if ((*it) == tj) {
      currenttransferjobs.erase(it);
      break;
    }
  }
  global->getEventLog()->log("Engine", tj->typeString() + " job complete: " + tj->getSrcFileName());
}

unsigned short Engine::calculateScore(File * f, Pointer<Race> & itr, FileList * fls, SiteRace * srs, FileList * fld, SiteRace * srd, int avgspeed, bool * prio, int prioritypoints, bool racemode) const {
  // sfv and nfo files have top priority
  if (f->getExtension().compare("sfv") == 0 ||
      f->getExtension().compare("nfo") == 0) {
    *prio = true;
    return 10000;
  }

  unsigned short points = 0;
  unsigned long long int filesize = f->getSize();
  unsigned long long int maxfilesize = srs->getMaxFileSize();
  if (avgspeed > maxavgspeed) {
    avgspeed = maxavgspeed;
  }
  if (filesize > maxfilesize) {
    maxfilesize = filesize;
  }

  if (maxfilesize) {
    unsigned long long int pointsfilesize = maxpointsfilesize;
    pointsfilesize *= filesize;
    pointsfilesize /= maxfilesize;
    points += pointsfilesize;
  }

  points += getSpeedPoints(avgspeed);

  if (racemode) {
    unsigned long long int pointspercentageowned = maxpointspercentageowned;
    int unownedpercentage = 100 - fld->getOwnedPercentage();
    pointspercentageowned *= unownedpercentage;
    pointspercentageowned /= 100;
    points += pointspercentageowned;
  }
  else {
    unsigned long long int pointslowprogress = maxpointslowprogress;
    int maxprogress = itr->getMaxSiteNumFilesProgress();
    if (maxprogress) {
      pointslowprogress *= fld->getNumUploadedFiles();
      pointslowprogress /= maxprogress;
      points += pointslowprogress;
    }
  }

  points += prioritypoints;

  util::assert(points >= 0 && points < 10000);
  return points;
}

void Engine::setSpeedScale() {
  maxavgspeed = 1024;
  for (std::list<Pointer<Race> >::iterator itr = currentraces.begin(); itr != currentraces.end(); itr++) {
    for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator its = (*itr)->begin(); its != (*itr)->end(); its++) {
      for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator itd = (*itr)->begin(); itd != (*itr)->end(); itd++) {
        int avgspeed = its->second->getSite()->getAverageSpeed(itd->second->getSite()->getName());
        if (avgspeed > maxavgspeed) maxavgspeed = avgspeed;
      }
    }
  }
}

void Engine::preSeedPotentialData(Pointer<Race> & race) {
  race->begin();
  std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator srcit;
  std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator dstit;
  int highestprioritypoints = 0;
  int maxpointsexceptavgspeed = getMaxPointsFileSize() + getMaxPointsPriority() + getMaxPointsPercentageOwned();
  for (srcit = race->begin(); srcit != race->end(); srcit++) {
    int priopoints = getPriorityPoints(srcit->second->getSite()->getPriority());
    if (priopoints > highestprioritypoints) {
      highestprioritypoints = priopoints;
    }
  }
  for (srcit = race->begin(); srcit != race->end(); srcit++) {
    const Pointer<SiteLogic> & sls = srcit->second;
    if (!sls->getSite()->getAllowDownload()) continue;
    int maxavgspeed = 0;
    for (dstit = race->begin(); dstit != race->end(); dstit++) {
      const Pointer<SiteLogic> & sld = dstit->second;
      if (!raceTransferPossible(sls, sld, race)) continue;
      int avgspeed = sls->getSite()->getAverageSpeed(sld->getSite()->getName());
      if (avgspeed > maxavgspeed) {
        maxavgspeed = avgspeed;
      }
    }
    for (dstit = race->begin(); dstit != race->end(); dstit++) {
      const Pointer<SiteLogic> & sld = dstit->second;
      if (!raceTransferPossible(sls, sld, race)) continue;
      sls->pushPotential(maxpointsexceptavgspeed + getSpeedPoints(maxavgspeed), "preseed", sld);
    }
  }
}

bool Engine::raceTransferPossible(const Pointer<SiteLogic> & sls, const Pointer<SiteLogic> & sld, Pointer<Race> & race) const {
  if (sls == sld) return false;
  if (!sld->getSite()->getAllowUpload()) return false;
  if (!sls->getSite()->isAllowedTargetSite(sld->getSite())) return false;
  if (sld->getSite()->isAffiliated(race->getGroup()) && race->getProfile() != SPREAD_DISTRIBUTE) return false;
  if (sls->getSite()->hasBrokenPASV() &&
      sld->getSite()->hasBrokenPASV()) return false;
  //ssl check
  if ((sls->getSite()->getSSLTransferPolicy() == SITE_SSL_ALWAYS_OFF &&
      sld->getSite()->getSSLTransferPolicy() == SITE_SSL_ALWAYS_ON) ||
      (sls->getSite()->getSSLTransferPolicy() == SITE_SSL_ALWAYS_ON &&
          sld->getSite()->getSSLTransferPolicy() == SITE_SSL_ALWAYS_OFF)) {
    return false;
  }
  return true;
}

unsigned int Engine::preparedRaces() const {
  return preparedraces.size();
}

unsigned int Engine::currentRaces() const {
  return currentraces.size();
}

unsigned int Engine::allRaces() const {
  return allraces.size();
}

unsigned int Engine::currentTransferJobs() const {
  return currenttransferjobs.size();
}

unsigned int Engine::allTransferJobs() const {
  return alltransferjobs.size();
}

Pointer<Race> Engine::getRace(unsigned int id) const {
  std::list<Pointer<Race> >::const_iterator it;
  for (it = allraces.begin(); it != allraces.end(); it++) {
    if ((*it)->getId() == id) {
      return *it;
    }
  }
  return Pointer<Race>();
}

Pointer<Race> Engine::getRace(const std::string & race) const {
  std::list<Pointer<Race> >::const_iterator it;
  for (it = allraces.begin(); it != allraces.end(); it++) {
    if ((*it)->getName() == race) {
      return *it;
    }
  }
  return Pointer<Race>();
}

Pointer<TransferJob> Engine::getTransferJob(unsigned int id) const {
  std::list<Pointer<TransferJob> >::const_iterator it;
  for (it = alltransferjobs.begin(); it != alltransferjobs.end(); it++) {
    if ((*it)->getId() == id) {
      return *it;
    }
  }
  return Pointer<TransferJob>();
}

std::list<Pointer<PreparedRace> >::const_iterator Engine::getPreparedRacesBegin() const {
  return preparedraces.begin();
}

std::list<Pointer<PreparedRace> >::const_iterator Engine::getPreparedRacesEnd() const {
  return preparedraces.end();
}

std::list<Pointer<Race> >::const_iterator Engine::getRacesBegin() const {
  return allraces.begin();
}

std::list<Pointer<Race> >::const_iterator Engine::getRacesEnd() const {
  return allraces.end();
}

std::list<Pointer<TransferJob> >::const_iterator Engine::getTransferJobsBegin() const {
  return alltransferjobs.begin();
}

std::list<Pointer<TransferJob> >::const_iterator Engine::getTransferJobsEnd() const {
  return alltransferjobs.end();
}

void Engine::tick(int message) {
  for (std::list<Pointer<Race> >::iterator it = currentraces.begin(); it != currentraces.end(); it++) {
    if ((*it)->checksSinceLastUpdate() >= MAXCHECKSTIMEOUT) {
      if ((*it)->failedTransfersCleared()) {
        global->getEventLog()->log("Engine", "No activity for " + util::int2Str(MAXCHECKSTIMEOUT) +
            " seconds, aborting race: " + (*it)->getName());
        for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator its = (*it)->begin(); its != (*it)->end(); its++) {
          its->second->raceLocalComplete(its->first, 0);
        }
        (*it)->setTimeout();
        issueGlobalComplete(*it);
        currentraces.erase(it);
        break;
      }
      else {
        if ((*it)->clearTransferAttempts()) {
          (*it)->resetUpdateCheckCounter();
        }
      }
    }
  }
  for (std::list<Pointer<TransferJob> >::const_iterator it = currenttransferjobs.begin(); it != currenttransferjobs.end(); it++) {
    if ((*it)->isDone()) {
      transferJobComplete(*it);
      break;
    }
  }
  std::list<unsigned int> removeids;
  for (std::list<Pointer<PreparedRace> >::const_iterator it = preparedraces.begin(); it != preparedraces.end(); it++) {
    (*it)->tick();
    if ((*it)->getRemainingTime() < 0) {
      removeids.push_back((*it)->getId());
    }
  }
  while (removeids.size() > 0) {
    unsigned int id = removeids.front();
    removeids.pop_front();
    for (std::list<Pointer<PreparedRace> >::iterator it = preparedraces.begin(); it != preparedraces.end(); it++) {
      if ((*it)->getId() == id) {
        preparedraces.erase(it);
        break;
      }
    }
  }
  if (!currentraces.size() && !currenttransferjobs.size() && !preparedraces.size() && pokeregistered) {
    global->getTickPoke()->stopPoke(this, 0);
    pokeregistered = false;
  }
  estimateRaceSizes();
}

void Engine::issueGlobalComplete(Pointer<Race> & race) {
  for (std::list<std::pair<SiteRace *, Pointer<SiteLogic> > >::const_iterator itd = race->begin(); itd != race->end(); itd++) {
    itd->second->raceGlobalComplete();
  }
}

Pointer<ScoreBoard> Engine::getScoreBoard() const {
  return scoreboard;
}

void Engine::checkStartPoke() {
  if (!pokeregistered) {
    global->getTickPoke()->startPoke(this, "Engine", POKEINTERVAL, 0);
    pokeregistered = true;
  }
}

Pointer<Race> Engine::getCurrentRace(const std::string & release) const {
  for (std::list<Pointer<Race> >::const_iterator it = currentraces.begin(); it != currentraces.end(); it++) {
    if ((*it)->getName() == release) {
      return *it;
    }
  }
  return Pointer<Race>();
}

void Engine::addSiteToRace(Pointer<Race> & race, const std::string & site) {
  const Pointer<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(site);
  if (!checkBannedGroup(race->getSection(), sl->getSite(), race->getGroup())) {
    SiteRace * sr = sl->addRace(race, race->getSection(), race->getName());
    race->addSite(sr, sl);
  }
}

bool Engine::checkBannedGroup(const std::string & section, const Pointer<Site> & site, const std::string & group) {
  if (site->isBannedGroup(section, group)) {
    global->getEventLog()->log("Engine", "Ignoring site: " + site->getName() +
        " because the group is banned: " + group);
    return true;
  }
  return false;
}

int Engine::getMaxPointsRaceTotal() const {
  return getMaxPointsFileSize() + getMaxPointsAvgSpeed() + getMaxPointsPriority() + getMaxPointsPercentageOwned();
}

int Engine::getMaxPointsFileSize() const {
  return maxpointsfilesize;
}

int Engine::getMaxPointsAvgSpeed() const {
  return maxpointsavgspeed;
}

int Engine::getMaxPointsPriority() const {
  return maxpointspriority;
}

int Engine::getMaxPointsPercentageOwned() const {
  return maxpointspercentageowned;
}

int Engine::getMaxPointsLowProgress() const {
  return maxpointslowprogress;
}

int Engine::getPriorityPoints(int priority) const {
  switch (priority) {
    case SITE_PRIORITY_VERY_LOW:
      return 0;
    case SITE_PRIORITY_LOW:
      return maxpointspriority * 0.2;
    case SITE_PRIORITY_NORMAL:
      return maxpointspriority * 0.4;
    case SITE_PRIORITY_HIGH:
      return maxpointspriority * 0.6;
    case SITE_PRIORITY_VERY_HIGH:
      return maxpointspriority;
  }
  return 0;
}

int Engine::getSpeedPoints(int avgspeed) const {
  if (maxavgspeed) {
    unsigned long long int pointsavgspeed = maxpointsavgspeed;
    pointsavgspeed *= avgspeed;
    pointsavgspeed /= maxavgspeed;
    return pointsavgspeed;
  }
  return 0;
}

int Engine::getPreparedRaceExpiryTime() const {
  return preparedraceexpirytime;
}

void Engine::setPreparedRaceExpiryTime(int expirytime) {
  preparedraceexpirytime = expirytime;
}

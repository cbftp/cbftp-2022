#include "restapi.h"

#include <list>
#include <memory>

#include "core/tickpoke.h"
#include "core/types.h"
#include "ext/json.hpp"
#include "http/request.h"
#include "http/response.h"

#include "crypto.h"
#include "engine.h"
#include "eventlog.h"
#include "file.h"
#include "filelist.h"
#include "filelistdata.h"
#include "globalcontext.h"
#include "path.h"
#include "race.h"
#include "racestatus.h"
#include "remotecommandhandler.h"
#include "restapicallback.h"
#include "sectionmanager.h"
#include "settingsloadersaver.h"
#include "site.h"
#include "sitelogic.h"
#include "sitelogicmanager.h"
#include "sitemanager.h"
#include "siterace.h"
#include "skiplist.h"
#include "util.h"

namespace {

#define RESTAPI_TICK_INTERVAL_MS 500
#define DEFAULT_TIMEOUT_SECONDS 60

std::list<std::shared_ptr<SiteLogic>> getSiteLogicList(const nlohmann::json& jsondata) {
  std::set<std::shared_ptr<SiteLogic>> sitelogics;
  std::list<std::string> sites;
  auto sitesit = jsondata.find("sites");
  if (sitesit != jsondata.end()) {
    for (auto it = sitesit->begin(); it != sitesit->end(); ++it) {
      sites.push_back(*it);
    }
  }
  for (std::list<std::string>::const_iterator it = sites.begin(); it != sites.end(); it++) {
    const std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(*it);
    if (!sl) {
      global->getEventLog()->log("RemoteCommandHandler", "Site not found: " + *it);
      continue;
    }
    sitelogics.insert(sl);
  }
  std::list<std::string> sections;
  auto sectionsit = jsondata.find("sites_with_sections");
  if (sectionsit != jsondata.end()) {
    for (auto it = sectionsit->begin(); it != sectionsit->end(); ++it) {
      sections.push_back(*it);
    }
  }

  auto allit = jsondata.find("sites_all");
  if (allit != jsondata.end() && allit->get<bool>()) {
    std::vector<std::shared_ptr<Site> >::const_iterator it;
    for (it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); it++) {
      if (!(*it)->getDisabled()) {
        std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic((*it)->getName());
        sitelogics.insert(sl);
      }
    }
  }
  for (const std::string& section : sections) {
    if (!global->getSectionManager()->getSection(section)) {
      global->getEventLog()->log("RemoteCommandHandler", "Section not found: " + section);
      continue;
    }
    for (std::vector<std::shared_ptr<Site> >::const_iterator it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); ++it) {
      if ((*it)->hasSection(section) && !(*it)->getDisabled()) {
        std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic((*it)->getName());
        sitelogics.insert(sl);
      }
    }
  }
  return std::list<std::shared_ptr<SiteLogic>>(sitelogics.begin(), sitelogics.end());
}

bool useOrSectionTranslate(Path& path, const std::shared_ptr<Site>& site) {
  if (path.isRelative()) {
    path.level(0);
    std::string section = path.level(0).toString();
    if (site->hasSection(section)) {
      path = site->getSectionPath(section) / path.cutLevels(-1);
    }
    else {
      return false;
    }
  }
  return true;
}

http::Response badRequestResponse(const std::string& error, int code = 400)
{
  http::Response response(400);
  nlohmann::json j = {{"error", error}};
  std::string errorjson = j.dump(2);
  response.setBody(std::vector<char>(errorjson.begin(), errorjson.end()));
  response.addHeader("Content-Type", "application/json");
  return response;
}

std::string listCommandToString(int listcommand) {
  switch (listcommand) {
    case SITE_LIST_STAT:
      return "STAT_L";
    case SITE_LIST_LIST:
      return "LIST";
  }
  return "<unknown list command type " + std::to_string(listcommand) + ">";
}

std::string tlsModeToString(TLSMode mode) {
  switch (mode) {
    case TLSMode::NONE:
      return "NONE";
    case TLSMode::AUTH_TLS:
      return "AUTH_TLS";
    case TLSMode::IMPLICIT:
      return "IMPLICIT";
  }
  return "<unknown tls mode type " + std::to_string(static_cast<int>(mode)) + ">";
}

std::string tlsTransferPolicyToString(int policy) {
  switch (policy) {
    case SITE_SSL_ALWAYS_OFF:
      return "ALWAYS_OFF";
    case SITE_SSL_PREFER_OFF:
      return "PREFER_OFF";
    case SITE_SSL_PREFER_ON:
      return "PREFER_ON";
    case SITE_SSL_ALWAYS_ON:
      return "ALWAYS_ON";
  }
  return "<unknown tls policy type " + std::to_string(policy) + ">";
}

std::string transferProtocolToString(TransferProtocol proto) {
  switch (proto) {
    case TransferProtocol::IPV4_ONLY:
      return "IPV4_ONLY";
    case TransferProtocol::PREFER_IPV4:
      return "PREFER_IPV4";
    case TransferProtocol::PREFER_IPV6:
      return "PREFER_IPV6";
    case TransferProtocol::IPV6_ONLY:
      return "IPV6_ONLY";
  }
  return "<unknown transfer protocol type " + std::to_string(static_cast<int>(proto)) + ">";
}

std::string siteAllowTransferToString(SiteAllowTransfer allow) {
  switch (allow) {
    case SITE_ALLOW_TRANSFER_NO:
      return "NO";
    case SITE_ALLOW_TRANSFER_YES:
      return "YES";
    case SITE_ALLOW_DOWNLOAD_MATCH_ONLY:
      return "MATCH_ONLY";
  }
  return "<unknown site allow transfer type " + std::to_string(static_cast<int>(allow)) + ">";
}

std::string priorityToString(SitePriority priority) {
  switch (priority) {
    case SitePriority::VERY_LOW:
      return "VERY_LOW";
    case SitePriority::LOW:
      return "LOW";
    case SitePriority::NORMAL:
      return "NORMAL";
    case SitePriority::HIGH:
      return "HIGH";
    case SitePriority::VERY_HIGH:
      return "VERY_HIGH";
  }
  return "<unknown priority type " + std::to_string(static_cast<int>(priority)) + ">";
}

std::string siteTransferPolicyToString(int policy) {
  switch (policy) {
    case SITE_TRANSFER_POLICY_ALLOW:
      return "ALLOW";
    case SITE_TRANSFER_POLICY_BLOCK:
      return "BLOCK";
  }
  return "<unknown site transfer policy type " + std::to_string(policy) + ">";
}

std::string skiplistActionToString(SkipListAction action) {
  switch (action) {
    case SKIPLIST_ALLOW:
      return "ALLOW";
    case SKIPLIST_DENY:
      return "DENY";
    case SKIPLIST_UNIQUE:
      return "UNIQUE";
    case SKIPLIST_SIMILAR:
      return "SIMILAR";
    case SKIPLIST_NONE:
      return "NONE";
  }
  return "<unknown skiplist action type " + std::to_string(action) + ">";
}

std::string skiplistScopeToString(int scope) {
  switch (scope) {
    case SCOPE_IN_RACE:
      return "IN_RACE";
    case SCOPE_ALL:
      return "ALL";
  }
  return "<unknown skiplist scope type " + std::to_string(scope) + ">";
}

std::string proxyTypeToString(int type) {
  switch (type) {
    case SITE_PROXY_GLOBAL:
      return "GLOBAL";
    case SITE_PROXY_NONE:
      return "NONE";
    case SITE_PROXY_USE:
      return "USE";
  }
  return "<unknown proxy type " + std::to_string(type) + ">";
}

std::string profileToString(SpreadProfile profile) {
  switch (profile) {
    case SPREAD_RACE:
      return "RACE";
    case SPREAD_DISTRIBUTE:
      return "DISTRIBUTE";
    case SPREAD_PREPARE:
      return "PREPARE";
  }
  return "<unknown profile type " + std::to_string(profile) + ">";
}

std::string raceStatusToString(RaceStatus status) {
  switch (status) {
    case RaceStatus::RUNNING:
      return "RUNNING";
    case RaceStatus::DONE:
      return "DONE";
    case RaceStatus::TIMEOUT:
      return "TIMEOUT";
    case RaceStatus::ABORTED:
      return "ABORTED";
  }
  return "<unknown race status type " + std::to_string(static_cast<int>(status)) + ">";
}

nlohmann::json jsonSkipList(const SkipList& skiplist) {
  nlohmann::json out = nlohmann::json::array();
  for (std::list<SkiplistItem>::const_iterator it = skiplist.entriesBegin(); it != skiplist.entriesEnd(); ++it) {
    nlohmann::json entry;
    entry["regex"] = it->matchRegex();
    entry["pattern"] = it->matchPattern();
    entry["file"] = it->matchFile();
    entry["dir"] = it->matchDir();
    entry["action"] = skiplistActionToString(it->getAction());
    entry["scope"] = skiplistScopeToString(it->matchScope());
    out.push_back(entry);
  }
  return out;
}

int stringToSkiplistScope(const std::string& scope) {
  if (scope == "IN_RACE") {
    return SCOPE_IN_RACE;
  }
  else if (scope == "ALL") {
    return SCOPE_ALL;
  }
  return -1;
}

SkipListAction stringToSkiplistAction(const std::string& action) {
  if (action == "ALLOW") {
    return SKIPLIST_ALLOW;
  }
  else if (action == "DENY") {
    return SKIPLIST_DENY;
  }
  else if (action == "UNIQUE") {
    return SKIPLIST_UNIQUE;
  }
  else if (action == "SIMILAR") {
    return SKIPLIST_SIMILAR;
  }
  return SKIPLIST_NONE;
}

int stringToTransferPolicy(const std::string& policy) {
  if (policy == "ALLOW") {
    return SITE_TRANSFER_POLICY_ALLOW;
  }
  else if (policy == "BLOCK") {
    return SITE_TRANSFER_POLICY_BLOCK;
  }
  return -1;
}

SitePriority stringToSitePriority(const std::string& priority) {
  if (priority == "VERY_LOW") {
    return SitePriority::VERY_LOW;
  }
  if (priority == "LOW") {
    return SitePriority::LOW;
  }
  if (priority == "HIGH") {
    return SitePriority::HIGH;
  }
  if (priority == "VERY_HIGH") {
    return SitePriority::VERY_HIGH;
  }
  return SitePriority::NORMAL;
}

TransferProtocol stringToTransferProtocol(const std::string& protocol) {
  if (protocol == "IPV4_ONLY") {
    return TransferProtocol::IPV4_ONLY;
  }
  if (protocol == "PREFER_IPV4") {
    return TransferProtocol::PREFER_IPV4;
  }
  if (protocol == "PREFER_IPV6") {
    return TransferProtocol::PREFER_IPV6;
  }
  return TransferProtocol::IPV6_ONLY;
}

int stringToTlsTransferPolicy(const std::string& policy) {
  if (policy == "ALWAYS_OFF") {
    return SITE_SSL_ALWAYS_OFF;
  }
  if (policy == "PREFER_OFF") {
    return SITE_SSL_PREFER_OFF;
  }
  if (policy == "PREFER_ON") {
    return SITE_SSL_PREFER_ON;
  }
  return SITE_SSL_ALWAYS_ON;
}

TLSMode stringToTlsMode(const std::string& mode) {
  if (mode == "NONE") {
    return TLSMode::NONE;
  }
  if (mode == "AUTH_TLS") {
    return TLSMode::AUTH_TLS;
  }
  return TLSMode::IMPLICIT;
}

int stringToListCommand(const std::string& command) {
  if (command == "STAT_L") {
    return SITE_LIST_STAT;
  }
  return SITE_LIST_LIST;
}

SiteAllowTransfer stringToSiteAllowTransfer(const std::string& allow) {
  if (allow == "NO") {
    return SITE_ALLOW_TRANSFER_NO;
  }
  if (allow == "YES") {
    return SITE_ALLOW_TRANSFER_YES;
  }
  return SITE_ALLOW_DOWNLOAD_MATCH_ONLY;
}

int stringToProxyType(const std::string& type) {
  if (type == "GLOBAL") {
    return SITE_PROXY_GLOBAL;
  }
  if (type == "NONE") {
    return SITE_PROXY_NONE;
  }
  return SITE_PROXY_USE;
}

SpreadProfile stringToProfile(const std::string& profile) {
  if (profile == "DISTRIBUTE") {
    return SPREAD_DISTRIBUTE;
  }
  if (profile == "PREPARE") {
    return SPREAD_PREPARE;
  }
  return SPREAD_RACE;
}

void updateSkipList(SkipList& skiplist, nlohmann::json jsonlist) {
  skiplist.clearEntries();
  for (nlohmann::json::const_iterator it = jsonlist.begin(); it != jsonlist.end(); ++it) {
    bool regex = it.value()["regex"];
    std::string pattern = it.value()["pattern"];
    bool file = it.value()["file"];
    bool dir = it.value()["dir"];
    int scope = stringToSkiplistScope(it.value()["scope"]);
    SkipListAction action = stringToSkiplistAction(it.value()["action"]);
    skiplist.addEntry(regex, pattern, file, dir, scope, action);
  }
}

std::shared_ptr<http::Response> updateSite(std::shared_ptr<Site>& site, nlohmann::json jsondata, bool add) {
  bool changedname = false;
  std::list<std::string> exceptsrclist;
  std::list<std::string> exceptdstlist;
  for (std::set<std::shared_ptr<Site> >::const_iterator it = site->exceptSourceSitesBegin(); it != site->exceptSourceSitesEnd(); ++it) {
    exceptsrclist.push_back((*it)->getName());
  }
  for (std::set<std::shared_ptr<Site> >::const_iterator it = site->exceptTargetSitesBegin(); it != site->exceptTargetSitesEnd(); ++it) {
    exceptdstlist.push_back((*it)->getName());
  }
  for (nlohmann::json::const_iterator it = jsondata.begin(); it != jsondata.end(); ++it) {
    if (it.key() == "name") {
      changedname = !add && std::string(it.value()) != site->getName();
      site->setName(it.value());
    }
    else if (it.key() == "addresses") {
      std::string addrports;
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        addrports += std::string(it2.value()) + " ";
      }
      site->setAddresses(addrports);
    }
    else if (it.key() == "user") {
      site->setUser(it.value());
    }
    else if (it.key() == "password") {
      site->setPass(it.value());
    }
    else if (it.key() == "base_path") {
      site->setBasePath(it.value());
    }
    else if (it.key() == "max_logins") {
      site->setMaxLogins(it.value());
    }
    else if (it.key() == "max_sim_up") {
      site->setMaxUp(it.value());
    }
    else if (it.key() == "max_sim_down") {
      site->setMaxDn(it.value());
    }
    else if (it.key() == "max_sim_down_pre") {
      site->setMaxDnPre(it.value());
    }
    else if (it.key() == "max_sim_down_complete") {
      site->setMaxDnComplete(it.value());
    }
    else if (it.key() == "max_sim_down_transferjob") {
      site->setMaxDnTransferJob(it.value());
    }
    else if (it.key() == "max_idle_time") {
      site->setMaxIdleTime(it.value());
    }
    else if (it.key() == "pret") {
      site->setPRET(it.value());
    }
    else if (it.key() == "force_binary_mode") {
      site->setForceBinaryMode(it.value());
    }
    else if (it.key() == "list_command") {
      site->setListCommand(stringToListCommand(it.value()));
    }
    else if (it.key() == "tls_mode") {
      site->setTLSMode(stringToTlsMode(it.value()));
    }
    else if (it.key() == "tls_transfer_policy") {
      site->setSSLTransferPolicy(stringToTlsTransferPolicy(it.value()));
    }
    else if (it.key() == "transfer_protocol") {
      site->setTransferProtocol(stringToTransferProtocol(it.value()));
    }
    else if (it.key() == "sscn") {
      site->setSupportsSSCN(it.value());
    }
    else if (it.key() == "cpsv") {
      site->setSupportsCPSV(it.value());
    }
    else if (it.key() == "cepr") {
      site->setSupportsCEPR(it.value());
    }
    else if (it.key() == "broken_pasv") {
      site->setBrokenPASV(it.value());
    }
    else if (it.key() == "disabled") {
      site->setDisabled(it.value());
    }
    else if (it.key() == "allow_upload") {
      site->setAllowUpload(stringToSiteAllowTransfer(it.value()));
    }
    else if (it.key() == "allow_download") {
      site->setAllowDownload(stringToSiteAllowTransfer(it.value()));
    }
    else if (it.key() == "priority") {
      site->setPriority(stringToSitePriority(it.value()));
    }
    else if (it.key() == "xdupe") {
      site->setUseXDUPE(it.value());
    }
    else if (it.key() == "sections") {
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        site->addSection(it2.key(), it2.value());
      }
    }
    else if (it.key() == "avg_speed") {
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        site->setAverageSpeed(it2.key(), it2.value());
      }
    }
    else if (it.key() == "affils") {
      site->clearAffils();
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        site->addAffil(it2.value());
      }
    }
    else if (it.key() == "transfer_source_policy") {
      site->setTransferSourcePolicy(stringToTransferPolicy(it.value()));
    }
    else if (it.key() == "transfer_target_policy") {
      site->setTransferTargetPolicy(stringToTransferPolicy(it.value()));
    }
    else if (it.key() == "except_source_sites") {
      exceptsrclist.clear();
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        exceptsrclist.push_back(it2.value());
      }
    }
    else if (it.key() == "except_target_sites") {
      exceptdstlist.clear();
      for (nlohmann::json::const_iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
        exceptdstlist.push_back(it2.value());
      }
    }
    else if (it.key() == "leave_free_slot") {
      site->setLeaveFreeSlot(it.value());
    }
    else if (it.key() == "stay_logged_in") {
      site->setStayLoggedIn(it.value());
    }
    else if (it.key() == "skiplist") {
      updateSkipList(site->getSkipList(), it.value());
    }
    else if (it.key() == "proxy_type") {
      site->setProxyType(stringToProxyType(it.value()));
    }
    else if (it.key() == "proxy_name") {
      site->setProxy(it.value());
    }
    else {
      return std::make_shared<http::Response>(badRequestResponse("Unrecognized key: " + it.key()));
    }
  }
  if (add) {
    global->getSiteManager()->addSite(site);
  }
  else {
    global->getSiteManager()->sortSites();
  }
  std::string sitename = site->getName();
  global->getSiteManager()->resetSitePairsForSite(sitename);
  for (const std::string& exceptsrcsite : exceptsrclist) {
    global->getSiteManager()->addExceptSourceForSite(sitename, exceptsrcsite);
  }
  for (const std::string& exceptdstsite : exceptdstlist) {
    global->getSiteManager()->addExceptTargetForSite(sitename, exceptdstsite);
  }

  global->getSiteLogicManager()->getSiteLogic(site->getName())->siteUpdated();
  if (changedname) {
    global->getSiteLogicManager()->getSiteLogic(site->getName())->updateName();
  }

  global->getSettingsLoaderSaver()->saveSettings();
  return nullptr;
}

nlohmann::json getJsonFromBody(const http::Request& request) {
  std::shared_ptr<std::vector<char>> body = request.getBody();
  nlohmann::json jsondata;
  if (body && !body->empty()) {
    jsondata = nlohmann::json::parse(std::string(body->begin(), body->end()));
  }
  return jsondata;
}

}

RestApi::RestApi() : nextrequestid(0) {
  endpoints[std::make_pair(Path("/filelist"), 1)]["GET"] = &RestApi::handleFileListGet;
  endpoints[std::make_pair(Path("/raw"), 1)]["POST"] = &RestApi::handleRawPost;
  endpoints[std::make_pair(Path("/raw"), 2)]["GET"] = &RestApi::handleRawGet;
  endpoints[std::make_pair(Path("/sites"), 1)]["GET"] = &RestApi::handleSitesGet;
  endpoints[std::make_pair(Path("/sites"), 1)]["POST"] = &RestApi::handleSitesPost;
  endpoints[std::make_pair(Path("/sites"), 2)]["GET"] = &RestApi::handleSiteGet;
  endpoints[std::make_pair(Path("/sites"), 2)]["PATCH"] = &RestApi::handleSitePatch;
  endpoints[std::make_pair(Path("/sites"), 2)]["DELETE"] = &RestApi::handleSiteDelete;
  endpoints[std::make_pair(Path("/spreadjobs"), 1)]["POST"] = &RestApi::handleSpreadJobPost;
  endpoints[std::make_pair(Path("/spreadjobs"), 1)]["GET"] = &RestApi::handleSpreadJobsGet;
  endpoints[std::make_pair(Path("/spreadjobs"), 2)]["GET"] = &RestApi::handleSpreadJobGet;
  global->getTickPoke()->startPoke(this, "RestApi", RESTAPI_TICK_INTERVAL_MS, 0);
}

RestApi::~RestApi() {
  global->getTickPoke()->stopPoke(this, 0);
}

void RestApi::handleRawPost(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  nlohmann::json jsondata = getJsonFromBody(request);
  std::list<std::shared_ptr<SiteLogic>> sites = getSiteLogicList(jsondata);
  if (sites.empty()) {
    cb->requestHandled(connrequestid, badRequestResponse("No sites specified"));
    return;
  }
  auto commandit = jsondata.find("command");
  if (commandit == jsondata.end()) {
    cb->requestHandled(connrequestid, badRequestResponse("Missing key: command"));
    return;
  }
  std::string command = *commandit;
  auto pathit = jsondata.find("path_section");
  bool pathsection = pathit != jsondata.end();
  std::string path;
  if (pathsection) {
    path = *pathit;
  }
  if (!pathsection) {
    pathit = jsondata.find("path");
    if (pathit != jsondata.end()) {
      path = *pathit;
    }
  }
  auto asyncit = jsondata.find("async");
  auto timeoutit = jsondata.find("timeout");
  OngoingRequest ongoingrequest;
  ongoingrequest.type = OngoingRequestType::RAW_COMMAND;
  ongoingrequest.connrequestid = connrequestid;
  ongoingrequest.apirequestid = nextrequestid++;
  ongoingrequest.cb = cb;
  ongoingrequest.async = asyncit != jsondata.end() && static_cast<bool>(*asyncit);
  ongoingrequest.timeout = timeoutit != jsondata.end() ? static_cast<int>(*timeoutit) : DEFAULT_TIMEOUT_SECONDS;
  for (std::list<std::shared_ptr<SiteLogic> >::const_iterator it = sites.begin(); it != sites.end(); it++) {
    std::string thispath;
    if (pathsection) {
      thispath = (*it)->getSite()->getSectionPath(path).toString();
    }
    else {
      thispath = path;
    }
    int servicerequestid = (*it)->requestRawCommand(this, thispath.empty() ? (*it)->getSite()->getBasePath().toString() : thispath, command);
    ongoingrequest.ongoingservicerequests.insert(std::make_pair(it->get(), servicerequestid));
  }
  ongoingrequests.push_back(ongoingrequest);
  if (ongoingrequest.async) {
    respondAsynced(ongoingrequest);
  }
}

void RestApi::handleRawGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  Path path = request.getPath();
  std::string apirequestidstr = path.level(1).toString();
  int apirequestid;
  try {
    apirequestid = std::stoi(apirequestidstr);
  }
  catch(std::exception& e) {
    cb->requestHandled(connrequestid, badRequestResponse("Invalid request id: " + apirequestidstr));
    return;
  }
  OngoingRequest* ongoingrequest = findOngoingRequest(apirequestid);
  if (!ongoingrequest) {
    http::Response response(404);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  if (ongoingrequest->ongoingservicerequests.empty()) {
    finalize(*ongoingrequest);
    return;
  }
  else {
    http::Response response(202);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
}

void RestApi::handleSitesGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  nlohmann::json sitelist = nlohmann::json::array();
  for (std::vector<std::shared_ptr<Site>>::const_iterator it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); ++it) {
    sitelist.push_back((*it)->getName());
  }
  http::Response response(200);
  std::string jsondump = sitelist.dump(2);
  response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
  response.addHeader("Content-Type", "application/json");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSitesPost(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  nlohmann::json jsondata = getJsonFromBody(request);
  std::shared_ptr<Site> site = global->getSiteManager()->createNewSite();
  auto nameit = jsondata.find("name");
  if (nameit == jsondata.end()) {
    cb->requestHandled(connrequestid, badRequestResponse("Missing key: name"));
    return;
  }
  if (global->getSiteManager()->getSite(std::string(*nameit))) {
    http::Response response(409);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  std::shared_ptr<http::Response> updateresponse = updateSite(site, jsondata, true);
  if (updateresponse) {
    cb->requestHandled(connrequestid, *updateresponse);
    return;
  }
  http::Response response(201);
  response.appendHeader("Content-Length", "0");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSiteGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  Path path = request.getPath();
  std::string sitename = path.level(1).toString();
  std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitename);
  std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(sitename);
  if (!site || !sl) {
    http::Response response(404);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  nlohmann::json j;
  nlohmann::json addrlist = nlohmann::json::array();
  for (Address addr : site->getAddresses()) {
    addrlist.push_back(addr.toString());
  }
  j["addresses"] = addrlist;
  j["user"] = site->getUser();
  j["password"] = site->getPass();
  j["base_path"] = site->getBasePath().toString();
  j["max_logins"] = site->getMaxLogins();
  j["max_sim_up"] = site->getInternMaxUp();
  j["max_sim_down"] = site->getInternMaxDown();
  j["max_sim_down_pre"] = site->getInternMaxDownPre();
  j["max_sim_down_complete"] = site->getInternMaxDownComplete();
  j["max_sim_down_transferjob"] = site->getInternMaxDownTransferJob();
  j["max_idle_time"] = site->getMaxIdleTime();
  j["pret"] = site->needsPRET();
  j["force_binary_mode"] = site->forceBinaryMode();
  j["list_command"] = listCommandToString(site->getListCommand());
  j["tls_mode"] = tlsModeToString(site->getTLSMode());
  j["tls_transfer_policy"] = tlsTransferPolicyToString(site->getSSLTransferPolicy());
  j["transfer_protocol"] = transferProtocolToString(site->getTransferProtocol());
  j["sscn"] = site->supportsSSCN();
  j["cpsv"] = site->supportsCPSV();
  j["cepr"] = site->supportsCEPR();
  j["broken_pasv"] = site->hasBrokenPASV();
  j["disabled"] = site->getDisabled();
  j["allow_upload"] = siteAllowTransferToString(site->getAllowUpload());
  j["allow_download"] = siteAllowTransferToString(site->getAllowDownload());
  j["priority"] = priorityToString(site->getPriority());
  j["xdupe"] = site->useXDUPE();
  for (std::map<std::string, Path>::const_iterator it = site->sectionsBegin(); it != site->sectionsEnd(); ++it) {
    j["sections"][it->first] = it->second.toString();
  }
  for (std::map<std::string, int>::const_iterator it = site->avgspeedBegin(); it != site->avgspeedEnd(); ++it) {
    j["avg_speed"][it->first] = it->second;
  }
  nlohmann::json affils = nlohmann::json::array();
  for (std::set<std::string>::const_iterator it = site->affilsBegin(); it != site->affilsEnd(); ++it) {
    affils.push_back(*it);
  }
  j["affils"] = affils;
  j["transfer_source_policy"] = siteTransferPolicyToString(site->getTransferSourcePolicy());
  j["transfer_target_policy"] = siteTransferPolicyToString(site->getTransferTargetPolicy());
  nlohmann::json exceptsource = nlohmann::json::array();
  for (std::set<std::shared_ptr<Site> >::const_iterator it = site->exceptSourceSitesBegin(); it != site->exceptSourceSitesEnd(); ++it) {
    exceptsource.push_back((*it)->getName());
  }
  j["except_source_sites"] = exceptsource;
  nlohmann::json excepttarget = nlohmann::json::array();
  for (std::set<std::shared_ptr<Site> >::const_iterator it = site->exceptTargetSitesBegin(); it != site->exceptTargetSitesEnd(); ++it) {
    excepttarget.push_back((*it)->getName());
  }
  j["except_target_sites"] = excepttarget;
  j["leave_free_slot"] = site->getLeaveFreeSlot();
  j["stay_logged_in"] = site->getStayLoggedIn();
  j["skiplist"] = jsonSkipList(site->getSkipList());
  j["proxy_type"] = proxyTypeToString(site->getProxyType());
  j["proxy_name"] = site->getProxy();
  j["var"]["size_up_all"] = site->getSizeUp().getAll();
  j["var"]["size_up_24h"] = site->getSizeUp().getLast24Hours();
  j["var"]["files_up_all"] = site->getFilesUp().getAll();
  j["var"]["files_up_24h"] = site->getFilesUp().getLast24Hours();
  j["var"]["size_down_all"] = site->getSizeDown().getAll();
  j["var"]["size_down_24h"] = site->getSizeDown().getLast24Hours();
  j["var"]["files_down_all"] = site->getFilesDown().getAll();
  j["var"]["files_down_24h"] = site->getFilesDown().getLast24Hours();
  j["var"]["current_logins"] = sl->getCurrLogins();
  j["var"]["current_up"] = sl->getCurrUp();
  j["var"]["current_down"] = sl->getCurrDown();
  http::Response response(200);
  std::string jsondump = j.dump(2);
  response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
  response.addHeader("Content-Type", "application/json");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSitePatch(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  Path path = request.getPath();
  std::string sitename = path.level(1).toString();
  std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitename);
  if (!site) {
    http::Response response(404);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  nlohmann::json jsondata = getJsonFromBody(request);
  std::shared_ptr<http::Response> updateresponse = updateSite(site, jsondata, false);
  if (updateresponse) {
    cb->requestHandled(connrequestid, *updateresponse);
    return;
  }
  http::Response response(204);
  response.appendHeader("Content-Length", "0");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSiteDelete(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  Path path = request.getPath();
  std::string sitename = path.level(1).toString();
  global->getSiteManager()->deleteSite(sitename);
  http::Response response(204);
  response.appendHeader("Content-Length", "0");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleFileListGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  if (!request.hasQueryParam("site")) {
    cb->requestHandled(connrequestid, badRequestResponse("Missing query parameter: site"));
    return;
  }
  if (!request.hasQueryParam("path")) {
    cb->requestHandled(connrequestid, badRequestResponse("Missing query parameter: path"));
    return;
  }
  std::string sitestr = request.getQueryParamValue("site");
  std::shared_ptr<Site> site = global->getSiteManager()->getSite(sitestr);
  std::shared_ptr<SiteLogic> sl = global->getSiteLogicManager()->getSiteLogic(sitestr);
  if (!site || !sl) {
    cb->requestHandled(connrequestid, badRequestResponse("Site not found: " + sitestr));
    return;
  }
  Path path = request.getQueryParamValue("path");
  if (!useOrSectionTranslate(path, site)) {
    cb->requestHandled(connrequestid, badRequestResponse("Path must be absolute or a section on " + sitestr + ": " + path.toString()));
    return;
  }
  int timeout = DEFAULT_TIMEOUT_SECONDS;
  if (request.hasQueryParam("timeout")) {
    std::string timeoutstr = request.getQueryParamValue("timeout");
    try {

      timeout = std::stoi(timeoutstr);
    }
    catch(std::exception& e) {
      cb->requestHandled(connrequestid, badRequestResponse("Invalid timeout value: " + timeoutstr));
      return;
    }
  }
  int servicerequestid = sl->requestFileList(this, path);
  OngoingRequest ongoingrequest;
  ongoingrequest.type = OngoingRequestType::FILE_LIST;
  ongoingrequest.connrequestid = connrequestid;
  ongoingrequest.apirequestid = nextrequestid++;
  ongoingrequest.cb = cb;
  ongoingrequest.timeout = timeout;
  ongoingrequest.ongoingservicerequests.insert(std::make_pair(sl.get(), servicerequestid));
  ongoingrequests.push_back(ongoingrequest);
}

void RestApi::handleSpreadJobPost(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  nlohmann::json jsondata = getJsonFromBody(request);
  auto sectionit = jsondata.find("section");
  if (sectionit == jsondata.end()) {

    cb->requestHandled(connrequestid, badRequestResponse("Missing key: section"));
    return;
  }
  std::string section = *sectionit;
  auto nameit = jsondata.find("name");
  if (nameit == jsondata.end()) {
    cb->requestHandled(connrequestid, badRequestResponse("Missing key: name"));
    return;
  }
  std::string name = *nameit;
  std::list<std::string> sites;
  auto sitesit = jsondata.find("sites");
  if (sitesit != jsondata.end()) {
    for (auto it = sitesit->begin(); it != sitesit->end(); ++it) {
      sites.push_back(*it);
    }
  }
  std::list<std::string> dlonlysites;
  auto dlonlysitesit = jsondata.find("sites_dlonly");
  if (dlonlysitesit != jsondata.end()) {
    for (auto it = dlonlysitesit->begin(); it != dlonlysitesit->end(); ++it) {
      dlonlysites.push_back(*it);
    }
  }
  auto allit = jsondata.find("sites_all");
  if (allit != jsondata.end() && allit->get<bool>()) {
    sites.clear();
    std::vector<std::shared_ptr<Site>>::const_iterator it;
    for (it = global->getSiteManager()->begin(); it != global->getSiteManager()->end(); it++) {
      if (!(*it)->getDisabled() && (*it)->hasSection(section)) {
        sites.push_back((*it)->getName());
      }
    }
  }
  auto resetit = jsondata.find("reset");
  bool reset = resetit != jsondata.end() && resetit->get<bool>();
  bool success = false;
  SpreadProfile profile = SPREAD_RACE;
  auto profileit = jsondata.find("profile");
  if (profileit != jsondata.end()) {
    profile = stringToProfile(*profileit);
  }
  switch (profile) {
    case  SPREAD_DISTRIBUTE:
      success = !!global->getEngine()->newDistribute(name, section, sites, reset, dlonlysites);
      break;
    case SPREAD_PREPARE:
      success = global->getEngine()->prepareRace(name, section, sites, reset, dlonlysites);
      break;
    case SPREAD_RACE:
      success = !!global->getEngine()->newRace(name, section, sites, reset, dlonlysites);
      break;
  }
  http::Response response(201);
  response.appendHeader("Content-Length", "0");
  if (!success) {
    response.setStatusCode(503);
  }
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSpreadJobGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  std::string name = Path(request.getPath()).baseName();
  std::shared_ptr<Race> spreadjob = global->getEngine()->getRace(name);
  if (!spreadjob) {
    http::Response response(404);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  nlohmann::json j;

  nlohmann::json sites = nlohmann::json::array();
  nlohmann::json dlonlysites = nlohmann::json::array();
  nlohmann::json incsites = nlohmann::json::array();
  std::set<std::pair<std::shared_ptr<SiteRace>, std::shared_ptr<SiteLogic>>>::const_iterator it;
  for (it = spreadjob->begin(); it != spreadjob->end(); ++it) {
    std::string sitename = it->first->getSiteName();
    sites.push_back(sitename);
    if (it->first->isDownloadOnly()) {
      dlonlysites.push_back(sitename);
    }
    if (it->first->getStatus() != RaceStatus::DONE) {
      incsites.push_back(sitename);
    }
  }
  j["section"] = spreadjob->getSection();
  j["sites"] = sites;
  j["sites_dlonly"] = dlonlysites;
  j["sites_incomplete"] = incsites;
  j["profile"] = profileToString(spreadjob->getProfile());
  j["status"] = raceStatusToString(spreadjob->getStatus());
  j["time_started"] = spreadjob->getTimeStamp();
  j["time_spent_seconds"] = spreadjob->getTimeSpent();
  j["size_estimated_bytes"] = spreadjob->estimatedTotalSize();
  j["percentage_complete_worst"] = spreadjob->getWorstCompletionPercentage();
  j["percentage_complete_average"] = spreadjob->getAverageCompletionPercentage();
  j["percentage_complete_best"] = spreadjob->getBestCompletionPercentage();
  http::Response response(200);
  std::string jsondump = j.dump(2);
  response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
  response.addHeader("Content-Type", "application/json");
  cb->requestHandled(connrequestid, response);
}

void RestApi::handleSpreadJobsGet(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  nlohmann::json joblist = nlohmann::json::array();
  std::list<std::shared_ptr<Race>>::const_iterator it;
  for (it = global->getEngine()->getRacesBegin(); it != global->getEngine()->getRacesEnd(); ++it) {
    joblist.push_back((*it)->getName());
  }
  http::Response response(200);
  std::string jsondump = joblist.dump(2);
  response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
  response.addHeader("Content-Type", "application/json");
  cb->requestHandled(connrequestid, response);

}

void RestApi::handleRequest(RestApiCallback* cb, int connrequestid, const http::Request& request) {
  bool authorized = false;
  if (request.hasHeader("Authorization")) {
    std::string encoded = request.getHeaderValue("Authorization");
    if (!encoded.compare(0, 6, "Basic ")) {
      Core::BinaryData out;
      Crypto::base64Decode(Core::BinaryData(encoded.begin() + 6, encoded.end()), out);
      std::string decoded(out.begin(), out.end());
      size_t split = decoded.find(':');
      if (split != std::string::npos) {
        std::string password = decoded.substr(split + 1);
        if (password == global->getRemoteCommandHandler()->getPassword()) {
          authorized = true;
        }
      }
    }
  }
  if (!authorized) {
    http::Response response(401);
    response.addHeader("WWW-Authenticate", "Basic");
    response.addHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  Path path(request.getPath());
  global->getEventLog()->log("RestApi", "Received request for: " + request.getMethod() + " " + path.toString());
  std::map<std::pair<Path, int>, std::map<std::string, EndpointPointer>>::iterator it;
  it = endpoints.find(std::make_pair(path, path.levels()));
  if (it == endpoints.end()) {
    it = endpoints.find(std::make_pair(path.cutLevels(1), path.levels()));
  }
  if (it == endpoints.end()) {
    http::Response response(404);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  std::map<std::string, EndpointPointer>::iterator it2 = it->second.find(request.getMethod());
  if (it2 == it->second.end()) {
    http::Response response(405);
    response.appendHeader("Content-Length", "0");
    cb->requestHandled(connrequestid, response);
    return;
  }
  try {
    (this->*(it2->second))(cb, connrequestid, request);
  }
  catch (nlohmann::json::exception& e) {
    cb->requestHandled(connrequestid, badRequestResponse(e.what()));
    return;
  }
}

OngoingRequest* RestApi::findOngoingRequest(void* service, int servicerequestid) {
  std::pair<void*, int> pair = std::make_pair(service, servicerequestid);
  for (OngoingRequest& request : ongoingrequests) {
    if (request.ongoingservicerequests.find(pair) != request.ongoingservicerequests.end()) {
      return &request;
    }
  }
  return nullptr;
}

OngoingRequest* RestApi::findOngoingRequest(int apirequestid) {
  for (OngoingRequest& request : ongoingrequests) {
    if (request.apirequestid == apirequestid) {
      return &request;
    }
  }
  return nullptr;
}

void RestApi::requestReady(void* service, int servicerequestid) {
  OngoingRequest* request = findOngoingRequest(service, servicerequestid);
  if (!request) {
    return;
  }

  switch (request->type) {
    case OngoingRequestType::RAW_COMMAND: {
      SiteLogic* sl = static_cast<SiteLogic*>(service);
      bool status = sl->requestStatus(servicerequestid);
      if (!status) {
        request->failures.emplace_back(service, "failed");
      }
      else {
        std::string result = sl->getRawCommandResult(servicerequestid);
        request->successes.emplace_back(service, result);
      }
      sl->finishRequest(servicerequestid);
      request->ongoingservicerequests.erase(std::make_pair(service, servicerequestid));
      if (request->ongoingservicerequests.empty() && !request->async) {
        finalize(*request);
      }
      return;
    }
    case OngoingRequestType::FILE_LIST: {
      SiteLogic* sl = static_cast<SiteLogic*>(service);
      bool status = sl->requestStatus(servicerequestid);
      if (!status) {
        http::Response response(502);
        response.appendHeader("Content-Length", "0");
        request->cb->requestHandled(request->connrequestid, response);
      }
      else {
        FileListData* data = sl->getFileListData(servicerequestid);
        std::shared_ptr<FileList> fl = data->getFileList();
        nlohmann::json j = nlohmann::json::object();
        for (std::list<File*>::const_iterator it = fl->begin(); it != fl->end(); ++it) {
          File* f = *it;
          std::string name = f->getName();
          j[name]["size"] = f->getSize();
          j[name]["user"] = f->getOwner();
          j[name]["group"] = f->getGroup();
          j[name]["type"] = f->isDirectory() ? "DIR" : (f->isLink() ? "LINK" : "FILE");
          j[name]["last_modified"] = f->getLastModified();
          if (f->isLink()) {
            j[name]["link_target"] = f->getLinkTarget();
          }
        }
        http::Response response(200);
        std::string jsondump = j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
        response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
        response.addHeader("Content-Type", "application/json");
        request->cb->requestHandled(request->connrequestid, response);
      }
      sl->finishRequest(servicerequestid);
      for (std::list<OngoingRequest>::iterator it = ongoingrequests.begin(); it != ongoingrequests.end(); ++it) {
        if (&*it == request) {
          ongoingrequests.erase(it);
          break;
        }
      }
      return;
    }
  }
}

void RestApi::finalize(OngoingRequest& request) {
  switch (request.type) {
    case OngoingRequestType::RAW_COMMAND: {
      nlohmann::json successlist = nlohmann::json::array();
      nlohmann::json failurelist = nlohmann::json::array();
      for (const std::pair<void*, std::string>& success : request.successes) {
        SiteLogic* sl = static_cast<SiteLogic*>(success.first);
        nlohmann::json site;
        site["name"] = sl->getSite()->getName();
        site["result"] = success.second;
        successlist.push_back(site);
      }
      for (const std::pair<void*, int>& ongoing : request.ongoingservicerequests) {
        request.failures.emplace_back(ongoing.first, "timeout");
      }
      for (const std::pair<void*, std::string>& failure : request.failures) {
        SiteLogic* sl = static_cast<SiteLogic*>(failure.first);
        nlohmann::json site;
        site["name"] = sl->getSite()->getName();
        site["reason"] = failure.second;
        failurelist.push_back(site);
      }
      nlohmann::json j;
      j["successes"] = successlist;
      j["failures"] = failurelist;
      http::Response response(200);
      std::string jsondump = j.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);
      response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
      response.addHeader("Content-Type", "application/json");
      request.cb->requestHandled(request.connrequestid, response);
      break;
    }
    case OngoingRequestType::FILE_LIST:
      http::Response response(504);
      response.appendHeader("Content-Length", "0");
      request.cb->requestHandled(request.connrequestid, response);
      break;
  }
  for (std::list<OngoingRequest>::iterator it = ongoingrequests.begin(); it != ongoingrequests.end(); ++it) {
    if (&*it == &request) {
      ongoingrequests.erase(it);
      break;
    }
  }
}

void RestApi::respondAsynced(OngoingRequest& request) {
  http::Response response(202);
  nlohmann::json j;
  j["request_id"] = request.apirequestid;
  response.addHeader("Content-Type", "application/json");
  std::string jsondump = j.dump(2);
  response.setBody(std::vector<char>(jsondump.begin(), jsondump.end()));
  request.cb->requestHandled(request.connrequestid, response);
}

void RestApi::tick(int message) {
  std::list<OngoingRequest*> finalizerequests;
  for (OngoingRequest& request : ongoingrequests) {
    request.timepassed += RESTAPI_TICK_INTERVAL_MS;
    if (request.timepassed > request.timeout * 1000) {
      finalizerequests.push_back(&request);
    }
  }
  for (OngoingRequest* request : finalizerequests) {
    finalize(*request);
  }
}

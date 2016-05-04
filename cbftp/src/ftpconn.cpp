#include "ftpconn.h"

#include <stdlib.h>

#include <cstring>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include "core/workmanager.h"
#include "core/iomanager.h"
#include "core/tickpoke.h"
#include "ftpconnect.h"
#include "filelist.h"
#include "site.h"
#include "globalcontext.h"
#include "rawbuffer.h"
#include "sitelogic.h"
#include "eventlog.h"
#include "proxymanager.h"
#include "util.h"

extern GlobalContext * global;

#define FTPCONN_TICK_INTERVAL 1000

FTPConn::FTPConn(SiteLogic * sl, int id) {
  this->sl = sl;
  this->id = id;
  this->site = sl->getSite();
  this->status = "disconnected";
  nextconnectorid = 0;
  processing = false;
  rawbuf = new RawBuffer(RAWBUFMAXLEN, site->getName(), util::int2Str(id));
  aggregatedrawbuf = sl->getAggregatedRawBuffer();
  iom = global->getIOManager();
  databuflen = DATABUF;
  databuf = (char *) malloc(databuflen);
  databufpos = 0;
  protectedmode = PROT_UNSET;
  sscnmode = false;
  mkdtarget = false;
  currentpath = "/";
  state = STATE_DISCONNECTED;
}

FTPConn::~FTPConn() {
  global->getTickPoke()->stopPoke(this, 0);
  if (isConnected()) {
    iom->closeSocket(sockid);
  }
  delete rawbuf;
  delete databuf;
  clearConnectors();
}

int FTPConn::getId() const {
  return id;
}

void FTPConn::setId(int id) {
  this->id = id;
  rawbuf->setId(id);
}

std::string FTPConn::getStatus() const {
  return status;
}

void FTPConn::login() {
  if (state != STATE_DISCONNECTED) {
    return;
  }
  protectedmode = PROT_UNSET;
  sscnmode = false;
  mkdtarget = false;
  databufpos = 0;
  processing = true;
  currentpath = "/";
  state = STATE_CONNECTING;
  connectors.push_back(makePointer<FTPConnect>(nextconnectorid++, this, site->getAddress(), site->getPort(), getProxy(), true));
  if (site->getAddresses().size() > 1) {
    ticker = 0;
    global->getTickPoke()->startPoke(this, "FTPConn", FTPCONN_TICK_INTERVAL, 0);
  }
}

void FTPConn::connectAllAddresses() {
  std::list<std::pair<std::string, std::string> > addresses = site->getAddresses();
  Proxy * proxy = getProxy();
  for (std::list<std::pair<std::string, std::string> >::const_iterator it = addresses.begin(); it != addresses.end(); it++) {
    if (it == addresses.begin()) continue; // first one is already connected
    connectors.push_back(makePointer<FTPConnect>(nextconnectorid++, this, it->first, it->second, proxy, false));
  }
}

Proxy * FTPConn::getProxy() const {
  Proxy * proxy = NULL;
  int proxytype = site->getProxyType();
  if (proxytype == SITE_PROXY_USE) {
    proxy = global->getProxyManager()->getProxy(site->getProxy());
  }
  else if (proxytype == SITE_PROXY_GLOBAL) {
    proxy = global->getProxyManager()->getDefaultProxy();
  }
  return proxy;
}

void FTPConn::clearConnectors() {
  std::list<Pointer<FTPConnect> >::const_iterator it;
  for (std::list<Pointer<FTPConnect> >::const_iterator it = connectors.begin(); it != connectors.end(); it++) {
    (*it)->disengage();
    global->getWorkManager()->deferDelete(*it);
  }
  connectors.clear();
}

void FTPConn::FDDisconnected(int sockid) {
  if (state != STATE_DISCONNECTED) {
    rawBufWriteLine("[Disconnected]");
    this->status = "disconnected";
    state = STATE_DISCONNECTED;
    sl->disconnected(id);
  }
}

void FTPConn::FDSSLSuccess(int sockid) {
  printCipher(sockid);
  if (state == STATE_AUTH_TLS) {
    doUSER(false);
  }
  else {
    state = STATE_IDLE;
  }
}

void FTPConn::FDSSLFail(int sockid) {

}

void FTPConn::printCipher(int sockid) {
  rawBufWriteLine("[Cipher: " + iom->getCipher(sockid) + "]");
}

bool FTPConn::parseData(char * data, unsigned int datalen, char ** databuf, unsigned int & databuflen, unsigned int & databufpos, int & databufcode) {
  while (databufpos + datalen > databuflen) {
    databuflen = databuflen * 2;
    char * newdatabuf = (char *) malloc(databuflen);
    memcpy(newdatabuf, *databuf, databufpos);
    delete *databuf;
    *databuf = newdatabuf;
  }
  memcpy(*databuf + databufpos, data, datalen);
  databufpos += datalen;
  bool messagecomplete = false;
  char * loc = 0;
  if((*databuf)[databufpos - 1] == '\n') {
    loc = *databuf + databufpos - 5;
    while (loc >= *databuf) {
      if (isdigit(*loc) && isdigit(*(loc+1)) && isdigit(*(loc+2))) {
        if ((*(loc+3) == ' ' || *(loc+3) == '\n') && (loc == *databuf || *(loc-1) == '\n')) {
          messagecomplete = true;
          databufcode = atoi(std::string(loc, 3).data());
          break;
        }
      }
      --loc;
    }
  }
  if (messagecomplete) {
    if (databufcode == 550) {
      // workaround for a glftpd bug causing an extra row '550 Unable to load your own user file!.' on retr/stor
      if (*(loc+4) == 'U' && *(loc+5) == 'n' && *(loc+28) == 'u' && *(loc+33) == 'f') {
        databufpos = 0;
        return false;
      }
    }
  }
  else if (datalen == databufpos && datalen >= 3 && (!isdigit(**databuf) ||
           !isdigit(*(*databuf+1)) || !isdigit(*(*databuf+2))))
  {
    databufcode = 0;
    return true;
  }
  return messagecomplete;
}

void FTPConn::FDData(int sockid, char * data, unsigned int datalen) {
  if (state != STATE_STAT) {
    rawBufWrite(std::string(data, datalen));
  }
  if (parseData(data, datalen, &databuf, databuflen, databufpos, databufcode)) {
    switch(state) {
      case STATE_AUTH_TLS: // awaiting AUTH TLS response
        AUTHTLSResponse();
        break;
      case STATE_USER: // awaiting USER response
        USERResponse();
        break;
      case STATE_PASS: // awaiting PASS response
        PASSResponse();
        break;
      case STATE_STAT: // awaiting STAT response
        STATResponse();
        break;
      case STATE_PWD: // awaiting PWD response
        PWDResponse();
        break;
      case STATE_PROT_P: // awaiting PROT P response
        PROTPResponse();
        break;
      case STATE_PROT_C:  // awaiting PROT C response
        PROTCResponse();
        break;
      case STATE_RAW: // awaiting raw response
        RawResponse();
        break;
      case STATE_CPSV: // awaiting CPSV response
        CPSVResponse();
        break;
      case STATE_PASV: // awaiting PASV response
        PASVResponse();
        break;
      case STATE_PORT: // awaiting PORT response
        PORTResponse();
        break;
      case STATE_CWD: // awaiting CWD response
        CWDResponse();
        break;
      case STATE_MKD: // awaiting MKD response
        MKDResponse();
        break;
      case STATE_PRET_RETR: // awaiting PRET RETR response
        PRETRETRResponse();
        break;
      case STATE_PRET_STOR: // awaiting PRET STOR response
        PRETSTORResponse();
        break;
      case STATE_RETR: // awaiting RETR response
        RETRResponse();
        break;
      case STATE_RETR_COMPLETE: // awaiting RETR complete
        RETRComplete();
        break;
      case STATE_STOR: // awaiting STOR response
        STORResponse();
        break;
      case STATE_STOR_COMPLETE: // awaiting STOR complete
        STORComplete();
        break;
      case STATE_ABOR: // awaiting ABOR response
        ABORResponse();
        break;
      case STATE_QUIT: // awaiting QUIT response
        QUITResponse();
        break;
      case STATE_USER_LOGINKILL: // awaiting loginkilling USER response
        USERResponse();
        break;
      case STATE_PASS_LOGINKILL: // awaiting loginkilling PASS response
        PASSResponse();
        break;
      case STATE_WIPE: // awaiting WIPE response
        WIPEResponse();
        break;
      case STATE_DELE: // awaiting DELE response
        DELEResponse();
        break;
      case STATE_NUKE: // awaiting SITE NUKE response
        NUKEResponse();
        break;
      case STATE_LIST: // awaiting LIST response
        LISTResponse();
        break;
      case STATE_PRET_LIST: // awaiting PRET LIST response
        PRETLISTResponse();
        break;
      case STATE_LIST_COMPLETE: // awaiting LIST complete
        LISTComplete();
        break;
      case STATE_SSCN_ON: // awaiting SSCN ON response
        SSCNONResponse();
        break;
      case STATE_SSCN_OFF: // awaiting SSCN OFF response
        SSCNOFFResponse();
        break;
      case STATE_PASV_ABORT: // awaiting aborting PASV
        PASVAbortResponse();
        break;
      case STATE_PBSZ: // awaiting PBSZ 0 response
        PBSZ0Response();
        break;
      case STATE_TYPEI: // awaiting TYPE I response
        TYPEIResponse();
        break;
      case STATE_IDLE: // not expecting any response
        break;
      default: // something is wrong
        util::assert(false);
        break;
    }
    if (!processing) {
      state = STATE_IDLE;
    }
    databufpos = 0;
  }
}

void FTPConn::ftpConnectInfo(int connectorid, const std::string & info) {
  rawBufWriteLine(info);
}

void FTPConn::ftpConnectSuccess(int connectorid) {
  if (state != STATE_CONNECTING) {
    return;
  }
  std::list<Pointer<FTPConnect> >::const_iterator it;
  for (it = connectors.begin(); it != connectors.end(); it++) {
    if ((*it)->getId() == connectorid) {
      break;
    }
  }
  util::assert(it != connectors.end());
  sockid = (*it)->handedOver();
  iom->adopt(this, sockid);
  if ((*it)->isPrimary()) {
    global->getTickPoke()->stopPoke(this, 0);
  }
  else {
    std::string addr = (*it)->getAddress();
    std::string port = (*it)->getPort();
    site->setPrimaryAddress((*it)->getAddress(), (*it)->getPort());
    rawBufWriteLine("[Setting " + addr + ":" + port + " as primary]");
  }
  if (connectors.size() > 1) {
    rawBufWriteLine("[Disconnecting other attempts]");
  }
  clearConnectors();
  if (site->SSL()) {
    state = STATE_AUTH_TLS;
    sendEcho("AUTH TLS");
  }
  else {
    doUSER(false);
  }
}

void FTPConn::ftpConnectFail(int connectorid) {
  if (state != STATE_CONNECTING) {
    return;
  }
  std::list<Pointer<FTPConnect> >::iterator it;
  for (it = connectors.begin(); it != connectors.end(); it++) {
    if ((*it)->getId() == connectorid) {
      break;
    }
  }
  util::assert(it != connectors.end());
  bool primary = (*it)->isPrimary();
  global->getWorkManager()->deferDelete(*it);
  connectors.erase(it);
  if (primary) {
    global->getTickPoke()->stopPoke(this, 0);
    if (site->getAddresses().size() > 1) {
      connectAllAddresses();
    }
  }
  if (!connectors.size()) {
    state = STATE_DISCONNECTED;
    sl->connectFailed(id);
  }
}

void FTPConn::tick(int) {
  ticker += FTPCONN_TICK_INTERVAL;
  if (ticker >= 1000) {
    if (state == STATE_CONNECTING) {
      connectAllAddresses();
    }
    global->getTickPoke()->stopPoke(this, 0);
  }
}

void FTPConn::sendEcho(const std::string & data) {
  rawBufWriteLine(data);
  processing = true;
  status = data;
  iom->sendData(sockid, data + "\r\n");
}

void FTPConn::AUTHTLSResponse() {
  if (databufcode == 234) {
    iom->negotiateSSLConnect(sockid);
  }
  else {
    rawBufWriteLine("[Unknown response]");
    state = STATE_DISCONNECTED;
    processing = false;
    iom->closeSocket(sockid);
    sl->TLSFailed(id);
  }
}

void FTPConn::doUSER(bool killer) {
  if (!killer) {
    state = STATE_USER;
  }
  else {
    state = STATE_USER_LOGINKILL;
  }
  sendEcho((std::string("USER ") + (killer ? "!" : "") + site->getUser()).data());
}

void FTPConn::USERResponse() {
  if (databufcode == 331) {
    std::string pass = site->getPass();
    std::string passc = "";
    for (unsigned int i = 0; i < pass.length(); i++) passc.append("*");
    std::string output = "PASS " + std::string(passc);
    rawBufWriteLine(output);
    status = output;
    if (state == STATE_USER) {
      state = STATE_PASS;
    }
    else {
      state = STATE_PASS_LOGINKILL;
    }
    iom->sendData(sockid, std::string("PASS ") + site->getPass()  + "\r\n");
  }
  else {
    processing = false;
    bool sitefull = false;
    bool simultaneous = false;
    std::string reply = std::string(databuf, databufpos);
    if (databufcode == 530 || databufcode == 550) {
      if (reply.find("site") != std::string::npos && reply.find("full") != std::string::npos) {
        sitefull = true;
      }
      else if (reply.find("simultaneous") != std::string::npos) {
        simultaneous = true;
      }
    }
    if (sitefull) {
      sl->userDeniedSiteFull(id);
    }
    else if (state == STATE_USER) {
      if (simultaneous) {
        sl->userDeniedSimultaneousLogins(id);
      }
      else {
        sl->userDenied(id);
      }
    }
    else {
      sl->loginKillFailed(id);
    }
  }
}

void FTPConn::PASSResponse() {
  processing = false;
  this->status = "connected";
  if (databufcode == 230) {
    if (site->forceBinaryMode()) {
      state = STATE_TYPEI;
      doTYPEI();
    }
    else {
      state = STATE_PASS;
      sl->commandSuccess(id);
    }
  }
  else {
    bool sitefull = false;
    bool simultaneous = false;
    std::string reply = std::string(databuf, databufpos);
    if (databufcode == 530 || databufcode == 550) {
      if (reply.find("site") != std::string::npos && reply.find("full") != std::string::npos) {
        sitefull = true;
      }
      else if (reply.find("simultaneous") != std::string::npos) {
        simultaneous = true;
      }
    }
    if (sitefull) {
      sl->userDeniedSiteFull(id);
    }
    else if (state == STATE_PASS) {
      if (simultaneous) {
        sl->userDeniedSimultaneousLogins(id);
      }
      else {
        sl->passDenied(id);
      }
    }
    else {
      sl->loginKillFailed(id);
    }
  }
}

void FTPConn::TYPEIResponse() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::reconnect() {
  disconnect();
  login();
}

void FTPConn::doSTAT() {
  doSTAT(NULL, new FileList(site->getUser(), currentpath));
}

void FTPConn::doSTAT(CommandOwner * co, FileList * filelist) {
  state = STATE_STAT;
  currentco = co;
  currentfl = filelist;
  sendEcho("STAT -l");
}

void FTPConn::doSTATla() {
  state = STATE_STAT;
  currentco = NULL;
  currentfl = new FileList(site->getUser(), currentpath);
  sendEcho("STAT -la");
}

void FTPConn::prepareLIST() {
  currentco = NULL;
  currentfl = new FileList(site->getUser(), currentpath);
}

void FTPConn::prepareLIST(CommandOwner * co, FileList * filelist) {
  currentco = co;
  currentfl = filelist;
}

void FTPConn::doLIST() {
  state = STATE_LIST;
  sendEcho("LIST");
}

void FTPConn::doLISTa() {
  state = STATE_LIST;
  sendEcho("LIST -a");
}

void FTPConn::STATResponse() {
  processing = false;
  if (databufcode == 211 || databufcode == 212 || databufcode == 213) {
    parseFileList(databuf, databufpos);
    std::string output = "[File list retrieved]";
    rawBufWriteLine(output);
    sl->listRefreshed(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::LISTResponse() {
  if (databufcode == 150 || databufcode == 125) {
    sl->commandSuccess(id);
    state = STATE_LIST_COMPLETE;
  }
  else {
    processing = false;
    sl->commandFail(id);
  }
}

void FTPConn::LISTComplete() {
  processing = false;
  if (databufcode == 226) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::updateName() {
  rawbuf->rename(site->getName());
}

std::string FTPConn::getCurrentPath() const {
  return currentpath;
}
void FTPConn::doPWD() {
  state = STATE_PWD;
  sendEcho("PWD");
}

void FTPConn::PWDResponse() {
  processing = false;
  if (databufcode == 257) {
    std::string line(databuf, databufpos);
    int loc = 0;
    while(line[++loc] != '"');
    int start = loc + 1;
    while(line[++loc] != '"');
    sl->gotPath(id, line.substr(start, loc - start));
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doPROTP() {
  state = STATE_PROT_P;
  sendEcho("PROT P");
}

void FTPConn::PROTPResponse() {
  processing = false;
  if (databufcode == 200) {
    protectedmode = PROT_P;
    sl->commandSuccess(id);
  }
  else {
    if (databufcode == 503) {
      std::string line(databuf, databufpos);
      if (line.find("PBSZ") != std::string::npos) {
        doPBSZ0();
        return;
      }
    }
    sl->commandFail(id);
  }
}

void FTPConn::doPROTC() {
  state = STATE_PROT_C;
  sendEcho("PROT C");
}

void FTPConn::PROTCResponse() {
  processing = false;
  if (databufcode == 200) {
    protectedmode = PROT_C;
    sl->commandSuccess(id);
  }
  else {
    if (databufcode == 503) {
      std::string line(databuf, databufpos);
      if (line.find("PBSZ") != std::string::npos) {
        doPBSZ0();
        return;
      }
    }
    sl->commandFail(id);
  }
}

void FTPConn::doSSCN(bool on) {
  if (on) {
    state = STATE_SSCN_ON;
    sendEcho("SSCN ON");
  }
  else {
    state = STATE_SSCN_OFF;
    sendEcho("SSCN OFF");
  }
}

void FTPConn::SSCNONResponse() {
  processing = false;
  if (databufcode == 200) {
    sscnmode = true;
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::SSCNOFFResponse() {
  processing = false;
  if (databufcode == 200) {
    sscnmode = false;
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doRaw(std::string command) {
  state = STATE_RAW;
  sendEcho(command.c_str());
}

void FTPConn::doWipe(std::string path, bool recursive) {
  state = STATE_WIPE;
  sendEcho(std::string("SITE WIPE ") + (recursive ? "-r " : "") + path);
}

void FTPConn::doNuke(std::string path, int multiplier, std::string reason) {
  state = STATE_NUKE;
  sendEcho("SITE NUKE " + path + " " + util::int2Str(multiplier) + " " + reason);
}
void FTPConn::doDELE(std::string path) {
  state = STATE_DELE;
  sendEcho("DELE " + path);
}

void FTPConn::doPBSZ0() {
  state = STATE_PBSZ;
  sendEcho("PBSZ 0");
}

void FTPConn::doTYPEI() {
  state = STATE_TYPEI;
  sendEcho("TYPE I");
}

void FTPConn::PBSZ0Response() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::RawResponse() {
  processing = false;
  std::string ret = std::string(databuf, databufpos);
  sl->rawCommandResultRetrieved(id, ret);
}

void FTPConn::WIPEResponse() {
  processing = false;
  if (databufcode == 200) {
    std::string data = std::string(databuf, databufpos);
    if (data.find("successfully") != std::string::npos) {
      sl->commandSuccess(id);
      return;
    }
  }
  sl->commandFail(id);
}

void FTPConn::DELEResponse() {
  processing = false;
  if (databufcode == 250) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::NUKEResponse() {
  processing = false;
  if (databufcode == 200) {
    std::string data = std::string(databuf, databufpos);
    if (data.find("uccess") != std::string::npos) {
      sl->commandSuccess(id);
    }
    else {
      sl->commandFail(id);
    }
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doCPSV() {
  state = STATE_CPSV;
  sendEcho("CPSV");
}

void FTPConn::CPSVResponse() {
  processing = false;
  if (databufcode == 227) {
    std::string data = std::string(databuf, databufpos);
    size_t start = data.find('(') + 1;
    size_t end = data.find(')');
    sl->gotPassiveAddress(id, data.substr(start, end-start));
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doPASV() {
  state = STATE_PASV;
  sendEcho("PASV");
}

void FTPConn::PASVResponse() {
  processing = false;
  if (databufcode == 227) {
    std::string data = std::string(databuf, databufpos);
    size_t start = data.find('(') + 1;
    size_t end = data.find(')');
    sl->gotPassiveAddress(id, data.substr(start, end-start));
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doPORT(std::string addr) {
  state = STATE_PORT;
  sendEcho(("PORT " + addr).c_str());
}

void FTPConn::PORTResponse() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doCWD(std::string path) {
  if (path == currentpath) {
    global->getEventLog()->log("FTPConn " + site->getName() + util::int2Str(id),
        "WARNING: Noop CWD requested: " + path);
    return;
  }
  targetpath = path;
  state = STATE_CWD;
  sendEcho(("CWD " + path).c_str());
}

void FTPConn::CWDResponse() {
  processing = false;
  if (databufcode == 250) {
    currentpath = targetpath;
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doMKD(std::string dir) {
  targetpath = dir;
  state = STATE_MKD;
  sendEcho(("MKD " + dir).c_str());
}

void FTPConn::MKDResponse() {
  processing = false;
  if (databufcode == 257) {
    sl->commandSuccess(id);
  }
  else {
    if (databufcode == 550 &&
        std::string(databuf, databufpos).find("File exist") !=
            std::string::npos) {
      sl->commandSuccess(id);
    }
    else {
      sl->commandFail(id);
    }
  }
}

void FTPConn::doPRETRETR(std::string file) {
  state = STATE_PRET_RETR;
  sendEcho(("PRET RETR " + file).c_str());
}

void FTPConn::PRETRETRResponse() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doPRETSTOR(std::string file) {
  state = STATE_PRET_STOR;
  sendEcho(("PRET STOR " + file).c_str());
}

void FTPConn::PRETSTORResponse() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doPRETLIST() {
  state = STATE_PRET_LIST;
  sendEcho("PRET LIST");
}

void FTPConn::PRETLISTResponse() {
  processing = false;
  if (databufcode == 200) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doRETR(std::string file) {
  state = STATE_RETR;
  sendEcho(("RETR " + file).c_str());
}

void FTPConn::RETRResponse() {
  if (databufcode == 150 || databufcode == 125) {
    sl->commandSuccess(id);
    state = STATE_RETR_COMPLETE;
  }
  else {
    processing = false;
    sl->commandFail(id);
  }
}

void FTPConn::RETRComplete() {
  processing = false;
  if (databufcode == 226) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::doSTOR(std::string file) {
  state = STATE_STOR;
  sendEcho(("STOR " + file).c_str());
}

void FTPConn::STORResponse() {
  if (databufcode == 150 || databufcode == 125) {
    sl->commandSuccess(id);
    state = STATE_STOR_COMPLETE;
  }
  else {
    processing = false;
    sl->commandFail(id);
  }
}

void FTPConn::STORComplete() {
  processing = false;
  if (databufcode == 226) {
    sl->commandSuccess(id);
  }
  else {
    sl->commandFail(id);
  }
}

void FTPConn::abortTransfer() {
  state = STATE_ABOR;
  sendEcho("ABOR");
}

void FTPConn::abortTransferPASV() {
  state = STATE_PASV_ABORT;
  sendEcho("PASV");
}

void FTPConn::ABORResponse() {
  processing = false;
  sl->commandSuccess(id);
}

void FTPConn::PASVAbortResponse() {
  processing = false;
  sl->commandSuccess(id);
}

void FTPConn::doQUIT() {
  if (state != STATE_DISCONNECTED) {
    state = STATE_QUIT;
    sendEcho("QUIT");
  }
}

void FTPConn::doSSLHandshake() {
  state = STATE_SSL_HANDSHAKE;
  iom->forceSSLhandshake(sockid);
}

void FTPConn::QUITResponse() {
  processing = false;
  disconnect();
}

void FTPConn::disconnect() {
  if (state == STATE_CONNECTING) {
    global->getTickPoke()->stopPoke(this, 0);
  }
  if (state != STATE_DISCONNECTED) {
    state = STATE_DISCONNECTED;
    iom->closeSocket(sockid);
    clearConnectors();
    this->status = "disconnected";
    rawBufWriteLine("[Disconnected]");
  }
}

RawBuffer * FTPConn::getRawBuffer() const {
  return rawbuf;
}

FTPConnState FTPConn::getState() const {
  return state;
}

std::string FTPConn::getConnectedAddress() const {
  return iom->getSocketAddress(sockid);
}

std::string FTPConn::getInterfaceAddress() const {
  return iom->getInterfaceAddress(sockid);
}

ProtMode FTPConn::getProtectedMode() const {
  return protectedmode;
}

bool FTPConn::getSSCNMode() const {
  return sscnmode;
}

void FTPConn::setMKDCWDTarget(std::string section, std::string subpath) {
  mkdtarget = true;
  mkdsect = section;
  mkdpath = subpath;
  size_t lastpos = 0;
  mkdsubdirs.clear();
  while (true) {
    size_t splitpos = mkdpath.find("/", lastpos);
    if (splitpos != std::string::npos) {
      mkdsubdirs.push_back(mkdpath.substr(lastpos, splitpos - lastpos));
    }
    else {
      mkdsubdirs.push_back(mkdpath.substr(lastpos));
      break;
    }
    lastpos = splitpos + 1;
  }
}

void FTPConn::finishMKDCWDTarget() {
  mkdtarget = false;
}

bool FTPConn::hasMKDCWDTarget() const {
  return mkdtarget;
}

std::string FTPConn::getTargetPath() const {
  return targetpath;
}

std::string FTPConn::getMKDCWDTargetSection() const {
  return mkdsect;
}
std::string FTPConn::getMKDCWDTargetPath() const {
  return mkdpath;
}

std::list<std::string> * FTPConn::getMKDSubdirs() {
  return &mkdsubdirs;
}

FileList * FTPConn::currentFileList() const {
  return currentfl;
}

CommandOwner * FTPConn::currentCommandOwner() const {
  return currentco;
}

void FTPConn::setCurrentCommandOwner(CommandOwner * co) {
  currentco = co;
}

bool FTPConn::isProcessing() const {
  return processing;
}

void FTPConn::parseFileList(char * buf, unsigned int buflen) {
  char * loc = buf, * start;
  unsigned int files = 0;
  int touch = rand();
  while (loc + 4 < buf + buflen && !(*(loc + 1) == '2' && *(loc + 2) == '1' && *(loc + 4) == ' ')) {
    if (*(loc + 1) == '2' && *(loc + 2) == '1' && *(loc + 4) == '-') loc += 4;
    start = loc;
    while (loc < buf + buflen && loc - start < 40) {
      start = loc;
      while(loc < buf + buflen && *loc++ != '\n');
    }
    if (loc - start >= 40) {
      if (currentfl->updateFile(std::string(start, loc - start), touch)) {
        files++;
      }
    }
  }
  if (currentfl->getSize() > files) {
    currentfl->cleanSweep(touch);
  }
  if (!currentfl->isFilled()) currentfl->setFilled();
}

bool FTPConn::isConnected() const {
  return state != STATE_DISCONNECTED;
}

void FTPConn::rawBufWrite(const std::string& data) {
  rawbuf->write(data);
  aggregatedrawbuf->write(rawbuf->getTag(), data);
}

void FTPConn::rawBufWriteLine(const std::string& data) {
  rawbuf->writeLine(data);
  aggregatedrawbuf->writeLine(rawbuf->getTag(), data);
}

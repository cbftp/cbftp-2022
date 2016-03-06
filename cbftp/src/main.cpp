#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "settingsloadersaver.h"
#include "sitelogicmanager.h"
#include "transfermanager.h"
#include "sitemanager.h"
#include "globalcontext.h"
#include "engine.h"
#include "tickpoke.h"
#include "remotecommandhandler.h"
#include "iomanager.h"
#include "workmanager.h"
#include "skiplist.h"
#include "eventlog.h"
#include "proxymanager.h"
#include "localstorage.h"
#include "externalfileviewing.h"
#include "timereference.h"
#include "threading.h"
#include "uibase.h"

GlobalContext * global;

namespace {

class Main {
public:
  Main() {
    TimeReference::updateTime();

    global = new GlobalContext();
    EventLog * el = new EventLog();
    global->linkEventLog(el);
    WorkManager * wm = new WorkManager();
    global->linkWorkManager(wm);
    TickPoke * tp = new TickPoke();
    global->linkTickPoke(tp);
    SettingsLoaderSaver * sls = new SettingsLoaderSaver();
    LocalStorage * ls = new LocalStorage();
    IOManager * iom = new IOManager();
    Engine * e = new Engine();
    SiteManager * sm = new SiteManager();
    SiteLogicManager * slm = new SiteLogicManager();
    TransferManager * tm = new TransferManager();
    RemoteCommandHandler * rch = new RemoteCommandHandler();
    SkipList * sl = new SkipList();
    ProxyManager * pm = new ProxyManager();
    ExternalFileViewing * efv = new ExternalFileViewing();
    TimeReference * tr = new TimeReference();

    UIBase * uibase = UIBase::instance();

    global->linkComponents(sls, iom, e, uibase, sm, slm, tm, rch, sl, pm, ls, efv, tr);

    Threading::setCurrentThreadName("cbftp");

    if (!uibase->init()) exit(1);
    wm->init();
    iom->init();
    tp->tickerLoop();
    global->getExternalFileViewing()->killAll();
    uibase->kill();
    sls->saveSettings();
  }
};

void sighandler(int sig) {
  global->getTickPoke()->breakLoop();
}

}

int main(int argc, char * argv[]) {
  struct sigaction sa;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);
  sa.sa_handler = sighandler;
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  Main();
}



#pragma once

#include <openssl/ssl.h>
#include <pthread.h>

class Engine;
class UIBase;
class SiteLogicManager;
class SiteManager;
class TransferManager;
class DataFileHandler;
class TickPoke;
class RemoteCommandHandler;
class IOManager;
class WorkManager;
class SkipList;
class EventLog;
class ProxyManager;
class LocalStorage;
class ExternalFileViewing;
class TimeReference;

class GlobalContext {
  private:
    SSL_CTX * ssl_ctx;
    pthread_attr_t attr;
    Engine * e;
    DataFileHandler * dfh;
    IOManager * iom;
    WorkManager * wm;
    UIBase * uib;
    SiteManager * sm;
    SiteLogicManager * slm;
    TransferManager * tm;
    TickPoke * tp;
    RemoteCommandHandler * rch;
    SkipList * sl;
    EventLog * el;
    ProxyManager * pm;
    LocalStorage * ls;
    ExternalFileViewing * efv;
    TimeReference * tr;
    int currentyear;
    int currentmonth;
    int currentday;
  public:
    void init();
    GlobalContext();
    void linkEventLog(EventLog *);
    void linkWorkManager(WorkManager *);
    void linkTickPoke(TickPoke *);
    void linkComponents(DataFileHandler *, IOManager *, Engine *,
        UIBase *, SiteManager *, SiteLogicManager *, TransferManager *,
        RemoteCommandHandler *, SkipList *, ProxyManager *,
        LocalStorage *, ExternalFileViewing *, TimeReference *);
    SSL_CTX * getSSLCTX() const;
    Engine * getEngine() const;
    DataFileHandler * getDataFileHandler() const;
    WorkManager * getWorkManager() const;
    IOManager * getIOManager() const;
    UIBase * getUIBase() const;
    SiteManager * getSiteManager() const;
    SiteLogicManager * getSiteLogicManager() const;
    TransferManager * getTransferManager() const;
    TickPoke * getTickPoke() const;
    RemoteCommandHandler * getRemoteCommandHandler() const;
    SkipList * getSkipList() const;
    EventLog * getEventLog() const;
    ProxyManager * getProxyManager() const;
    LocalStorage * getLocalStorage() const;
    ExternalFileViewing * getExternalFileViewing() const;
    TimeReference * getTimeReference() const;
    pthread_attr_t * getPthreadAttr();
    void updateTime();
    int currentYear() const;
    int currentMonth() const;
    int currentDay() const;
};
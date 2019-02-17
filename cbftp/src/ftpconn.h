#pragma once

#include <list>
#include <memory>
#include <string>

#include "core/eventreceiver.h"
#include "ftpconnectowner.h"
#include "path.h"

enum FTPConnState {
  STATE_DISCONNECTED,
  STATE_CONNECTING,
  STATE_AUTH_TLS,
  STATE_USER,
  STATE_PASS,
  STATE_LOGIN,
  STATE_STAT,
  STATE_PWD,
  STATE_PROT_P,
  STATE_PROT_C,
  STATE_RAW,
  STATE_CPSV,
  STATE_PASV,
  STATE_PORT,
  STATE_CWD,
  STATE_MKD,
  STATE_PRET_RETR,
  STATE_PRET_STOR,
  STATE_RETR,
  STATE_RETR_COMPLETE,
  STATE_STOR,
  STATE_STOR_COMPLETE,
  STATE_ABOR,
  STATE_QUIT,
  STATE_USER_LOGINKILL,
  STATE_PASS_LOGINKILL,
  STATE_WIPE,
  STATE_DELE,
  STATE_RMD,
  STATE_NUKE,
  STATE_LIST,
  STATE_PRET_LIST,
  STATE_LIST_COMPLETE,
  STATE_SSCN_ON,
  STATE_SSCN_OFF,
  STATE_SSL_HANDSHAKE,
  STATE_PASV_ABORT,
  STATE_PBSZ,
  STATE_TYPEI,
  STATE_XDUPE,
  STATE_IDLE
};

enum ProtMode {
  PROT_UNSET,
  PROT_C,
  PROT_P
};

enum FailureType {
  FAIL_UNDEFINED,
  FAIL_DUPE
};

class FTPConnect;
class SiteRace;
class FileList;
class SiteLogic;
class IOManager;
class RawBuffer;
class Site;
class ProxySession;
class CommandOwner;
class Proxy;
class RawBufferCallback;

#define RAWBUFMAXLEN 1024
#define DATABUF 2048

class FTPConn : private EventReceiver, public FTPConnectOwner {
  private:
    std::list<std::shared_ptr<FTPConnect> > connectors;
    int nextconnectorid;
    IOManager * iom;
    ProxySession * proxysession;
    unsigned int databuflen;
    char * databuf;
    unsigned int databufpos;
    int databufcode;
    int id;
    bool processing;
    bool allconnectattempted;
    SiteLogic * sl;
    std::string status;
    std::shared_ptr<Site> site;
    int transferstatus;
    int sockid;
    FTPConnState state;
    bool aborted;
    FileList * currentfl;
    std::shared_ptr<CommandOwner> currentco;
    Path currentpath;
    ProtMode protectedmode;
    bool sscnmode;
    Path targetpath;
    bool mkdtarget;
    Path mkdsect;
    Path mkdpath;
    std::list<std::string> mkdsubdirs;
    RawBuffer * rawbuf;
    RawBuffer * aggregatedrawbuf;
    RawBuffer * cwdrawbuf;
    int ticker;
    std::list<std::string> xdupelist;
    bool xduperun;
    bool typeirun;
    bool cleanlyclosed;
    void AUTHTLSResponse();
    void USERResponse();
    void PASSResponse();
    void STATResponse();
    void PWDResponse();
    void PROTPResponse();
    void PROTCResponse();
    void RawResponse();
    void CPSVResponse();
    void PASVResponse();
    void PORTResponse();
    void CWDResponse();
    void MKDResponse();
    void PRETRETRResponse();
    void PRETSTORResponse();
    void PRETLISTResponse();
    void RETRResponse();
    void RETRComplete();
    void STORResponse();
    void STORComplete();
    void ABORResponse();
    void QUITResponse();
    void WIPEResponse();
    void DELEResponse();
    void RMDResponse();
    void NUKEResponse();
    void LISTResponse();
    void LISTComplete();
    void SSCNONResponse();
    void SSCNOFFResponse();
    void PASVAbortResponse();
    void PBSZ0Response();
    void TYPEIResponse();
    void XDUPEResponse();
    void proxySessionInit(bool);
    void sendEcho(const std::string &);
    void connectAllAddresses();
    Proxy * getProxy() const;
    void clearConnectors();
    void rawBufWrite(const std::string &);
    void rawBufWriteLine(const std::string &);
    void doCWD(const Path &, FileList *, const std::shared_ptr<CommandOwner> & co);
    void doMKD(const Path &, FileList *, const std::shared_ptr<CommandOwner> & co);
    void parseXDUPEData();
    void finishLogin();
  public:
    int getId() const;
    void setId(int);
    std::string getStatus() const;
    void login();
    void reconnect();
    bool isProcessing() const;
    FTPConn(SiteLogic *, int);
    ~FTPConn();
    int updateFileList(FileList *);
    void updateName();
    const Path & getCurrentPath() const;
    void doUSER(bool);
    void doAUTHTLS();
    void doPWD();
    void doPROTP();
    void doPROTC();
    void doRaw(const std::string &);
    void doWipe(const Path &, bool);
    void doNuke(const Path &, int, const std::string &);
    void doDELE(const Path &);
    void doRMD(const Path &);
    void doSTAT();
    void doSTAT(const std::shared_ptr<CommandOwner> & co, FileList *);
    void doLIST();
    void doLISTa();
    FileList * newFileList();
    void setListData(const std::shared_ptr<CommandOwner> & co, FileList *);
    void doSTATla();
    void doSSCN(bool);
    void doCPSV();
    void doPASV();
    void doPORT(const std::string &, int);
    void doCWD(const Path &, const std::shared_ptr<CommandOwner> & co = std::shared_ptr<CommandOwner>());
    void doCWD(FileList *, const std::shared_ptr<CommandOwner> & co = std::shared_ptr<CommandOwner>());
    void doMKD(const Path &, const std::shared_ptr<CommandOwner> & co = std::shared_ptr<CommandOwner>());
    void doMKD(FileList *, const std::shared_ptr<CommandOwner> & co);
    void doPRETRETR(const std::string &);
    void doRETR(const std::string &);
    void doPRETSTOR(const std::string &);
    void doSTOR(const std::string &);
    void doPRETLIST();
    void abortTransfer();
    void abortTransferPASV();
    void doPBSZ0();
    void doTYPEI();
    void doXDUPE();
    void doQUIT();
    void disconnect();
    FTPConnState getState() const;
    std::string getConnectedAddress() const;
    std::string getInterfaceAddress() const;
    ProtMode getProtectedMode() const;
    bool getSSCNMode() const;
    void setMKDCWDTarget(const Path &, const Path &);
    bool hasMKDCWDTarget() const;
    const Path & getTargetPath() const;
    const Path & getMKDCWDTargetSection() const;
    const Path & getMKDCWDTargetPath() const;
    void finishMKDCWDTarget();
    const std::list<std::string> & getMKDSubdirs();
    RawBuffer * getRawBuffer() const;
    RawBuffer * getCwdRawBuffer() const;
    int getSockId() const;
    bool isCleanlyClosed() const;
    static bool parseData(char *, unsigned int, char **, unsigned int &, unsigned int &, int &);
    void FDData(int, char *, unsigned int);
    void FDDisconnected(int);
    void FDSSLSuccess(int, const std::string &);
    void FDSSLFail(int);
    void printCipher(const std::string &);
    void ftpConnectInfo(int, const std::string &);
    void ftpConnectSuccess(int);
    void ftpConnectFail(int);
    void tick(int);
    FileList * currentFileList() const;
    const std::shared_ptr<CommandOwner> & currentCommandOwner() const;
    void resetCurrentCommandOwner();
    void parseFileList(char *, unsigned int);
    bool isConnected() const;
    void setRawBufferCallback(RawBufferCallback *);
    void unsetRawBufferCallback();
    const std::list<std::string> & getXDUPEList() const;
};

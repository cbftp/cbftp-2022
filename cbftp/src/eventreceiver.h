#pragma once

#include <string>

class EventReceiver {
public:
  virtual ~EventReceiver();
  virtual void tick(int);
  virtual void signal(int);
  virtual void FDNew(int);
  virtual void FDConnected(int);
  virtual void FDData(int);
  virtual void FDData(int, char *, unsigned int);
  virtual void FDDisconnected(int);
  virtual void FDFail(int, std::string);
  virtual void FDSSLSuccess(int);
  virtual void FDSSLFail(int);
  virtual void FDSendComplete(int);
};

#pragma once

#include "core/eventreceiver.h"

#define COMMANDOWNER_SITERACE 543
#define COMMANDOWNER_TRANSFERJOB 544

class FileList;

class CommandOwner : public EventReceiver {
public:
  virtual ~CommandOwner() {
  }
  virtual int classType() const = 0;
  virtual void fileListUpdated(FileList *) = 0;
};

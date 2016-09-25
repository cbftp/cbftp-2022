#pragma once

#include <string>
#include <list>

#define TRANSFERSTATUS_TYPE_FXP 1892
#define TRANSFERSTATUS_TYPE_DOWNLOAD 1893
#define TRANSFERSTATUS_TYPE_UPLOAD 1894

#define TRANSFERSTATUS_STATE_IN_PROGRESS 1901
#define TRANSFERSTATUS_STATE_SUCCESSFUL 1902
#define TRANSFERSTATUS_STATE_FAILED 1903
#define TRANSFERSTATUS_STATE_DUPE 1904

class TransferStatusCallback;
class FileList;

class TransferStatus {
public:
  TransferStatus(int, std::string, std::string, std::string, std::string, FileList *, std::string, FileList *, std::string, unsigned long long int, unsigned int, int, int, bool, bool);
  std::string getSource() const;
  std::string getTarget() const;
  std::string getRelease() const;
  std::string getFile() const;
  std::string getSourcePath() const;
  std::string getTargetPath() const;
  FileList * getSourceFileList() const;
  FileList * getTargetFileList() const;
  unsigned long long int sourceSize() const;
  unsigned long long int targetSize() const;
  unsigned long long int knownTargetSize() const;
  unsigned int getSpeed() const;
  unsigned int getTimeSpent() const;
  unsigned int getTimeRemaining() const;
  unsigned int getProgress() const;
  std::string getTimestamp() const;
  TransferStatusCallback * getCallback() const;
  int getState() const;
  int getType() const;
  bool isAwaited() const;
  int getSourceSlot() const;
  int getTargetSlot() const;
  bool getSSL() const;
  bool getDefaultActive() const;
  std::string getPassiveAddress() const;
  std::string getCipher() const;
  const std::list<std::string> & getLogLines() const;
  void setFinished();
  void setFailed();
  void setDupe();
  void setAwaited(bool);
  void setCallback(TransferStatusCallback *);
  void setTargetSize(unsigned long long int);
  void interpolateAddSize(unsigned long long int);
  void setSpeed(unsigned int);
  void setTimeSpent(unsigned int);
  void setPassiveAddress(const std::string &);
  void setCipher(const std::string &);
  void addLogLine(const std::string &);
private:
  void updateProgress();
  int type;
  std::string source;
  std::string target;
  std::string release;
  std::string file;
  std::string timestamp;
  std::string sourcepath;
  std::string targetpath;
  unsigned long long int sourcesize;
  unsigned long long int knowntargetsize;
  unsigned long long int interpolatedtargetsize;
  unsigned long long int interpolationfilltargetsize;
  unsigned int speed;
  int state;
  unsigned int timespent;
  unsigned int timeremaining;
  unsigned int progress;
  bool awaited;
  TransferStatusCallback * callback;
  FileList * fls;
  FileList * fld;
  int srcslot;
  int dstslot;
  bool ssl;
  bool defaultactive;
  std::string passiveaddr;
  std::string cipher;
  std::list<std::string> loglines;
};

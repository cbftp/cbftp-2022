#include "rawbuffer.h"

#include "uibase.h"
#include "globalcontext.h"
#include "util.h"
#include "rawbuffercallback.h"

RawBuffer::RawBuffer(unsigned int maxlength, std::string site, std::string id) :
  latestp(0),
  latestpcopy(0),
  maxlength(maxlength),
  bookmarklines(0),
  site(site),
  id(id),
  inprogress(false),
  uiwatching(false),
  threads(true),
  eventlog(false),
  callback(NULL)
{
  writeLine("Log window initialized. Site: " + site + " Thread id: " + id);
}

RawBuffer::RawBuffer(std::string site) :
  latestp(0),
  latestpcopy(0),
  maxlength(1024),
  bookmarklines(0),
  site(site),
  inprogress(false),
  uiwatching(false),
  threads(false),
  eventlog(false),
  callback(NULL)
{
  writeLine("Raw command window initialized. Site: " + site);
}

RawBuffer::RawBuffer() :
  latestp(0),
  latestpcopy(0),
  maxlength(1024),
  bookmarklines(0),
  inprogress(false),
  uiwatching(false),
  threads(false),
  eventlog(true),
  callback(NULL)
{
  writeLine("Event log initialized.");
}

void RawBuffer::setCallback(RawBufferCallback * callback) {
  this->callback = callback;
}

void RawBuffer::unsetCallback() {
  callback = NULL;
}

void RawBuffer::bookmark() {
  bookmarklines = 0;
}

unsigned int RawBuffer::linesSinceBookmark() const {
  return bookmarklines;
}

void RawBuffer::lineFinished() {
  inprogress = false;
  if (callback != NULL) {
    callback->newRawBufferLine(getLine(0));
  }
}
void RawBuffer::write(const std::string & s) {
  write(getTag(), s);
}

void RawBuffer::write(const std::string & tag, const std::string & s) {
  size_t split = s.find("\r\n");
  if (!split) {
    if (s.length() > 2) write(tag, s.substr(2));
  }
  else if (split != std::string::npos) {
    write(tag, s.substr(0, split));
    lineFinished();
    if (s.length() > split + 2) write(tag, s.substr(split + 2));
  }
  else {
    split = s.find("\n");
    if (!split) {
      if (s.length() > 1) write(tag, s.substr(1));
    }
    else if (split != std::string::npos) {
      write(tag, s.substr(0, split));
      lineFinished();
      if (s.length() > split + 1) write(tag, s.substr(split + 1));
    }
    else {
      if (inprogress) {
        log[(latestp > 0 ? latestp : maxlength) - 1].second.append(s);
      }
      else {
        if (log.size() < maxlength) {
          log.push_back(std::pair<std::string, std::string>(tag, s));
        }
        else {
          log[latestp] = std::pair<std::string, std::string>(tag, s);
        }
        if (++latestp == maxlength) {
          latestp = 0;
        }
        ++bookmarklines;
        inprogress = true;
      }
    }
  }
  if (uiwatching) {
    global->getUIBase()->backendPush();
  }
}

void RawBuffer::writeLine(const std::string & s) {
  writeLine(getTag(), s);
}

void RawBuffer::writeLine(const std::string & tag, const std::string & s) {
  write(tag, s + "\n");
}

void RawBuffer::rename(std::string name) {
  writeLine("Changing site name to: " + name);
  site = name;
}

std::string RawBuffer::getTag() const {
  return "[" + util::ctimeLog() + (eventlog ? "" : " " + site + (threads ? " " + id : "")) + "]";
}

void RawBuffer::setId(int id) {
  this->id = util::int2Str(id);
}

const std::pair<std::string, std::string> & RawBuffer::getLineCopy(unsigned int num) const {
  unsigned int size = getCopySize();
  util::assert(num < size);
  int pos = (num < latestpcopy ? latestpcopy - num - 1 : size + latestpcopy - num - 1);
  return logcopy[pos];
}

const std::pair<std::string, std::string> & RawBuffer::getLine(unsigned int num) const {
  unsigned int size = getSize();
  util::assert(num < size);
  int pos = (num < latestp ? latestp - num - 1 : size + latestp - num - 1);
  return log[pos];
}

unsigned int RawBuffer::getSize() const {
  return log.size();
}

unsigned int RawBuffer::getCopySize() const {
  return logcopy.size();
}

void RawBuffer::freezeCopy() {
  logcopy = log;
  latestpcopy = latestp;
}

void RawBuffer::setUiWatching(bool watching) {
  uiwatching = watching;
}

void RawBuffer::clear() {
  log.clear();
  logcopy.clear();
  latestp = 0;
  latestpcopy = 0;
  inprogress = false;
}

#pragma once

#include <string>
#include <vector>
#include <utility>

class RawBuffer {
  private:
    std::vector<std::pair<std::string, std::string> > log;
    std::vector<std::pair<std::string, std::string> > logcopy;
    unsigned int latestp;
    unsigned int latestpcopy;
    unsigned int maxlength;
    unsigned int bookmarklines;
    std::string site;
    std::string id;
    bool inprogress;
    bool uiwatching;
    bool threads;
    bool eventlog;
  public:
    RawBuffer(unsigned int, std::string, std::string);
    RawBuffer(std::string);
    RawBuffer();
    void bookmark();
    unsigned int linesSinceBookmark() const;
    void setId(int);
    void write(const std::string &);
    void write(const std::string &, const std::string &);
    void writeLine(const std::string &);
    void writeLine(const std::string &, const std::string &);
    void rename(std::string);
    std::string getTag() const;
    unsigned int getSize() const;
    unsigned int getCopySize() const;
    const std::pair<std::string, std::string> & getLine(unsigned int) const;
    const std::pair<std::string, std::string> & getLineCopy(unsigned int) const;
    void freezeCopy();
    void uiWatching(bool);
};

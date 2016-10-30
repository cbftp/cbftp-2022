#include "localfilelist.h"

#include <utility>

#include "util.h"

LocalFileList::LocalFileList(const Path & path) :
  path(path), sizefiles(0) {
}

void LocalFileList::addFile(const std::string & name, unsigned long long int size, bool isdir, const std::string & user, const std::string & group, int year, int month, int day, int hour, int minute) {
  LocalFile file(name, size, isdir, user, group, year, month, day, hour, minute);
  files.insert(std::pair<std::string, LocalFile>(name, file));
  lowercasefilemap[util::toLower(name)] = name;
  if (!isdir) {
    sizefiles++;
  }
}

void LocalFileList::touchFile(const std::string & name) {
  addFile(name, 512, false, "", "", 0, 0, 0, 0, 0);
}

std::map<std::string, LocalFile>::const_iterator LocalFileList::begin() const {
  return files.begin();
}

std::map<std::string, LocalFile>::const_iterator LocalFileList::end() const {
  return files.end();
}

std::map<std::string, LocalFile>::const_iterator LocalFileList::find(const std::string & file) const {
  std::map<std::string, LocalFile>::const_iterator it = files.find(file);
  if (it == files.end()) {
    std::map<std::string, std::string>::const_iterator it2 = lowercasefilemap.find(util::toLower(file));
    if (it2 != lowercasefilemap.end()) {
      it = files.find(it2->second);
    }
  }
  return it;
}

const Path & LocalFileList::getPath() const {
  return path;
}

unsigned int LocalFileList::size() const {
  return files.size();
}

unsigned int LocalFileList::sizeFiles() const {
  return sizefiles;
}

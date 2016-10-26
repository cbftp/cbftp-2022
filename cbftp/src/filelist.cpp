#include "filelist.h"

#include <list>

#include "file.h"
#include "site.h"
#include "globalcontext.h"
#include "timereference.h"
#include "util.h"

FileList::FileList(const std::string & username, const std::string & path) {
  init(username, path, FILELIST_UNKNOWN);
}

FileList::FileList(const std::string & username, const std::string & path, FileListState state) {
  init(username, path, state);
}

FileList::~FileList() {
  for (std::map<std::string, File *>::iterator it = files.begin(); it != files.end(); it++) {
    delete it->second;
  }
}

void FileList::init(const std::string & username, const std::string & path, FileListState state) {
  this->username = username;
  this->path = util::cleanPath(path);
  this->state = state;
  owned = 0;
  ownpercentage = 0;
  maxfilesize = 0;
  uploadedfiles = 0;
  locked = false;
  listchanged = false;
  lastchangedstamp = 0;
  totalfilesize = 0;
  uploading = 0;
}

bool FileList::updateFile(const std::string & start, int touch) {
  File * file = new File(start, touch);
  std::string name = file->getName();
  if (name == "." || name == "..") {
    delete file;
    return false;
  }
  File * updatefile;
  if ((updatefile = getFile(name)) != NULL) {
    if (updatefile->getSize() == 0 && file->getSize() > 0 && !file->isDirectory()) uploadedfiles++;
    unsigned long long int oldsize = updatefile->getSize();
    unsigned long long int newsize = file->getSize();
    if (updatefile->setSize(newsize)) {
      if (!updatefile->isDirectory()) {
        totalfilesize += newsize - oldsize;
      }
      setChanged();
    }
    if (newsize > maxfilesize) maxfilesize = newsize;
    if (updatefile->getOwner().compare(file->getOwner()) != 0) {
      if (file->getOwner().compare(username) == 0) {
        editOwnedFileCount(true);
      }
      else if (updatefile->getOwner().compare(username) == 0) {
        editOwnedFileCount(false);
      }
    }
    if (updatefile->setOwner(file->getOwner())) {
      setChanged();
    }
    if (updatefile->setGroup(file->getGroup())) {
      setChanged();
    }
    if (updatefile->setLastModified(file->getLastModified())) {
      setChanged();
    }
    updatefile->setTouch(file->getTouch());
    if (updatefile->updateFlagSet()) {
      if (updatefile->getOwner().compare(username) == 0) {
        updatefile->getUpdateSrc()->pushTransferSpeed(updatefile->getUpdateDst(),
                                                      updatefile->getUpdateSpeed(),
                                                      updatefile->getSize());
        //std::cout << "pushed new transfer speed..." << std::endl;
      }
      updatefile->unsetUpdateFlag();
    }
    delete file;
    return true;
  }
  else {
    files[name] = file;
    lowercasefilemap[util::toLower(name)] = name;
    unsigned long long int filesize = file->getSize();
    if (filesize > 0 && !file->isDirectory()) {
      uploadedfiles++;
      totalfilesize += filesize;
    }
    if (filesize > maxfilesize) maxfilesize = filesize;
    if (file->getOwner().compare(username) == 0) {
      editOwnedFileCount(true);
    }
    setChanged();
  }
  return true;
}

void FileList::touchFile(const std::string & name, const std::string & user) {
  touchFile(name, user, false);
}

void FileList::touchFile(const std::string & name, const std::string & user, bool upload) {
  File * file;
  if ((file = getFile(name)) != NULL) {
    file->unsetUpdateFlag();
  }
  else {
    file = new File(name, user);
    files[name] = file;
    lowercasefilemap[util::toLower(name)] = name;
    if (user.compare(username) == 0) {
      editOwnedFileCount(true);
    }
    setChanged();
  }
  if (upload && !file->isUploading()) {
    file->upload();
    uploading++;
  }
}

void FileList::removeFile(const std::string & name) {
  File * f;
  if ((f = getFile(name)) != NULL) {
    if (f->getOwner().compare(username) == 0) {
      editOwnedFileCount(false);
    }
    if (f->getSize() > 0 && !f->isDirectory()) {
      uploadedfiles--;
      totalfilesize -= f->getSize();
    }
    if (f->isUploading()) {
      uploading--;
    }
    delete f;
    files.erase(name);
    setChanged();
  }
}

void FileList::setFileUpdateFlag(const std::string & name, unsigned long long int size, unsigned int speed, const Pointer<Site> & src, const std::string & dst) {
  File * file;
  if ((file = getFile(name)) != NULL) {
    unsigned long long int oldsize = file->getSize();
    if (file->setSize(size)) {
      if (!oldsize) {
        uploadedfiles++;
      }
      totalfilesize += size - oldsize;
      setChanged();
    }
    file->setUpdateFlag(src, dst, speed);
  }
}

File * FileList::getFile(const std::string & name) const {
  std::map<std::string, File *>::const_iterator it = files.find(name);
  if (it == files.end()) {
    std::map<std::string, std::string>::const_iterator it2 = lowercasefilemap.find(util::toLower(name));
    if (it2 != lowercasefilemap.end()) {
      it = files.find(it2->second);
    }
    if (it == files.end()) {
      return NULL;
    }
  }
  return it->second;
}

FileListState FileList::getState() const {
  return state;
}

void FileList::setNonExistent() {
  state = FILELIST_NONEXISTENT;
}

void FileList::setExists() {
  state = FILELIST_EXISTS;
}

void FileList::setFilled() {
  state = FILELIST_LISTED;
}

std::map<std::string, File *>::iterator FileList::begin() {
  return files.begin();
}

std::map<std::string, File *>::iterator FileList::end() {
  return files.end();
}

std::map<std::string, File *>::const_iterator FileList::begin() const {
  return files.begin();
}

std::map<std::string, File *>::const_iterator FileList::end() const {
  return files.end();
}

bool FileList::contains(const std::string & name) const {
  if (files.find(name) != files.end()) {
    return true;
  }
  std::map<std::string, std::string>::const_iterator it = lowercasefilemap.find(util::toLower(name));
  if (it != lowercasefilemap.end() && files.find(it->second) != files.end()) {
    return true;
  }
  return false;
}

unsigned int FileList::getSize() const {
  return files.size();
}

unsigned long long int FileList::getTotalFileSize() const {
  return totalfilesize;
}

unsigned int FileList::getNumUploadedFiles() const {
  return uploadedfiles;
}

bool FileList::hasSFV() const {
  std::map<std::string, File *>::const_iterator it;
  for (it = files.begin(); it != files.end(); it++) {
    if(it->second->getExtension() == ("sfv") &&
        it->second->getSize() > 0) {
      return true;
    }
  }
  return false;
}

int FileList::getOwnedPercentage() const {
  return ownpercentage;
}

unsigned long long int FileList::getMaxFileSize() const {
  return maxfilesize;
}

std::string FileList::getPath() const {
  return path;
}

void FileList::cleanSweep(int touch) {
  std::list<std::string> eraselist;
  std::map<std::string, File *>::iterator it;
  for (it = files.begin(); it != files.end(); it++) {
    File * f = it->second;
    if (f->getTouch() != touch && !f->isUploading()) {
      eraselist.push_back(it->first);
    }
  }
  if (eraselist.size() > 0) {
    for (std::list<std::string>::iterator it2 = eraselist.begin(); it2 != eraselist.end(); it2++) {
      removeFile(*it2);
      files.erase(*it2);
      lowercasefilemap.erase(util::toLower(*it2));
    }
    maxfilesize = 0;
    for (it = files.begin(); it != files.end(); it++) {
      if (it->second->getSize() > maxfilesize) maxfilesize = it->second->getSize();
    }
    setChanged();
  }
}

void FileList::flush() {
  std::map<std::string, File *>::iterator it;
  for (it = files.begin(); it != files.end(); it++) {
    delete (*it).second;
  }
  files.clear();
  lowercasefilemap.clear();
  maxfilesize = 0;
  totalfilesize = 0;
  uploadedfiles = 0;  
  owned = 0;
  ownpercentage = 0;
  uploading = 0;
}

void FileList::editOwnedFileCount(bool add) {
 if (add) ++owned;
 else --owned;
 ownpercentage = (owned * 100) / files.size();
}

bool FileList::listChanged() const {
  return listchanged;
}

void FileList::resetListChanged() {
  listchanged = false;
}

void FileList::setChanged() {
  listchanged = true;
  lastchangedstamp = global->getTimeReference()->timeReference();
}

unsigned long long FileList::timeSinceLastChanged() {
  return global->getTimeReference()->timePassedSince(lastchangedstamp);
}

std::string FileList::getUser() const {
  return username;
}

void FileList::finishUpload(const std::string & file) {
  File * fileobj = getFile(file);
  if (fileobj != NULL && fileobj->isUploading()) {
    fileobj->finishUpload();
    uploading--;
    setChanged();
  }
}

void FileList::finishDownload(const std::string & file) {
  File * fileobj = getFile(file);
  if (fileobj != NULL && fileobj->isDownloading()) {
    fileobj->finishDownload();
  }
}

void FileList::download(const std::string & file) {
  File * fileobj = getFile(file);
  if (fileobj != NULL && !fileobj->isDownloading()) {
    fileobj->download();
  }
}

bool FileList::hasFilesUploading() const {
  return uploading > 0;
}

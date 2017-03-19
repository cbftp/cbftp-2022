#pragma once

#include <map>
#include <string>

#include "core/pointer.h"
#include "path.h"

enum FileListState {
 FILELIST_UNKNOWN,
 FILELIST_NONEXISTENT,
 FILELIST_EXISTS,
 FILELIST_LISTED,
 FILELIST_FAILED
};

class File;
class Site;

class FileList {
  private:
    FileList(const FileList &);
    std::map<std::string, File *> files;
    std::map<std::string, std::string> lowercasefilemap;
    std::string username;
    Path path;
    FileListState state;
    bool locked;
    bool listchanged;
    unsigned long long lastchangedstamp;
    int owned;
    int ownpercentage;
    int uploading;
    unsigned long long int maxfilesize;
    unsigned long long int totalfilesize;
    unsigned int uploadedfiles;
    int refreshedtime;
    void editOwnedFileCount(bool);
    void init(const std::string &, const Path &, FileListState);
  public:
    FileList(const std::string &, const Path &);
    FileList(const std::string &, const Path &, FileListState);
    ~FileList();
    bool updateFile(const std::string &, int);
    void touchFile(const std::string &, const std::string &);
    void touchFile(const std::string &, const std::string &, bool);
    void removeFile(const std::string &);
    void setFileUpdateFlag(const std::string &, unsigned long long int, unsigned int, const Pointer<Site> &, const std::string &);
    File * getFile(const std::string &) const;
    FileListState getState() const;
    void setNonExistent();
    void setExists();
    void setFilled();
    void setFailed();
    std::map<std::string, File *>::iterator begin();
    std::map<std::string, File *>::iterator end();
    std::map<std::string, File *>::const_iterator begin() const;
    std::map<std::string, File *>::const_iterator end() const;
    bool contains(const std::string &) const;
    unsigned int getSize() const;
    unsigned long long int getTotalFileSize() const;
    unsigned int getNumUploadedFiles() const;
    const Path & getPath() const;
    bool hasSFV() const;
    int getOwnedPercentage() const;
    unsigned long long int getMaxFileSize() const;
    void cleanSweep(int);
    void flush();
    bool listChanged() const;
    void resetListChanged();
    unsigned long long timeSinceLastChanged();
    std::string getUser() const;
    void finishUpload(const std::string &);
    void finishDownload(const std::string &);
    void download(const std::string &);
    bool hasFilesUploading() const;
    void setChanged();
    void setRefreshedTime(int);
    int getRefreshedTime();
};

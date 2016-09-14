#pragma once

#include <string>
#include <vector>
#include <list>

#include "uifile.h"

#include "../core/pointer.h"

class FileList;
class LocalFileList;

class UIFileList {
private:
  std::vector<UIFile> files;
  std::vector<UIFile *> sortedfiles;
  std::list<unsigned int> selectedfiles;
  unsigned int currentposition;
  UIFile * currentcursored;
  std::string path;
  unsigned int numfiles;
  unsigned int numdirs;
  unsigned long long int totalsize;
  unsigned int filterednumfiles;
  unsigned int filterednumdirs;
  unsigned long long int filteredtotalsize;
  std::string sortmethod;
  bool separators;
  std::list<std::string> filters;
  void setNewCurrentPosition();
  void removeSeparators();
  void fillSortedFiles();
public:
  UIFileList();
  void sortCombined();
  void sortName(bool);
  void sortTime(bool);
  void sortSize(bool);
  void sortOwner(bool);
  void parse(FileList *);
  void parse(Pointer<LocalFileList> &);
  UIFile * cursoredFile() const;
  void selectFileName(std::string);
  bool goNext();
  bool goPrevious();
  void toggleSelectCurrent();
  void unSelect();
  unsigned int size() const;
  unsigned int sizeFiles() const;
  unsigned int sizeDirs() const;
  unsigned long long int getTotalSize() const;
  unsigned int filteredSizeFiles() const;
  unsigned int filteredSizeDirs() const;
  unsigned long long int getFilteredTotalSize() const;
  const std::vector <UIFile *> * getSortedList() const;
  unsigned int currentCursorPosition() const;
  std::string getPath() const;
  std::string getSortMethod() const;
  bool separatorsEnabled() const;
  void removeFile(std::string);
  void toggleSeparators();
  void setCursorPosition(unsigned int);
  bool hasFilters() const;
  std::list<std::string> getFilters() const;
  void setFilters(const std::list<std::string> &);
  void unsetFilters();
};

bool combinedSort(UIFile *, UIFile *);
bool nameSortAsc(UIFile *, UIFile *);
bool nameSortDesc(UIFile *, UIFile *);
bool timeSortAsc(UIFile *, UIFile *);
bool timeSortDesc(UIFile *, UIFile *);
bool sizeSortAsc(UIFile *, UIFile *);
bool sizeSortDesc(UIFile *, UIFile *);
bool ownerSortAsc(UIFile *, UIFile *);
bool ownerSortDesc(UIFile *, UIFile *);

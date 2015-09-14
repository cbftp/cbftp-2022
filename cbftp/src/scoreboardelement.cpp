#include "scoreboardelement.h"

ScoreBoardElement::ScoreBoardElement(std::string filename, unsigned short score, bool prio, SiteLogic * src, FileList * fls, SiteLogic * dst, FileList * fld) {
  reset(filename, score, prio, src, fls, dst, fld);
}

void ScoreBoardElement::reset(std::string filename, unsigned short score, bool prio, SiteLogic * src, FileList * fls, SiteLogic * dst, FileList * fld) {
  this->filename = filename;
  this->src = src;
  this->fls = fls;
  this->dst = dst;
  this->fld = fld;
  this->score = score;
  this->prio = prio;
}

std::string ScoreBoardElement::fileName() const {
  return filename;
}

SiteLogic * ScoreBoardElement::getSource() const {
  return src;
}

SiteLogic * ScoreBoardElement::getDestination() const {
  return dst;
}

FileList * ScoreBoardElement::getSourceFileList() const {
  return fls;
}

FileList * ScoreBoardElement::getDestinationFileList() const {
  return fld;
}

unsigned short ScoreBoardElement::getScore() const {
  return score;
}

bool ScoreBoardElement::isPrioritized() const {
  return prio;
}

std::ostream & operator<<(std::ostream & out, const ScoreBoardElement & sbe) {
  return out << sbe.fileName() << " - " << sbe.getScore();
}

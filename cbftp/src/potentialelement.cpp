#include "potentialelement.h"

#include "sitelogic.h"

PotentialElement::PotentialElement() : site(NULL), potential(0), dnslots(0) {
}

const Pointer<SiteLogic> & PotentialElement::getSite() const {
  return site;
}

int PotentialElement::getPotential() const {
  return potential;
}

void PotentialElement::update(const Pointer<SiteLogic> & site, int dnslots, int potential, const std::string & filename) {
  this->site = site;
  this->dnslots = dnslots;
  this->potential = potential;
  this->filename = filename;
}

void PotentialElement::reset() {
  site.reset();
  potential = 0;
  dnslots = 0;
  filename = "";
}

int PotentialElement::getSiteDownloadSlots() const {
  return dnslots;
}

std::string PotentialElement::getFileName() const {
  return filename;
}

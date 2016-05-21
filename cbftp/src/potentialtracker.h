#pragma once

#include <string>
#include <list>
#include <vector>

#include "core/eventreceiver.h"

#define POTENTIALITY_LIFESPAN 3000
#define POTENTIALITY_SLICES 10

class PotentialElement;
class PotentialListElement;
class SiteLogic;

class PotentialTracker : private EventReceiver {
  private:
    std::list<PotentialListElement *> potentiallist;
    std::list<PotentialElement *> top;
    void tick(int);
  public:
    PotentialTracker(int);
    ~PotentialTracker();
    int getMaxAvailablePotential();
    void pushPotential(int, const std::string &, SiteLogic *, int);
    std::list<PotentialElement *>::iterator findFirstOfSite(SiteLogic *);
    void updateSlots(int);
    bool allTopSlotsUsedForSite(PotentialElement *) const;
};

#include "uibase.h"

#include <iostream>
#include <unistd.h>

#include "util.h"

static UIBase * uibase = NULL;

UIBase::UIBase() {
  util::assert(uibase == NULL);
  uibase = this;
}

UIBase::~UIBase() {

}

class UIBaseDefault : public UIBase {
public:
  UIBaseDefault() {
    std::cout << "WARNING: cbftp was built without UI - falling back to UIBaseDefault." << std::endl;
  }
  bool init() {
    std::cout << "UIBase::init() called." << std::endl;
    return true;
  }
  void backendPush() {
    std::cout << "UIBase::backendPush() called." << std::endl;
  }
  void kill() {
    std::cout << "UIBase::kill() called." << std::endl;
  }
};

UIBase * UIBase::instance() {
  if (uibase == NULL) {
    new UIBaseDefault();
  }
  return uibase;
}

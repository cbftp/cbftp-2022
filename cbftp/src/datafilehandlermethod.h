#pragma once

#include "core/types.h"
#include "crypto.h"

namespace {

namespace DataFileHandlerMethod {

bool encrypt(const Core::BinaryData& indata, const Core::BinaryData& pass, Core::BinaryData& outdata) {
  if (!Crypto::isMostlyASCII(indata)) {
    return false;
  }
  Crypto::encrypt(indata, pass, outdata);
  return true;
}

bool decrypt(const Core::BinaryData& indata, const Core::BinaryData& pass, Core::BinaryData& outdata) {
  Crypto::decrypt(indata, pass, outdata);
  return Crypto::isMostlyASCII(outdata);
}

Path getDataFile() {
  std::string datafilestr = DATA_FILE;
  char * overridedf = getenv("CBFTP_DATA_PATH");
  if (overridedf != nullptr) {
    datafilestr = overridedf;
  }
  size_t homepos = datafilestr.find('~');
  if (homepos != std::string::npos) {
    datafilestr = datafilestr.substr(0, homepos) + getenv("HOME") + datafilestr.substr(homepos + 1);
  }
  Path datafile(datafilestr);
  if (datafile.isRelative()) {
    char buf[512];
    char* cwd = getcwd(buf, sizeof(buf));
    if (cwd == nullptr) {
      perror("Error: getcwd() failed.");
      exit(1);
    }
    return Path(std::string(cwd)) / datafile;
  }
  return datafile;
}

}

}

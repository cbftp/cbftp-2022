#pragma once

#include <string>
#include <list>
#include <openssl/ssl.h>

#include "datablock.h"

enum SocketType {
  FD_UNUSED,
  FD_KEYBOARD,
  FD_TCP_PLAIN,
  FD_TCP_SSL,
  FD_UDP,
  FD_TCP_SERVER,
  FD_TCP_PLAIN_LISTEN,
  FD_TCP_SSL_NEG_CONNECT,
  FD_TCP_SSL_NEG_REDO_CONNECT,
  FD_TCP_SSL_NEG_ACCEPT,
  FD_TCP_SSL_NEG_REDO_ACCEPT,
  FD_TCP_SSL_NEG_REDO_HANDSHAKE,
  FD_TCP_CONNECTING,
  FD_TCP_RESOLVING
};

class EventReceiver;

class SocketInfo {
public:
  SocketInfo() : type(FD_UNUSED), fd(0), id(0), receiver(NULL), ssl(NULL)
#ifdef __linux
  , anlargs(NULL)
#endif
  {
  }
  SocketType type;
  int fd;
  int id;
  std::string addr;
  EventReceiver * receiver;
  mutable std::list<DataBlock> sendqueue;
  SSL * ssl;
#ifdef __linux
  struct gaicb * anlargs;
#endif
};

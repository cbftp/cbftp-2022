#include "proxy.h"

Proxy::Proxy() {

}

Proxy::Proxy(const std::string& name) : name(name), addr("127.0.0.1"),
    port("8080"), authmethod(PROXY_AUTH_NONE), resolvehosts(true)
{

}

std::string Proxy::getName() const {
  return name;
}

std::string Proxy::getAddr() const {
  return addr;
}

std::string Proxy::getPort() const {
  return port;
}

int Proxy::getAuthMethod() const {
  return authmethod;
}

std::string Proxy::getAuthMethodText() const {
  switch (authmethod) {
    case PROXY_AUTH_NONE:
      return "None";
    case PROXY_AUTH_USERPASS:
      return "User/Pass";
  }
  return "Unknown";
}

std::string Proxy::getUser() const {
  return user;
}

std::string Proxy::getPass() const {
  return pass;
}

bool Proxy::getResolveHosts() const {
  return resolvehosts;
}

void Proxy::setName(const std::string& name) {
  this->name = name;
}

void Proxy::setAddr(const std::string& addr) {
  this->addr = addr;
}

void Proxy::setPort(const std::string& port) {
  this->port = port;
}

void Proxy::setAuthMethod(int authmethod) {
  this->authmethod = authmethod;
}

void Proxy::setUser(const std::string& user) {
  this->user = user;
}

void Proxy::setPass(const std::string& pass) {
  this->pass = pass;
}

void Proxy::setResolveHosts(bool resolve) {
  resolvehosts = resolve;
}

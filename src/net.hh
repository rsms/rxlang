#pragma once
#include "error.hh"
#include "util.hh"
#include "async.hh"
namespace rx {
namespace net {

using std::string;

string to_string(const sockaddr*);
  // Printable rep of a socket address. If the port is zero, not port info is added to the address.
  // Examples: "[2607:f8b0:4010:801::1005]:80", "192.30.252.153:443", "2607:f8b0:4010:801::1005"

struct AddrInfo;
enum Proto { TCP, UDP };

using AddrInfoCallback = func<void(Error, const AddrInfo& ai)>;
void getAddrInfo(Async&, Proto, const string& nodename, const string& service, AddrInfoCallback);
  // Resolve `nodename` to a list of addresses. If `nodename` is a DNS name, a DNS query will be
  // performed.


struct AddrInfo {
  AddrInfo(const addrinfo* ai) : ai{ai} {}
  struct iterator;
  iterator begin() const { return {ai}; }
  iterator end() const { return {nullptr}; }
  struct iterator {
    iterator(const addrinfo* ai) : ai{ai} {}
    const iterator& operator++() { ai = ai->ai_next; return *this; }
    bool operator!=(const iterator& other) { return ai != other.ai; }
    const sockaddr* operator*() const { return ai ? ai->ai_addr : nullptr; }
    const addrinfo* ai;
  };
  const addrinfo* ai;
};


}} // namespace

/* Examples
---------------------------------------------------------------------------------------------------

auto& async = Async::main();
net::getAddrInfo(async, net::TCP, "facebook.com", "80", [](Error err, const net::AddrInfo& ai) {
  if (err) {
    cerr << "resolveHostname error: " << err << endl;
  } else {
    cerr << "resolveHostname success:" << endl;
    for (auto addr : ai) {
      cerr << "  '" << net::to_string(addr) << "' " << (void*)addr << endl;
    }
  }
});

---------------------------------------------------------------------------------------------------
*/
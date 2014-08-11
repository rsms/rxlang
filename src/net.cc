#include "net.hh"

using std::cerr;
using std::endl;
#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)

namespace rx {
namespace net {


static void GetAddrInfoCB(uv_getaddrinfo_t* req, int err, struct addrinfo* res) {
  auto* cb = (AddrInfoCallback*)req->data;
  (*cb)(UVError(err), AddrInfo{res});
  delete cb;
  if (res) uv_freeaddrinfo(res);
  ::free(req);
}


void getAddrInfo(
    Async&           async,
    Proto            proto,
    const string&    hostname,
    const string&    service,
    AddrInfoCallback cb)
{
  auto* req = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));
  req->data = (void*)new AddrInfoCallback{cb};

  addrinfo hints;
  memset((void*)&hints, 0, sizeof(addrinfo));
  hints.ai_family = PF_UNSPEC; // return both v4 and v6 addresses

  switch (proto) {
    case Proto::TCP: {
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
      break;
    }
    case Proto::UDP: {
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = IPPROTO_UDP;
      break;
    }
  }

  int r = uv_getaddrinfo(
    async.uvloop(),
    req,
    GetAddrInfoCB,
    hostname.c_str(),
    service.c_str(),
    &hints);

  assert(r == 0);
}


string to_string(const sockaddr* addr) {
  std::string s;
  if (addr->sa_family == AF_INET) {
    size_t z = INET_ADDRSTRLEN+1;
    auto* a = ((sockaddr_in*)addr);
    s.resize(z);
    if (!uv_inet_ntop(AF_INET, &a->sin_addr, (char*)s.data(), z)) {
      size_t e = s.rfind('\0');
      assert(e != std::string::npos);
      s.resize(e);
      if (a->sin_port != 0) {
        s += ':';
        s += std::to_string(ntohs(a->sin_port));
      }
    } else {
      s.clear();
    }
  } else {
    size_t z = INET6_ADDRSTRLEN+1;
    auto* a = ((sockaddr_in6*)addr);
    s.resize(z);
    if (!uv_inet_ntop(AF_INET6, &a->sin6_addr, (char*)s.data(), z)) {
      size_t e = s.rfind('\0');
      assert(e != std::string::npos);
      s.resize(e);
      if (a->sin6_port != 0) {
        s = '[' + s + "]:" + std::to_string(ntohs(a->sin6_port));
      }
    } else {
      s.clear();
    }
  }
  return std::move(s);
}


}} // namespace

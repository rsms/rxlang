#pragma once
#include "error.hh"
#include "util.hh"
#include "ref.hh"

#define template _template
#include "uv/uv.h"  // Defines some things like addrinfo
#undef template

namespace rx {

struct Async {
  Async();
  ~Async();
  static Async& main();

  uv_loop_t* uvloop();
  void run();

private:
  struct Imp; Imp* self;
};

inline Error UVError(int err) {
  return (err < 0) ? Error{err, uv_strerror(err)} : Error{};
}

} // namespace

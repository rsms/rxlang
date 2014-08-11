#include "async.hh"
#include "util.hh"

using std::cerr;
using std::endl;
#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)

namespace rx {


#if 0
static uv_key_t kMainTLSKey;
struct _init {_init() {
  uv_key_create(&kMainTLSKey);
  uv_key_set(&kMainTLSKey, (void*)new Async);
}} __init;
Async& Async::main() {
  return *(Async*)uv_key_get(&kMainTLSKey);
}
#else
// This version allows the first caller to define what is the "main" thread
Async& Async::main() {
  static uv_key_t tlsKey;
  RX_ONCE({
    uv_key_create(&tlsKey);
    uv_key_set(&tlsKey, (void*)new Async);
  });
  return *(Async*)uv_key_get(&tlsKey);
}
#endif


struct Async::Imp {
  uv_loop_t uvloop;

  Imp() { uv_loop_init(&uvloop); }
};


Async::Async() : self{new Imp} {}
Async::~Async() { assert(self != nullptr); delete self; }

uv_loop_t* Async::uvloop() {
  return &self->uvloop;
}

void Async::run() {
  uv_run(&self->uvloop, UV_RUN_DEFAULT);
}


} // namespace
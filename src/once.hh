#pragma once
namespace rx {

struct OnceFlag { volatile long s = 0; };
template<class F> inline void once(OnceFlag& pred, F f) {
  if (pred.s == 0L && __sync_bool_compare_and_swap(&pred.s, 0L, 1L)) f();
}
  // Perform f exactly once with no risk of thread race conditions.
  // Example:
  //   static OnceFlag flag; once(flag, []{ cerr << "happens exactly once" << endl; });


#define RX_ONCE(block) { static OnceFlag __once; once(__once, []block); }
  // Example:
  //   RX_ONCE({ cerr << "happens exactly once" << endl; });
  //

} // namespace

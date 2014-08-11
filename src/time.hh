#pragma once
namespace rx {

struct Time {
  Time();
  Time(uint64_t nsec);
  Time(time_t);
  Time(const ::timespec&);
  Time(const ::timeval&);

  Time& operator=(const ::timespec&);
  Time& operator=(const ::timeval&);

  bool operator==(Time& rhs) const { return _usec == rhs._usec; }
  bool operator<=(Time& rhs) const { return _usec <= rhs._usec; }
  bool operator>=(Time& rhs) const { return _usec >= rhs._usec; }
  bool operator<(Time& rhs) const  { return _usec < rhs._usec; }
  bool operator>(Time& rhs) const  { return _usec > rhs._usec; }

  operator uint64_t() const { return _usec; }
  operator std::string() const;
private:
  uint64_t _usec = 0;
};

// ===============================================================================================

constexpr inline uint64_t to_usec(const ::timespec& ts) {
  return (uint64_t(ts.tv_nsec) / 1000ull) + (uint64_t(ts.tv_sec) * 1000000ull);
}

constexpr inline uint64_t to_usec(const ::timeval& tv) {
  return uint64_t(tv.tv_usec) + (uint64_t(tv.tv_sec) * 1000000ull);
}
inline Time::Time(uint64_t nsec) : _usec{nsec} {}
inline Time::Time(time_t v) : _usec{uint64_t(v) * 1000000ull} {}
inline Time::Time(const ::timespec& v) : _usec{to_usec(v)} {}
inline Time::Time(const ::timeval& v) : _usec{to_usec(v)} {}
inline Time& Time::operator=(const ::timespec& v) { _usec = to_usec(v); return *this; }
inline Time& Time::operator=(const ::timeval& v) { _usec = to_usec(v); return *this; }

inline std::ostream& operator<< (std::ostream& os, const Time& v) {
  return os << static_cast<std::string>(v);
}

// Test
// Time a{123};
// Time b{456};
// Time c{456};
// assert(b == c);
// assert(a < b);
// assert(a <= b);
// assert(b <= c);
// assert(b > a);
// assert(b >= a);
// assert(b >= c);



} // namespace

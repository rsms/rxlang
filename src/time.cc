#include "time.hh"
namespace rx {


Time::Time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  _usec = to_usec(tv);
}


Time::operator std::string() const {
  std::string s;
  s.resize(24); // YYYY-MM-DD HH:MM:SS[.mmm]
  auto seconds = _usec / 1000000ull;
  time_t t = (time_t)seconds;
  struct tm* T = localtime(&t);
  size_t z = strftime((char*)s.data(), s.capacity(), "%F %T", T);
  s.resize(z);

  auto ms = (_usec - (seconds * 1000000ull)) / 1000ull;
  if (ms) s += '.' + std::to_string(ms);

  return std::move(s);
}


} // namespace

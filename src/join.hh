#pragma once
namespace rx {

template <typename Iterator>
static std::string join(Iterator I, Iterator E, const std::string& glue) {
  std::string s;
  if (I != E) s.assign(*I++);
  while (I != E) {
    s.append(glue);
    s.append(*I++);
  }
  return s;
}

template <typename ConstIterable>
static std::string join(ConstIterable v, const std::string& glue) {
  return join(v.cbegin(), v.cend(), std::forward<decltype(glue)>(glue));
}

template <typename Iterator, typename F>
static std::string join(Iterator I, Iterator E, const std::string& glue, F toString) {
  std::string s;
  if (I != E) s.assign(toString(*I++));
  while (I != E) {
    s.append(glue);
    s.append(toString(*I++));
  }
  return s;
}

template <typename ConstIterable, typename F>
static std::string join(ConstIterable v, const std::string& glue, F toString) {
  return join(v.cbegin(), v.cend(), std::forward<decltype(glue)>(glue), std::forward<F>(toString));
}

} // namespace

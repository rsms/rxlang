#pragma once
namespace rx {

// Error type which has a very low cost when there's no error.
//
// - When representing "no error" (Error::OK()) the code generated is simply a pointer-sized int.
//
// - When representing an error, a single memory allocation is incured at construction which itself
//   represents both the error code (first byte) as well as any message (remaining bytes) terminated
//   by a NUL sentinel byte.
//

struct Error {
  using Code = uint32_t;

  static Error OK(); // == OK (no error)
  Error(Code);
  Error(int);
  Error(Code, const std::string& error_message);
  Error(int, const std::string& error_message);
  Error(const std::string& error_message);

  Error() noexcept; // == OK (no error)
  Error(std::nullptr_t) noexcept; // == OK (no error)
  ~Error();

  bool ok() const; // true if no error
  Code code() const;
  const char* message() const;
  operator bool() const;    // == !ok() -- true when representing an error

  Error(const Error& other);
  Error(Error&& other);
  Error& operator=(const Error& s);
  Error& operator=(Error&& s);


// ------------------------------------------------------------------------------------------------
private:
  void _set_state(Code code, const std::string& message);
  static const char* _copy_state(const char* other);
  // OK status has a NULL _state. Otherwise, _state is:
  //   _state[0..sizeof(Code)]  == code
  //   _state[sizeof(Code)..]   == message c-string
  const char* _state;
};

inline Error::Error() noexcept : _state{NULL} {} // == OK
inline Error::Error(std::nullptr_t _) noexcept : _state{NULL} {} // == OK
inline Error::~Error() { if (_state) delete[] _state; }
inline Error Error::OK() { return Error{}; }
inline bool Error::ok() const { return !_state; }
inline Error::operator bool() const { return _state; }
inline Error::Code Error::code() const { return _state ? *(Code*)_state : 0; }
inline const char* Error::message() const { return _state ? &_state[sizeof(Code)] : ""; }

inline std::ostream& operator<< (std::ostream& os, const Error& e) {
  if (!e) {
    return os << "OK";
  } else {
    return os << e.message() << " (#" << (int)e.code() << ')';
  }
}

inline const char* Error::_copy_state(const char* other) {
  if (other == 0) {
    return 0;
  } else {
    size_t size = strlen(&other[sizeof(Code)]) + sizeof(Code) + 1;
    char* state = new char[size];
    memcpy(state, other, size);
    return state;
  }
}

inline void Error::_set_state(Code code, const std::string& message) {
  char* state = new char[sizeof(Code) + message.size() + 1];
  *(Code*)state = code;
  // state[0] = static_cast<char>(code);
  memcpy(state + sizeof(Code), message.data(), message.size());
  state[sizeof(Code) + message.size()] = '\0';
  _state = state;
}

inline Error::Error(const Error& other) {
  _state = _copy_state(other._state);
}

inline Error::Error(Error&& other) {
  _state = nullptr;
  std::swap(other._state, _state);
}

inline Error::Error(Code code) {
  char* state = new char[sizeof(Code)+1];
  *(Code*)state = code;
  // state[0] = static_cast<char>(code);
  state[sizeof(Code)] = 0;
  _state = state;
}

inline Error::Error(int code) : Error{(Error::Code)code} {}

inline Error::Error(Code code, const std::string& msg) {
  _set_state(code, msg);
}

inline Error::Error(int code, const std::string& msg) : Error{(Error::Code)code, msg} {}

inline Error::Error(const std::string& message) {
  _set_state(0, message);
}

inline Error& Error::operator=(const Error& other) {
  if (_state != other._state) {
    delete[] _state;
    _state = _copy_state(other._state);
  }
  return *this;
}

inline Error& Error::operator=(Error&& other) {
  if (_state != other._state) {
    std::swap(other._state, _state);
  }
  return *this;
}

} // namespace

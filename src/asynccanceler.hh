#pragma once
#include "func.hh"
namespace rx {

using AsyncCanceler = func<void()>;
  // Returned from asynchronous tasks which can be canceled by calling this object.
  // A canceler can be called multiple times by multiple threads w/o any side effects.

} // namespace

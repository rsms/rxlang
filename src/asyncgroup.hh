#pragma once
#include "ref.hh"
#include "once.hh"
#include "error.hh"
#include "asynccanceler.hh"

namespace rx {
using std::string;

// ----------------------------------------------------------------------------------------------

struct _AsyncGroup;
using AsyncGroupID = size_t;

struct AsyncGroup : Ref<_AsyncGroup> {
  struct Job; // opaque type that can be assigned AsyncCanceler

  AsyncGroup(func<void(Error)> cb);
  AsyncGroup(const AsyncGroup&);
  AsyncGroup(AsyncGroup&&);

  bool cancel() const;
    // Cancel any incomplete job. Returns true if the call caused the set to be canceled.
    // After calling this with a true return value, the callback passed when constructing the
    // AsyncGroup is guaranteed *not* to be called. This is also true for any in-flight jobs:
    // their callbacks won't be called either. Thread safe.

  AsyncCanceler canceler();
    // A canceler. Essentially just a wrapper around `cancel()` with the addition of holding a
    // reference to this AsyncGroup until either disposed or called. Just like any AsyncCanceler,
    // subsequent calls have no effect.

  const Job* begin() const;
    // Start a new job

  void end(const Job*, Error err=nullptr) const;
    // End a job
};


/* Example:

AsyncCanceler DoThingAsync(Thing);

AsyncCanceler DoManyThingsAsync(Things things, func<void(Error)> cb) {
  AsyncGroup G{cb};
  for (auto& thing : things) {
    // Capture `job` in closure and assign DoThingAsync canceler to the job
    auto* job = G.begin();
    *job = DoThingAsync(thing, [=](Error err) {
      G.end(job, err);
    });
  }
  return G.canceler();
}

*/


// ===============================================================================================

struct AsyncGroup::Job {
  Job() {}
  Job(const Job&) = delete;
  Job(Job&&) = default;
  Job(size_t jobID) : _jobID{jobID} {}
  const Job& operator=(AsyncCanceler&&) const;
  const Job& operator=(const AsyncCanceler&) const;

  size_t        _jobID = SIZE_MAX;
  AsyncCanceler _canceler;
};


struct _AsyncGroup : SafeRefCounted<_AsyncGroup> {
  struct JobEQ {
    constexpr bool operator()(const AsyncGroup::Job& lhs, const AsyncGroup::Job& rhs) const {
      return lhs._jobID == rhs._jobID;
    }
  };
  struct JobHash {
    constexpr size_t operator()(const AsyncGroup::Job& v) const { return v._jobID; }
  };
  using JobSet  = std::unordered_set<AsyncGroup::Job,JobHash,JobEQ>;

  _AsyncGroup(func<void(Error)> cb) : _cb{cb} {}
  ~_AsyncGroup();
  void cancelAllJobs();

  func<void(Error)> _cb;
  JobSet            _jobs;
  size_t            _nextJobID = 0;
  OnceFlag          _endFlag;
};

inline AsyncGroup::AsyncGroup(func<void(Error)> cb) : Ref{new _AsyncGroup{cb}} {}
inline AsyncGroup::AsyncGroup(const AsyncGroup& rhs) : Ref{rhs} {}
inline AsyncGroup::AsyncGroup(AsyncGroup&& rhs) : Ref{rhs} {}


} // namespace
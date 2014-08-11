#include "asyncgroup.hh"

using std::cerr;
using std::endl;
// #define DBG(...) std::cout << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
#define DBG(...)
#define fwdarg(a) ::std::forward<decltype(a)>(a)

namespace rx {


_AsyncGroup::~_AsyncGroup() {
  once(_endFlag, [&]{
    if (!_jobs.empty()) {
      _cb({"Some jobs did not end properly."
           " Call `asyncGroup.end(job[, error])` to mark a job as completed"});
    } else {
      _cb(nullptr);
    }
  });
}


const AsyncGroup::Job* AsyncGroup::begin() const {
  auto jobID = self->_nextJobID++;
  DBG("AS@"<<(void*)self<<" begin job #"<<jobID)
  return &*self->_jobs.emplace(jobID).first;
}


const AsyncGroup::Job& AsyncGroup::Job::operator=(AsyncCanceler&& canceler) const {
  DBG("assign canceler to job #"<<_jobID)
  const_cast<Job*>(this)->_canceler = fwdarg(canceler);
  return *this;
}


const AsyncGroup::Job& AsyncGroup::Job::operator=(const AsyncCanceler& canceler) const {
  DBG("assign canceler to job #"<<_jobID)
  const_cast<Job*>(this)->_canceler = fwdarg(canceler);
  return *this;
}


void AsyncGroup::end(const Job* job, Error err) const {
  DBG("AS@"<<(void*)self<<" end job #"<<job->_jobID)
  auto& jobs = self->_jobs;
  auto I = jobs.find(*job);

  if (I == jobs.end()) {
    // Should never happen. If we have been canceled, then the job should have been canceled too.
    err = {"Trying to end job #" + std::to_string(job->_jobID) + " that has already ended"};
  } else {
    jobs.erase(I);
  }

  if (err || jobs.empty()) {
    once(self->_endFlag, [&]{
      if (err) self->cancelAllJobs();
      self->_cb(err);
    });
  }
}


void _AsyncGroup::cancelAllJobs() {
  DBG("AS@"<<(void*)this<<" canceling all jobs")
  for (auto& job : _jobs) {
    if (job._canceler) {
      DBG("AS@"<<(void*)this<<" canceling job #"<<job._jobID)
      job._canceler();
    }
  }
}


bool AsyncGroup::cancel() const {  // returns true if the call caused the set to be canceled. Thread safe.
  bool didCancel = false;
  once(self->_endFlag, [&]{
    self->cancelAllJobs();
    didCancel = true;
  });
  return didCancel;
}


AsyncCanceler AsyncGroup::canceler() {
  auto ref = *this;
  return [ref]{
    if (ref) {
      ref.cancel();
      ref.resetSelf();
    }
  };
}


} // namespace
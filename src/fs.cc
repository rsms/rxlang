#include "fs.hh"
#include "ref.hh"
#include "asyncgroup.hh"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

using std::cerr;
using std::endl;
#define DBG(...) cerr << "[" << rx::cx_basename(__FILE__) << "] " <<  __VA_ARGS__ << endl;
// #define DBG(...)

namespace rx {
namespace fs {


string pathExt(const string& fn) {
  auto i = fn.size();
  while (i && fn[--i] != '.') {}
  return i ? fn.substr(i+1) : string{};
};


string pathJoin(std::initializer_list<const string> l) {
  auto I = l.begin();
  auto E = l.end();
  bool needSlash = false;
  string s;
  for (; I != E; I++) {
    if (needSlash) {
      s.push_back('/');
      needSlash = false;
    }
    if (!I->empty()) {
      s += *I;
      needSlash = true;
    }
  }
  return std::move(s);
}


// ===============================================================================================


struct Req {
  volatile long canceled = 0;
  // virtual void finalize(uv_fs_t* r) = 0;
};


struct FSReq : Req {
  uv_fs_t       uvreq;
};


template <typename T> AsyncCanceler makeReqCanceler(T* req) {
  typename T::Ref reqR{req, /*add_ref=*/true};
  return std::move([reqR]() {
    if (reqR) {
      assert(reqR == true);
      assert(&reqR->uvreq != nullptr);
      uv_cancel((uv_req_t*)&reqR->uvreq); // ok to call multiple times, so we're thread-safe
      reqR->canceled = 1;
      reqR.resetSelf();
    }
  });
}


struct StatReq final : FSReq, SafeRefCounted<StatReq> {
  StatReq(StatCallback&& cb) : FSReq{}, cb{fwdarg(cb)} { this->uvreq.data = (void*)this; }
  // virtual void finalize(uv_fs_t* r) final {  }
  StatCallback cb;
};


struct ReadDirReq final : FSReq, SafeRefCounted<ReadDirReq> {
  ReadDirReq(ReadDirCallback&& cb) : FSReq{}, cb{fwdarg(cb)} { this->uvreq.data = (void*)this; }
  ReadDirCallback cb;
};


struct ReadLinkReq final : FSReq, SafeRefCounted<ReadLinkReq> {
  ReadLinkReq(ReadLinkCallback&& cb) : FSReq{}, cb{fwdarg(cb)} { this->uvreq.data = (void*)this; }
  ReadLinkCallback cb;
};


inline Error UVError(ssize_t err) {
  // Specialized for ssize_t
  return (err < 0) ? UVError((int)err) : Error{};
}


template <typename ReqT>
static void FinalizeAsyncReq(uv_fs_t* r) {
  auto* req = (ReqT*)r->data;
  if (r->result != UV_ECANCELED && !req->canceled) {
    req->cb(UVError(r->result), {*r});
  }
  req->releaseRef();
}


static void AsyncFSCallBack(uv_fs_t* r) {
  switch (r->fs_type) {
    case UV_FS_LSTAT:
    case UV_FS_STAT:     { FinalizeAsyncReq<StatReq>(r); break; }
    case UV_FS_READDIR:  { FinalizeAsyncReq<ReadDirReq>(r); break; }
    case UV_FS_READLINK: { FinalizeAsyncReq<ReadLinkReq>(r); break; }
    default: {
      assert(!"Unexpected FS request");
      break;
    }
  }
}


template <typename ReqT, typename CB, typename UVF, typename... UVFArgs>
inline AsyncCanceler _AsyncFSCall(
    const string& path,
    Async&     a,
    CB&&       cb,
    UVF&&      uvcall,
    UVFArgs... uvargs)
{
  // Note: We can perform the sync equivalent by passing NULL instead of AsyncFSCallBack
  auto* req = new ReqT{fwdarg(cb)};
  auto st = uvcall(a.uvloop(), &req->uvreq, path.c_str(), uvargs...);
  if (st != 0) {
    cb(UVError(st), {});
    delete req; //req->releaseRef();
    return []{};
  } else {
    return makeReqCanceler(req);
  }
}


template <typename T, typename UVF, typename... UVFArgs>
inline Error _SyncFSCall(const string& path, T& result, UVF&& uvcall, UVFArgs... uvargs) {
  uv_fs_t r;
  auto err = UVError(uvcall(Async::main().uvloop(), &r, path.c_str(), uvargs...));
  if (!err) result = T{r};
  return err;
}


template <typename ReqT, typename CB, typename UVF>
inline AsyncCanceler AsyncFSCall(const string& path, Async& a, CB&& cb, UVF&& uvcall) {
  return _AsyncFSCall<ReqT>(path, a, fwdarg(cb), fwdarg(uvcall), AsyncFSCallBack);
}

template <typename ReqT, typename CB, typename UVF, typename... UVFArgs>
inline AsyncCanceler AsyncFSCall(
    const string& path, Async& a, CB&& cb, UVF&& uvcall, UVFArgs... uvargs)
{
  return _AsyncFSCall<ReqT>(path, a, fwdarg(cb), fwdarg(uvcall), uvargs..., AsyncFSCallBack);
}


template <typename T, typename UVF>
inline Error SyncFSCall(const string& path, T& result, UVF&& uvcall) {
  return _SyncFSCall(path, result, fwdarg(uvcall), (uv_fs_cb)NULL);
}

template <typename T, typename UVF, typename... UVFArgs>
inline Error SyncFSCall(const string& path, T& result, UVF&& uvcall, UVFArgs... uvargs) {
  return _SyncFSCall(path, result, fwdarg(uvcall), uvargs..., (uv_fs_cb)NULL);
}


// ------------------------------------------------------------------------------------------------

struct CustomReq final : Req, SafeRefCounted<CustomReq> {
  using Callback = func<void()>;
  using MainFunc = func<Callback()>;
  CustomReq(MainFunc&& main) : Req{}, main{fwdarg(main)} { this->uvreq.data = (void*)this; }
  MainFunc  main;
  Callback  cb;
  uv_work_t uvreq;
};


static void UVWorkCB(uv_work_t* r) {
  // Called in some thread to perform blocking work
  auto* req = (CustomReq*)r->data;
  if (!req->canceled) {
    req->cb = req->main();
  }
}

static void UVAfterWorkCB(uv_work_t* r, int status) {
  // Called in user-designated thread when work is complete
  auto* req = (CustomReq*)r->data;
  if (!req->canceled && req->cb) {
    req->cb();
  }
  req->releaseRef();
}


AsyncCanceler AsyncWork(Async& a, Error& err, func<func<void()>()> f) {
  auto* req = new CustomReq{fwdarg(f)};
  auto st = uv_queue_work(a.uvloop(), &req->uvreq, UVWorkCB, UVAfterWorkCB);
  if (st != 0) {
    err = UVError(st);
    delete req;
    return nullptr;
  } else {
    return makeReqCanceler(req);
  }
}


FileData::~FileData() {
  if (p != nullptr) ::munmap(p, z);
  if (fd != -1) ::close(fd);
}


FileData& FileData::operator=(FileData&& other) {
  if (p != nullptr) ::munmap(p, z);
  if (fd != -1) ::close(fd);
  p  = other.p;  other.p = nullptr;
  fd = other.fd; other.fd = -1;
  std::swap(z, other.z);
  return *this;
}


static char* MMapFileReadOnly(const char* path, size_t& size, int& fd) {
  fd = ::open(path, O_RDONLY);
  if (fd < 0) return nullptr;

  if (size == 0) {
    struct stat buf;
    if (::fstat(fd, &buf) < 0) {
      ::close(fd);
      return nullptr;
    }
    size = buf.st_size;
  }

  void* ptr = mmap(0, size, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) {
    ::close(fd);
    return nullptr;
  }

  return (char*)ptr;
}


AsyncCanceler readfile(Async& a, const string& path, size_t size, ReadFileCallback&& cb) {
  Error err;
  auto ac = AsyncWork(a, err, [=]() -> CustomReq::Callback {
    int fd;
    size_t z = size;
    char* ptr = MMapFileReadOnly(path.c_str(), z, fd);
    auto errnox = errno;
    return [=]{
      if (ptr == nullptr) {
        cb(Error(strerror(errnox)), {});
      } else {
        cb(nullptr, {ptr, z, fd});
      }
    };
  });
  if (!ac) cb(err, {});
  return std::move(ac);
}


Error readfile(const string& path, size_t size, FileData& result) {
  int fd;
  char* ptr = MMapFileReadOnly(path.c_str(), size, fd);
  if (ptr == nullptr) {
    return Error(strerror(errno));
  } else {
    result.p = ptr;
    result.z = size;
    result.fd = fd;
    return nullptr;
  }
}


// ------------------------------------------------------------------------------------------------


AsyncCanceler readlink(const string& path, Async& a, ReadLinkCallback&& cb) {
  return AsyncFSCall<ReadLinkReq>(path, a, fwdarg(cb), uv_fs_readlink);
}


Error readlink(const string& path, SymLink& result) {
  return SyncFSCall(path, result, uv_fs_readlink);
}


// ------------------------------------------------------------------------------------------------


AsyncCanceler readdir(const string& path, Async& a, ReadDirCallback&& cb) {
  return AsyncFSCall<ReadDirReq>(path, a, fwdarg(cb), uv_fs_readdir, O_RDONLY);
}


Error readdir(const string& path, DirEnts& result) {
  return SyncFSCall(path, result, uv_fs_readdir, O_RDONLY);
}


size_t DirEnts::VZ() const {
  if (_vz) return _vz;
  if (_c == 0) return 0;
  ssize_t c = _c;
  const char* v = _v;
  while (c--) {
    v = (const char*)memchr((const void*)v, 0, PATH_MAX+1) + 1;
  }
  return v - _v;
}


DirEnts::iterator::iterator(ssize_t c, const char* vp) : _c{c}, _vp{vp} {
  if (_c) {
    // Advance past end of first entry
    _ve = (const char*)memchr((const void*)_vp, 0, PATH_MAX+1) + 1;
  } else {
    _vp = _ve = nullptr;
  }
}


DirEnts::iterator& DirEnts::iterator::operator++() {
  if (--_c == 0) {
    _vp = nullptr;
  } else {
    // Set entry=end_of_last_entry and advance past end of entry
    _vp = _ve;
    _ve = (const char*)memchr((const void*)_ve, 0, PATH_MAX+1) + 1;
  }
  return *this;
}


DirEnts::DirEnts(const DirEnts& other)
  : _c{other._c}
  , _vz{other.VZ()}
  , _v{copy(other._v, _vz)}
{}


DirEnts& DirEnts::operator=(const DirEnts& other) {
  _c = other._c;
  if (_vz) dealloc(_v);
  _vz = other.VZ();
  _v = copy(other._v, _vz);
  return *this;
}


DirEnts& DirEnts::operator=(DirEnts&& other) {
  if (other._vz) {
    // transfer existing ownership
    std::swap(_vz, other._vz);
    std::swap(_v,  other._v);
  } else {
    // create new ownership via copy
    _vz = other.VZ();
    _v = copy(other._v, _vz);
  }
  std::swap(_c,  other._c);
  return *this;
}


// ------------------------------------------------------------------------------------------------

AsyncCanceler stat(const string& path, Async& a, StatCallback&& cb) {
  return AsyncFSCall<StatReq>(path, a, fwdarg(cb), uv_fs_stat);
}

Error stat(const string& path, Stat& result) {
  return SyncFSCall(path, result, uv_fs_stat);
}

AsyncCanceler lstat(const string& path, Async& a, StatCallback&& cb) {
  return AsyncFSCall<StatReq>(path, a, fwdarg(cb), uv_fs_lstat);
}

Error lstat(const string& path, Stat& result) {
  return SyncFSCall(path, result, uv_fs_lstat);
}

inline uint64_t to_usec(const uv_timespec_t& ts) {
  return (uint64_t(ts.tv_nsec) / 1000ull) + (uint64_t(ts.tv_sec) * 1000000ull);
}

Stat::Stat(const uv_fs_t& r)
    : dev{r.statbuf.st_dev}
    , ino{r.statbuf.st_ino}
    , mode{r.statbuf.st_mode}
    , nlink{r.statbuf.st_nlink}
    , uid{r.statbuf.st_uid}
    , gid{r.statbuf.st_gid}
    , rdev{r.statbuf.st_rdev}
    , size{r.statbuf.st_size}
    , blksize{r.statbuf.st_blksize}
    , blocks{r.statbuf.st_blocks}
    , atime{to_usec(r.statbuf.st_atim)}
    , mtime{to_usec(r.statbuf.st_mtim)}
    , ctime{to_usec(r.statbuf.st_ctim)}
{}

// ===============================================================================================


struct ScanDirCtx : SafeRefCounted<ScanDirCtx> {
  string         basedir;
  Async&         async;
  size_t         depthLimit;
  ScanDirFunc    eachFunc;
  ScanDirCB      cb;
  AsyncGroup     asyncGroup;

  ScanDirCtx(const string& basedir, Async& async, size_t depth, ScanDirFunc&& f, ScanDirCB&& cb)
    : basedir{basedir}
    , async{async}
    , depthLimit{depth}
    , eachFunc{f}
    , cb{cb}
    , asyncGroup{
      [=](Error err) {
        cb(err);
        this->releaseRef();
      }
    }
  {}

  bool cancel() {
    if (asyncGroup.cancel()) {
      this->releaseRef();
      return true;
    } else {
      return false;
    }
  }

  bool considerDirEntry(const string& dirname, const string& filename, fs::Stat&& st, size_t depth) {
    assert(!st.isSymlink()); // because we're using fs::stat, not fs::lstat
    if (!eachFunc(dirname, filename, st)) {
      // eachFunc returned false to signal that we should stop digging
      return false;
    } else if (depth < depthLimit && st.isDir()) {
      dispatchReadDir(pathJoin(dirname,filename), depth+1);
    }
    return true;
  }

  void dispatchStat(const string& dirname, const string& filename, size_t depth) {
    auto path = pathJoin(basedir,dirname,filename);
    auto job = asyncGroup.begin();
    *job = fs::stat(path, async, [=](Error err, fs::Stat&& st) {
      if (!err) {
        if (considerDirEntry(dirname, filename, fwdarg(st), depth)) {
          asyncGroup.end(job); // Task completed.
        } else {
          // eachFunc signalled "abort!"
          // Note that we should _not_ mark the job as completed as it's about to be canceled.
          cancel();
        }
      } else {
        // Task completed. We ignore stat errors
        asyncGroup.end(job);
      }
    });
  }

  void dispatchReadDir(const string& path, size_t depth) {
    auto job = asyncGroup.begin();
    *job = fs::readdir(pathJoin(basedir,path), async, [=](Error err, const fs::DirEnts& entries) {
      if (!err) for (auto ent : entries) {
        auto filename = string(ent);
        dispatchStat(path, filename, depth);
      }
      asyncGroup.end(job, fwdarg(err));
    });
  }
};


AsyncCanceler scandir(const string& dirname, Async& a, size_t d, ScanDirFunc&& f, ScanDirCB&& cb) {
  ScanDirCtx::Ref ctx{new ScanDirCtx{dirname, a, d, fwdarg(f), fwdarg(cb)}};
  ctx->retainRef(); // we release this when invoking cb or cancel
  ctx->dispatchReadDir({}, 0);
  return [ctx]{
    if (ctx && ctx->cancel()) {
      ctx.resetSelf(); // clear ref held by this closure
    }
  };
}



}} // namespace

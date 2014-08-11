#pragma once
#include "time.hh"
#include "async.hh"
#include "asynccanceler.hh"
#include "error.hh"
#include "util.hh"

namespace rx {
namespace fs {
using std::string;

string pathExt(const string& fn);
  // E.g. ("/foo.app/bar.lol.baz") -> "baz", ("nothing") -> ""

string pathJoin(std::initializer_list<const string>);
template <typename... Args> string pathJoin(Args...);
  // E.g. ("a/b","c/d") -> "a/b/c/d", ("","c/d") -> "c/d", ("a/b","") -> "a/b"

// stat
struct Stat;
using StatCallback = func<void(Error,Stat&&)>;
AsyncCanceler stat(const string& path, Async&, StatCallback&&);
Error         stat(const string& path, Stat&); // sync
AsyncCanceler lstat(const string& path, Async&, StatCallback&&);
Error         lstat(const string& path, Stat&); // sync
  // Retrieve file status. `stat` automatically traverses symlinks while `lstat` doesn't.

// readdir
struct DirEnts;
using ReadDirCallback = func<void(Error,const DirEnts&)>;
AsyncCanceler readdir(const string& path, Async&, ReadDirCallback&&);
Error         readdir(const string& path, DirEnts&); // sync
  // Read list of directory contents.

// scandir
using ScanDirCB = func<void(Error)>;
using ScanDirFunc = func<bool(const string& dirname, const string& filename, const fs::Stat&)>;
AsyncCanceler scandir(const string& path, Async&, size_t depth, ScanDirFunc&&, ScanDirCB&&);
  // Perform a deep search for directory entries starting in directory at `path`, calling
  // ScanDirFunc for each file entry found.
  // `depth` limits subdirectory traversal. When depth=0 no subdirectories are traversed.
  // When either all files have been visited, or digging ended for some reason, `ScanDirCB` is
  // called.
  // If ScanDirFunc returns false, digging stops immediately.
  // As usual with invoking cancelers, the callback will not be invoked so take care to clean up
  // any resources used during find (e.g. collections on the heap) when canceling.

// readlink
struct SymLink;
using ReadLinkCallback = func<void(Error,SymLink&&)>;
AsyncCanceler readlink(const string& path, Async&, ReadLinkCallback&&);
Error         readlink(const string& path, SymLink&); // sync
  // Read the value of a symbolic link.

// readfile
struct FileData;
using ReadFileCallback = func<void(Error,FileData&&)>;
AsyncCanceler readfile(Async&, const string& path, size_t size, ReadFileCallback&&);
AsyncCanceler readfile(Async&, const string& path, ReadFileCallback&&);
Error         readfile(const string& path, size_t size, FileData&);
Error         readfile(const string& path, FileData&);
  // Reads the contents of a file by means of memory-mapping.
  // If `size` is zero, the size is calculated automatically at the expense of one stat call.
  // The forms w/o a `size` argument are simply convenience wrappers for `size=0`.

struct FileData {
  FileData() {}
  FileData(char* p, size_t z, int fd) : p{p}, z{z}, fd{fd} {}
  ~FileData();
  FileData(const FileData&) = delete;
  FileData(FileData&&) = default;
  FileData& operator=(const FileData&) = delete;
  FileData& operator=(FileData&&);
  size_t size() const { return z; }
  const char* data() const { return p; }
  char operator[](size_t i) const { return p[i]; }

  char*  p = nullptr;
  size_t z = 0;
  int    fd = -1;
};

struct Stat {
  Stat() {};
  Stat(const uv_fs_t&);
  bool isFile() const;
  bool isSymlink() const;
  bool isDir() const;
  bool isSocket() const;

  uint64_t dev;     // ID of device containing file
  uint64_t ino;     // inode number
  uint64_t mode;    // protection
  uint64_t nlink;   // number of hard links
  uint64_t uid;     // user ID of owner
  uint64_t gid;     // group ID of owner
  uint64_t rdev;    // device ID (if special file)
  uint64_t size;    // total size, in bytes
  uint64_t blksize; // blocksize for file system I/O
  uint64_t blocks;  // number of 512B blocks allocated
  Time     atime;   // time of last access
  Time     mtime;   // time of last modification
  Time     ctime;   // time of last status change
};

struct DirEnts {
  struct iterator;
  DirEnts();
  DirEnts(ssize_t c, const char* v) : _c(c), _v{v} {}
  DirEnts(const uv_fs_t&);
  DirEnts(const DirEnts&);
  DirEnts(DirEnts&&) = default;
  ~DirEnts();
  DirEnts& operator=(const DirEnts&);
  DirEnts& operator=(DirEnts&&);

  ssize_t size() const { return _c; }
  ssize_t empty() const { return !_c; }
  iterator begin() const { return {_c,_v}; }
  iterator end() const { return {}; }

  struct iterator {
    iterator() : _c{0}, _vp{nullptr}, _ve{nullptr} {}
    iterator(ssize_t c, const char* vp);
    iterator& operator++();
    bool operator!=(const iterator& other) const { return _vp != other._vp; }
    const iterator& operator*() const { return *this; }
    operator string() const;
    const char* c_str() const;
  private:
    ssize_t     _c;  // Number of remaining entries
    const char* _vp; // Start of current entry
    const char* _ve; // End of current entry
  };
private:
  size_t VZ() const;   // Calculate vz by traversing _v limited by _c. Used on first copy.
  ssize_t     _c = 0;  // Number of c-string entries in _v
  size_t      _vz = 0; // Non-zero if we own _v, in which case it's _v's size
  const char* _v = NULL;
};

struct SymLink {
  SymLink();
  SymLink(const uv_fs_t&);
  string target;
};


// ================================================================================================
template <typename... Args> inline string pathJoin(Args... args) { return pathJoin({args...}); }

inline bool Stat::isFile() const    { return ((mode & S_IFMT) == S_IFREG); }
inline bool Stat::isSymlink() const { return ((mode & S_IFMT) == S_IFLNK); }
inline bool Stat::isDir() const     { return ((mode & S_IFMT) == S_IFDIR); }
inline bool Stat::isSocket() const  { return ((mode & S_IFMT) == S_IFSOCK); }
  // See http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/stat.h.html


inline DirEnts::DirEnts() {}
inline DirEnts::DirEnts(const uv_fs_t& r) : DirEnts{r.result, (const char*)r.ptr} {}
inline DirEnts::~DirEnts() { if (_vz) dealloc(_v); }
inline DirEnts::iterator::operator string() const {
  return _vp ? string{_vp, static_cast<size_t>(_ve - _vp - 1)} : string{};
}
inline const char* DirEnts::iterator::c_str() const { return _vp; }

inline std::ostream& operator<< (std::ostream& os, const DirEnts::iterator& v) {
  return os << v.c_str();
}

inline SymLink::SymLink() {}
inline SymLink::SymLink(const uv_fs_t& r) : target{r.ptr ? (const char*)r.ptr : ""} {}

inline AsyncCanceler readfile(Async& a, const string& path, ReadFileCallback cb) {
  return readfile(a, path, 0, fwdarg(cb));
}

inline Error readfile(const string& path, FileData& d) {
  return readfile(path, 0, d);
}

}} // namespace

/* Examples
---------------------------------------------------------------------------------------------------

auto handleStat = [](Error err, fs::Stat&& st){
  if (err) {
    cerr << "stat(hello.cc) sync failed: " << err << endl;
  } else {
    cerr << "stat(hello.cc) sync returned: mtime=" << st.mtime << endl;
  }
};

// Sync
fs::Stat st;
handleStat(fs::stat("hello.cc", st), std::move(st));

// Async
auto canceler = fs::stat("hello.cc", Async::main(), handleStat);

---------------------------------------------------------------------------------------------------
*/

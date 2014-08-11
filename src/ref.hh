#pragma once
namespace rx {

struct PlainRefCounter {
  using value_type = uint32_t;
  static void retain(value_type& v) { ++v; }
  static bool release(value_type& v) { return --v == 0; }
};

struct AtomicRefCounter {
  using value_type = volatile uint32_t;
  static void retain(value_type& v) { __sync_add_and_fetch(&v, 1); }
  static bool release(value_type& v) { return __sync_sub_and_fetch(&v, 1) == 0; }
};


template <typename T>
struct Ref {
  T* self;
  Ref() : self{nullptr} {}
  Ref(std::nullptr_t) : self{nullptr} {}
  explicit Ref(T* p, bool add_ref=false) : self{p} { if (add_ref && p) { self->retainRef(); } }
  Ref(const Ref& rhs) : self{rhs.self} { if (self) self->retainRef(); }
  Ref(const Ref* rhs) : self{rhs->self} { if (self) self->retainRef(); }
  Ref(Ref&& rhs) { self = std::move(rhs.self); rhs.self = 0; }
  ~Ref() { if (self) self->releaseRef(); }
  void resetSelf(std::nullptr_t=nullptr) const {
    if (self) {
      auto* s = const_cast<Ref*>(this);
      s->self->releaseRef();
      s->self = nullptr;
    }
  }
  Ref& resetSelf(const T* p) const {
    T* old = self;
    const_cast<Ref*>(this)->self = const_cast<T*>(p);
    if (self) self->retainRef();
    if (old) old->releaseRef();
    return *const_cast<Ref*>(this);
  }
  Ref& operator=(const Ref& rhs) {  return resetSelf(rhs.self); }
  Ref& operator=(T* rhs) {          return resetSelf(rhs); }
  Ref& operator=(const T* rhs) {    return resetSelf(rhs); }
  Ref& operator=(std::nullptr_t) {  return resetSelf(nullptr); }
  Ref& operator=(Ref&& rhs) {
    if (self != rhs.self && self) {
      self->releaseRef();
      self = nullptr;
    }
    std::swap(self, rhs.self);
    return *this;
  }
  T* operator->() const { return self; }
  operator bool() const { return self != nullptr; }
};


template <typename T, class C>
struct RefCounted {
  typename C::value_type __refc = 1;
  virtual ~RefCounted() = default;
  void retainRef() { C::retain(__refc); }
  bool releaseRef() { return C::release(__refc) && ({ delete this; true; }); }
  using Ref = Ref<T>;
};

template <typename T> using UnsafeRefCounted = RefCounted<T, PlainRefCounter>;
template <typename T> using SafeRefCounted = RefCounted<T, AtomicRefCounter>;

// Example:
//
//   struct ReqBase {
//     uv_fs_t uvreq;
//   };
//  
//   struct StatReq final : ReqBase, SafeRefCounted<StatReq> {
//     StatReq(StatCallback&& cb) : Req{}, cb{fwdarg(cb)} {}
//     StatCallback cb;
//   };
//

} // namespace

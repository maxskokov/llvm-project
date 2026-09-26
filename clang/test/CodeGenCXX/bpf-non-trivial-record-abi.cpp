// REQUIRES: bpf-registered-target
// RUN: %clang_cc1 -triple bpf -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple bpf -emit-llvm -fclang-abi-compat=23 -o - %s \
// RUN:   | FileCheck %s --check-prefix=COMPAT23

// Records that are non-trivial for the purpose of calls must be passed and
// returned indirectly, even if they would fit in one or two registers. The
// caller owns the temporary and destroys it after the call, so passing its
// contents in registers leaves both sides owning the same value.

struct Dtor8 {
  void *p;
  ~Dtor8();
};

struct Dtor16 {
  void *a, *b;
  ~Dtor16();
};

struct Copy8 {
  void *p;
  Copy8(const Copy8 &);
};

struct [[clang::trivial_abi]] TrivialAbi8 {
  void *p;
  ~TrivialAbi8();
};

struct Trivial8 {
  void *p;
};

struct Trivial16 {
  void *a, *b;
};

// CHECK: define dso_local void @_Z5dtor85Dtor8(ptr {{.*}}%x)
// COMPAT23: define dso_local void @_Z5dtor85Dtor8(i64 %x.coerce)
void dtor8(Dtor8 x) {}
// CHECK: define dso_local void @_Z6dtor166Dtor16(ptr {{.*}}%x)
// COMPAT23: define dso_local void @_Z6dtor166Dtor16([2 x i64] %x.coerce)
void dtor16(Dtor16 x) {}
// CHECK: define dso_local void @_Z5copy85Copy8(ptr {{.*}}%x)
void copy8(Copy8 x) {}
// CHECK: define dso_local void @_Z11trivialabi811TrivialAbi8(i64 %x.coerce)
void trivialabi8(TrivialAbi8 x) {}
// CHECK: define dso_local void @_Z8trivial88Trivial8(i64 %x.coerce)
void trivial8(Trivial8 x) {}
// CHECK: define dso_local void @_Z9trivial169Trivial16([2 x i64] %x.coerce)
void trivial16(Trivial16 x) {}

Dtor8 make_dtor8();
Dtor16 make_dtor16();
Copy8 make_copy8();
TrivialAbi8 make_trivialabi8();
Trivial8 make_trivial8();
Trivial16 make_trivial16();

// CHECK: define dso_local void @_Z9ret_dtor8v(ptr {{.*}}sret(%struct.Dtor8) {{.*}}%agg.result)
// CHECK: call void @_Z10make_dtor8v(ptr {{.*}}sret(%struct.Dtor8) {{.*}}%agg.result)
Dtor8 ret_dtor8() { return make_dtor8(); }
// CHECK: define dso_local void @_Z10ret_dtor16v(ptr {{.*}}sret(%struct.Dtor16) {{.*}}%agg.result)
// COMPAT23: define dso_local [2 x i64] @_Z10ret_dtor16v()
Dtor16 ret_dtor16() { return make_dtor16(); }
// CHECK: define dso_local void @_Z9ret_copy8v(ptr {{.*}}sret(%struct.Copy8) {{.*}}%agg.result)
Copy8 ret_copy8() { return make_copy8(); }
// CHECK: define dso_local i64 @_Z15ret_trivialabi8v()
TrivialAbi8 ret_trivialabi8() { return make_trivialabi8(); }
// CHECK: define dso_local i64 @_Z12ret_trivial8v()
Trivial8 ret_trivial8() { return make_trivial8(); }
// CHECK: define dso_local [2 x i64] @_Z13ret_trivial16v()
Trivial16 ret_trivial16() { return make_trivial16(); }

// The caller passes a pointer to its temporary and destroys that temporary
// after the call.
void take_dtor8(Dtor8 x);
// CHECK-LABEL: define dso_local void @_Z10pass_dtor8v()
// CHECK: %[[TMP:.*]] = alloca %struct.Dtor8, align 8
// CHECK: call void @_Z10make_dtor8v(ptr {{.*}}sret(%struct.Dtor8) {{.*}}%[[TMP]])
// CHECK: call void @_Z10take_dtor85Dtor8(ptr {{.*}}%[[TMP]])
// CHECK: call void @_ZN5Dtor8D1Ev(ptr {{.*}}%[[TMP]])
void pass_dtor8() { take_dtor8(make_dtor8()); }

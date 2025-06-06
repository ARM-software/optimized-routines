# 🧵 String

The string sub-project contains string routines (`memcpy`, `strcmp`, ...)
optimized specifically for Arm architecture.  These routines can be used as
replacement to string.h routines in the standard C library.

This document provides style requirements and contribution guidelines for
string routines.

---

## 📂 Subdirectory Structure

Here is a quick overview of the **`string/`** subdirectory's layout:

- **`<arch>/`**: `<arch>`-specific string routines sources for `<arch>=aarch64`, and `arm`.
- **`aarch64/experimental/`**: Experimental string routines that fail to meet quality requirements listed below.
- **`include/`**: string library public headers.
- **`test/`**: string test and benchmark related sources.

---

## 🎨 Style Requirements

1. **Upstream Compatibility**:
   Code should follow GNU Coding Standard and glibc specific conventions to
ease upstreaming.

2. **ABI and Symbols**:
   Code should be written so it is suitable for inclusion into a libc with
minimal changes, e.g.
   - Internal symbols should be hidden and in the implementation reserved
     namespace according to ISO C and POSIX rules.
   - If possible the built shared libraries and static library archives should
     be usable to override libc symbols at link time (or at runtime via
     LD_PRELOAD).
   - This requires the symbols to follow the glibc ABI (other than symbol
     versioning), this cannot be done reliably for static linking so this is a
     best effort requirement.

3. **API Headers**:
   Include headers should be suitable for benchmarking and testing code and
should not conflict with libc headers.

---

## ✅ Contribution Guidelines

- Clearly document assumptions in the code.
- Maintain consistent assembly style across different implementations.
- Benchmarking is needed on several microarchitectures.

---

By adhering to these guidelines, you will help maintain the quality and
consistency of the Arm Optimized Routines project. We appreciate your
contributions and look forward to collaborating with you!


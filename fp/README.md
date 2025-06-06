# ➕ FP

This subdirectory contains optimized assembler implementations of basic
floating-point arithmetic, suitable for use on Arm platforms which do not
implement the operation in question or hardware FP (or do not have hardware FP
at all).

It includes implementations of the same function for multiple versions of the
Arm architecture, optimized differently to take advantage of instruction
availability.

---

## 📂 Subdirectory Structure

Here is a quick overview of the **`fp/`** subdirectory's layout:

- **`armv6-m/`**: Thumb-1 code, compatible with Arm v6-M itself and also Arm v8-M Baseline.
- **`at32/`**: Code that can be assembled as either Arm or Thumb-2.
- **`common/`**: Common and shared sources.
- **`test/`**: fp test and benchmark related sources.
- **`include/`**: header files included by the source code, providing common definitions such as register aliases.

---

## 🧑‍💻 User and Contributor Guidelines

This section provides some key information on how users can import these FP
routines into their own library, and if these functions have the semantics they
need for their project.  It also sets some important code style and quality
requirement guidelines to facilitate contributions into the FP subproject.

1. **Upstream compatibility**:
The source code in this subdirectory is intended to be
usable with either the GNU toolchain or the LLVM toolchain. Avoid
using assembler idioms that are not supported by both, such as the
pseudo-instruction `adrl`.

2. **Helper functions**:
Some helper functions which are not performance-critical are written
in C. This allows them to be written only once and recompiled for the
appropriate architecture. These functions are expected to be used by
uncommon cases off the "fast path": handling unusual types of input
(such as NaNs and denormals), or generating unusual output (such as
handling underflow by generating a denormal).

3. **Link/Run-time compatibility**:
Entry points of these functions have nonstandard names such as
`arm_fp_fmul`, rather than names like `__aeabi_fmul` or `__mulsf3`
used by toolchains' actual run-time libraries. This is so that they
can be linked into a test program compiled with a standard toolchain,
without causing link-time symbol conflicts with the existing
implementation of the same function in the toolchain's library.

4. **Default semantics**:
The default semantics for a function in this directory is to conform
precisely to IEEE 754 in terms of the input and output values, plus
Arm hardware's conventions for NaN semantics, but not to report
floating-point exceptions via any secondary output, or to support
dynamic rounding mode configuration via any secondary input.

> [!IMPORTANT]
> Implementations making a different choice in this area are welcome,
but should be commented as diverging from this default.

In particular, the default semantics are:

 - Denormals are handled correctly without being flushed to zero.

 - Signed zeroes are handled correctly, and the correct sign of zero
   is returned.

 - A NaN is considered signalling if its topmost mantissa bit is 0,
   and quiet if that bit is 1.

 - An input signalling NaN is converted into an output quiet NaN by
   setting the topmost mantissa bit and otherwise changing nothing. If
   both inputs to a dyadic function are signalling NaNs, the output is
   based on the first input.

 - In the absence of signalling NaNs, an input quiet NaN is propagated
   unchanged to the output. Again, the first input takes priority if
   both inputs are quiet NaNs.

---

By adhering to these guidelines, you will help maintain the quality and
consistency of the Arm Optimized Routines project. We appreciate your
contributions and look forward to collaborating with you!


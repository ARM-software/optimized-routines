# 📐 Math

The math sub-project contains elementary, transcendental and special math
routines optimized specifically for Arm architecture.  These routines can be
used as replacement to math.h routines in the standard C library.

Math provides scalar libm routines as well as vector variants (libmvec) acting
on input vector registers.  While scalar math tends to be cross-platform,
vector code is optimised specifically for AdvSIMD or SVE enabled
micro-architectures.

This document provides a user guide, as well as style requirements and
contribution guidelines for math routines.

---

## 📂 Subdirectory Structure

Here is a quick overview of the **`math/`** subdirectory's layout:

- **`aarch64/`**: AArch64-specific math sources and shared sources.
  - **`advsimd/`**: AdvSIMD-specific math sources.
  - **`experimental/`**: Experimental math sources that do not meet the quality requirements stated below.
  - **`sve/`**: SVE-specific math sources.
- **`include/`**: Public headers for the math library.
- **`test/`**: Sources related to testing and benchmarking math routines.
- **`tools/`**: Tools used for designing algorithms.

---

## 🧑‍💻 User Guidelines

### Building and Testing

#### Building Math on Linux

In order to build and test the Math subproject only:

```bash
make all-math
make check-math
```

> [!WARNING]
> SVE routines are built by default. Ensure that on AArch64, you're
using GCC ≥ 10 or LLVM ≥ 5 for SVE ACLE compatibility. There is no explicit
check for a compatible compiler; thus, SVE routines will fail to build if the
compiler is too old.

#### Testing Math on Linux

The test system requires `libmpfr` and `libmpc`. On Debian-based systems, you
can install them using:

```bash
sudo apt install libmpfr-dev libmpc-dev
```

#### Building on Windows or macOS

Math can be built on Windows or macOS.  The `config.mk.dist` file contains some
presets and guidance for compilation on such platforms.  In particular, on
Windows we recommend using an Msys2 environment for maximum compatibility.

On a different environment users may want to specify different paths to
dependencies like `mpc`, `mpfr`, `gmp`, ...

A simple way to install such dependencies on a Windows platform is to rely on
`pacman`.

```bash
pacman \
 mingw-w64-clang-aarch64-gmp \
 mingw-w64-clang-aarch64-mpc \
 mingw-w64-clang-aarch64-mpfr
```

### Tools

The math sub-project comes with a suite of tools to assess accuracy and measure
performance of math routines.  It also provides scripts to reproduce
coefficients and tables used in each algorithm.  This section provides some
guidelines on how to use these tools.

#### Assessing accuracy

Accuracy of math routines is measured as the maximum distance (in ULP) between
the exact value evaluated in extended precision and the approximate value.  In
order to assess this maximum accuracy we can run the `ulp` program for a given
function on a given interval. The `ulp` manual is invoked by executing the
program without option or with any unrecognised option, e.g.

```bash
./build/bin/ulp
```

The maximum error can be measured by performing an exhaustive search for single
precision routines, e.g.

```bash
./build/bin/ulp -q -e 3.0 <function name> 0 inf
./build/bin/ulp -q -e 3.0 <function name> -0 -inf
```

> [!NOTE]
> The `ulp` manual displays a list of all supported `<function name>` in a way
> that is easily parsed.

However, in double precision it has to be estimated from a sufficiently large
random sample, e.g.

```bash
./build/bin/ulp -q -e 3.0 <function name> 0 inf 1000000
```

All errors above a specified threshold (`-e`) are displayed and can be piped
into a python script plotting error against input values.

```bash
./build/bin/ulp -q -e 3.0 <function name> 0 1 1000 | ./math/tools/plot.py
```

#### Evaluating performance

The performance of scalar and vector math routines is evaluated via 2 metrics
- the reciprocal throughput (ns/elem)
- the latency (ns/call)

In order to benchmark math routines on given intervals we use the `mathbench`
program.  Results can be compared to that of system libm, by using
`mathbench_libc`.

The `mathbench` manual is invoked by executing the program without option or
using any unrecognised option, e.g.

```bash
./build/bin/mathbench
```

The number of samples and iterations are hardcoded to get lowest variability
across all routines.  When passed a `<function name>` only, the `mathbench`
program will run on a set of predefined intervals:

```bash
./build/bin/mathbench <function name>
```

> [!NOTE]
> The `mathbench` manual displays a list of all supported `<function name>` in
> a way that is easily parsed.

#### Reproducible algorithms

Algorithms for math routines rely heavily on polynomial approximations. Such
polynomial are optimized to minimize approximation error using Remez or
FPMinimax algorithms. In order to reproduce/generate the polynomial
coefficients, users need to install the `Sollya` library via `apt`, e.g.

```bash
sudo apt install libsollya-dev
```

or directly from sources, then run some of the pre-defined `*.sollya` scripts
in [math/tools/](math/tools/)

```bash
sollya math/tools/exp.sollya
```

---

## ✅ Contribution Guidelines

This section outlines the style requirements and contribution guidelines to
ensure consistency and quality across the math routines.

### 🎨 Style Requirements

1. **Upstream Compatibility**:
   Most code here is intended for upstreaming into glibc (except for
   `aarch64/experimental/`).
   - Follow the GNU Coding Standards and glibc-specific conventions.

2. **ABI and Symbols**:
   - Code should be suitable for inclusion into a libc with minimal changes.
   - Internal symbols should be hidden and in the reserved namespace as per ISO
     C and POSIX rules.
   - Shared libraries and static archives should be usable to override libc
     symbols.
   - Symbols should follow the glibc ABI (excluding symbol versioning); this is
     best-effort for static linking.

3. **API Headers**:
   - Include headers that can be used for benchmarking and testing without
     conflicting with libc headers.

### 💻 Code Requirements

Math functions have quality (accuracy and standard compliance) and performance
requirements.

#### Quality Requirements

- Worst-case ULP error should be small in the entire input domain, typically:
  - < 0.66 for double-precision scalar functions (tested via random sampling)
  - < 1 for single-precision (tested exhaustively)
  - < 3.5 for performance-optimized variants
  The `ulp` tool is designed to assess accuracy of all routines.

- All standard rounding modes should be supported for scalar implementations,
  even if slower or slightly less accurate.

- Special cases and error handling need to follow ISO C Annex F requirements,
  POSIX requirements, IEEE 754-2008 requirements and [Glibc
  requirements](https://www.gnu.org/software/libc/manual/html_mono/libc.html#Errors-in-Math-Functions)
  this should be tested by directed tests.

- Use helper functions to isolate errno and exception handling from
  approximation logic.

- Vector math need not support non-nearest rounding or fenv exception and errno
  handling.

- Error bounds of the approximation should be clearly documented.

- Code must build and pass tests on Arm (32-bit), AArch64, and x86_64 GNU linux
  systems.  Routines and features can be disabled on specific targets, but the
  build must complete. On AArch64, both little- and big-endian targets are
  supported as well as valid combinations of architecture extensions.  The
  configurations that should be tested depend on the contribution.

#### Performance Requirements

- Benchmark on modern AArch64 microarchitectures over typical inputs.

- Document relative performance improvement. Use the `mathbench` tool and
  update it with new functions.

- Attention should be paid to the compilation flags: for aarch64 fma
  contraction should be on and math errno turned off so some builtins can be
  inlined.

- The code should be reasonably performant on x86_64 too, e.g. some rounding
  instructions and fma may not be available on x86_64, such builtins turn into
  libc calls with slow code. Such slowdown is not acceptable, a faster fallback
  should be present: glibc and bionic use the same code on all targets. (This
  does not apply to vector math code).

---

By adhering to these guidelines, you will help maintain the quality and
consistency of the Arm Optimized Routines project. We appreciate your
contributions and look forward to collaborating with you!


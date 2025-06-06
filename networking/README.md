# 🛜 Networking

This section provides style requirements and contribution guidelines for
networking routines.

---

## 📂 Subdirectory Structure

Here is a quick overview of the **`networking/`** subdirectory's layout:

- **`include/`**: networking library public headers.
- **`test/`**: networking test and benchmark related sources.

---

## 🧑‍💻 Style Requirements

1. **Upstream Compatibility**:
   Code should follow GNU Coding Standard and glibc specific conventions to
ease upstreaming.

2. **ABI and Symbols**:
   Code should be written so it is suitable for inclusion into a libc with
minimal changes.

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


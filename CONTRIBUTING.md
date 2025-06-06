# 🤝 Contribution Guidelines for Arm Optimized Routines

Thank you for your interest in contributing to the Arm Optimized Routines
project! This document outlines the general guidelines for contributing to the
project. Please note that each sub-project may have its own specific
contribution requirements detailed in their respective `README.md` files.

---

## 📁 Sub-Project Specific Guidelines

Each sub-project within this repository is maintained independently and may
have its own contribution rules. If a `README.md` file exists in the
sub-directory you are contributing to, please ensure you follow its guidelines.

---

## 📝 Legal Requirements

- **Contributor Assignment Agreement**:
  If you are not employed by Arm, you must sign an Assignment Agreement before
your contributions can be accepted. Please refer to the
[`contributor-agreement.pdf`](contributor-agreement.pdf) file for instructions.

- **Copyright and Licensing**:
  All code contributions must be copyright owned by Arm Limited. Ensure that
each source file includes the appropriate copyright notice and license
identifier.

---

## 🛠️ Build Guidelines

- **Dependencies**:
  The build process should rely only on GNU Make, POSIX utilities (such as
`shell`, `awk`, `sed`), and a C toolchain.

- **Compatibility**:
  The build should succeed with the default configuration (`config.mk.dist`)
and other supported configurations using both GCC and Clang toolchains. Avoid
dependencies on recent toolchain features; new features should be optional and
disable-able.

- **Configuration**:
  Currently, there is no automated configuration system. Target-specific
configurations should be managed via make variables in `config.mk`. This file
serves as the user interface to the build system and should be well-documented
and kept stable.

---

## ✅ Testing Guidelines

- **Platform Support**:
  All tests must pass on AArch64 platforms. If your code behaves differently
under certain configurations (e.g., different `CFLAGS`), ensure those
configurations are tested accordingly.

- **New Symbols**:
  Any new symbols introduced should be accompanied by appropriate test code.
Ideally, benchmark code should also be provided to assess performance.

---

## 🧾 Commit Guidelines

- **Commit Messages**:
  Write descriptive commit messages that avoid referencing internal Arm
information (such as ticket numbers or internal discussions). If a decision is
not obvious, document it in the commit message and source comments.

- **Tools and Scripts**:
  If you used any tools or scripts to develop your code, consider adding them
to the repository or at least mention them in your commit message.

- **Atomic Commits**:
  Ensure that each commit represents a logically independent change. Avoid
mixing unrelated changes in a single commit.

---

## 🎨 Coding Style

- **Formatting**:
  Unless specified otherwise by the sub-project, use `clang-format` with the
style configuration from the GCC `contrib/` directory.

---

By adhering to these guidelines, you'll help maintain the quality and
consistency of the Arm Optimized Routines project. We appreciate your
contributions and look forward to collaborating with you!


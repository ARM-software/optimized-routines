# 🚀 Arm Optimized Routines

![master branch](https://github.com/ARM-software/optimized-routines/actions/workflows/tests/badge.svg)

**High-performance, architecture-aware implementations of common library
functions for Arm processors.**

This repository offers a collection of optimized implementations of various
library functions tailored for Arm processors. Whether you are developing for
embedded systems, mobile devices, or high-performance computing, these routines
are designed to maximize efficiency and performance on Arm platforms.

> [!NOTE]
> Arm is a registered trademark of Arm Limited (or its subsidiaries or affiliates).

---

## 📂 Repository Structure

Here is a quick overview of the repository's layout:

- **`build/`**: Directory created by the build process.
- **`fp/`**: Floating-point basic arithmetic sources.
- **`math/`**: Elementary math subproject sources.
- **`networking/`**: Networking subproject sources.
- **`string/`**: String routines subproject sources.

---

## 🛠️ Building and Testing

To build and test the routines in all sub-projects:

1. **Clone the repository**:

   ```bash
   git clone https://github.com/ARM-software/optimized-routines.git
   cd optimized-routines
   ```

2. **Set up the build configuration**:

   ```bash
   cp config.mk.dist config.mk
   # Edit config.mk as needed
   ```

3. **Build and test**:

   ```bash
   make
   make check
   ```

**Building and Testing a single subproject**:

In order to build a single subproject `<sub>` use the following commands

```bash
make all-<sub>
make check-<sub>
```

Alternatively, the `config.mk` file can be updated to specify a list of
sub-projects.

**Cross-Building**:

For cross-building, set `CROSS_COMPILE` in `config.mk` and define `EMULATOR`
for cross-testing (e.g., using `qemu-user` or remote access to a target
machine). Refer to examples in `config.mk.dist`.

---

## 📜 Licensing

This project is licensed under a dual license, allowing you to choose between:

- MIT License
- Apache License 2.0 with LLVM Exceptions

For full details, please refer to the [LICENSE](LICENSE) file.

---

## 🤝 Contributing

We welcome contributions from the community! To contribute:

1. **Sign the Contributor Assignment Agreement**:

   Please follow the instructions in
[contributor-agreement.pdf](contributor-agreement.pdf). This step is necessary
to allow upstreaming code to projects that require copyright assignment.

2. **Review Contribution Guidelines**:

   Please refer to the main [CONTRIBUTING.md](CONTRIBUTING.md) for our general
contribution guidelines.

   Each subdirectory may have its own `README.md` file detailing
specific contribution requirements. Please review these guidelines before
submitting your contributions.

   Each subdirectory has its own maintainers, please refer to
[MAINTAINERS.md](MAINTAINERS.md) for a list of maintainers to contact.

---

## 📦 Releases

Regular biannual releases are tagged as `vYY.MM`. All release are available at
[https://github.com/ARM-software/optimized-routines/releases]().

---

## 📬 Stay Connected

For updates and discussions:

- [**GitHub Issues**](https://github.com/ARM-software/optimized-routines/issues): Report bugs or request features.
- [**Pull Requests**](https://github.com/ARM-software/optimized-routines/pulls): Contribute code and improvements.

---

Thank you for your interest in the Arm Optimized Routines project. We look
forward to your contributions and feedback!


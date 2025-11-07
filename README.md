# **Mini Sandbox Environment**
PBL Project (Operating Systems)

A lightweight, educational sandbox environment built from the ground up using fundamental Linux primitives. This project demonstrates how core OS concepts like `namespaces`, `cgroups`, `chroot`, and `seccomp` are used to create a secure, isolated runtime for executing code, providing insight into the inner workings of modern container technologies.

This sandbox is designed to be minimal and transparent, bridging the gap between operating systems theory and the practical implementation of containerization.

# ✨ *Key Features*
* **Process Isolation:** Utilizes `CLONE_NEWPID` and `CLONE_NEWUTS` namespaces to give sandboxed processes their own unique process and hostname views.

* **Filesystem Isolation:** Leverages the `chroot` system call to confine the process to a dedicated root filesystem, preventing access to the host system's files.

* **Resource Control:** Implements `cgroups` to enforce strict CPU and memory limits, preventing resource exhaustion.

* **System Call Filtering:** Employs `seccomp-BPF` to create a whitelist of allowed system calls, drastically reducing the kernel's attack surface.

* **Simple CLI:** A command-line tool written in C provides an easy-to-use interface for creating and managing sandboxes.

# 🏛️ *How It Works: Architecture & Workflow*

The project follows a bottom-up approach, constructing the sandbox environment directly on top of Linux kernel primitives without relying on external container runtimes.

The workflow is as follows:

* **Initialization:** A new process is created using `clone()` in its own set of namespaces.

* **Confinement:** The process is locked into a minimal root filesystem using `chroot`.

* **Resource Limiting:** The process is added to a pre-configured `cgroup` to enforce resource constraints.

* **Hardening:** A `seccomp` filter is applied to the process to restrict its ability to make potentially harmful system calls.

* **Execution:** The user-specified command is then executed within this hardened and isolated environment.

This modular design demonstrates how layers of isolation can be combined to build a secure container-like structure from basic OS components.

# 🛠️ *Technology and Tools*
Language: C (glibc)

Core Primitives: Linux System Calls (`clone`, `chroot`, `mount`, `setrlimit`, `seccomp`)

Build Tools: `GCC` and `Make`

# 🚀 *Getting Started*

**Prerequisites**

* A Linux-based operating system

* Standard build tools (gcc, make)

* Root privileges (required for creating namespaces and using chroot)

**Installation & Usage**
* Clone the repository:
  

```bash
  git clone https://github.com/SLARV03/Mini_Sandbox_Environment.git
  cd linux-sandbox
```

* Compile the project: `make`


* Run a command inside the sandbox


# 🎓 *Educational Goals*

🧩The primary purpose of this project is to demystify containerization and provide a hands-on learning tool for understanding:

🧠The fundamentals of operating system security and isolation.

🔍How low-level Linux features enable container technologies like Docker.

💻The practical application of core OS concepts in a real-world context.

# 📄 *License*

This project is licensed under the MIT License.

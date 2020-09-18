#pragma once

// std includes
#include <iostream>
#include <optional>

// External lib include
#include <fmt/core.h>

// Linux header includes
#include <assert.h>
#include <cstdarg>
#include <fcntl.h>
#include <linux/kvm.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using std::cout, std::cerr, std::endl, fmt::format;
using std::showbase, std::hex;

const unsigned int PAGE_SIZE = 0x1000;

template <typename... Args> void panic(std::string error, Args... args) {
  cerr << "ERROR: " << strerror(errno) << endl;

  throw std::runtime_error(format(error, args...));
}

class KVM {
private:
  int kvm_fd;

public:
  size_t max_vcpus;
  size_t max_vcpu_mmap_size;
  KVM() {
    kvm_fd = open("/dev/kvm", O_RDWR);
    if (kvm_fd < 0) {
      panic("failed to open /dev/kvm");
    }

    cout << "KVM fd: " << kvm_fd << endl;

    max_vcpus = cap(KVM_CAP_NR_VCPUS);
    max_vcpu_mmap_size =
        send(KVM_GET_VCPU_MMAP_SIZE, 0); // Why do I need a 0 as arg?
  }

  template <typename... Args> int inline send(int request, Args... args) {
    int ret = ioctl(kvm_fd, request, args...);
    if (ret < 0) {
      panic("failed to send request {}", request);
    }
    return ret;
  }
  template <typename... Args> int inline cap(int request, Args... args) {
    int ret = ioctl(kvm_fd, KVM_CHECK_EXTENSION, request, args...);
    if (ret < 0) {
      panic("failed to KVM_CHECK_EXTENSION request {}", request);
    }
    return ret;
  }

  ~KVM() {
    int ret = close(kvm_fd);

    if (ret < 0) {
      panic("failed to close kvm_fd ");
    }
  }
};

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
};

class VM {
private:
  int vm_fd;
  int id;
  size_t max_vcpus;
  size_t max_vcpu_mmap_size;
  std::vector<struct vcpu> vcpus_vec;

public:
  std::vector<struct kvm_userspace_memory_region> memory_vec;
  struct kvm_userspace_memory_region mem;

  VM(KVM &kvm, int id) : id(id) {
    vm_fd = kvm.send(KVM_CREATE_VM, id);
    cout << "Created VM with id " << id << " and fd: " << vm_fd << endl;

    max_vcpu_mmap_size = kvm.max_vcpu_mmap_size;
    max_vcpus = kvm.max_vcpus;
  }

  template <typename... Args> int inline send(int request, Args... args) {
    int ret = ioctl(vm_fd, request, args...);
    if (ret < 0) {
      panic("failed to send request: {} vm_id: {}", request, id);
    }
    return ret;
  }

  void add_mem_pages(size_t pages) {
    assert(pages > 0);

    // Calc number of pages
    size_t mem_size = PAGE_SIZE * pages;
    cout << "Mapping " << hex << showbase << mem_size << " bytes of vm memory"
         << endl;

    // map memory
    char *mem_ptr = reinterpret_cast<char *>(
        mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0));
    if (mem_ptr == MAP_FAILED) {
      panic("Memory allocation failed");
    }

    // Enable Kernel same page merging (KSM)
    madvise(mem_ptr, mem_size, MADV_MERGEABLE);

    // Setup vm memory
    struct kvm_userspace_memory_region mem_arg;
    mem_arg.slot = 0;
    mem_arg.flags = 0;
    mem_arg.memory_size = mem_size;
    mem_arg.userspace_addr = (unsigned long)mem_ptr;

    // Calc last guest phys address
    if (!memory_vec.empty()) {
      const auto &last_mem = memory_vec.back();
      mem_arg.guest_phys_addr = last_mem.guest_phys_addr + last_mem.memory_size;
    } else {
      mem_arg.guest_phys_addr = 0;
    }

    // Set memory
    try {
      send(KVM_SET_USER_MEMORY_REGION, &mem_arg);
    } catch (const std::exception &e) {
      munmap(mem_ptr, mem_size);
      throw;
    }

    // Save memory blob
    memory_vec.emplace_back(mem_arg);
  }

  void add_vcpu() {
    unsigned int vsize = vcpus_vec.size();

    if (vsize <= max_vcpus) {
      int fd = send(KVM_CREATE_VCPU, vsize);

      struct vcpu vcpu;
      vcpu.fd = fd;
      vcpu.kvm_run = reinterpret_cast<struct kvm_run *>(mmap(
          NULL, max_vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

      if (vcpu.kvm_run == MAP_FAILED) {
        panic("mmap in add_vcpu failed");
      }

      vcpus_vec.emplace_back(vcpu);
    } else {
      panic("Can't add vcpu reached max_vcpus: {}", max_vcpus);
    }
  }

  template <typename... Args> int inline send_vcpu(int request, Args... args) {
    assert(!vcpus_vec.empty());

    int ret = ioctl(vcpus_vec.front().fd, request, args...);
    if (ret < 0) {
      panic("failed to send vcpu request: {} vm_id: {}", request, id);
    }
    return ret;
  }

  void set_real_mode() {
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    send_vcpu(KVM_GET_SREGS, &sregs);

    sregs.cs.selector = 0;
    sregs.cs.base = 0;

    send_vcpu(KVM_SET_SREGS, &sregs);

    memset(&regs, 0, sizeof(regs));

    /* regs.rflags = 2; */
    regs.rip = 0; // Instruction pointer

    send_vcpu(KVM_SET_REGS, &regs);
  }

  const struct vcpu &run() {
    const auto &vcpu = vcpus_vec.front();
    if (ioctl(vcpu.fd, KVM_RUN, 0) < 0) {
      panic("Could not execute KVM_RUN");
    }
    return vcpu;
  }

  ~VM() {
    int ret = close(vm_fd);

    if (ret < 0) {
      panic("failed to close kvm_fd");
    }
  }
};

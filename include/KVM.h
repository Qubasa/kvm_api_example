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

const unsigned int PAGE_SIZE = getpagesize();

#ifdef MY_DEBUG
constexpr bool DEBUG = true;
#else
constexpr bool DEBUG = false;
#endif

template <typename... Args>
constexpr void inline dbg(std::string_view msg, Args... args) {
  if constexpr (DEBUG) {
    fmt::print(format("DEBUG: {}", msg), args...);
  }
}

template <typename... Args> void panic(std::string_view error, Args... args) {
  cerr << "ERROR: " << strerror(errno) << endl;

  throw std::runtime_error(format(error, args...));
}

class KVM {
private:
  int kvm_fd;

public:
  size_t max_vcpus;
  size_t max_vcpu_mmap_size;
  size_t max_nr_mem_slots;
  KVM() {
    kvm_fd = open("/dev/kvm", O_RDWR);
    if (kvm_fd < 0) {
      panic("failed to open /dev/kvm");
    }

    max_vcpus = cap(KVM_CAP_NR_VCPUS);
    max_vcpu_mmap_size =
        send(KVM_GET_VCPU_MMAP_SIZE, 0); // Why do I need a 0 as arg?
    max_nr_mem_slots = cap(KVM_CAP_NR_MEMSLOTS);
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
  size_t max_nr_mem_slots;

public:
  std::vector<struct vcpu> vcpus_vec;
  std::vector<struct kvm_userspace_memory_region> mem_vec;

  VM(KVM &kvm, int id) : id(id) {
    vm_fd = kvm.send(KVM_CREATE_VM, id);

    max_vcpu_mmap_size = kvm.max_vcpu_mmap_size;
    max_vcpus = kvm.max_vcpus;
    max_nr_mem_slots = kvm.max_nr_mem_slots;
  }

  template <typename... Args> int inline send(int request, Args... args) {
    int ret = ioctl(vm_fd, request, args...);
    if (ret < 0) {
      panic("failed to send request: {} vm_id: {}", request, id);
    }
    return ret;
  }

  void add_mem_pages(size_t pages, unsigned int slot, unsigned int flags = 0) {
    assert(pages > 0);

    if (slot > max_nr_mem_slots) {
      panic("max mem slots reached");
    }

    for (const auto &m : mem_vec) {
      if (m.slot == slot) {
        panic("mem_page with slot id {} already exists", slot);
      }
    }

    // Calc number of pages
    size_t mem_size = PAGE_SIZE * pages;

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
    mem_arg.slot = slot;
    mem_arg.flags = flags;
    mem_arg.memory_size = mem_size;
    mem_arg.userspace_addr = (unsigned long)mem_ptr;

    // Calculate memory phys addr
    if (mem_vec.empty()) {
      mem_arg.guest_phys_addr = 0;
    } else {
      const auto &old_mem = mem_vec.back();
      mem_arg.guest_phys_addr = old_mem.guest_phys_addr + old_mem.memory_size;
    }

    // Set memory
    try {
      send(KVM_SET_USER_MEMORY_REGION, &mem_arg);
    } catch (const std::exception &e) {
      munmap(mem_ptr, mem_size);
      throw;
    }

    mem_vec.emplace_back(std::move(mem_arg));
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

  template <typename... Args> int inline send_vcpu(int request,size_t i_vcpu, Args... args) {
    int ret = ioctl(vcpus_vec.at(i_vcpu).fd, request, args...);
    if (ret < 0) {
      panic("failed to send vcpu request: {} vm_id: {}", request, id);
    }
    return ret;
  }
  template <typename... Args> int inline send_vcpu(int request, Args... args) {
    assert(!vcpus_vec.empty());

    int ret = ioctl(vcpus_vec.front().fd, request, args...);
    if (ret < 0) {
      panic("failed to send vcpu request: {} vm_id: {}", request, id);
    }
    return ret;
  }

  void set_real_mode(unsigned long rip=0) {
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    send_vcpu(KVM_GET_SREGS, &sregs);

    sregs.cs.selector = 0;
    sregs.cs.base = 0;

    send_vcpu(KVM_SET_SREGS, &sregs);

    memset(&regs, 0, sizeof(regs));

    regs.rflags = 0x2; // VT-X needs this
    regs.rip = rip;      // Instruction pointer

    send_vcpu(KVM_SET_REGS, &regs);
  }

  void copy_to_mem(unsigned long guest_addr, void *src, size_t size) {

    // Search for memory slot containing guest_addr
    for (const auto &memory : mem_vec) {
      if (memory.guest_phys_addr + memory.memory_size <= guest_addr) {
        continue;
      }
      dbg("copy_to_mem guest_addr {:#x}, found vm slot {} guest_addr {:#x} end "
          "{:#x}\n",
          guest_addr, memory.slot, memory.guest_phys_addr,
          memory.guest_phys_addr + memory.memory_size);

      // Get host memory pointer
      unsigned char *guest_mem =
          reinterpret_cast<unsigned char *>(memory.userspace_addr);


      // If copy size bigger then current slot memory
      // TODO: copy over slot boundaries
      if(memory.memory_size < size){
        panic("this slot only has {:#x} bytes of memory available", memory.memory_size);
      }

      // TODO: Remove after enough testing
      if(memory.guest_phys_addr > guest_addr){
        panic("BUG: This should not happen");
      }

      size_t offset = guest_addr - memory.guest_phys_addr;
      memcpy(guest_mem + offset, src, size);
      break;
    }
  }

  const struct vcpu &run(size_t index=0) {
    assert(!vcpus_vec.empty());

    const auto &vcpu = vcpus_vec.at(index);
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

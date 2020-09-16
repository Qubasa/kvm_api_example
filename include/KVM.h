#include <iostream>

using std::cout, std::cerr, std::endl;

void panic(std::string error) {
  cerr << "ERROR: " << strerror(errno) << endl;

  throw std::runtime_error(error);
}

class KVM {
  int kvm_fd;

public:
  KVM() {
    kvm_fd = open("/dev/kvm", O_RDWR);
    if (kvm_fd < 0) {
      panic("failed to open /dev/kvm");
    }
  }

  template <typename... Args> int send(int request, Args... args) {
    int ret = ioctl(kvm_fd, request, args...);
    if (ret < 0) {
      panic("failed to send request");
    }
    return ret;
  }
  template <typename... Args> int cap(int request, Args... args) {
    int ret = ioctl(kvm_fd, KVM_CHECK_EXTENSION, request, args...);
    if (ret < 0) {
      panic("failed to KVM_CHECK_EXTENSION request");
    }
    return ret;
  }

  ~KVM() {
    int ret = close(kvm_fd);

    if (ret < 0) {
      panic("failed to close kvm_fd");
    }
  }
};


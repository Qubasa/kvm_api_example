#include <iostream>

#include <cstdarg>
#include <fcntl.h>
#include <linux/kvm.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

// External include
#include <fmt/core.h>

// Local include
#include <KVM.h>


int main() {

  fmt::print("asd");

  cout << "Starting kvm api" << endl;
  KVM k = KVM();

  int vm_fd = k.send(KVM_CREATE_VM, 0);

  int max_vcpus = k.cap(KVM_CAP_NR_VCPUS);
  cout << "Max vcpus: " << max_vcpus << endl;
}

// Local include
#include <KVM.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

int main() {
  const unsigned int vm_memory = PAGE_SIZE;

  cout << "Starting kvm api" << endl;
  // Create handle to kvm
  KVM k = KVM();

  // Get max number vcpus
  cout << "Max vcpus: " << k.max_vcpus << endl;
  cout << "Max shared vcpu mem: " << hex << showbase << k.max_vcpu_mmap_size << endl;

  VM vm = VM(k, 0);

  // Setup vm memory
  vm.add_mem_pages(2);

  // Add vcpu
  vm.add_vcpu();
}

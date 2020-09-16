// Local include
#include <KVM.h>

#include <sys/ioctl.h>



int main() {

  const unsigned int vm_memory = PAGE_SIZE;

  cout << "Starting kvm api" << endl;
  KVM k = KVM();

  int vm_fd = k.send(KVM_CREATE_VM, 0);

  int max_vcpus = k.cap(KVM_CAP_NR_VCPUS);
  cout << "Max vcpus: " << max_vcpus << endl;


  size_t vcpu_mmap_size = k.send(KVM_GET_VCPU_MMAP_SIZE);
  cout << "Vcpu mmap size: " << vcpu_mmap_size << endl;
  /* char * mem = mmap(NULL, vm_memory, PROT_READ | PROT_WRITE); */

}

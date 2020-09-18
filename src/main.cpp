// Local include
#include <KVM.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

/* extern const unsigned char guest16[], guest16_end[]; */

unsigned int guest_code[] = {
0xf4
};

int main() {
  // Create handle to kvm
  KVM k = KVM();

  // Get max number vcpus
  cout << "Max vcpus: " << k.max_vcpus << endl;
  cout << "Max shared vcpu mem: " << hex << showbase << k.max_vcpu_mmap_size << endl;

  VM vm = VM(k, 0);

  // Setup vm memory
  vm.add_mem_pages(1);


  // Why though?
  vm.send(KVM_SET_TSS_ADDR, 0xfffbd000);

  // Add vcpu
  vm.add_vcpu();

  vm.set_real_mode();

  unsigned char * guest_mem = (unsigned char*) vm.memory_vec.front().userspace_addr;
  // Copy code
  guest_mem[0] = 0xf4;
  /* memcpy(guest_mem, guest_code, sizeof(guest_code)); */

  for(int i =0; i < 10; i++){
    fmt::print("{:2x}", guest_mem[i]);
  }
  cout << endl;

  while(true){
    cout << "Starting vm " << endl;
    const struct vcpu& vcpu0 = vm.run();
    cout << "Exited vm " << endl;

    unsigned int exit_reason = vcpu0.kvm_run->exit_reason;
    switch(exit_reason){
      case KVM_EXIT_HLT:
        goto check;

      // Print output written to port 0xE9
      case KVM_EXIT_IO:
        if(vcpu0.kvm_run->io.direction == KVM_EXIT_IO_OUT &&
            vcpu0.kvm_run->io.port == 0xE9){
          char *p = (char*) vcpu0.kvm_run;
          fwrite(p + vcpu0.kvm_run->io.data_offset, vcpu0.kvm_run->io.size, 1, stdout);
          fflush(stdout);
          continue;
        }

      default:
        panic("Exit reason not handled: {}", exit_reason);
    }
  }

check:
  struct kvm_regs regs;
  unsigned long memval;
	if (vm.send_vcpu(KVM_GET_REGS, &regs) < 0) {
		perror("KVM_GET_REGS");
		exit(1);
	}

	if (regs.rax != 42) {
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		return 0;
	}

  char* mem = (char*) vm.memory_vec.at(0).userspace_addr;
	memcpy(&memval, mem + 0x400, 8);
	if (memval != 42) {
		printf("Wrong result: memory at 0x400 is %lld\n",
		       (unsigned long long)memval);
		return 0;
	}

	return 1;

}

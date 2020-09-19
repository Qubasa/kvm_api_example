// Local include
#include <KVM.h>

#include <thread>

#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

/* extern const unsigned char guest16[], guest16_end[]; */

unsigned long guest_code[] = { 0xb82a00f4 };

void vm_thread(VM& vm){
  while (true) {
    cout << "Starting vm " << endl;
    const struct vcpu &vcpu0 = vm.run();
    cout << "Exited vm " << endl;

    unsigned int exit_reason = vcpu0.kvm_run->exit_reason;
    switch (exit_reason) {
    case KVM_EXIT_HLT:
      goto check;

    // Print output written to port 0xE9
    case KVM_EXIT_IO:
      if (vcpu0.kvm_run->io.direction == KVM_EXIT_IO_OUT &&
          vcpu0.kvm_run->io.port == 0xE9) {
        char *p = (char *)vcpu0.kvm_run;
        fwrite(p + vcpu0.kvm_run->io.data_offset, vcpu0.kvm_run->io.size, 1,
               stdout);
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
    exit(1);
  }

  cout << "Successfully executed vm with ax == 42" << endl;
  exit(0);
}

int main() {
  // Create handle to kvm
  KVM k = KVM();

  dbg("Max mem slots {}\n", k.max_nr_mem_slots);

  // Create vm
  VM vm = VM(k, 0);

  // Give vm one page of memory
  vm.add_mem_pages(2, 0);

  // Add vcpu
  vm.add_vcpu();

  /* vm.add_mem_pages(3, 1); */
  // Set vcpu into real mode
  vm.set_real_mode();

  // Copy hlt instruction to addr 0
  vm.copy_to_mem(0, guest_code, sizeof(guest_code));

  unsigned int* addr = (unsigned int*) (vm.mem_vec.front().userspace_addr);

  for(int i =0; i < 4; i++){
    fmt::print("{:4x}", addr[i]);
  }
  cout << endl;

  std::thread first(vm_thread, std::ref(vm));

  while(true){
    struct kvm_regs regs;
    unsigned long memval;
    vm.send_vcpu(KVM_GET_REGS, &regs);

    cout << "RIP: " << regs.rip << " RAX: " << regs.rax << endl;
  }

  first.join();
 /* char *mem = (char *)vm.mem.value().userspace_addr; */
  /* memcpy(&memval, mem + 0x400, 8); */
  /* if (memval != 42) { */
  /*   printf("Wrong result: memory at 0x400 is %lld\n", */
  /*          (unsigned long long)memval); */
  /*   return 0; */
  /* } */

  return 1;
}

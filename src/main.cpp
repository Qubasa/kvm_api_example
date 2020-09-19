// Local include
#include <KVM.h>

#include <thread>

#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

// mov ax, 42
// hlt
unsigned int guest_code[] = { 0xf4002ab8 };

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

  vm.send_vcpu(KVM_GET_REGS, &regs);

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

  // Give vm two pages of memory
  vm.add_mem_pages(2, 0);

  // Add vcpu
  vm.add_vcpu();

  // Set vcpu into real mode
  vm.set_real_mode();

  // Copy instructions to addr 0
  vm.copy_to_mem(0, guest_code, sizeof(guest_code));

  // Run vm
  // TODO: Make vm thread safe
  std::thread task(vm_thread, std::ref(vm));

  while(true){
    struct kvm_regs regs;
    unsigned long memval;
    vm.send_vcpu(KVM_GET_REGS, &regs);

    cout << "RIP: " << regs.rip << " RAX: " << regs.rax << endl;
  }

  task.join();

  return 1;
}

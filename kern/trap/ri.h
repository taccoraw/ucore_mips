#include <defs.h>
#include <trap.h>
#include <divmod.h>
#include <assert.h>
int ri_handler(struct trapframe* tf) {
    assert(((tf->tf_Cause >> 2) & 0x1f) == EXC_RI);
    uint32_t inst = *(uint32_t*)(tf->tf_EPC);
    if ((inst & 0xfc00ffff) == 0x0000001a) {  // div
        uint8_t reg1 = (inst >> 21) & 0x1f;
        uint8_t reg2 = (inst >> 16) & 0x1f;
        int* reg_base = (int*)&(tf->tf_regs.zero);
        int op1 = reg_base[reg1], op2 = reg_base[reg2];
        int ret = divmod(op1, op2, (int*)&(tf->tf_LO), (int*)&(tf->tf_HI));
        if (ret != 0) {
            cprintf("Divide By Zero!\n");
            return -1;
        }
        tf->tf_EPC += 4;
        return 0;
    } else if ((inst & 0xfc00ffff) == 0x0000001b) {  // divu
        // currently same as div
        uint8_t reg1 = (inst >> 21) & 0x1f;
        uint8_t reg2 = (inst >> 16) & 0x1f;
        int* reg_base = (int*)&(tf->tf_regs.zero);
        int op1 = reg_base[reg1], op2 = reg_base[reg2];
        int ret = divmod(op1, op2, (int*)&(tf->tf_LO), (int*)&(tf->tf_HI));
        if (ret != 0) {
            cprintf("Divide By Zero!\n");
            return -1;
        }
        tf->tf_EPC += 4;
        return 0;
    } else if ((inst & 0xfc00003f) == 0x00000034) {  // teq
        // ignore trap (noop)
        tf->tf_EPC += 4;
        return 0;
    } else if ((inst & 0xffff0000) == 0x04110000) {
        uint32_t pc = inst & 0xffff;
        if (inst & 0x8000)
            pc |= 0xffff0000;
        tf->tf_EPC = tf->tf_EPC + 4 + (pc << 2);
        return 0;
    } else
        return -1;
}


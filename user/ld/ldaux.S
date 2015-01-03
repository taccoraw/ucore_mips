.text

.globl  ld_entry
.ent    ld_entry
.type   ld_entry, @function
ld_entry:
    .set    noreorder
    lw      $a0, 0x0($sp)
    addiu   $t0, $sp, 0x8   // load delay slot
    sll     $a0, $a0, 2
    addu    $a0, $a0, $t0
1:
    lw      $t0, 0x0($a0)
    addiu   $a0, $a0, 0x4   // load delay slot
    bne     $t0, $zero, 1b

    move    $t1, $a0        // branch delay slot, ok to change t1
    li      $t2, 0x7        // ELF_AT_BASE == 0x7
1:
    lw      $t0, 0x0($t1)
    addiu   $t1, $t1, 0x8
    bne     $t0, $t2, 1b

    li      $t9, ld_main    // branch delay slot, ok to change t9
    lw      $t2, -0x4($t1)  // t1-8+4, t2 == BASE
    addiu   $sp, $sp, -0x10 // load delay slot
    addu    $t9, $t9, $t2   // `li $t9, ld_main` causes no reloc, do NOT use `la`
    jalr    $t9
    nop
    addiu   $sp, $sp, 0x10
    move    $ra, $zero
    jr      $v0
    nop
    .set    reorder
.end ld_entry

.globl ld_entry_got
.ent ld_entry_got
.type ld_entry_got, @function
ld_entry_got:
    .set    noreorder
    .cpload $t9
    .set    reorder
    sw      $a0, 0x0($sp)
    sw      $a1, 0x4($sp)
    sw      $a2, 0x8($sp)
    sw      $a3, 0xc($sp)
    addiu   $sp, $sp, -0x4
    sw      $t7, 0x0($sp)
    move    $a0, $t8
    move    $a1, $ra
    addiu   $sp, $sp, -0x10
    jal     ld_handle_got  // nop auto-inserted
    addiu   $sp, $sp, 0x10
    move    $t9, $v0
    lw      $ra, 0x0($sp)
    addiu   $sp, $sp, 0x4
    lw      $a0, 0x0($sp)
    lw      $a1, 0x4($sp)
    lw      $a2, 0x8($sp)
    lw      $a3, 0xc($sp)
    jr      $t9             // nop auto-inserted
.end ld_entry_got

.globl ld_entry_plt         // same as ld_entry_got, except `jal` target
.ent ld_entry_plt
.type ld_entry_plt, @function
ld_entry_plt:
    .set    noreorder
    .cpload $t9
    .set    reorder
    sw      $a0, 0x0($sp)
    sw      $a1, 0x4($sp)
    sw      $a2, 0x8($sp)
    sw      $a3, 0xc($sp)
    addiu   $sp, $sp, -0x4
    sw      $t7, 0x0($sp)
    move    $a0, $t8
    move    $a1, $ra
    addiu   $sp, $sp, -0x10
    jal     ld_handle_plt  // nop auto-inserted
    addiu   $sp, $sp, 0x10
    move    $t9, $v0
    lw      $ra, 0x0($sp)
    addiu   $sp, $sp, 0x4
    lw      $a0, 0x0($sp)
    lw      $a1, 0x4($sp)
    lw      $a2, 0x8($sp)
    lw      $a3, 0xc($sp)
    jr      $t9             // nop auto-inserted
.end ld_entry_plt

.globl do_syscall
.ent   do_syscall
.type  do_syscall, @function
do_syscall:
    lw      $v1, 0x10($sp)
    lw      $v0, 0x14($sp)
    syscall
    jr      $ra
.end   do_syscall
.size  do_syscall, .-do_syscall


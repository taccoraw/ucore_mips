.text
.globl _start
_start:
    # load argc and argv
    # pass through load_icode
    lw      $a0, 0x0($sp)
    addiu   $a1, $sp, 0x4
    sll     $a2, $a0, 2
    addu    $a2, $a2, $a1
    addiu   $a2, $a2, 0x4
    addiu   $sp, $sp, -0x10

    # move down the esp register
    # since it may cause page fault in backtrace
    # addiu   $sp, $sp, -0x20

    # call user-program function
    jal     umain
1:  j       1b


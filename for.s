.org 0x80000000
j _start

.include "./macros/data.s"     


.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    print_str msg 
    for i, .loop, .end 
      la t0, j
      sw 0, 0(t0)
      la t1, i
      lw t1, 0(t1)
      print_int_reg t1
      print_str ln
      for j, .loop2, .end2
        la t1, j
        lw t1, 0(t1)
        print_int_reg t1
        print_str space
      endfor j, .loop2, .end2
      print_str ln
    endfor i, .loop, .end

  halt


.data
msg: .asciz "Main file message...\n"
i: .word 0
j: .word 0

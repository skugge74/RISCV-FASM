.org 0x80000000
j _start

.include "./macros/data.s"     
.include "./macros/cond_loop.s"     


.text
.align 4
_start:
    li sp, 0x80100000
    li s1, 0x10000000
    
    print_str msg 
    for_range i, 0, 10 
      la t0, j
      sw 0, 0(t0)
      la t1, i
      lw t1, 0(t1)
      print_int_reg t1
      print_str ln
      for_range j, 0, 10
        la t1, j
        lw t1, 0(t1)
        print_int_reg t1
        print_str space
      endfor_range j
      print_str ln
    endfor_range i
  
  la t0, i
  sw 0, 0(t0)
  for_range i, 2, 5
    print_str msg
  endfor_range i
  qemu_off


.data
msg: .asciz "Main file message...\n"
i: .word 0
j: .word 0

macro print_secret
    print_str secret_msg 
endm

.data
.align 4
secret_msg:
    .asciz "SUCCESS: Inclusion logic is working!\n"

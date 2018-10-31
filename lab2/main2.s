  .syntax unified
  .cpu cortex-m4
  .thumb
.text
  .global main
  .equ N, 100

fibonacci:
    push  {r1-r3}
    mov   r1,  #0
    mov   r2,  #1
  fibloop:
      mov   r3,  r2
      adds   r2,  r1,  r2
      bvs overflow
      mov   r1,  r3
      subs   r0,  r0,  #1
      cmp   r0,  #1
      bne   fibloop
  mov   r0,  r2
  b FIBONACCI_EXIT

  overflow:
  mov r0, #-2

  FIBONACCI_EXIT:
  pop   {r1-r3}
  bx lr

fib:
  push {lr}

  cmp r0, #1
  blt out_of_bounds
  cmp r0, #100
  bgt out_of_bounds

    bl fibonacci
    mov r4, r0
    b FIB_EXIT

  out_of_bounds:
    mov r4, #-1
  FIB_EXIT:
  pop {lr}
  bx lr


main:
  movs r0, #N

  bl fib

  nop

L:   b L

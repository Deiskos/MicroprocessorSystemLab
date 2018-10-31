  .syntax unified
  .cpu cortex-m4
  .thumb
.data
  arr1: .byte 0x19, 0x34, 0x14, 0x32, 0x52, 0x23, 0x61, 0x29
  filler: .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  arr2: .byte 0x18, 0x17, 0x33, 0x16, 0xFA, 0x20, 0x55, 0xAC
.text
  .global main

do_sort:
  //r1 = swap
  //r2 = i
  //r3 = j
  //r4 = t
  //r5 = [i]
  //r6 = [i+1]
  push {r1-r6}
  mov r1, #1
  mov r3, #8

  SORT_WHILE:
    cmp r1, 0
    beq SORT_EXIT

    mov r1, 0

    mov r2, 1
    SORT_FOR:
      add r4, r0, r2
      ldrb r5, [r4]    // arr[i]

      add r4, r0, r2
      sub r4, r4, 1
      ldrb r6, [r4]    // arr[i+1]

      cmp r5, r6
      blt ELSE
        add r4, r0, r2
        strb r6, [r4]  // arr[i] = arr[i - 1];

        add r4, r0, r2
        sub r4, r4, 1
        strb r5, [r4]  // arr[i - 1] = arr[i];

        mov r1, 1

      ELSE:

      add r2, r2, 1
    cmp r2, r3
    blt SORT_FOR

    sub r3, r3, 1

  b SORT_WHILE

SORT_EXIT:
  pop {r1-r6}
  bx lr

main:
  ldr r0, =arr1
  bl do_sort

  ldr r0, =arr2
  bl do_sort

  nop

L:   b L

    .globl main
main:
    addi sp, sp, -8
    sd fp, 0(sp)
    mv fp, sp
    addi sp, sp, -0
  li a0, 0
  beqz a0, .L.else.1
  li a0, 2
  j .L.return
  j .L.end.1
.L.else 1:
.L.end.1:
  li a0, 3
  j .L.return
.L.return: 
  mv sp, fp
  ld fp, 0(sp)
  addi sp, sp, 8
    ret

	.globl context_switch 
	.even	;
context_switch:
  move.l %a6, -(%a7)
  move.l %d0, -(%a7)
  move.l #0, %d0 
  move.w %sr, %d0
  move.l %d0, -(%a7)
  move.l 16(%a7), %a6
  move.l %a6, -(%a7)
  move.l %a7, %a6
  move.l %a6, -(%a7)
  jsr context_switch_helper
  move.l (%a7)+, %a6
  move.l %a6, %a7
  addq.l #4, %a7 
  move.l #0, %d0 
  move.l (%a7)+, %d0
  move.w %d0, %sr
  move.l (%a7)+, %d0
  move.l (%a7)+, %a6
rts

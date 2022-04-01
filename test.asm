org 8000h

_main:
	call sum
	halt
	
	
sum:
	push BC
	push HL
	ld HL, num1
	ld BC, num2
	add HL, BC
	ld res, HL
	pop BC
	pop HL
	ret
	
num1:
	dw 5
num2:
	dw 5
res:
	dw 0
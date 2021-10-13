RAM1	equ	$2000

	global	vector_illegal_opcode
	global	vector_swi3
	global	vector_swi2
	global	vector_firq
	global	vector_irq
	global	vector_swi
	global	vector_nmi
	global	vector_reset

	section	TEXT

vector_illegal_opcode:

vector_swi3:
	abx
	sex
	ldx	#vector_reset
	rti

vector_swi2:

vector_firq:

vector_irq:

vector_swi:

vector_nmi:

vector_reset:
	; set stackpointers
	lds	#$1000
	ldu	#$0800

	ldb	data
	ldx	#(data+1)
	ldy	#$0042
.1	lda	,x+
	sta	,y+
	decb
	bne	.1


test:
	ldd	#$ffff
	cmpd	#$ffff
	rts

data:
	db	$06	; length of array
	db	$2a, $b5, $60, $3f, $d1, $19
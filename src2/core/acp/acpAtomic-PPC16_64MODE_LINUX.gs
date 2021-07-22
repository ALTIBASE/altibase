	.file	"ex2.c"
	.section	".toc","aw"
	.section	".text"
	.align 2
	.globl acpAtomicCas16
	.section	".opd","aw"
	.align 3
acpAtomicCas16:
	.quad	.L.acpAtomicCas16,.TOC.@tocbase
	.previous
	.type	acpAtomicCas16, @function
.L.acpAtomicCas16:
	std 31,-8(1)
	stdu 1,-80(1)
	mr 31,1
	std 3,128(31)
	mr 9,4
	mr 0,5
	stw 9,136(31)
	stw 0,144(31)
	ld 0,128(31)
	mr 9,0
	lbz 0,0(9)
	rldicl 0,0,0,56
	stw 0,48(31)
	lwz 0,48(31)
	extsw 9,0
	lwz 0,136(31)
	extsw 0,0
	cmpw 7,9,0
	bne 7,.L2
	ld 0,128(31)
	lwz 9,144(31)
	extsw 9,9
	rldicl 9,9,0,56
	mr 11,0
	stb 9,0(11)
.L2:
	lwz 0,48(31)
	extsw 0,0
	mr 3,0
	addi 1,31,80
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,0,128,1,0,1
	.size	acpAtomicCas16,.-.L.acpAtomicCas16
	.align 2
	.globl acpAtomicSet16
	.section	".opd","aw"
	.align 3
acpAtomicSet16:
	.quad	.L.acpAtomicSet16,.TOC.@tocbase
	.previous
	.type	acpAtomicSet16, @function
.L.acpAtomicSet16:
	std 31,-8(1)
	stdu 1,-80(1)
	mr 31,1
	std 3,128(31)
	mr 0,4
	stw 0,136(31)
	ld 0,128(31)
	lwz 9,136(31)
	extsw 9,9
	rldicl 9,9,0,56
	mr 11,0
	stb 9,0(11)
	li 0,0
	mr 3,0
	addi 1,31,80
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,0,128,1,0,1
	.size	acpAtomicSet16,.-.L.acpAtomicSet16
	.align 2
	.globl acpAtomicGet16
	.section	".opd","aw"
	.align 3
acpAtomicGet16:
	.quad	.L.acpAtomicGet16,.TOC.@tocbase
	.previous
	.type	acpAtomicGet16, @function
.L.acpAtomicGet16:
	std 31,-8(1)
	stdu 1,-80(1)
	mr 31,1
	std 3,128(31)
	ld 0,128(31)
	mr 9,0
	lbz 0,0(9)
	rldicl 0,0,0,56
	mr 3,0
	addi 1,31,80
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,0,128,1,0,1
	.size	acpAtomicGet16,.-.L.acpAtomicGet16
	.align 2
	.globl acpAtomicAdd16
	.section	".opd","aw"
	.align 3
acpAtomicAdd16:
	.quad	.L.acpAtomicAdd16,.TOC.@tocbase
	.previous
	.type	acpAtomicAdd16, @function
.L.acpAtomicAdd16:
	std 31,-8(1)
	stdu 1,-80(1)
	mr 31,1
	std 3,128(31)
	mr 0,4
	stw 0,136(31)
	ld 0,128(31)
	ld 9,128(31)
	lbz 9,0(9)
	rldicl 11,9,0,56
	lwz 9,136(31)
	extsw 9,9
	rldicl 9,9,0,56
	add 9,11,9
	rldicl 9,9,0,56
	mr 11,0
	stb 9,0(11)
	li 0,0
	mr 3,0
	addi 1,31,80
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,0,128,1,0,1
	.size	acpAtomicAdd16,.-.L.acpAtomicAdd16
	.ident	"GCC: (GNU) 4.4.7 20120313 (Red Hat 4.4.7-3)"
	.section	.note.GNU-stack,"",@progbits

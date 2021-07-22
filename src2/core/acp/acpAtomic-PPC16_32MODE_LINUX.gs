	.file	"ex.c"
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
	mflr 0
	std 0,16(1)
	std 31,-8(1)
	stdu 1,-144(1)
	mr 31,1
	std 3,192(31)
	mr 9,4
	mr 0,5
	stw 9,200(31)
	stw 0,208(31)
	lwz 0,200(31)
	extsw 9,0
	lwz 0,208(31)
	extsw 0,0
	ld 3,192(31)
	mr 4,9
	mr 5,0
	bl atomic_cas
	nop
	mr 0,3
	std 0,112(31)
	addi 1,31,144
	ld 0,16(1)
	mtlr 0
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,1,128,1,0,1
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
	mflr 0
	std 0,16(1)
	std 31,-8(1)
	stdu 1,-144(1)
	mr 31,1
	std 3,192(31)
	mr 0,4
	stw 0,200(31)
	lwz 0,200(31)
	extsw 0,0
	ld 3,192(31)
	mr 4,0
	bl atomic_set
	nop
	mr 0,3
	std 0,112(31)
	addi 1,31,144
	ld 0,16(1)
	mtlr 0
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,1,128,1,0,1
	.size	acpAtomicSet16,.-.L.acpAtomicSet16
	.align 2
	.globl acpAtomicAdd16
	.section	".opd","aw"
	.align 3
acpAtomicAdd16:
	.quad	.L.acpAtomicAdd16,.TOC.@tocbase
	.previous
	.type	acpAtomicAdd16, @function
.L.acpAtomicAdd16:
	mflr 0
	std 0,16(1)
	std 31,-8(1)
	stdu 1,-144(1)
	mr 31,1
	std 3,192(31)
	mr 0,4
	stw 0,200(31)
	lwz 0,200(31)
	extsw 0,0
	ld 3,192(31)
	mr 4,0
	bl atomic_add
	nop
	mr 0,3
	std 0,112(31)
	addi 1,31,144
	ld 0,16(1)
	mtlr 0
	ld 31,-8(1)
	blr
	.long 0
	.byte 0,0,0,1,128,1,0,1
	.size	acpAtomicAdd16,.-.L.acpAtomicAdd16
	.ident	"GCC: (GNU) 4.4.7 20120313 (Red Hat 4.4.7-3)"
	.section	.note.GNU-stack,"",@progbits

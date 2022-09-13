	.file	1 "hello.c"
	.section .mdebug.abi32
	.previous
	.abicalls
	.text
	.align	2
	.globl	main
	.ent	main
	.type	main, @function
main:
	.frame	$fp,24,$31		# vars= 8, regs= 1/0, args= 0, gp= 8
	.mask	0x40000000,-8
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	addiu	$sp,$sp,-24
	sw	$fp,16($sp)
	move	$fp,$sp
	sw	$0,8($fp)
	lw	$2,8($fp)
	addiu	$2,$2,1
	sw	$2,8($fp)
	move	$2,$0
	move	$sp,$fp
	lw	$fp,16($sp)
	addiu	$sp,$sp,24
	j	$31
	nop

	.set	macro
	.set	reorder
	.end	main
	.ident	"GCC: (GNU) 4.0.0 (DENX ELDK 4.1 4.0.0)"

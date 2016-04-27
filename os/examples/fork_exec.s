	.file	"fork_exec.c"
	.section	.rodata
.LC0:
	.string	"error in fork"
	.align 4
.LC1:
	.string	"in parent context pid is %d and ppid is %d\n"
.LC2:
	.string	"child with pid %d cleaned-up\n"
	.align 4
.LC3:
	.string	"normal termination and the exit status is %d\n"
	.align 4
.LC4:
	.string	"the process terminated abnormally"
	.align 4
.LC5:
	.string	"in child context pid is %d and ppid is %d\n"
.LC6:
	.string	"fork2n.c"
.LC7:
	.string	"vi"
.LC8:
	.string	"/usr/bin/vi"
.LC9:
	.string	"error in execl"
	.text
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	pushl	%ebx
	subl	$60, %esp
	call	getpid
	movl	%eax, 44(%esp)
	call	fork
	movl	%eax, 40(%esp)
	cmpl	$0, 40(%esp)
	jns	.L2
	movl	$.LC0, (%esp)
	call	perror
	movl	$1, (%esp)
	call	exit
.L2:
	cmpl	$0, 40(%esp)
	jle	.L3
	call	getppid
	movl	%eax, %ebx
	call	getpid
	movl	$.LC1, %edx
	movl	%ebx, 8(%esp)
	movl	%eax, 4(%esp)
	movl	%edx, (%esp)
	call	printf
	movl	$0, 8(%esp)
	leal	28(%esp), %eax
	movl	%eax, 4(%esp)
	movl	$-1, (%esp)
	call	waitpid
	movl	%eax, 40(%esp)
	cmpl	$0, 40(%esp)
	jle	.L4
	movl	$.LC2, %eax
	movl	40(%esp), %edx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	movl	28(%esp), %eax
	movl	$0, 32(%esp)
	movl	%eax, 32(%esp)
	movl	32(%esp), %eax
	andl	$127, %eax
	testl	%eax, %eax
	jne	.L5
	movl	28(%esp), %eax
	movl	$0, 36(%esp)
	movl	%eax, 36(%esp)
	movl	36(%esp), %eax
	andl	$65280, %eax
	movl	%eax, %edx
	sarl	$8, %edx
	movl	$.LC3, %eax
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	printf
	jmp	.L4
.L5:
	movl	$.LC4, (%esp)
	call	puts
.L4:
	call	pause
	movl	$0, (%esp)
	call	exit
.L3:
	call	getppid
	movl	%eax, %ebx
	call	getpid
	movl	$.LC5, %edx
	movl	%ebx, 8(%esp)
	movl	%eax, 4(%esp)
	movl	%edx, (%esp)
	call	printf
	call	pause
	movl	$0, 12(%esp)
	movl	$.LC6, 8(%esp)
	movl	$.LC7, 4(%esp)
	movl	$.LC8, (%esp)
	call	execl
	movl	%eax, 40(%esp)
	cmpl	$0, 40(%esp)
	jns	.L6
	movl	$.LC9, (%esp)
	call	perror
	movl	$33, (%esp)
	call	exit
.L6:
	movl	$0, (%esp)
	call	exit
	.size	main, .-main
	.ident	"GCC: (SUSE Linux) 4.5.0 20100604 [gcc-4_5-branch revision 160292]"
	.section	.comment.SUSE.OPTs,"MS",@progbits,1
	.string	"ospwg"
	.section	.note.GNU-stack,"",@progbits

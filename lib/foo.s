	.file	"foo.c"
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__SREG__ = 0x3f
__tmp_reg__ = 0
__zero_reg__ = 1
	.text
.global	foo
	.type	foo, @function
foo:
/* prologue: function */
/* frame size = 0 */
/* stack size = 0 */
.L__stack_usage = 0
	lds r24,t1
	subi r24,lo8(-(1))
	sts t1,r24
	cpi r24,lo8(100)
	brsh .L5
.L2:
	lds r24,t2
	subi r24,lo8(-(1))
	sts t2,r24
	cpi r24,lo8(62)
	brsh .L6
/* epilogue start */
	ret
.L6:
	call func2
	sts t2,__zero_reg__
/* epilogue start */
	ret
.L5:
	call func1
	sts t1,__zero_reg__
	rjmp .L2
	.size	foo, .-foo
.global	t2
	.section .bss
	.type	t2, @object
	.size	t2, 1
t2:
	.zero	1
.global	t1
	.type	t1, @object
	.size	t1, 1
t1:
	.zero	1
	.ident	"GCC: (GNU) 7.3.0"
.global __do_clear_bss

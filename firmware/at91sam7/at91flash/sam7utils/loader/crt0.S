//
// loader/crt0.S
//
// Copyright (C) 2006 Erik Gilling, all rights reserved
//
//	This program is free software; you can redistribute it and/or
//	modify it under the terms of the GNU General Public License as
//	published by the Free Software Foundation, version 2.
//
//
	.EQU	STACK, 0x201C00
		
	.code 32
	.align 2

	.global _entry
	.func	_entry
_entry:
	//  init stack
	ldr	sp, =STACK

	//  save lr
	stmfd	sp!, {lr}

	bl main

	ldmia	sp!, {r0}
	bx	r0

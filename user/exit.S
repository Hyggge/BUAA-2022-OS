#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
.text
.set noreorder
//	.set noat

LEAF(get_retval)

	j	 ra

END(get_retval)

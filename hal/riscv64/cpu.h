/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RV64 CPU related routines
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CPU_H_
#define _CPU_H_


/* CSR bits */
#define SSTATUS_SIE (1 << 1)
#define SSTATUS_FS  (3 << 13)

#define SCAUSE_INTR (1 << 63)


#ifndef __ASSEMBLY__


#define csr_set(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ __volatile__("csrs " #csr ", %0" \
							 : \
							 : "rK"(__v) \
							 : "memory"); \
		__v; \
	})


#define csr_write(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ __volatile__("csrw " #csr ", %0" \
							 : \
							 : "rK"(__v) \
							 : "memory"); \
	})


#define csr_clear(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ __volatile__("csrc " #csr ", %0" \
							 : \
							 : "rK"(__v) \
							 : "memory"); \
	})


#define csr_read(csr) \
	({ \
		register unsigned long __v; \
		__asm__ __volatile__("csrr %0, " #csr \
							 : "=r"(__v) \
							 : \
							 : "memory"); \
		__v; \
	})


static inline void hal_cpuHalt(void)
{
	__asm__ volatile("wfi");
}


static inline void hal_cpuDataStoreBarrier(void)
{
}


static inline void hal_cpuDataMemoryBarrier(void)
{
}


static inline void hal_cpuDataSyncBarrier(void)
{
}


static inline void hal_cpuInstrBarrier(void)
{
}


static inline void hal_cpuReboot(void)
{
}

#endif /* __ASSEMBLY__ */


#endif /* _CPU_H_ */

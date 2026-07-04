#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <arch/plic.h>
#include <kernel/serial.h>


extern void trap_entry();

void handle_irq()
{
	/* not implemented */
	u64 scause = csr_read(CSR_SCAUSE);

	switch (scause) {
	case TRAP_TIMER_IRQ:
		timer_irq();
		break;
	case TRAP_EXTERNAL_IRQ: {
		u32 irq = plic_hart_claim_irq(0);

		if (irq == IRQ_SERIAL) {
			serial_irq();
		}

		if (irq != 0) {
			plic_hart_complete_irq(0, irq);
		}
		break;
	}
	default:
		warn("unhandled irq: scause=%p\n", scause);
		break;
	}
}

void handle_exception()
{
	u64 scause = csr_read(CSR_SCAUSE);
	u64 stval = csr_read(CSR_STVAL);
	u64 sepc = csr_read(CSR_SEPC);

	switch (scause) {
	case EXCEPTION_INST_ACCESS_FAULT:
	case EXCEPTION_LOAD_ACCESS_FAULT:
	case EXCEPTION_STORE_ACCESS_FAULT:
	case EXCEPTION_INST_PAGE_FAULT:
	case EXCEPTION_LOAD_PAGE_FAULT:
	case EXCEPTION_STORE_PAGE_FAULT:
		error("page/access fault at pc=%p (scause=%d, stval=%p)\n",
		      sepc, scause, stval);
		break;
	default:
		error("unhandled exception (scause=%d, stval=%p, sepc=%p)\n",
		      scause, stval, sepc);
		break;
	}

	panic("unhandled exception\n");
}

void trap_setup()
{
	hart_irq_disable();
	csr_write(CSR_STVEC, (u64) trap_entry);
}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);

	if (scause & TRAP_IRQ_BIT) {
		handle_irq();
	} else {
		handle_exception();
	}
}

void hart_irq_enable()
{
	csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
	u64 sstatus = csr_read(CSR_SSTATUS);
	hart_irq_disable();
	return sstatus & CSR_SSTATUS_SIE;
}

void hart_irq_restore(u64 flags)
{
	if (flags) {
		hart_irq_enable();
	}
}

void hart_irq_disable()
{
	csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

#include <kernel/serial.h>
#include <kernel/panic.h>
#include <kernel/string.h>
#include <arch/csr.h>
#include <arch/io.h>
#include <arch/plic.h>
#include <arch/spinlock.h>

#define SERIAL_BUF_SIZE 256

struct serialdev {
	struct spinlock lock;
	char buf[SERIAL_BUF_SIZE];
	size_t len;
};

static struct serialdev dev;

static inline void serial_reg_write(u64 reg, u8 val)
{
	iowrite8(val, (void *)((u64) SERIAL_BASE + reg));
}

static inline u8 serial_reg_read(u64 reg)
{
	return ioread8((void *)((u64) SERIAL_BASE + reg));
}

void serial_init()
{
	spin_init(&dev.lock);
	dev.len = 0;

	serial_reg_write(SERIAL_FCR,
			  SERIAL_FCR_FIFO_ENABLE |
			  SERIAL_FCR_RX_FIFO_CLEAR |
			  SERIAL_FCR_TX_FIFO_CLEAR);

	serial_reg_write(SERIAL_IER, SERIAL_IER_ERBFI);

	plic_irq_set_priority(IRQ_SERIAL, 1);
	plic_hart_enable_irq(0, IRQ_SERIAL);
	plic_hart_set_threshold(0, 0);
}

void serial_irq_enable()
{
	csr_set(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_SEIE);
}

void serial_irq()
{
	while (serial_reg_read(SERIAL_LSR) & SERIAL_LSR_DTR) {
		char c = (char) serial_reg_read(SERIAL_RBR);

		spin_lock(&dev.lock);
		if (dev.len < SERIAL_BUF_SIZE) {
			dev.buf[dev.len++] = c;
		}
		spin_unlock(&dev.lock);
	}
}

size_t serial_read(char *buf)
{
	size_t size;
	u64 flags;

	flags = spin_lock_irqsave(&dev.lock);
	size = dev.len;
	memcpy(buf, dev.buf, size);
	dev.len = 0;
	spin_unlock_irqrestore(&dev.lock, flags);

	return size;
}

void serial_puts(char *str)
{
	while (*str) {
		serial_putc(*str);
		str++;
	}
}

void serial_putc(char c)
{
	while (!(serial_reg_read(SERIAL_LSR) & SERIAL_LSR_THRE)) { }
	serial_reg_write(SERIAL_THR, (u8) c);
}

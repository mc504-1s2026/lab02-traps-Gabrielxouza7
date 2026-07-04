#include <arch/timer.h>
#include <arch/csr.h>
#include <kernel/panic.h>
#include <kernel/printf.h>

static u64 boot_time;
static u64 alarm_time;
static bool alarm_pending;

u64 timer_read()
{
	return csr_read(CSR_TIME);
}

u64 timer_uptime()
{
	return (timer_read() - boot_time) / TIMER_FREQ;
}

void timer_irq_enable()
{
	boot_time = timer_read();
	alarm_pending = false;

	csr_write(CSR_STIMECMP, boot_time + TIMER_FREQ);
	csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
	csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
	alarm_time = timer_read() + secs * TIMER_FREQ;
	alarm_pending = true;
}

void timer_irq()
{
	u64 now = timer_read();

	if (alarm_pending && now >= alarm_time) {
		print("alarm\n");
		alarm_pending = false;
	}
	csr_write(CSR_STIMECMP, now + TIMER_FREQ);
}
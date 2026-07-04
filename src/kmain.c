#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];

#define LINE_BUF_SIZE   256
#define SERIAL_TMP_SIZE 256

static char line[LINE_BUF_SIZE];
static size_t line_len;

static void shell_prompt()
{
	serial_puts("> ");
}

static void shell_exec(char *cmd)
{
	if (strlen(cmd) == 0) {
		return;
	}

	if (strcmp(cmd, "uptime") == 0) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%ds\n", timer_uptime());
		serial_puts(buf);
	} else if (strncmp(cmd, "echo ", 5) == 0) {
		serial_puts(cmd + 5);
		serial_puts("\n");
	} else if (strncmp(cmd, "alarm ", 6) == 0) {
		u64 secs = strtou64(cmd + 6, 10);
		timer_set_alarm(secs);
	} else {
		serial_puts("unknown command\n");
	}
}

void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);

	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();

	info("enabling timer...\n");
	timer_irq_enable();

	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

	hart_irq_enable();

	shell_prompt();

	char tmp[SERIAL_TMP_SIZE];
	while (1) {
		size_t n = serial_read(tmp);

		for (size_t i = 0; i < n; i++) {
			char c = tmp[i];

			if (c == '\r') {
				serial_puts("\n");
				line[line_len] = '\0';
				shell_exec(line);
				line_len = 0;
				shell_prompt();
			} else {
				serial_putc(c);
				if (line_len < LINE_BUF_SIZE - 1) {
					line[line_len++] = c;
				}
			}
		}
	}
}
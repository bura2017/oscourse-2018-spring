/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/time.h>

extern int *vsys;

static void update (struct tm *time) {
    int updating = 1;
    while (updating) {
        outb(IO_RTC_CMND, RTC_AREG);
        updating = inb(IO_RTC_DATA) & RTC_UPDATE_IN_PROGRESS;
    }
    time->tm_sec = BCD2BIN(mc146818_read(RTC_SEC));
    time->tm_min = BCD2BIN(mc146818_read(RTC_MIN));
    time->tm_hour = BCD2BIN(mc146818_read(RTC_HOUR));
    time->tm_mday = BCD2BIN(mc146818_read(RTC_DAY));
    time->tm_mon = BCD2BIN(mc146818_read(RTC_MON) - 1);
    time->tm_year = BCD2BIN(mc146818_read(RTC_YEAR));
}

int gettime(void)
{
	nmi_disable();
	// LAB 12.

    struct tm time;
    struct tm curr_time;

    update (&time);
    do {
        curr_time = time;
        update (&time);
    } while ((curr_time.tm_sec != time.tm_sec) ||
            (curr_time.tm_min != time.tm_min) ||
            (curr_time.tm_hour != time.tm_hour) ||
            (curr_time.tm_mday != time.tm_mday) ||
            (curr_time.tm_mon != time.tm_mon) ||
            (curr_time.tm_year != time.tm_year));
    int t = timestamp(&time);
    nmi_enable();
	return t;
}

void
rtc_init(void)
{
	nmi_disable();
	// LAB 4

	outb(IO_RTC_CMND, NMI_LOCK | RTC_BREG);
	uint8_t b_reg = inb(IO_RTC_DATA);

	outb(IO_RTC_CMND, NMI_LOCK | RTC_BREG);
	outb(IO_RTC_DATA, b_reg | RTC_PIE);

	outb(IO_RTC_CMND, NMI_LOCK | RTC_AREG);
	uint8_t a_reg = inb(IO_RTC_DATA);
	outb(IO_RTC_CMND, NMI_LOCK | RTC_AREG);
	outb(IO_RTC_DATA, (a_reg & 0xF0) | 0xF); // rate = 0xF
    *vsys = gettime();

	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	// LAB 4
	outb(IO_RTC_CMND, RTC_CREG);
	status = inb(IO_RTC_DATA);
    *vsys = gettime();

	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}


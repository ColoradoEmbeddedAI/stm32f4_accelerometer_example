/* main.c – STM32F4DISCOVERY bare-metal IMU demo
 *
 * Streams accelerometer X/Y/Z acceleration in mg over USART2 (115200 8N1),
 * one reading every 200 ms. Idles waiting for a "START <n>" command on
 * USART2_RX, then captures exactly <n> readings before returning to idle.
 *
 * This file is identical between the real-hardware build (linked against
 * accel_real.c, real SPI1 capture from the onboard LIS3DSH/LIS302DL) and
 * the Renode build (linked against accel_synthetic.c instead, since Renode
 * has no model for either chip — see accel.h and ../CMakeLists.txt's
 * ACCEL_SOURCE option). main.c does not need to know which one it's linked
 * against.
 *
 * USART2_TX → PA2  (AF7)  ← connect USB-UART adapter here
 * USART2_RX → PA3  (AF7)  ← connect USB-UART adapter here (for START cmd)
 * LED green → PD12
 *
 * Clock: HSI 16 MHz (reset default, see hal.c).
 */

#include "accel.h"
#include "hal.h"
#include <stdint.h>

/* Parses "START <n>" (case-sensitive, one or more spaces before the count).
 * Returns 1 and sets *n on success, 0 otherwise. */
static int parse_start_cmd(const char *line, uint32_t *n)
{
    static const char kPrefix[] = "START";
    uint8_t i;
    for (i = 0; kPrefix[i] != '\0'; i++) {
        if (line[i] != kPrefix[i]) return 0;
    }
    const char *p = line + i;
    while (*p == ' ') p++;
    if (*p < '0' || *p > '9') return 0;

    uint32_t val = 0;
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (uint32_t)(*p - '0');
        p++;
    }
    *n = val;
    return 1;
}

/* ── main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    hal_init();

    uart_puts("\r\nSTM32F4-DISCOVERY IMU demo\r\n");

    accel_t chip = accel_init();
    if (chip == ACCEL_NONE) {
        while (1);
    }

    uart_puts("Format: X=<mg>  Y=<mg>  Z=<mg>  (200 ms)\r\n\r\n");

    /* Settle time: at 100 Hz ODR first sample arrives after 10 ms */
    delay_ms(50);

    const char *chip_name = accel_name(chip);
    char line[32];

    while (1) {
        uart_puts("READY (chip=");
        uart_puts(chip_name);
        uart_puts(") (Send \"START <n>\" to capture <n> samples.)\r\n");
        uart_getline(line, sizeof(line));

        uint32_t n;
        if (!parse_start_cmd(line, &n) || n == 0) {
            uart_puts("ERR unrecognized command\r\n");
            continue;
        }

        uart_puts("START ACK ");
        uart_print_int((int32_t)n);
        uart_puts(" chip=");
        uart_puts(chip_name);
        uart_puts("\r\n");

        uint32_t last_print = hal_ticks_ms();

        for (uint32_t i = 0; i < n; i++) {
            led_toggle();

            int32_t mx, my, mz;
            accel_read_mg(chip, &mx, &my, &mz);

            uart_puts("X=");
            uart_print_int(mx);
            uart_puts("  Y=");
            uart_print_int(my);
            uart_puts("  Z=");
            uart_print_int(mz);
            uart_puts(" mg\r\n");

            last_print += 200U;
            while ((int32_t)(hal_ticks_ms() - last_print) < 0);
        }

        uart_puts("DONE\r\n");
    }
}

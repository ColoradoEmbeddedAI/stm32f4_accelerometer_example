/* hal.h -- board bring-up and UART helpers shared by both accelerometer
 * drivers (accel_real.c / accel_synthetic.c) and main.c. Nothing in here is
 * accelerometer-specific: SPI1 and the CS pin are brought up separately by
 * accel_init() (see accel.h), since the synthetic build needs none of it. */
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configures the green LED (PD12), USART2 (PA2=TX/PA3=RX, 115200 8N1), and
 * SysTick (1 ms tick at HSI 16 MHz -- this project never leaves the reset
 * default clock). Must be the first call in main(). */
void hal_init(void);

/* Milliseconds elapsed since hal_init(). */
uint32_t hal_ticks_ms(void);

/* Blocking delay. */
void delay_ms(uint32_t ms);

/* Toggles the green LED (PD12) -- used as a per-sample heartbeat. */
void led_toggle(void);

/* ── UART (USART2, 115200 8N1) ──────────────────────────────────────────── */
void uart_putc(char c);
void uart_puts(const char *s);
void uart_print_hex(uint8_t n);
void uart_print_int(int32_t n);

/* Blocking readline: accumulates chars up to (maxlen-1) until CR or LF,
 * echoing each received character back so a terminal with local echo off
 * (e.g. picocom) shows what's being typed. A lone trailing LF from a prior
 * "\r\n" line ending is swallowed as an empty line so it doesn't show up as
 * a spurious blank command. */
void uart_getline(char *buf, uint16_t maxlen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HAL_H */

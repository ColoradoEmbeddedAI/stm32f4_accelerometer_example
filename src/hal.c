/* hal.c -- see hal.h. */
#include "hal.h"
#include "stm32f407.h"

/* ── Constants ──────────────────────────────────────────────────────────── */
#define SYSCLK_HZ       16000000UL
#define SYSTICK_1MS     (SYSCLK_HZ / 1000U - 1U)
/* USARTDIV = fCK / (16 × baud) = 16e6 / (16 × 115200) = 8.680
 * mantissa=8, fraction=round(0.680×16)=11 → BRR=(8<<4)|11=139=0x8B
 * actual baud = 16e6/(16×8.6875) = 115,108 (error 0.08%) */
#define USART2_BRR_VAL  139U

#define LED_PIN         12U   /* PD12 green LED */
#define UART_TX_PIN     2U    /* PA2 USART2_TX  AF7 */
#define UART_RX_PIN     3U    /* PA3 USART2_RX  AF7 */
#define UART_AF         7U

/* ── SysTick ────────────────────────────────────────────────────────────── */
static volatile uint32_t g_ticks;

void SysTick_Handler(void) { g_ticks++; }

uint32_t hal_ticks_ms(void) { return g_ticks; }

void delay_ms(uint32_t ms)
{
    uint32_t start = g_ticks;
    while ((g_ticks - start) < ms);
}

/* ── LED ────────────────────────────────────────────────────────────────── */
void led_toggle(void) { GPIOD->ODR ^= (1U << LED_PIN); }

/* ── UART ───────────────────────────────────────────────────────────────── */
void uart_putc(char c)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = (uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
    while (!(USART2->SR & USART_SR_TC));
}

static char uart_getc(void)
{
    while (!(USART2->SR & USART_SR_RXNE));
    return (char)USART2->DR;
}

void uart_getline(char *buf, uint16_t maxlen)
{
    uint16_t i = 0;
    while (1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_puts("\r\n");
            if (i == 0) continue;
            break;
        }
        uart_putc(c);
        if (i < (uint16_t)(maxlen - 1)) buf[i++] = c;
    }
    buf[i] = '\0';
}

void uart_print_hex(uint8_t n)
{
    const char *hex = "0123456789ABCDEF";
    uart_putc('0'); uart_putc('x');
    uart_putc(hex[n >> 4]);
    uart_putc(hex[n & 0xF]);
}

void uart_print_int(int32_t n)
{
    char buf[12];
    char *p = buf + sizeof(buf) - 1;
    *p = '\0';
    uint8_t neg = (n < 0);
    uint32_t u = neg ? (uint32_t)(-n) : (uint32_t)n;
    do {
        *--p = (char)('0' + u % 10);
        u /= 10;
    } while (u > 0);
    if (neg) *--p = '-';
    uart_puts(p);
}

/* ── hal_init ───────────────────────────────────────────────────────────── */
void hal_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    /* PD12: LED output */
    GPIOD->MODER &= ~(0x3U << (LED_PIN * 2));
    GPIOD->MODER |=  (GPIO_MODER_OUTPUT << (LED_PIN * 2));
    GPIOD->BSRR   = (1U << (LED_PIN + 16));  /* off */

    /* PA2: USART2_TX  AF7 */
    GPIOA->MODER   &= ~(0x3U << (UART_TX_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (UART_TX_PIN * 2));
    GPIOA->OTYPER  &= ~(1U << UART_TX_PIN);
    GPIOA->OSPEEDR |=  (0x2U << (UART_TX_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (UART_TX_PIN * 4));
    GPIOA->AFR[0]  |=  (UART_AF << (UART_TX_PIN * 4));

    /* PA3: USART2_RX  AF7 */
    GPIOA->MODER   &= ~(0x3U << (UART_RX_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (UART_RX_PIN * 2));
    GPIOA->PUPDR   &= ~(0x3U << (UART_RX_PIN * 2));
    GPIOA->PUPDR   |=  (GPIO_PUPDR_PU << (UART_RX_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (UART_RX_PIN * 4));
    GPIOA->AFR[0]  |=  (UART_AF << (UART_RX_PIN * 4));

    /* USART2: 115200 8N1 */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;
    USART2->CR1 = 0;
    USART2->BRR = USART2_BRR_VAL;
    USART2->CR2 = 0;
    USART2->CR3 = 0;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* SysTick: 1 ms at 16 MHz HSI */
    SYSTICK->LOAD = SYSTICK_1MS;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT
                  | SYSTICK_CTRL_ENABLE;
}

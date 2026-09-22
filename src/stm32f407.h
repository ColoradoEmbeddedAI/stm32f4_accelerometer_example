/* stm32f407.h – Bare-metal peripheral register definitions
 *
 * Peripherals: RCC, GPIOA, GPIOD, GPIOE, USART2, SPI1, SysTick
 * All addresses from STM32F407 reference manual RM0090.
 */

#ifndef STM32F407_H
#define STM32F407_H

#include <stdint.h>

/* ── Base addresses ─────────────────────────────────────────────────────── */
#define PERIPH_BASE         0x40000000UL
#define APB1_BASE           (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00010000UL)
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE          (AHB1_BASE  + 0x0000UL)
#define GPIOB_BASE          (AHB1_BASE  + 0x0400UL)
#define GPIOC_BASE          (AHB1_BASE  + 0x0800UL)
#define GPIOD_BASE          (AHB1_BASE  + 0x0C00UL)
#define GPIOE_BASE          (AHB1_BASE  + 0x1000UL)
#define RCC_BASE            (AHB1_BASE  + 0x3800UL)

#define SPI1_BASE           (APB2_BASE  + 0x3000UL)
#define SYSCFG_BASE         (APB2_BASE  + 0x3800UL)
#define EXTI_BASE           (APB2_BASE  + 0x3C00UL)
#define USART2_BASE         (APB1_BASE  + 0x4400UL)

#define NVIC_BASE           0xE000E100UL
#define SYSTICK_BASE        0xE000E010UL

/* ── GPIO ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_t;

#define GPIOA   ((GPIO_t *)GPIOA_BASE)
#define GPIOD   ((GPIO_t *)GPIOD_BASE)
#define GPIOE   ((GPIO_t *)GPIOE_BASE)

#define GPIO_MODER_INPUT    0x0U
#define GPIO_MODER_OUTPUT   0x1U
#define GPIO_MODER_AF       0x2U
#define GPIO_MODER_ANALOG   0x3U

#define GPIO_PUPDR_NONE     0x0U
#define GPIO_PUPDR_PU       0x1U
#define GPIO_PUPDR_PD       0x2U

/* ── RCC ────────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t          RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t          RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t          RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_t;

#define RCC     ((RCC_t *)RCC_BASE)

#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_AHB1ENR_GPIODEN     (1U << 3)
#define RCC_AHB1ENR_GPIOEEN     (1U << 4)

#define RCC_APB1ENR_USART2EN    (1U << 17)

#define RCC_APB2ENR_SPI1EN      (1U << 12)
#define RCC_APB2ENR_SYSCFGEN    (1U << 14)

/* ── SPI ────────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_t;

#define SPI1    ((SPI_t *)SPI1_BASE)

/* SPI CR1 bits */
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_BR_DIV4     (0x1U << 3)   /* fPCLK/4  */
#define SPI_CR1_BR_DIV8     (0x2U << 3)   /* fPCLK/8  */
#define SPI_CR1_BR_DIV16    (0x3U << 3)   /* fPCLK/16 */
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)

/* SPI SR bits */
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)

/* ── USART ──────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_t;

#define USART2  ((USART_t *)USART2_BASE)

#define USART_SR_TXE    (1U << 7)
#define USART_SR_TC     (1U << 6)
#define USART_SR_RXNE   (1U << 5)

#define USART_CR1_UE    (1U << 13)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)

/* ── EXTI ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_t;

#define EXTI    ((EXTI_t *)EXTI_BASE)

/* ── SYSCFG ─────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR[4];
    uint32_t          RESERVED[2];
    volatile uint32_t CMPCR;
} SYSCFG_t;

#define SYSCFG  ((SYSCFG_t *)SYSCFG_BASE)

/* ── NVIC ───────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t ISER[8];
    uint32_t          RESERVED0[24];
    volatile uint32_t ICER[8];
    uint32_t          RESERVED1[24];
    volatile uint32_t ISPR[8];
    uint32_t          RESERVED2[24];
    volatile uint32_t ICPR[8];
    uint32_t          RESERVED3[24];
    volatile uint32_t IABR[8];
    uint32_t          RESERVED4[56];
    volatile uint8_t  IP[240];
    uint32_t          RESERVED5[644];
    volatile uint32_t STIR;
} NVIC_t;

#define NVIC    ((NVIC_t *)NVIC_BASE)

#define EXTI0_IRQn      6
#define USART2_IRQn     38

static inline void NVIC_EnableIRQ(int irqn) {
    NVIC->ISER[irqn >> 5] = (1U << (irqn & 0x1F));
}

static inline void NVIC_SetPriority(int irqn, uint8_t priority) {
    NVIC->IP[irqn] = (uint8_t)(priority << 4);
}

/* ── SysTick ────────────────────────────────────────────────────────────── */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_t;

#define SYSTICK ((SysTick_t *)SYSTICK_BASE)

#define SYSTICK_CTRL_ENABLE     (1U << 0)
#define SYSTICK_CTRL_TICKINT    (1U << 1)
#define SYSTICK_CTRL_CLKSOURCE  (1U << 2)

#endif /* STM32F407_H */

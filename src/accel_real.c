/* accel_real.c -- real-hardware accelerometer driver (ACCEL_SOURCE=real).
 *
 * Reads the onboard MEMS accelerometer via SPI1. Supports both chips fitted
 * to STM32F4DISCOVERY boards:
 *   LIS3DSH  (MB997C / MB997D, WHO_AM_I = 0x3F) – 16-bit output, ±2g
 *   LIS302DL (MB997A / MB997B, WHO_AM_I = 0x3B) –  8-bit output, ±2.3g
 *
 * Fixed board wiring:
 *   SPI1_SCK  → PA5  (AF5)
 *   SPI1_MISO → PA6  (AF5)
 *   SPI1_MOSI → PA7  (AF5)
 *   CS        → PE3  (GPIO output, active-low)
 *
 * APB2 = 16 MHz (HSI, no PLL) → SPI1 clock = 4 MHz (PCLK2/4, within 10 MHz
 * limit).
 */
#include "accel.h"
#include "hal.h"
#include "stm32f407.h"

/* GPIO pins */
#define SPI_SCK_PIN     5U    /* PA5 SPI1_SCK   AF5 */
#define SPI_MISO_PIN    6U    /* PA6 SPI1_MISO  AF5 */
#define SPI_MOSI_PIN    7U    /* PA7 SPI1_MOSI  AF5 */
#define SPI_AF          5U
#define CS_PIN          3U    /* PE3 chip-select, active-low */

/* Accelerometer WHO_AM_I values */
#define WHO_AM_I_REG    0x0FU
#define LIS3DSH_ID      0x3FU
#define LIS302DL_ID     0x3BU

/* LIS3DSH registers */
#define LIS3DSH_CTRL4   0x20U   /* ODR, BDU, axis enable */
#define LIS3DSH_CTRL5   0x24U   /* bandwidth, full-scale; ST[2:1] = self-test */
#define LIS3DSH_CTRL6   0x25U   /* ADD_INC (bit 4) = SPI/I2C auto-increment   */

/* LIS302DL registers */
#define LIS302DL_CTRL1  0x20U   /* PD, data rate, axis enable */
#define LIS302DL_OUT_X  0x29U   /* 8-bit signed X */
#define LIS302DL_OUT_Y  0x2BU
#define LIS302DL_OUT_Z  0x2DU

/* SPI protocol flags (bit 7 = R/W) */
#define SPI_READ        0x80U

/* ── SPI low-level ──────────────────────────────────────────────────────── */
static uint8_t spi_xfer(uint8_t byte)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = byte;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

static void cs_low(void)  { GPIOE->BSRR = (1U << (CS_PIN + 16)); }
static void cs_high(void)
{
    while (SPI1->SR & SPI_SR_BSY);
    GPIOE->BSRR = (1U << CS_PIN);
}

static void spi_write_reg(uint8_t reg, uint8_t data)
{
    cs_low();
    spi_xfer(reg & 0x3FU);   /* write: bit7=0 */
    spi_xfer(data);
    cs_high();
}

static uint8_t spi_read_reg(uint8_t reg)
{
    cs_low();
    spi_xfer(reg | SPI_READ);
    uint8_t val = spi_xfer(0x00);
    cs_high();
    return val;
}

/* ── Hardware bring-up ──────────────────────────────────────────────────── */
static void spi_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN;
    (void)RCC->AHB1ENR;

    /* PA5: SPI1_SCK  AF5 */
    GPIOA->MODER   &= ~(0x3U << (SPI_SCK_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (SPI_SCK_PIN * 2));
    GPIOA->OTYPER  &= ~(1U << SPI_SCK_PIN);
    GPIOA->OSPEEDR |=  (0x3U << (SPI_SCK_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (SPI_SCK_PIN * 4));
    GPIOA->AFR[0]  |=  (SPI_AF << (SPI_SCK_PIN * 4));

    /* PA6: SPI1_MISO  AF5 */
    GPIOA->MODER   &= ~(0x3U << (SPI_MISO_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (SPI_MISO_PIN * 2));
    GPIOA->PUPDR   &= ~(0x3U << (SPI_MISO_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (SPI_MISO_PIN * 4));
    GPIOA->AFR[0]  |=  (SPI_AF << (SPI_MISO_PIN * 4));

    /* PA7: SPI1_MOSI  AF5 */
    GPIOA->MODER   &= ~(0x3U << (SPI_MOSI_PIN * 2));
    GPIOA->MODER   |=  (GPIO_MODER_AF << (SPI_MOSI_PIN * 2));
    GPIOA->OTYPER  &= ~(1U << SPI_MOSI_PIN);
    GPIOA->OSPEEDR |=  (0x3U << (SPI_MOSI_PIN * 2));
    GPIOA->AFR[0]  &= ~(0xFU << (SPI_MOSI_PIN * 4));
    GPIOA->AFR[0]  |=  (SPI_AF << (SPI_MOSI_PIN * 4));

    /* PE3: CS output, default high (deselected) */
    GPIOE->MODER  &= ~(0x3U << (CS_PIN * 2));
    GPIOE->MODER  |=  (GPIO_MODER_OUTPUT << (CS_PIN * 2));
    GPIOE->OTYPER &= ~(1U << CS_PIN);
    GPIOE->BSRR    = (1U << CS_PIN);
}

static void spi1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->CR1 = 0;
    /* Mode 3 (CPOL=1, CPHA=1), master, PCLK/4 = 4 MHz, software CS,
     * 8-bit data frame, MSB first */
    SPI1->CR1 = SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV4
              | SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ── accel.h interface ──────────────────────────────────────────────────── */
const char *accel_name(accel_t chip)
{
    switch (chip) {
        case ACCEL_LIS3DSH:  return "LIS3DSH";
        case ACCEL_LIS302DL: return "LIS302DL";
        default:              return "NONE";
    }
}

accel_t accel_init(void)
{
    spi_gpio_init();
    spi1_init();

    uint8_t id = spi_read_reg(WHO_AM_I_REG);

    if (id == LIS3DSH_ID) {
        /* 100 Hz ODR, BDU=0 (continuous), X/Y/Z enabled */
        spi_write_reg(LIS3DSH_CTRL4, 0x67U);
        /* ±2g full scale, no self-test */
        spi_write_reg(LIS3DSH_CTRL5, 0x00U);
        /* ADD_INC=1 (bit 4): enables register auto-increment on multi-byte SPI */
        spi_write_reg(LIS3DSH_CTRL6, 0x10U);

        /* Brief self-test pulse to apply electrostatic force to proof mass.
         * This can dislodge a stuck (stiction) proof mass. */
        delay_ms(20);
        spi_write_reg(LIS3DSH_CTRL5, 0x02U);   /* ST+ on */
        delay_ms(20);
        spi_write_reg(LIS3DSH_CTRL5, 0x00U);   /* ST off */
        delay_ms(10);

        uint8_t ctrl4 = spi_read_reg(LIS3DSH_CTRL4);
        uart_puts("Chip: LIS3DSH  WHO_AM_I=");
        uart_print_hex(LIS3DSH_ID);
        uart_puts("  CTRL_REG4=");
        uart_print_hex(ctrl4);
        uart_puts(ctrl4 == 0x67U ? " OK\r\n" : " !! EXPECTED 0x67 -- SPI write failed\r\n");

        return ACCEL_LIS3DSH;
    }

    if (id == LIS302DL_ID) {
        /* Active mode, 100 Hz, X/Y/Z enabled */
        spi_write_reg(LIS302DL_CTRL1, 0x47U);

        uart_puts("Chip: LIS302DL  WHO_AM_I=");
        uart_print_hex(LIS302DL_ID);
        uart_puts("\r\n");

        return ACCEL_LIS302DL;
    }

    uart_puts("ERROR: accelerometer not found (check SPI wiring)\r\n");
    return ACCEL_NONE;
}

void accel_read_mg(accel_t chip, int32_t *mx, int32_t *my, int32_t *mz)
{
    if (chip == ACCEL_LIS3DSH) {
        /* No STATUS polling — reading STATUS clears ZYXDA on LIS3DSH.
         * At 200 ms read interval and 100 Hz ODR there is always fresh data. */
        int16_t rx = (int16_t)((uint16_t)spi_read_reg(0x29U) << 8 | spi_read_reg(0x28U));
        int16_t ry = (int16_t)((uint16_t)spi_read_reg(0x2BU) << 8 | spi_read_reg(0x2AU));
        int16_t rz = (int16_t)((uint16_t)spi_read_reg(0x2DU) << 8 | spi_read_reg(0x2CU));
        *mx = (int32_t)rx * 6 / 100;
        *my = (int32_t)ry * 6 / 100;
        *mz = (int32_t)rz * 6 / 100;
    } else {
        int8_t rx = (int8_t)spi_read_reg(LIS302DL_OUT_X);
        int8_t ry = (int8_t)spi_read_reg(LIS302DL_OUT_Y);
        int8_t rz = (int8_t)spi_read_reg(LIS302DL_OUT_Z);
        /* sensitivity ≈ 18 mg/LSB at ±2.3g */
        *mx = (int32_t)rx * 18;
        *my = (int32_t)ry * 18;
        *mz = (int32_t)rz * 18;
    }
}

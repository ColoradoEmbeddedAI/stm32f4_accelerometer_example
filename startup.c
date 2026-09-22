/* startup.c – Bare-metal startup for STM32F407VG */

#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
int  main(void);

static void Default_Handler(void) { while (1); }

#define WEAK_ALIAS(name) \
    void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_ALIAS(NMI_Handler);
WEAK_ALIAS(HardFault_Handler);
WEAK_ALIAS(MemManage_Handler);
WEAK_ALIAS(BusFault_Handler);
WEAK_ALIAS(UsageFault_Handler);
WEAK_ALIAS(SVC_Handler);
WEAK_ALIAS(DebugMon_Handler);
WEAK_ALIAS(PendSV_Handler);
WEAK_ALIAS(SysTick_Handler);

WEAK_ALIAS(WWDG_IRQHandler);
WEAK_ALIAS(PVD_IRQHandler);
WEAK_ALIAS(TAMP_STAMP_IRQHandler);
WEAK_ALIAS(RTC_WKUP_IRQHandler);
WEAK_ALIAS(FLASH_IRQHandler);
WEAK_ALIAS(RCC_IRQHandler);
WEAK_ALIAS(EXTI0_IRQHandler);
WEAK_ALIAS(EXTI1_IRQHandler);
WEAK_ALIAS(EXTI2_IRQHandler);
WEAK_ALIAS(EXTI3_IRQHandler);
WEAK_ALIAS(EXTI4_IRQHandler);
WEAK_ALIAS(DMA1_Stream0_IRQHandler);
WEAK_ALIAS(DMA1_Stream1_IRQHandler);
WEAK_ALIAS(DMA1_Stream2_IRQHandler);
WEAK_ALIAS(DMA1_Stream3_IRQHandler);
WEAK_ALIAS(DMA1_Stream4_IRQHandler);
WEAK_ALIAS(DMA1_Stream5_IRQHandler);
WEAK_ALIAS(DMA1_Stream6_IRQHandler);
WEAK_ALIAS(ADC_IRQHandler);
WEAK_ALIAS(CAN1_TX_IRQHandler);
WEAK_ALIAS(CAN1_RX0_IRQHandler);
WEAK_ALIAS(CAN1_RX1_IRQHandler);
WEAK_ALIAS(CAN1_SCE_IRQHandler);
WEAK_ALIAS(EXTI9_5_IRQHandler);
WEAK_ALIAS(TIM1_BRK_TIM9_IRQHandler);
WEAK_ALIAS(TIM1_UP_TIM10_IRQHandler);
WEAK_ALIAS(TIM1_TRG_COM_TIM11_IRQHandler);
WEAK_ALIAS(TIM1_CC_IRQHandler);
WEAK_ALIAS(TIM2_IRQHandler);
WEAK_ALIAS(TIM3_IRQHandler);
WEAK_ALIAS(TIM4_IRQHandler);
WEAK_ALIAS(I2C1_EV_IRQHandler);
WEAK_ALIAS(I2C1_ER_IRQHandler);
WEAK_ALIAS(I2C2_EV_IRQHandler);
WEAK_ALIAS(I2C2_ER_IRQHandler);
WEAK_ALIAS(SPI1_IRQHandler);
WEAK_ALIAS(SPI2_IRQHandler);
WEAK_ALIAS(USART1_IRQHandler);
WEAK_ALIAS(USART2_IRQHandler);
WEAK_ALIAS(USART3_IRQHandler);
WEAK_ALIAS(EXTI15_10_IRQHandler);
WEAK_ALIAS(RTC_Alarm_IRQHandler);
WEAK_ALIAS(OTG_FS_WKUP_IRQHandler);
WEAK_ALIAS(TIM8_BRK_TIM12_IRQHandler);
WEAK_ALIAS(TIM8_UP_TIM13_IRQHandler);
WEAK_ALIAS(TIM8_TRG_COM_TIM14_IRQHandler);
WEAK_ALIAS(TIM8_CC_IRQHandler);
WEAK_ALIAS(DMA1_Stream7_IRQHandler);
WEAK_ALIAS(FSMC_IRQHandler);
WEAK_ALIAS(SDIO_IRQHandler);
WEAK_ALIAS(TIM5_IRQHandler);
WEAK_ALIAS(SPI3_IRQHandler);
WEAK_ALIAS(UART4_IRQHandler);
WEAK_ALIAS(UART5_IRQHandler);
WEAK_ALIAS(TIM6_DAC_IRQHandler);
WEAK_ALIAS(TIM7_IRQHandler);
WEAK_ALIAS(DMA2_Stream0_IRQHandler);
WEAK_ALIAS(DMA2_Stream1_IRQHandler);
WEAK_ALIAS(DMA2_Stream2_IRQHandler);
WEAK_ALIAS(DMA2_Stream3_IRQHandler);
WEAK_ALIAS(DMA2_Stream4_IRQHandler);
WEAK_ALIAS(ETH_IRQHandler);
WEAK_ALIAS(ETH_WKUP_IRQHandler);
WEAK_ALIAS(CAN2_TX_IRQHandler);
WEAK_ALIAS(CAN2_RX0_IRQHandler);
WEAK_ALIAS(CAN2_RX1_IRQHandler);
WEAK_ALIAS(CAN2_SCE_IRQHandler);
WEAK_ALIAS(OTG_FS_IRQHandler);
WEAK_ALIAS(DMA2_Stream5_IRQHandler);
WEAK_ALIAS(DMA2_Stream6_IRQHandler);
WEAK_ALIAS(DMA2_Stream7_IRQHandler);
WEAK_ALIAS(USART6_IRQHandler);
WEAK_ALIAS(I2C3_EV_IRQHandler);
WEAK_ALIAS(I2C3_ER_IRQHandler);
WEAK_ALIAS(OTG_HS_EP1_OUT_IRQHandler);
WEAK_ALIAS(OTG_HS_EP1_IN_IRQHandler);
WEAK_ALIAS(OTG_HS_WKUP_IRQHandler);
WEAK_ALIAS(OTG_HS_IRQHandler);
WEAK_ALIAS(DCMI_IRQHandler);
WEAK_ALIAS(CRYP_IRQHandler);
WEAK_ALIAS(HASH_RNG_IRQHandler);
WEAK_ALIAS(FPU_IRQHandler);

typedef void (*isr_t)(void);

__attribute__((section(".isr_vector"), used))
static const isr_t vector_table[] = {
    (isr_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FSMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    CRYP_IRQHandler,
    HASH_RNG_IRQHandler,
    FPU_IRQHandler,
};

void Reset_Handler(void)
{
    /* Enable FPU (CP10 + CP11 full access) — must precede any data copy */
    *((volatile uint32_t *)0xE000ED88U) |= (0xFU << 20);
    __asm volatile ("dsb");
    __asm volatile ("isb");

    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    main();
    while (1);
}

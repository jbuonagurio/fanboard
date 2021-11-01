//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "Board.h"

#include <stddef.h>
#include <stdint.h>

#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_types.h>

#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/gpio.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/spi.h>
#include <ti/devices/cc32xx/driverlib/timer.h>
#include <ti/devices/cc32xx/driverlib/uart.h>
#include <ti/devices/cc32xx/driverlib/udma.h>
#include <ti/devices/cc32xx/driverlib/wdt.h>

#include <ti/drivers/crypto/CryptoCC32XX.h>
#include <ti/drivers/dma/UDMACC32XX.h>
#include <ti/drivers/gpio/GPIOCC32XX.h>
#include <ti/drivers/i2c/I2CCC32XX.h>
#include <ti/drivers/power/PowerCC32XX.h>
#include <ti/drivers/pwm/PWMTimerCC32XX.h>
#include <ti/drivers/spi/SPICC32XXDMA.h>
#include <ti/drivers/timer/TimerCC32XX.h>
#include <ti/drivers/uart/UARTCC32XX.h>
#include <ti/drivers/watchdog/WatchdogCC32XX.h>
#include <ti/drivers/net/wifi/simplelink.h>

#include <ti/drivers/Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/ITM.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/PWM.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/apps/LED.h>

//--------------------------------------------------------------------
// Crypto
//--------------------------------------------------------------------

static CryptoCC32XX_Object cryptoCC32XXObjects[BOARD_CRYPTOCOUNT];

const CryptoCC32XX_Config CryptoCC32XX_config[BOARD_CRYPTOCOUNT] = {
    {
        .object = &cryptoCC32XXObjects[BOARD_CRYPTO0]
    }
};

const uint_least8_t CryptoCC32XX_count = BOARD_CRYPTOCOUNT;

//--------------------------------------------------------------------
// DMA
//--------------------------------------------------------------------

// Ensure DMA control table is aligned as required by the uDMA Hardware
static tDMAControlTable dmaControlTable[64] __attribute__ ((aligned (1024)));

// Handler for the uDMA error interrupt.
static void dmaErrorFxn(uintptr_t arg)
{
    __attribute__((unused)) int status = MAP_uDMAErrorStatusGet();
    MAP_uDMAErrorStatusClear();
    while (1);
}

static UDMACC32XX_Object udmaCC32XXObject;

static const UDMACC32XX_HWAttrs udmaCC32XXHWAttrs = {
    .controlBaseAddr = (void *)dmaControlTable,
    .dmaErrorFxn = (UDMACC32XX_ErrorFxn)dmaErrorFxn,
    .intNum = INT_UDMAERR,
    .intPriority = (~0)
};

const UDMACC32XX_Config UDMACC32XX_config = {
    .object  = &udmaCC32XXObject,
    .hwAttrs = &udmaCC32XXHWAttrs
};

//--------------------------------------------------------------------
// GPIO
//--------------------------------------------------------------------

GPIO_PinConfig gpioPinConfigs[] = {
    GPIOCC32XX_GPIO_09 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW, // GPIO09, Yellow LED
    GPIOCC32XX_GPIO_10 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW, // GPIO10: Blue LED
};

GPIO_CallbackFxn gpioCallbackFunctions[] = {};

const GPIOCC32XX_Config GPIOCC32XX_config = {
    .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
    .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    .numberOfCallbacks = 0,
    .intPriority = (~0)
};

//--------------------------------------------------------------------
// I2C
//--------------------------------------------------------------------

I2CCC32XX_Object i2cCC32XXObjects[BOARD_I2CCOUNT];

const I2CCC32XX_HWAttrsV1 i2cCC32XXHWAttrs[BOARD_I2CCOUNT] = {
    {
        .baseAddr = I2CA0_BASE,
        .intNum = INT_I2CA0,
        .intPriority = (~0),
        .sclTimeout = 0x0,
        .clkPin  = I2CCC32XX_PIN_05_I2C_SCL,
        .dataPin = I2CCC32XX_PIN_06_I2C_SDA
    }
};

const I2C_Config I2C_config[BOARD_I2CCOUNT] = {
    {
        .object = &i2cCC32XXObjects[BOARD_I2C0],
        .hwAttrs = &i2cCC32XXHWAttrs[BOARD_I2C0]
    }
};

const uint_least8_t I2C_count = BOARD_I2CCOUNT;

//--------------------------------------------------------------------
// ITM
//--------------------------------------------------------------------

// NRZ (UART) encoding, 4 Mbaud
static const ITM_HWAttrs itmCC32XXHWAttrs = {
    .format = ITM_TPIU_SWO_UART,
    .tpiuPrescaler = 19,        // 80000000 / 4000000 - 1 = 19
    .fullPacketInCycles = 640,  // 32 * 80000000 / 4000000 = 640
    .traceEnable = 0xFFFFFFFF,
};

void *itmHwAttrs = (void *)&itmCC32XXHWAttrs;

extern void ITM_commonFlush(void);
extern void ITM_commonRestore(void);

void ITM_flush(void) { ITM_commonFlush(); }
void ITM_restore(void) { ITM_commonRestore(); }

//--------------------------------------------------------------------
// Power
//--------------------------------------------------------------------

// This table defines the parking state to be set for each parkable pin
// during LPDS. (Device resources must be parked during LPDS to achieve maximum
// power savings.)  If the pin should be left unparked, specify the state
// PowerCC32XX_DONT_PARK.  For example, for a UART TX pin, the device
// will automatically park the pin in a high state during transition to LPDS,
// so the Power Manager does not need to explictly park the pin.  So the
// corresponding entries in this table should indicate PowerCC32XX_DONT_PARK.
//
static PowerCC32XX_ParkInfo parkInfo[] = {
    {PowerCC32XX_PIN01, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO10 (GT_PWM06)
    {PowerCC32XX_PIN02, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO11 (GT_PWM07)
    {PowerCC32XX_PIN03, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO12
    {PowerCC32XX_PIN04, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO13
    {PowerCC32XX_PIN05, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO14 (I2C_SCL)
    {PowerCC32XX_PIN06, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO15 (I2C_SDA)
    {PowerCC32XX_PIN07, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO16
    {PowerCC32XX_PIN08, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO17
    {PowerCC32XX_PIN13, PowerCC32XX_WEAK_PULL_DOWN_STD}, // FLASH_SPI_DIN
    {PowerCC32XX_PIN15, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO22
    {PowerCC32XX_PIN16, PowerCC32XX_WEAK_PULL_DOWN_STD}, // TDI (JTAG)
    {PowerCC32XX_PIN17, PowerCC32XX_WEAK_PULL_DOWN_STD}, // TDO (JTAG)
    {PowerCC32XX_PIN18, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO28
    {PowerCC32XX_PIN19, PowerCC32XX_WEAK_PULL_DOWN_STD}, // TCK (JTAG)
    {PowerCC32XX_PIN20, PowerCC32XX_WEAK_PULL_DOWN_STD}, // TMS (JTAG)
    {PowerCC32XX_PIN21, PowerCC32XX_WEAK_PULL_DOWN_STD}, // SOP2
    {PowerCC32XX_PIN29, PowerCC32XX_WEAK_PULL_DOWN_STD}, // ANTSEL1
    {PowerCC32XX_PIN30, PowerCC32XX_WEAK_PULL_DOWN_STD}, // ANTSEL2
    {PowerCC32XX_PIN45, PowerCC32XX_WEAK_PULL_DOWN_STD}, // DCDC_ANA2_SW_P
    {PowerCC32XX_PIN50, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO0
    {PowerCC32XX_PIN52, PowerCC32XX_WEAK_PULL_DOWN_STD}, // RTC_XTAL_N
    {PowerCC32XX_PIN53, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO30
    {PowerCC32XX_PIN55, PowerCC32XX_WEAK_PULL_UP_STD  }, // GPIO1 (UART0_TX)
    {PowerCC32XX_PIN57, PowerCC32XX_WEAK_PULL_UP_STD  }, // GPIO2 (UART0_RX)
    {PowerCC32XX_PIN58, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO3 (UART1_TX)
    {PowerCC32XX_PIN59, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO4 (UART1_RX)
    {PowerCC32XX_PIN60, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO5
    {PowerCC32XX_PIN61, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO6
    {PowerCC32XX_PIN62, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO7
    {PowerCC32XX_PIN63, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO8
    {PowerCC32XX_PIN64, PowerCC32XX_WEAK_PULL_DOWN_STD}, // GPIO9 (GT_PWM05)
};

const PowerCC32XX_ConfigV1 PowerCC32XX_config = {
    .policyInitFxn = &PowerCC32XX_initPolicy,
    .policyFxn = &PowerCC32XX_sleepPolicy,
    .enterLPDSHookFxn = NULL,
    .resumeLPDSHookFxn = NULL,
    .enablePolicy = false,
    .enableGPIOWakeupLPDS = true,
    .enableGPIOWakeupShutdown = true,
    .enableNetworkWakeupLPDS = true,
    .wakeupGPIOSourceLPDS = PRCM_LPDS_GPIO13,
    .wakeupGPIOTypeLPDS = PRCM_LPDS_FALL_EDGE,
    .wakeupGPIOFxnLPDS = NULL,
    .wakeupGPIOFxnLPDSArg = 0,
    .wakeupGPIOSourceShutdown = PRCM_HIB_GPIO13,
    .wakeupGPIOTypeShutdown = PRCM_HIB_RISE_EDGE,
    .ramRetentionMaskLPDS = PRCM_SRAM_COL_1 | PRCM_SRAM_COL_2 | PRCM_SRAM_COL_3 | PRCM_SRAM_COL_4,
    .latencyForLPDS = 20000,
    .keepDebugActiveDuringLPDS = false,
    .ioRetentionShutdown = PRCM_IO_RET_GRP_0 | PRCM_IO_RET_GRP_1 | PRCM_IO_RET_GRP_2 | PRCM_IO_RET_GRP_3,
    .pinParkDefs = parkInfo,
    .numPins = sizeof(parkInfo) / sizeof(PowerCC32XX_ParkInfo)
};

//--------------------------------------------------------------------
// PWM
//--------------------------------------------------------------------

static PWMTimerCC32XX_Object pwmTimerCC32XXObjects[BOARD_PWMCOUNT];

static const PWMTimerCC32XX_HWAttrsV2 pwmTimerCC32XXHWAttrs[BOARD_PWMCOUNT] = {
    {
        .pwmPin = PWMTimerCC32XX_PIN_64, // GPIO09, uses Timer2B for PWM.
    },
    {
        .pwmPin = PWMTimerCC32XX_PIN_01, // GPIO10, uses Timer3A for PWM.
    }
};

const PWM_Config PWM_config[BOARD_PWMCOUNT] = {
    {
        .fxnTablePtr = &PWMTimerCC32XX_fxnTable,
        .object = &pwmTimerCC32XXObjects[BOARD_PWM0],
        .hwAttrs = &pwmTimerCC32XXHWAttrs[BOARD_PWM0]
    },
    {
        .fxnTablePtr = &PWMTimerCC32XX_fxnTable,
        .object = &pwmTimerCC32XXObjects[BOARD_PWM1],
        .hwAttrs = &pwmTimerCC32XXHWAttrs[BOARD_PWM1]
    }
};

const uint_least8_t PWM_count = BOARD_PWMCOUNT;

//--------------------------------------------------------------------
// LED
//--------------------------------------------------------------------

static LED_Object ledObjects[BOARD_LEDCOUNT];

static const LED_HWAttrs ledHWAttrs[BOARD_LEDCOUNT] = {
    {
        .index = BOARD_PWM0,
        .type = LED_PWM_CONTROLLED
    },
    {
        .index = BOARD_PWM1,
        .type = LED_PWM_CONTROLLED
    }
};

const LED_Config LED_config[BOARD_LEDCOUNT] = {
    {
        .object = &ledObjects[BOARD_LED0],
        .hwAttrs = &ledHWAttrs[BOARD_LED0]
    },
    {
        .object = &ledObjects[BOARD_LED1],
        .hwAttrs = &ledHWAttrs[BOARD_LED1]
    }
};

const uint_least8_t LED_count = BOARD_LEDCOUNT;

//--------------------------------------------------------------------
// SPI
//--------------------------------------------------------------------

static SPICC32XXDMA_Object spiCC32XXDMAObjects[BOARD_SPICOUNT];

static uint32_t spiCC32XXDMAscratchBuf[BOARD_SPICOUNT];

static const SPICC32XXDMA_HWAttrsV1 spiCC32XXDMAHWAttrs[BOARD_SPICOUNT] = {
    {
        .baseAddr = LSPI_BASE,
        .intNum = INT_LSPI,
        .intPriority = (~0),
        .spiPRCM = PRCM_LSPI,
        .csControl = SPI_SW_CTRL_CS,
        .csPolarity = SPI_CS_ACTIVEHIGH,
        .pinMode = SPI_4PIN_MODE,
        .turboMode = SPI_TURBO_OFF,
        .scratchBufPtr = &spiCC32XXDMAscratchBuf[BOARD_SPI0],
        .defaultTxBufValue = 0,
        .rxChannelIndex = UDMA_CH12_LSPI_RX,
        .txChannelIndex = UDMA_CH13_LSPI_TX,
        .minDmaTransferSize = 100,
        .mosiPin = SPICC32XXDMA_PIN_NO_CONFIG,
        .misoPin = SPICC32XXDMA_PIN_NO_CONFIG,
        .clkPin  = SPICC32XXDMA_PIN_NO_CONFIG,
        .csPin  = SPICC32XXDMA_PIN_NO_CONFIG
    },
    {
        .baseAddr = GSPI_BASE,
        .intNum = INT_GSPI,
        .intPriority = (~0),
        .spiPRCM = PRCM_GSPI,
        .csControl = SPI_HW_CTRL_CS,
        .csPolarity = SPI_CS_ACTIVELOW,
        .pinMode = SPI_4PIN_MODE,
        .turboMode = SPI_TURBO_OFF,
        .scratchBufPtr = &spiCC32XXDMAscratchBuf[BOARD_SPI1],
        .defaultTxBufValue = 0,
        .rxChannelIndex = UDMA_CH6_GSPI_RX,
        .txChannelIndex = UDMA_CH7_GSPI_TX,
        .minDmaTransferSize = 10,
        .mosiPin = SPICC32XXDMA_PIN_07_MOSI,
        .misoPin = SPICC32XXDMA_PIN_06_MISO,
        .clkPin  = SPICC32XXDMA_PIN_05_CLK,
        .csPin  = SPICC32XXDMA_PIN_08_CS
    }
};

const SPI_Config SPI_config[BOARD_SPICOUNT] = {
    {
        .fxnTablePtr = &SPICC32XXDMA_fxnTable,
        .object = &spiCC32XXDMAObjects[BOARD_SPI0],
        .hwAttrs = &spiCC32XXDMAHWAttrs[BOARD_SPI0]
    },
    {
        .fxnTablePtr = &SPICC32XXDMA_fxnTable,
        .object = &spiCC32XXDMAObjects[BOARD_SPI1],
        .hwAttrs = &spiCC32XXDMAHWAttrs[BOARD_SPI1]
    }
};

const uint_least8_t SPI_count = BOARD_SPICOUNT;

//--------------------------------------------------------------------
// Timer
//--------------------------------------------------------------------

static TimerCC32XX_Object timerCC32XXObjects[BOARD_TIMERCOUNT];

static const TimerCC32XX_HWAttrs timerCC32XXHWAttrs[BOARD_TIMERCOUNT] = {
    {
        .baseAddress = TIMERA0_BASE,
        .subTimer = TimerCC32XX_timer32,
        .intNum = INT_TIMERA0A,
        .intPriority = ~0
    },
    {
        .baseAddress = TIMERA1_BASE,
        .subTimer = TimerCC32XX_timer16A,
        .intNum = INT_TIMERA1A,
        .intPriority = ~0
    },
    {
        .baseAddress = TIMERA1_BASE,
        .subTimer = TimerCC32XX_timer16B,
        .intNum = INT_TIMERA1B,
        .intPriority = ~0
    }
};

const Timer_Config Timer_config[BOARD_TIMERCOUNT] = {
    {
        .object = &timerCC32XXObjects[BOARD_TIMER0],
        .hwAttrs = &timerCC32XXHWAttrs[BOARD_TIMER0]
    },
    {
        .object = &timerCC32XXObjects[BOARD_TIMER1],
        .hwAttrs = &timerCC32XXHWAttrs[BOARD_TIMER1]
    },
    {
        .object = &timerCC32XXObjects[BOARD_TIMER2],
        .hwAttrs = &timerCC32XXHWAttrs[BOARD_TIMER2]
    }
};

const uint_least8_t Timer_count = BOARD_TIMERCOUNT;

//--------------------------------------------------------------------
// UART
//--------------------------------------------------------------------

unsigned char uartRingBuffer0[32];

static UARTCC32XX_Object uartCC32XXObjects[BOARD_UARTCOUNT];

static const UARTCC32XX_HWAttrsV1 uartCC32XXHWAttrs[BOARD_UARTCOUNT] = {
    {
        .baseAddr = UARTA0_BASE,
        .intNum = INT_UARTA0,
        .intPriority = (~0),
        .flowControl = UARTCC32XX_FLOWCTRL_NONE,
        .ringBufPtr  = uartRingBuffer0,
        .ringBufSize = sizeof(uartRingBuffer0),
        .rxPin = UARTCC32XX_PIN_57_UART0_RX, // GPIO02, Module Pin 47
        .txPin = UARTCC32XX_PIN_55_UART0_TX, // GPIO01, Module Pin 46
        .ctsPin = UARTCC32XX_PIN_UNASSIGNED,
        .rtsPin = UARTCC32XX_PIN_UNASSIGNED,
        .errorFxn = NULL
    }
};

const UART_Config UART_config[BOARD_UARTCOUNT] = {
    {
        .fxnTablePtr = &UARTCC32XX_fxnTable,
        .object = &uartCC32XXObjects[BOARD_UART0],
        .hwAttrs = &uartCC32XXHWAttrs[BOARD_UART0]
    }
};

const uint_least8_t UART_count = BOARD_UARTCOUNT;

//--------------------------------------------------------------------
// Watchdog
//--------------------------------------------------------------------

static WatchdogCC32XX_Object watchdogCC32XXObjects[BOARD_WATCHDOGCOUNT];

static const WatchdogCC32XX_HWAttrs watchdogCC32XXHWAttrs[BOARD_WATCHDOGCOUNT] = {
    {
        .baseAddr = WDT_BASE,
        .intNum = INT_WDT,
        .intPriority = 0x20,
        .reloadValue = 80000000 // 1 second period at 80 MHz CPU clock
    }
};

const Watchdog_Config Watchdog_config[BOARD_WATCHDOGCOUNT] = {
    {
        .fxnTablePtr = &WatchdogCC32XX_fxnTable,
        .object = &watchdogCC32XXObjects[BOARD_WATCHDOG0],
        .hwAttrs = &watchdogCC32XXHWAttrs[BOARD_WATCHDOG0]
    }
};

const uint_least8_t Watchdog_count = BOARD_WATCHDOGCOUNT;

//--------------------------------------------------------------------
// Board
//--------------------------------------------------------------------

// Perform any board-specific initialization needed at startup.
void InitializeBoard(void)
{
    PRCMCC3200MCUInit();
    Power_init();

    // Enable DTHE peripheral clocks for CRC.
    Power_setDependency(PowerCC32XX_PERIPH_DTHE);
    PRCMPeripheralReset(PRCM_DTHE);

    // Initialize peripherals.
    SPI_init();
    Timer_init();
    UART_init();
    LED_init();
}

// This structure prevents the CC32XXSF bootloader from overwriting the
// internal FLASH; this allows us to flash a program that will not be
// overwritten by the bootloader with the encrypted program saved in
// "secure/serial flash".
//
// This structure must be placed at the beginning of internal FLASH (so
// the bootloader is able to recognize that it should not overwrite
// internal FLASH).
//
#if defined(__SF_DEBUG__)
__attribute__((section(".dbghdr"))) const uint32_t Board_debugHeader[] = {
    0x5AA5A55A,
    0x000FF800,
    0xEFA3247D
};
#endif

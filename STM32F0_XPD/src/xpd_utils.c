/**
  ******************************************************************************
  * @file    xpd_utils.c
  * @author  Benedek Kupper
  * @version V0.2
  * @date    2017-04-26
  * @brief   STM32 eXtensible Peripheral Drivers Utilities Module
  *
  *  This file is part of STM32_XPD.
  *
  *  STM32_XPD is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  *
  *  STM32_XPD is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with STM32_XPD.  If not, see <http://www.gnu.org/licenses/>.
  */
#include "xpd_utils.h"
#include "xpd_rcc.h"
#include "xpd_core.h"
#include "xpd_flash.h"

extern uint32_t SystemCoreClock;

/** @addtogroup XPD_Utils
 * @{ */

/** @defgroup XPD_Exported_Functions XPD Exported Functions
 * @{ */

/** @defgroup XPD_Exported_Functions_Timer XPD Timer Handling Functions
 *  @brief    XPD Utilities millisecond timer handling functions
 * @{
 */

/**
 * @brief Millisecond timer initializer utility. [overrideable]
 */
__weak void XPD_InitTimer(void)
{
    /* Enable SysTick and configure 1ms tick */
    XPD_SysTick_Init(SystemCoreClock / 1000, SYSTICK_CLOCKSOURCE_HCLK);
    XPD_SysTick_Enable();
}

/**
 * @brief Inserts code delay of the specified time in milliseconds.
 * @note  The milliseconds based waiting utilities shall not be used concurrently.
 *        The time of the preempted waiters do not elapse.
 * @param milliseconds: the desired delay in ms
 */
__weak void XPD_Delay_ms(uint32_t milliseconds)
{
    /* Initially clear flag */
    __IO uint32_t dummy = SysTick->CTRL.b.COUNTFLAG;
    while (milliseconds != 0)
    {
        /* COUNTFLAG returns 1 if timer counted to 0 since the last flag read */
        milliseconds -= SysTick->CTRL.b.COUNTFLAG;
    }
}

/**
 * @brief Inserts code delay of the specified time in microseconds.
 * @param microseconds: the desired delay in us
 */
__weak void XPD_Delay_us(uint32_t microseconds)
{
    microseconds *= SystemCoreClock / 1000000;
    while (microseconds != 0)
    {
        microseconds--;
    }
}

/**
 * @brief Waits until the masked value read from address matches the input match, or until times out. [overrideable]
 * @note  The milliseconds based waiting utilities shall not be used concurrently.
 *        The time of the preempted waiters do not elapse.
 * @param varAddress: the word address that needs to be monitored
 * @param bitSelector: a bit mask that selects which bits should be considered
 * @param match: the expected value to wait for
 * @param mstimeout: pointer to the timeout in ms
 * @return TIMEOUT if timed out, or OK if match occurred within the deadline
 */
__weak XPD_ReturnType XPD_WaitForMatch(
        volatile uint32_t * varAddress, uint32_t bitSelector, uint32_t match,
        uint32_t * mstimeout)
{
    /* Initially clear flag */
    __IO uint32_t dummy = SysTick->CTRL.b.COUNTFLAG;
    XPD_ReturnType result = XPD_OK;

    while ((*varAddress & bitSelector) != match)
    {
        if (*mstimeout == 0)
        {
            result = XPD_TIMEOUT;
            break;
        }
        /* COUNTFLAG returns 1 if timer counted to 0 since the last flag read */
        *mstimeout -= SysTick->CTRL.b.COUNTFLAG;
    }
    return result;
}

/**
 * @brief Waits until the masked value read from address differs from the input match, or until times out. [overrideable]
 * @note  The milliseconds based waiting utilities shall not be used concurrently.
 *        The time of the preempted waiters do not elapse.
 * @param varAddress: the word address that needs to be monitored
 * @param bitSelector: a bit mask that selects which bits should be considered
 * @param match: the initial value that needs to differ
 * @param mstimeout: pointer to the timeout in ms
 * @return TIMEOUT if timed out, or OK if match occurred within the deadline
 */
__weak XPD_ReturnType XPD_WaitForDiff(
        volatile uint32_t * varAddress, uint32_t bitSelector, uint32_t match,
        uint32_t * mstimeout)
{
    /* Initially clear flag */
    __IO uint32_t dummy = SysTick->CTRL.b.COUNTFLAG;
    XPD_ReturnType result = XPD_OK;

    while ((*varAddress & bitSelector) == match)
    {
        if (*mstimeout == 0)
        {
            result = XPD_TIMEOUT;
            break;
        }
        /* COUNTFLAG returns 1 if timer counted to 0 since the last flag read */
        *mstimeout -= SysTick->CTRL.b.COUNTFLAG;
    }
    return result;
}

/** @} */

/** @defgroup XPD_Exported_Functions_Stream XPD Data Stream Handling Functions
 *  @brief    XPD Utilities data stream handlers
 * @{
 */

/**
 * @brief Reads new register data to the stream and updates its context.
 * @param reg: pointer to the register to read from
 * @param stream: pointer to the destination stream
 */
void XPD_ReadToStream(volatile uint32_t * reg, DataStreamType * stream)
{
    /* Different size of data transferred */
    switch (stream->size)
    {
        case 1:
            *((uint8_t*) stream->buffer) = *((__IO uint8_t  *)reg);
            break;
        case 2:
            *((uint16_t*)stream->buffer) = *((__IO uint16_t *)reg);
            break;
        default:
            *((uint32_t*)stream->buffer) = *((__IO uint32_t *)reg);
            break;
    }
    /* Stream context update */
    stream->buffer += stream->size;
    stream->length--;
}

/**
 * @brief Writes a new stream data element to the register and updates the stream context.
 * @param reg: pointer to the register to write to
 * @param stream: pointer to the source stream
 */
void XPD_WriteFromStream(volatile uint32_t * reg, DataStreamType * stream)
{
    /* Different size of data transferred */
    switch (stream->size)
    {
        case 1:
            *((__IO uint8_t  *)reg) = *((uint8_t*) stream->buffer);
            break;
        case 2:
            *((__IO uint16_t *)reg) = *((uint16_t*)stream->buffer);
            break;
        default:
            *((__IO uint32_t *)reg) = *((uint32_t*)stream->buffer);
            break;
    }
    /* Stream context update */
    stream->buffer += stream->size;
    stream->length--;
}

/** @} */

/** @defgroup XPD_Exported_Functions_Init XPD Startup and Shutdown Functions
 *  @brief    XPD Utilities startup and shutdown functions
 * @{
 */

/**
 * @brief Initializes the basic services of the device:
 *        @arg Memory access
 *        @arg System Timer utility
 *        @arg Interrupt priority group selection
 *        @arg Enable PWR and SYSCFG clocks
 */
void XPD_Init(void)
{
#if (PREFETCH_ENABLE != 0)
    XPD_FLASH_PrefetchBufferCtrl(ENABLE);
#endif

    /* Configure systick timer */
    XPD_InitTimer();

    /* enable clock for PWR */
    XPD_PWR_ClockCtrl(ENABLE);

    /* Enable SYSCFG clock  */
    XPD_SYSCFG_ClockCtrl(ENABLE);
}

/**
 * @brief Deinitializes the basic services of the device:
 *        @arg Timer utility
 *        @arg Resets all peripherals
 */
void XPD_Deinit(void)
{
    XPD_SuspendTimer();

    /* Reset of all peripherals */
    XPD_RCC_ResetAHB();
    XPD_RCC_ResetAPB1();
    XPD_RCC_ResetAPB2();

    /* disable clocks */
    XPD_PWR_ClockCtrl(DISABLE);
    XPD_SYSCFG_ClockCtrl(DISABLE);
}

/** @} */

/** @} */

/** @} */

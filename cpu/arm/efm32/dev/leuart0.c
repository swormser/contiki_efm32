/*
 * Copyright (c) 2013, Kerlink
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */
/**
 * \addtogroup efm32-devices
 * @{
 */
/**
 * \file
 *         EFM32 LEUART0 driver
 * \author
 *         Martin Chaplet <m.chaplet@kerlink.fr>
 */

#include "contiki.h"
#include <stdlib.h>
#include "sys/energest.h"

#include <efm32.h>
#include "em_device.h"
#include "em_cmu.h"
#include "em_leuart.h"
#include "leuart0.h"
#include "dev/watchdog.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int (*leuart0_input_handler)(unsigned char c);

/*---------------------------------------------------------------------------*/
void LEUART0_IRQHandler(void)
{
  unsigned char c;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  if(LEUART0->STATUS & LEUART_STATUS_RXDATAV)
  {
    c = LEUART_Rx(LEUART0);
    if(leuart0_input_handler != NULL) leuart0_input_handler(c);
  }
  /*
  else
  {
	  PRINTF("ERROR: control reg = 0x%lX", ???);
	  // Disable all errors
  }
*/
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
uint8_t
leuart0_active(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
void
leuart0_set_input(int (*input)(unsigned char c))
{
  leuart0_input_handler = input;
}
/*---------------------------------------------------------------------------*/
int *leuart0_get_input(void)
{
  return (int*) leuart0_input_handler;
}
/*---------------------------------------------------------------------------*/
void
leuart0_writeb(unsigned char c)
{
  watchdog_periodic();
#ifdef LEUART0_LF_TO_CRLF
  if(c == '\n')
  {
    LEUART_Tx(LEUART0, '\r');
  }
#endif

  LEUART_Tx(LEUART0, c);

#ifdef LEUART0_LF_TO_CRLF
  if(c == '\r')
  {
    LEUART_Tx(LEUART0, '\n');
  }
#endif
}
/*---------------------------------------------------------------------------*/
unsigned int
leuart0_send_bytes(const unsigned char *seq, unsigned int len)
{
  /* TODO : Use DMA Here ... */

  int i=0;
  for(i=0;i<len;i++)
  {
    leuart0_writeb(seq[i]);
  }
  return len;
}
/*---------------------------------------------------------------------------*/
void
leuart0_drain(void)
{
  while (!(LEUART0->STATUS & LEUART_STATUS_TXBL));
}
/*---------------------------------------------------------------------------*/
/**
 * Initialize the UART.
 *
 */
void
leuart0_init(unsigned long baudrate)
{
  LEUART_Init_TypeDef init =   LEUART_INIT_DEFAULT;
  /* Configure controller */

  // Enable clocks
  CMU_ClockEnable(cmuClock_LFB, true);
  CMU_ClockEnable(cmuClock_LEUART0, true);

  init.enable = leuartDisable;
  if(baudrate > LEUART_MAX_BAUDRATE)
  {
    baudrate = LEUART_MAX_BAUDRATE;
  }
  else
  {
    init.baudrate = baudrate;
  }

  LEUART_Init(LEUART0, &init);

  /* Enable pins at LEUART0 location #2 */
  LEUART0->ROUTE = LEUART_ROUTE_RXPEN | LEUART_ROUTE_TXPEN |
                  (LEUART0_LOCATION << _LEUART_ROUTE_LOCATION_SHIFT);

  /* Clear previous RX interrupts */
  LEUART_IntClear(LEUART0, LEUART_IF_RXDATAV);
  NVIC_ClearPendingIRQ(LEUART0_IRQn);

  /* Enable RX interrupts */
  LEUART_IntEnable(LEUART0, LEUART_IF_RXDATAV);
  NVIC_EnableIRQ(LEUART0_IRQn);

  /* Finally enable it */
  LEUART_Enable(LEUART0, leuartEnable);
}
/*---------------------------------------------------------------------------*/

/** @} */

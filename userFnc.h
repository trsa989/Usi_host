/**
 * \file
 *
 * \brief USI Host User Defined Functions
 *
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
 /*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef userFnch
#define userFnch

#ifdef __cplusplus
extern "C" {
#endif

/* *** Functions prototypes ************************************************** */
/**@brief Open communication port (parameters are defined in PrjCfg.h)
  @param port_type: Port Type (UART_TYPE, USART_TYPE, COM_TYPE)
  @param commPort: Physical Communication Port
  @param speed:Communication speed
  @return 0 on success.
*/
int8_t addUsi_Open(uint8_t port_type, uint8_t port, uint32_t bauds);

/**@brief Transmit a message through the interface
  @param port: port number configured in PrjCfg.h
  @param msg: buffer holding the message
  @param msglen:buffer length
  @return the number of bytes written.
*/
uint16_t addUsi_TxMsg(uint8_t port_type, uint8_t port, uint8_t *msg, uint16_t msglen);

/**@brief Read char from port
  @param port: port number configured in PrjCfg.h
  @param c: pointer to a character
  @return 0 if a charater has been read, otherwise -1.
*/
int8_t addUsi_RxChar(uint8_t port_type, uint8_t port, uint8_t *c);                     

int8_t addUsi_Log(int loglevel, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

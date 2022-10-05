/**
 * \file
 *
 * \brief USI Host Config file
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

#include <PrjCfg.h>
#include "../addUsi.h"
#include "UsiCfg.h"


//*** Declarations **********************************************************

#ifdef USE_MNGP_PRIME_PORT
  #pragma message("USI_CFG: USE_MNGP_PRIME_PORT")  
  #include "../mngLayerHost.h"  			/*Prime Management Plane*/
  usi_decode_cmd_cb cbFnMngLay = mngLay_receivedCmd;
#else
  usi_decode_cmd_cb cbFnMngLay = NULL;
#endif

#ifdef USE_PROTOCOL_SNIF_PRIME_PORT	
	#pragma message("USI_CFG: USE_PROTOCOL_SNIF_PRIME_PORT")
  #include "../ifacePrimeSniffer.h"		/*Sniffer Prime*/
  usi_decode_cmd_cb cbFnSnifferPrime = prime_sniffer_receivedCmd;
#else
  usi_decode_cmd_cb cbFnSnifferPrime = NULL;
#endif

#ifdef USE_PROTOCOL_PRIME_API
    #pragma message("USI_CFG: USE_PROTOCOL_PRIME_API")
  #include "../prime_api_host.h"			/*Prime API*/
  usi_decode_cmd_cb cbFn_prime_api = ifacePrime_api_ReceivedCmd;
#else
  usi_decode_cmd_cb cbFn_prime_api = NULL;
#endif

#ifdef USE_PROTOCOL_PHY_SERIAL_PRIME
    #pragma message("USI_CFG: USE_PROTOCOL_PHY_SERIAL_PRIME")
  #include "../ifacePrimeUdp.h"			/*Prime Over Udp*/
  usi_decode_cmd_cb cbFn_primeoudp = primeoudp_rcv;
#else
  usi_decode_cmd_cb cbFn_primeoudp = NULL;
#endif

#ifdef USE_PROTOCOL_SNIF_G3_PORT
    #pragma message("USI_CFG: USE_PROTOCOL_SNIF_G3_PORT")
  #include "../ifaceG3Sniffer.h"		/*Sniffer G3*/
  usi_decode_cmd_cb cbFnSnifferG3 = g3_sniffer_receivedCmd;
#else
  usi_decode_cmd_cb cbFnSnifferG3 = NULL;
#endif

#ifdef USE_PROTOCOL_MAC_G3_PORT
    #pragma message("USI_CFG: USE_PROTOCOL_MAC_G3_PORT")
  #include "../G3.h"
  Pt2DecodeCmd cbFnG3Mac = (Pt2DecodeCmd)g3_MAC_receivedCmd;
#else
  Pt2DecodeCmd cbFnG3Mac = NULL;
#endif

#ifdef USE_PROTOCOL_ADP_G3_PORT
    #pragma message("USI_CFG: USE_PROTOCOL_ADP_G3_PORT")
  #include "../G3.h"
  Pt2DecodeCmd cbFnG3Adp = (Pt2DecodeCmd)g3_ADP_receivedCmd;
#else
  Pt2DecodeCmd cbFnG3Adp = NULL;
#endif

#ifdef USE_PROTOCOL_COORD_G3_PORT
    #pragma message("USI_CFG: USE_PROTOCOL_COORD_G3_PORT")
  #include "../G3.h"
  Pt2DecodeCmd cbFnG3Coord = (Pt2DecodeCmd)g3_COORD_receivedCmd;
#else
  Pt2DecodeCmd cbFnG3Coord = NULL;
#endif

//--------------------------------------------------------------------------------------
/// Function Pointers for USI protocols	 


//--------------------------------------------------------------------------------------
#undef CONF_PORT 
#define CONF_PORT(type, channel, speed, txSize, rxSize) {type, channel, speed}

/// Port Mapping. It is configured with the values provided in Header file 
static MapPorts usiMapPorts[NUM_PORTS+1] =
{   // PORT TYPE, PORT CHANNEL, PORT SPEED, TX BUFFER SIZE, RX BUFFER SIZE
#ifdef PORT_0
	PORT_0,
#endif
#ifdef PORT_1
	PORT_1,
#endif
#ifdef PORT_2
	PORT_2,
#endif
#ifdef PORT_3
	PORT_3,
#endif
    {0xFF, 0xFF, 0}
};


//--------------------------------------------------------------------------------------

/// Protocol Mapping. It is configured with the values provided in Header file 
static const MapProtocols usiMapProtocols[NUM_PROTOCOLS + 1] =
{   // PROTOCOL TYPE, PORT INDEX
#ifdef USE_MNGP_PRIME_PORT
    {MNGP_PRIME, USE_MNGP_PRIME_PORT},
#endif

#ifdef USE_PROTOCOL_PHY_SERIAL_PRIME
    {PROTOCOL_PHY_SERIAL_PRIME, USE_PROTOCOL_PHY_SERIAL_PRIME},
#endif

#ifdef USE_PROTOCOL_SNIF_PRIME_PORT	
	{PROTOCOL_SNIF_PRIME, USE_PROTOCOL_SNIF_PRIME_PORT},    
#endif

#ifdef USE_PROTOCOL_PRIME_API
    {PROTOCOL_PRIME_API, USE_PROTOCOL_PRIME_API},
#endif

#ifdef USE_PROTOCOL_SNIF_G3_PORT
    {PROTOCOL_SNIF_G3, USE_PROTOCOL_SNIF_G3_PORT},
#endif

#ifdef USE_PROTOCOL_MAC_G3_PORT
    {PROTOCOL_MAC_G3, USE_PROTOCOL_MAC_G3_PORT},
#endif

#ifdef USE_PROTOCOL_ADP_G3_PORT
    {PROTOCOL_ADP_G3, USE_PROTOCOL_ADP_G3_PORT},
#endif

#ifdef USE_PROTOCOL_COORD_G3_PORT
    {PROTOCOL_COORD_G3, USE_PROTOCOL_COORD_G3_PORT},
#endif

    {0xff, 0xff}
};


//--------------------------------------------------------------------------------------
/// Reception Buffers Mapping
#undef CONF_PORT 
#define CONF_PORT(type, channel, speed, txSize, rxSize) rxSize

#ifdef PORT_0
static uint8_t rxbuf0[PORT_0];
#endif

#ifdef PORT_1
static uint8_t rxbuf1[PORT_1];
#endif

#ifdef PORT_2
static uint8_t rxbuf2[PORT_2];
#endif

#ifdef PORT_3
static uint8_t rxbuf3[PORT_3];
#endif

static const MapBuffers usiRxBuf[NUM_PORTS + 1] =
{
#ifdef PORT_0
  	{PORT_0, &rxbuf0[0]},
#endif
#ifdef PORT_1
	{PORT_1, &rxbuf1[0]},
#endif
#ifdef PORT_2
	{PORT_2, &rxbuf2[0]},
#endif
#ifdef PORT_3
	{PORT_3, &rxbuf3[0]},
#endif
    {0xFF , NULL}
};

//--------------------------------------------------------------------------------------
/// Transmission Buffers Mapping
#undef CONF_PORT 
#define CONF_PORT(type, channel, speed, txSize, rxSize) txSize

#ifdef PORT_0
static uint8_t txbuf0[PORT_0];
#endif
#ifdef PORT_1
static uint8_t txbuf1[PORT_1];
#endif
#ifdef PORT_2
static uint8_t txbuf2[PORT_2];
#endif
#ifdef PORT_3
static uint8_t txbuf3[PORT_3];
#endif

static const MapBuffers usiTxBuf[NUM_PORTS + 1] =
{
#ifdef PORT_0
  	{PORT_0, &txbuf0[0]},
#endif
#ifdef PORT_1
  	{PORT_1, &txbuf1[0]},
#endif
#ifdef PORT_2
	{PORT_2, &txbuf2[0]},
#endif
#ifdef PORT_3
	{PORT_3, &txbuf3[0]},
#endif
    {0xFF , NULL}
};

//***************************************************************************

/// Transmission buffers size configuration
#define TXAUX_SIZE 0
#ifdef PORT_0
	#if (PORT_0 > TXAUX_SIZE)
		#undef TXAUX_SIZE
		#define TXAUX_SIZE   	PORT_0
	#endif
#endif
#ifdef PORT_1
	#if (PORT_1 > TXAUX_SIZE)
		#undef TXAUX_SIZE
		#define TXAUX_SIZE   	PORT_1
	#endif
#endif
#ifdef PORT_2
	#if (PORT_2 > TXAUX_SIZE)
		#undef TXAUX_SIZE
		#define TXAUX_SIZE   	PORT_2
	#endif
#endif
#ifdef PORT_3
	#if (PORT_3 > TXAUX_SIZE)
		#undef TXAUX_SIZE
		#define TXAUX_SIZE   	PORT_3
	#endif
#endif

static uint8_t txauxbuf[TXAUX_SIZE+2];
static const MapBuffers usiAuxTxBuf = {TXAUX_SIZE+2, &txauxbuf[0]};


//--------------------------------------------------------------------------------------
/// Control parameters in communications
static RxParam usiRxParam[NUM_PORTS];
static TxParam usiTxParam[NUM_PORTS];
//--------------------------------------------------------------------------------------

//*** Local variables *******************************************************

//*** Public variables ******************************************************
const uint8_t usiCfgNumProtocols = NUM_PROTOCOLS;
const uint8_t usiCfgNumPorts = NUM_PORTS;
MapPorts * const usiCfgMapPorts = &usiMapPorts[0];
const MapProtocols * const usiCfgMapProtocols = &usiMapProtocols[0];
const MapBuffers * const usiCfgRxBuf = &usiRxBuf[0];
const MapBuffers * const usiCfgTxBuf = &usiTxBuf[0];
const MapBuffers * const usiCfgAuxTxBuf = &usiAuxTxBuf;
RxParam * const usiCfgRxParam = &usiRxParam[0];
TxParam * const usiCfgTxParam = &usiTxParam[0];







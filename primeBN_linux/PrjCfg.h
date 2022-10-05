#ifndef __PRIMEPRJCFG_H__
#define __PRIMEPRJCFG_H__


#ifdef __cplusplus
extern "C" {
#endif

#define NUM_PORTS			1
#define PORT_0 CONF_PORT(COM_TYPE, 3,115200,0x7FFF, 0x7FFF)
#define NUM_PROTOCOLS			3
#define USE_MNGP_PRIME_PORT		0
#define USE_PROTOCOL_SNIF_PRIME_PORT 	0
#define USE_PROTOCOL_PRIME_API		0

#ifdef __cplusplus
}
#endif

#endif

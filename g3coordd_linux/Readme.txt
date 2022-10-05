1. Introduction
-----------------------
This application note implements an example of G3-PLC Coordinator making use of Microchip USI_HOST package on
top of a device running as G3-PLC ADP MAC serialized modem.
The application a similar to the embedded G3-PLC Coordinator DLMS Application where everything related with the DLMS has been removed
Additionally, a Linux IPv6 interface (TUN) is created; it allows to communicate with the G3-PLC Network with network applications like "ping".
 

2. Makefile Definitions
-----------------------
* SPEC_COMPLIANCE=17
  From G3-PLC v1.3.0 stack version, the firmware stack implements the 2017 specification
  The  main difference with 2015 specification is that it appears POS Tables instead of Neighbour Table
* APP_CONFORMANCE_TEST
  Definition for certification purposes
* G3_HYBRID_PROFILE
  Definition necessary when ADP MAC serialized modem implements the Hybrid Profile.

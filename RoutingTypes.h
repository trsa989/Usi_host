/**********************************************************************************************************************/
/** \addtogroup AdaptationSublayer
 * @{
 **********************************************************************************************************************/

/**********************************************************************************************************************/
/** This file contains configuration definitions used to tune the memory usage of the Routing layer.
 ***********************************************************************************************************************
 *
 * @file
 *
 **********************************************************************************************************************/

#ifndef __ROUTING_TYPES_H__
#define __ROUTING_TYPES_H__

#include <RoutingApi.h>
#include <QueueMng.h>
#include <Timer.h>

struct TAdpRoutingTableEntry {
  /// The 16 bit short link layer address of the final destination of a route
  uint16_t m_u16DstAddr;
  /// The 16 bit short link layer addresses of the next hop node to the destination
  uint16_t m_u16NextHopAddr;

  uint16_t m_u16SeqNo;
  uint16_t m_u16RouteCost;
  uint8_t m_u8HopCount : 4;
  uint8_t m_u8WeakLinkCount : 4;

  uint8_t m_u8ValidSeqNo : 1;
  uint8_t m_u8Bidirectional : 1;
  uint8_t m_u8RREPSent : 1;
  uint8_t m_u8MetricType : 2;

  /// Absolute time in milliseconds when the entry expires
  int32_t m_i32ValidTime;
};

struct TAdpBlacklistTableEntry {
  uint16_t m_u16Addr;
  /// Absolute time in milliseconds when the entry expires
  int32_t m_i32ValidTime;
};

struct TDiscoverRouteEntry {
  // Callback called when the discovery is finished
  LOADNG_DiscoverRoute_Callback m_fcntCallback;

  // Final destination of the discover
  uint16_t m_u16DstAddr;

  // Max hops parameter
  uint8_t m_u8MaxHops;

  // Repair route: true / false
  bool m_bRepair;

  // User data
  void *m_pUserData;

  // Timer to control the discovery process if no response is received
  struct TTimer m_Timer;

  // Current try number
  uint8_t m_u8Try;

  // Discover route sequence number
  uint16_t m_u16SeqNo;
};

struct TDiscoverPath {
  uint16_t m_u16DstAddr;
  // Callback called when the discovery is finished
  LOADNG_DiscoverPath_Callback m_fnctCallback;
  // Timer to control the discovery process if no response is received
  struct TTimer m_Timer;
};

struct TRRepGeneration {
  uint16_t m_u16OrigAddr;   // RREQ originator (and final destination of RREP)
  uint16_t m_u16DstAddr;   // RREQ destination (and originator of RREP). Unused in SPEC-15
  uint16_t m_u16RREQSeqNum;   // RREQ Sequence number to be able to manage different RREQ from same node
  uint8_t m_u8Flags;   // Flags received from RREQ
  uint8_t m_u8MetricType;   // MetricType received from RREQ
  uint8_t m_bWaitingForAck;   // Flag to indicate entry is waiting for ACK, timer can be expired but RREP were not sent due to channel saturation
  struct TTimer m_Timer;   // Timer to control the RREP sending
};

struct TRERREntry {
  uint16_t m_u16DstAddr;
  uint16_t m_u16OrigAddr;
  uint16_t m_u16UnreachableAddress;
  uint8_t m_u8ErrorCode;
  uint8_t m_u8HopLimit;
};

struct TRouteCostParameters {
  enum EAdpMac_Modulation m_eModulation;
  uint8_t m_u8NumberOfActiveTones;
  uint8_t m_u8NumberOfSubCarriers;
  uint8_t m_u8LQI;
};

struct TRoutingTables {
  uint8_t m_PendingRREQRERRTableSize;
  uint8_t m_PendingRERRTableSize;
  uint8_t m_RRepGenerationTableSize;
  uint8_t m_DiscoverRouteTableSize;
  /**
   * Contains the length of the Routing table
   */
  uint16_t m_AdpRoutingTableSize;
  /**
   * Contains the length of the blacklisted neighbours table
   */
  uint8_t m_AdpBlacklistTableSize;
  /**
   * Contains the length of the routing set
   */
  uint16_t m_AdpRoutingSetSize;
  /**
   * Contains the size of the destination address set
   */
  uint16_t m_AdpDestinationAddressSetSize;
  /**
   * Contains the routing table
   */

  struct TQueueElement *m_PendingRREQRERRTable;
  // table to store information about pending RERR
  struct TRERREntry *m_PendingRERRTable;
  struct TRRepGeneration *m_RRepGenerationTable;
  struct TDiscoverRouteEntry *m_DiscoverRouteTable;

  struct TAdpRoutingTableEntry *m_AdpRoutingTable;
  /**
   * Contains the list of the blacklisted neighbours
   */
  struct TAdpBlacklistTableEntry *m_AdpBlacklistTable;
  /**
   * Contains the routing set
   */
  struct TAdpRoutingTableEntry *m_AdpRoutingSet;
  /**
   * Contains the list of destination addresses.
   */
  uint16_t *m_AdpDestinationAddressSet;
};
#endif


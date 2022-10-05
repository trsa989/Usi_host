/**
 * \file
 *
 * \brief Base Node Network file.
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

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

/************************************************************
*       Includes                                            *
*************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "conf_global.h"

#include "prime_utils.h"
#include "prime_log.h"
#include "return_codes.h"

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "cl_432_defs.h"
#include "mac_pib.h"
#include "mac_defs.h"

#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_mng_fw_upgrade.h"
#include "base_node_network.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define DUK_LEN 16
#define NUM_MAX_PRIME_SN 2000
#define NUM_MAX_PRIME_MAC_CONNECTIONS 10
#define NUM_MAX_PRIME_LEVELS 63

/* Linked List for PRIME Network visibility */
mchp_list prime_network;
/* Number of PRIME Service Nodes Registered */
uint16_t prime_network_sn = 0;
/* Number of Levels on PRIME Network */
uint8_t  prime_network_max_level = 0;
/* Mutex for accessing PRIME Network Information */
pthread_mutex_t prime_network_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * \brief PRIME Network Mutex Lock
 * \return 0
 */
int prime_network_mutex_lock()
{
    return pthread_mutex_lock(&prime_network_mutex);
}

/*
 * \brief PRIME Network Mutex Unlock
 * \return 0
 */
int prime_network_mutex_unlock()
{
    return pthread_mutex_unlock(&prime_network_mutex);
}

/*
 * \brief Create a new PRIME SN
 * \param p_prime_network PRIME SN List
 * \param eui48           EUI48 MAC Address
 * \param admin_en        Administrative enabling
 * \param sec_profile     Security profile
 * \param duk             DUK Encryption Key
 * \param
 *
 * \return Pointer to the new structure
 */
 prime_sn * prime_network_add_sn(mchp_list *p_prime_network, uint8_t *eui48, uint8_t admin_en, uint8_t sec_profile, uint8_t *duk)
 {
 /*********************************************************
 *       Vars                                             *
 *********************************************************/
     prime_sn * p_prime_sn;
     //time_t timestamp = time(NULL);
     struct TmacSetConfirm x_pib_confirm;
     uint8_t buffer[22];
 /*********************************************************
 *       Code                                             *
 *********************************************************/
     pthread_mutex_lock(&prime_network_mutex);
 	   if (prime_network_sn == NUM_MAX_PRIME_SN){
 		     PRIME_LOG(LOG_ERR,"Maximum Number of PRIME SN registered\r\n");
 		     pthread_mutex_unlock(&prime_network_mutex);
 		     return (prime_sn *) NULL;
 	   }
     /// Initial Malloc
     p_prime_sn = (prime_sn *) malloc(sizeof(prime_sn));
     if (p_prime_sn == (prime_sn *) NULL){
         PRIME_LOG(LOG_ERR,"Impossible to allocate space for a new PRIME SN\r\n");
         pthread_mutex_unlock(&prime_network_mutex);
         return (prime_sn *) NULL;
     }
     // Initialize Entry
     memset(p_prime_sn, 0, sizeof(prime_sn));
     // Set Entry - MAC is necessar
     memcpy(p_prime_sn->regEntryID, eui48, EUI48_LEN);
     memcpy(&buffer[0],eui48,EUI48_LEN);
     p_prime_sn->security_profile = sec_profile;
     if (duk != NULL){
        memcpy(p_prime_sn->duk, duk, DUK_LEN);
        memcpy(&buffer[6],duk,DUK_LEN);
        prime_cl_null_mlme_set_request_sync(PIB_MAC_SEC_DUK_BN,buffer,22,PRIME_SYNC_TIMEOUT_SET_REQUEST,&x_pib_confirm);
        if (x_pib_confirm.m_u8Status != MLME_RESULT_DONE){
            PRIME_LOG(LOG_ERR, "Error adding Service Node to whitelist\r\n");
            free(p_prime_sn);
            pthread_mutex_unlock(&prime_network_mutex);
            return (prime_sn *) NULL;
        }
     }
     // If Register from CMDLINE, enable administrative state
     p_prime_sn->admin_en = admin_en;

     // Initialize Connection List
     INIT_LIST_HEAD(&p_prime_sn->macConnList);
     p_prime_sn->macConns = 0;
     /// Include SN on Prime Network
     mchp_list_add_tail(p_prime_sn, p_prime_network);
     prime_network_sn++;
     PRIME_LOG(LOG_DBG, "New PRIME SN Device created\r\n");
     prime_network_print_sn(p_prime_sn);
     pthread_mutex_unlock(&prime_network_mutex);
     return p_prime_sn;
 }

/*
 * \brief Delete a Device from the List
 * \param p_prime_sn pointer to PRIME SN Device
 *
 * \return -
 */
 void prime_network_del_sn(prime_sn * p_prime_sn)
 {
 /*********************************************************
 *       Local Vars                                       *
 *********************************************************/
     mchp_list *entry, *tmp;
     mac_conn * p_mac_conn;
 /*********************************************************
 *       Code                                             *
 *********************************************************/
     PRIME_LOG(LOG_INFO, "Deleting PRIME SN 0x%s\r\n",eui48_to_str(p_prime_sn->regEntryID,NULL));
     pthread_mutex_lock(&prime_network_mutex);
     /// Delete frame queues on RX
     //prime_device_frame_queue_destroy(p_g3plc_device->frame_queue);
     /// Delete Service Node State Machine
     //prime_device_fsm_exit(p_g3plc_device);

     /// Free Memory on MAC Connection List
     list_for_each_safe(entry, tmp, &p_prime_sn->macConnList) {
         p_mac_conn = list_entry(entry, mac_conn, list);
         if (p_mac_conn != NULL)
            free(p_mac_conn);
     }
     /// Unlink Service Node on PRIME Network
     mchp_list_del(p_prime_sn);
     /// Free Memory on PRIME Network List
     free(p_prime_sn);
     prime_network_sn-- ;
     pthread_mutex_unlock(&prime_network_mutex);
 }

 /*
  * \brief Look for a PRIME SN MAC address on the PRIME Network List
  * \param p_prime_network PRIME SN List
  * \param regEntryID      PRIME SN EUI48 Hardware Address
  *
  * \return Pointer to the PRIME SN Structure or NULL if doesn't exist
  */
 prime_sn * prime_network_find_sn(mchp_list * p_prime_network, const unsigned char * regEntryID)
 {
 /*********************************************************
 *       Variables Locales Definidas                      *
 *********************************************************/
     mchp_list *entry, *tmp;
     prime_sn * p_prime_sn;
 /*********************************************************
 *       Code                                           *
 *********************************************************/
     if (prime_network_sn == 0){
 		    //PRIME_LOG(LOG_DEBUG, "No PRIME SN present\r\n");
 		    return (prime_sn *) NULL;
 	   }
     list_for_each_safe(entry, tmp, p_prime_network) {
         p_prime_sn = list_entry(entry, prime_sn, list);
         //prime_network_print_sn(p_prime_sn);
         if (memcmp(p_prime_sn->regEntryID, regEntryID, EUI48_LEN) == 0){
 			      //PRIME_LOG(LOG_DEBUG, "PRIME SN 0x%s found!\r\n",eui48_to_str(regEntryID,NULL));
            return p_prime_sn;
         }
     }
     return (prime_sn *) NULL;
 }

/*
* \brief Look for a PRIME SN on the list for SID
* \param p_prime_network PRIME SN List
* \param lsid            PRIME LSID
*
* \return Pointer to the PRIME SN Structure or NULL if doesn't exist
*/
prime_sn * prime_network_find_sn_lsid(mchp_list * p_prime_network, uint8_t lsid)
 {
 /*********************************************************
 *       Variables Locales Definidas                      *
 *********************************************************/
     mchp_list *entry, *tmp;
     prime_sn * p_prime_sn;
 /*********************************************************
 *       Code                                           *
 *********************************************************/
     if (prime_network_sn == 0){
 		    //PRIME_LOG(LOG_DEBUG, "No PRIME SN present\r\n");
 		    return (prime_sn *) NULL;
 	   }
     list_for_each_safe(entry, tmp, p_prime_network) {
         p_prime_sn = list_entry(entry, prime_sn, list);
         //prime_network_print_sn(p_prime_sn);
         if (p_prime_sn->regEntryLSID == lsid){
 			      //PRIME_LOG(LOG_DEBUG, "LSID found on PRIME SN 0x%s!\r\n",eui48_to_str(p_prime_sn->regEntryID,NULL));
            return p_prime_sn;
         }
     }
     return (prime_sn *) NULL;
 }

 /*
 * \brief Look for a PRIME SN on the list for CL4-32 address
 * \param p_prime_network PRIME SN List
 * \param cl432_address   CL4-32 Address
 *
 * \return Pointer to the PRIME SN Structure or NULL if doesn't exist
 */
 prime_sn * prime_network_find_sn_cl432_address(mchp_list * p_prime_network, uint16_t cl432_address)
  {
  /*********************************************************
  *       Variables Locales Definidas                      *
  *********************************************************/
      mchp_list *entry, *tmp;
      prime_sn * p_prime_sn;
  /*********************************************************
  *       Code                                           *
  *********************************************************/
      if (prime_network_sn == 0){
  		    //PRIME_LOG(LOG_DEBUG, "No PRIME SN present\r\n");
  		    return (prime_sn *) NULL;
  	   }
      list_for_each_safe(entry, tmp, p_prime_network) {
          p_prime_sn = list_entry(entry, prime_sn, list);
          //prime_network_print_sn(p_prime_sn);
          if (p_prime_sn->cl432Conn.connAddress == cl432_address){
  			      //PRIME_LOG(LOG_DEBUG, "CL4-32 Address found on PRIME SN 0x%s!\r\n",eui48_to_str(p_prime_sn->regEntryID,NULL));
             return p_prime_sn;
          }
      }
      return (prime_sn *) NULL;
  }

/*
 * \brief Print List of PRIME SN
 * \param p_prime_network PRIME SN linked list Pointer
 *
 * \return -
 */
void prime_network_print_sn_list(mchp_list * p_prime_network)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
    mchp_list *entry, *tmp;
    prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
    if (prime_network_sn == 0){
		   PRIME_LOG(LOG_DEBUG, "No PRIME SN present\r\n");
	  }else{
	     pthread_mutex_lock(&prime_network_mutex);
       list_for_each_safe(entry, tmp, p_prime_network) {
          p_prime_sn = list_entry(entry, prime_sn, list);
          prime_network_print_sn(p_prime_sn);
       }
       pthread_mutex_unlock(&prime_network_mutex);
    }
}

/*
 * \brief Shows information about PRIME SN
 * \param p_prime_sn PRIME SN Device Pointer
 *
 * \return -
 */
int prime_network_print_sn(prime_sn * p_prime_sn)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
    struct tm *tm ;
/*********************************************************
*       Code                                             *
*********************************************************/
    if (p_prime_sn == (prime_sn *) NULL)
		   return 0;

    tm = localtime(&p_prime_sn->regEntryTimeStamp);
    PRIME_LOG(LOG_DEBUG,"Service Node   %s\r\n"        \
                        "\t* Register TS  : %s\r\n"    \
                        "\t* LNID         : %05d\r\n"  \
                        "\t* State        : %03d\r\n"  \
                        "\t* LSID         : %03d\r\n"  \
                        "\t* SID          : %03d\r\n"  \
                        "\t* Level        : %03d\r\n"  \
                        "\t* TCap         : %03d\r\n"  \
                        "\t* SwCap        : %03d\r\n", \
                              eui48_to_str(p_prime_sn->regEntryID,NULL), \
                              asctime(tm),                               \
                              p_prime_sn->regEntryLNID,                  \
                              p_prime_sn->regEntryState,                 \
                              p_prime_sn->regEntryLSID,                  \
                              p_prime_sn->regEntrySID,                   \
                              p_prime_sn->regEntryLevel,                 \
                              p_prime_sn->regEntryTCap,                  \
                              p_prime_sn->regEntrySwCap);
    return SUCCESS;
}

/*
 * \brief Add a new MAC Connection
 * \param sn        PRIME Service Node Pointer structure
 * \param lcid      LCID
 * \param connType  Connection Type
 * \param
 * \return Pointer to the new structure
 */
mac_conn* prime_sn_add_mac_connection(prime_sn *sn, uint16_t lcid, uint8_t connType)
 {
 /*********************************************************
 *       Vars                                             *
 *********************************************************/
     mac_conn * p_mac_conn;
 /*********************************************************
 *       Code                                           *
 *********************************************************/
     prime_network_mutex_lock();
 	   if (sn->macConns == NUM_MAX_PRIME_MAC_CONNECTIONS){
 		     PRIME_LOG(LOG_ERR,"Maximum Number of PRIME MAC Connections\r\n");
         prime_network_mutex_unlock();
 		     return (mac_conn *) NULL;
 	   }
     /// Initial Malloc
     p_mac_conn = (mac_conn *) malloc(sizeof(mac_conn));
     if (p_mac_conn == (mac_conn *) NULL){
         PRIME_LOG(LOG_ERR, "Impossible to allocate space for a new MAC connection\r\n");
         prime_network_mutex_unlock();
         return (mac_conn *) NULL;
     }
     // Initialize Entry
     memset(p_mac_conn, 0, sizeof(mac_conn));
     p_mac_conn->connEntryLCID = lcid;
     p_mac_conn->connType = connType;

     /// Include MAC Connection on SN
     mchp_list_add_tail(p_mac_conn, &sn->macConnList);
     sn->macConns++;
     PRIME_LOG(LOG_DEBUG,"New PRIME MAC Connection for %s, lcid=%d, type=%d\r\n", eui48_to_str(sn->regEntryID,NULL), lcid, connType);
     prime_sn_print_mac_connection(p_mac_conn);
     prime_network_mutex_unlock();
     return p_mac_conn;
}

/*
 * \brief Look for MAC Connections for a PRIME SN
 * \param sn        PRIME SN Pointer Structure
 * \param lcid      PRIME SN MAC Connecion LCID
 *
 * \return Pointer to the PRIME SN MAC Connection Structure or NULL if doesn't exist
 */
mac_conn * prime_sn_find_mac_connection(prime_sn *sn, uint16_t lcid)
{
/*********************************************************
*       Variables Locales Definidas                      *
*********************************************************/
    mchp_list *entry, *tmp;
    mac_conn * p_mac_conn;
/*********************************************************
*       Code                                           *
*********************************************************/
    //PRIME_LOG(LOG_DEBUG,"Looking for PRIME SN MAC connection with lcid: %d\r\n",lcid);
    if (sn->macConns == 0){
       //PRIME_LOG(LOG_ERR, "No PRIME SN MAC Connections present\r\n");
       return (mac_conn *) NULL;
    }
    list_for_each_safe(entry, tmp, &sn->macConnList) {
        p_mac_conn = list_entry(entry, mac_conn, list);
        if (p_mac_conn->connEntryLCID == lcid){
           //PRIME_LOG(LOG_DEBUG, "PRIME SN MAC connection found!\r\n");
           return p_mac_conn;
        }
    }
    return (mac_conn *) NULL;
}

/*
 * \brief Print MAC Connection for a PRIME SN
 * \param mc        MAC Connection Pointer Structure
 *
 * \return -
 */
int prime_sn_print_mac_connection(mac_conn *mc)
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  /*********************************************************
  *       Code                                             *
  *********************************************************/
      if (mc == (mac_conn *) NULL)
  		   return -1;

      PRIME_LOG(LOG_DEBUG,"LCID      : %d\r\n",mc->connEntryLCID);
      PRIME_LOG(LOG_DEBUG,"ConnType  : %d\r\n",mc->connType);
      return 0;
}

/*
 * \brief Delete MAC Connections
 * \param sn        PRIME Service Node Pointer structure
 * \return Pointer to the new structure
 */
int prime_sn_del_mac_connections(prime_sn *sn)
 {
 /*********************************************************
 *       Local Vars                                       *
 *********************************************************/
     mchp_list *entry, *tmp;
     mac_conn * p_mac_conn;
 /*********************************************************
 *       Code                                             *
 *********************************************************/
     PRIME_LOG(LOG_INFO, "Deleting MAC Connections for PRIME SN 0x%s\r\n",eui48_to_str(sn->regEntryID,NULL));
     //pthread_mutex_lock(&prime_network_mutex);   // Not needed because must be locked before for getting the Service Node
     /// Free Memory on MAC Connection List
     list_for_each_safe(entry, tmp, &sn->macConnList) {
         p_mac_conn = list_entry(entry, mac_conn, list);
         if (p_mac_conn != NULL)
            free(p_mac_conn);
     }
     sn->macConns = 0;
     //pthread_mutex_unlock(&prime_network_mutex); // Not needed because must be locked before for getting the Service Node
     return 0;
}

/*
 * \brief Get PRIME Network Maximum Level
 *
 * \return 0
 */
uint8_t prime_network_get_max_level()
{
/*********************************************************
*       Code                                             *
*********************************************************/
   return prime_network_max_level;
}

/*
 * \brief Set PRIME Network Maximum Level
 * \param level    Prime Network Max Level
 * \return 0
 */
uint8_t prime_network_set_max_level(uint8_t level)
{
/*********************************************************
*       Code                                             *
*********************************************************/
   prime_network_max_level = level;
   return 0;
}

/*
 * \brief Init PRIME Network
 * \param
 *
 * \return -
 */
int prime_network_init()
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************************
*       Code                                             *
*********************************************************/
    // Initialize PRIME Network List
    INIT_LIST_HEAD(&prime_network);
    prime_network_sn = 0;

    // Add Base Node Device itself

    // Get Registered Devices from Base Node Modem
    prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_REGISTER_DEVICES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
    if (x_pib_confirm.m_u8Status != MLME_RESULT_DONE){
       PRIME_LOG(LOG_ERR, "PRIME PIB-MAC macListRegDevices request failed\r\n");
       return ERROR_PRIME_NETWORK_INIT;
    }
    // Next Events from PRIME Network will be handled by PRIME_API Callbacks

    return SUCCESS;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

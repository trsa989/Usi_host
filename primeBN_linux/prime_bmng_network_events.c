/**
 * \file
 *
 * \brief PRIME Base Management Network Events file.
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
#include <stdbool.h>
#include <stdlib.h>

#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sqlite3.h>

#include "globals.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_network.h"
#include "prime_bmng_network_events.h"
#include "prime_utils.h"
#include "prime_log.h"
#include "return_codes.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define DATABASE "/etc/config/prime_network_events.sql"
/************************************************************
*       Global Vars                                         *
*************************************************************/
static int prime_bmng_network_events_loglevel = PRIME_LOG_ERR;
/* Mutex for accessing PRIME BMNG Network Events Database */
pthread_mutex_t prime_bmng_network_events_mutex = PTHREAD_MUTEX_INITIALIZER;

/************************************************************
*       External Global Vars                                *
*************************************************************/
extern mchp_list prime_network;
extern struct st_configuration g_st_config;

/*
 * \brief  Get PRIME Network Management Loglevel
 *
 * \return loglevel
 */
int prime_bmng_network_event_get_loglevel()
{
	 return prime_bmng_network_events_loglevel;
}

/*
 * \brief  	   Set PRIME Network Management Loglevel
 *
 * \param [in]  int loglevel
 * \return 0
 */
int prime_bmng_network_event_set_loglevel(int loglevel)
{
	 PRIME_LOG(LOG_INFO,"Setting Network Events Loglevel to %i\r\n",loglevel);
   prime_bmng_network_events_loglevel = loglevel;
	 return SUCCESS;
}

/*
 * \brief PRIME BMNG Network Events Mutex Lock
 * \return
 */
int prime_bmng_network_events_mutex_lock()
{
    return pthread_mutex_lock(&prime_bmng_network_events_mutex);
}

/*
 * \brief PRIME BMNG Network Events Mutex Unlock
 * \return
 */
int prime_bmng_network_events_mutex_unlock()
{
    return pthread_mutex_unlock(&prime_bmng_network_events_mutex);
}

/*
 * \brief  PRIME Network Events Data Access Object
 * \param  cmd      DB Command
 * \param  event    Pointer to Network Event Structure
 * \return      integer
 */
int prime_network_events_db_dao (int cmd, bmng_net_event_t *event) //, sort sortby)
{
	FILE *filedb;
	sqlite3 *db;
	char sql_query[256];
	char *sql;
	char *error = 0;
	bool exists = false;
	int res;

  sql = &sql_query[0];
  /* Check if Database exists */
  if ((filedb = fopen(DATABASE, "r"))){
        fclose(filedb);
        exists = true;
  }
	/* Open Database */
	sqlite3_initialize();
	res = sqlite3_open(DATABASE,&db);
	if (res != SQLITE_OK){
		 /* Error creating/openning SQL Database */
		 PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"ERROR creating/opening SQLite DB in memory: %s\n",sqlite3_errmsg(db));
		 return -1;
	}
	/* Create Table if necessary */
	if (!exists){
//              "`timestamp` DATETIME, "
		 sql = "CREATE TABLE EVENTS ("
					 "`event` TEXT, "
					 "`eui48` TEXT, "
					 "`lnid` NUMBER, "
					 "`lsid` NUMBER, "
					 "`sid` NUMBER, "
					 "`alvRxcnt` NUMBER, "
					 "`alvTxcnt` NUMBER, "
					 "`alvTime` NUMBER)";
		/* Execute SQL statement */
		res = sqlite3_exec(db, sql, NULL, 0, &error);
		if (res != SQLITE_OK){
			 PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Error creating SQL Network Events Table\r\n");
			 sqlite3_free(error);
		}
		PRIME_NETWORK_EVENTS_LOG(LOG_INFO,"SQL Network Events Table created\r\n");
	}

	switch (cmd) {
			case NEW_ENTRY:
				// sprintf(sql_query,"INSERT INTO events VALUES (DATETIME(STRFTIME('%s','now'), 'unixepoch'), %s, %s, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
			 sprintf(sql_query,"INSERT INTO EVENTS VALUES ('%s', '%s', %d, %d, %d, %d, %d, %d);",      \
																																											 network_event_to_str(event->net_event, NULL),     \
																																																		 eui48_to_str(event->mac, NULL),     \
																																																												event->lnid,     \
																																																												event->lsid,     \
																																																												event->sid,      \
																																																												event->alvRxcnt, \
																																																												event->alvTxcnt, \
																																																												event->alvTime);
				res = sqlite3_exec(db, sql_query, NULL, 0, &error);
				if (res != SQLITE_OK){
						PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"SQL error: %s\n", error);
						sqlite3_free(error);
				}else{
						PRIME_NETWORK_EVENTS_LOG(LOG_INFO,"Inserted new Network Event on SQL Database\r\n");
				}
				break;
			case DB_REMOVE:
				strcpy(sql_query,"DELETE from EVENTS; VACUUM;");
				res = sqlite3_exec(db, sql_query, NULL, 0, &error);
				if (res != SQLITE_OK){
						PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"SQL error: %s\n", error);
						sqlite3_free(error);
				}
				PRIME_NETWORK_EVENTS_LOG(LOG_INFO,"Network Event SQL Database cleaned\r\n");
				break;
			default:
				break;
	}
	sqlite3_shutdown();
	sqlite3_close(db);
	return res;
}

/**
* \brief   Base Management Callback Network Event Indication
*          Asyncronous events from PRIME Network - Cqan interfere normal use of USI Interface
* \param   px_net_event: event
*/
void prime_bmng_network_event_msg_cb(bmng_net_event_t *px_net_event)
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
   prime_sn * sn = (prime_sn *) NULL;
 /*********************************************
 *       Code                                 *
 **********************************************/
   PRIME_NETWORK_EVENTS_LOG(LOG_DEBUG,"Network Event %s\r\n"                                 \
   																		" * MAC      : %s\r\n"																 \
																			" * SID      : %d\r\n" 																 \
																			" * LNID     : %d\r\n"																 \
																			" * LSID     : %d\r\n"																 \
																			" * alvRxcnt : %d\r\n"																 \
																			" * alvTxcnt : %d\r\n" 																 \
																			" * alvTime  : %d\r\n",																 \
	 																			network_event_to_str(px_net_event->net_event,NULL),	 \
																				eui48_to_str(px_net_event->mac, NULL),							 \
																				px_net_event->sid,																	 \
																				px_net_event->lnid,																	 \
																				px_net_event->lsid,																	 \
																				px_net_event->alvRxcnt,															 \
																				px_net_event->alvTxcnt,															 \
																				px_net_event->alvTime);

	 /* Add Network Event to SQL Database */
   prime_network_events_db_dao(NEW_ENTRY,px_net_event);

   sn = prime_network_find_sn(&prime_network,px_net_event->mac);
   if (sn == (prime_sn *) NULL){
      PRIME_NETWORK_EVENTS_LOG(LOG_DBG,"Service Node not registered\r\n");
   }

   // Decode Events
   switch (px_net_event->net_event){
     case BMNG_NET_EVENT_REGISTER:
        if (sn == (prime_sn *) NULL){
           /* First Time Manager sees this Service Node */
           sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
           if (sn == (prime_sn *) NULL){
              PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
              break;
           }
        }
        prime_network_mutex_lock();
        sn->state = SN_STATE_TERMINAL;
        sn->regEntryLNID  = px_net_event->lnid;       // LNID allocated to this Node
        sn->regEntryLSID  = px_net_event->lsid;       // SID Allocated to this Nodes
        sn->regEntrySID   = px_net_event->sid;        // SID of Switch through which this Node is connected
        sn->regEntryState = REGISTER_STATE_TERMINAL;  // Service Node Register State
        sn->alvRxcnt      = px_net_event->alvRxcnt;   // Alive Received Counter
        sn->alvTxcnt      = px_net_event->alvTxcnt;   // Alive Transmitted Counter
        sn->alvTime       = px_net_event->alvTime;    // Alive Time
        sn->state         = SN_STATE_TERMINAL;        // Service Node State
        sn->registered    = 1;
        prime_network_mutex_unlock();
        break;
     case BMNG_NET_EVENT_UNREGISTER:
        if (sn == (prime_sn *) NULL){
           /* First Time Manager sees this Service Node */
           sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
           if (sn == (prime_sn *) NULL){
              PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
              break;
           }
        }
        prime_network_mutex_lock();
        sn->state = SN_STATE_DISCONNECTED;
        sn->regEntryLNID = px_net_event->lnid;       // LNID allocated to this Node
        sn->regEntryLSID = px_net_event->lsid;       // SID Allocated to this Nodes
        sn->regEntrySID  = px_net_event->sid;        // SID of Switch through which this Node is connected
        sn->alvRxcnt     = px_net_event->alvRxcnt;   // Alive Received Counter
        sn->alvTxcnt     = px_net_event->alvTxcnt;   // Alive Transmitted Counter
        sn->alvTime      = px_net_event->alvTime;    // Alive Time
        sn->state        = SN_STATE_DISCONNECTED;    // Service Node State Disconnected
        sn->registered   = 0;
        prime_network_mutex_unlock();
        break;
     case BMNG_NET_EVENT_PROMOTE:
        /* Service Node Promotion Event */
        if (sn == (prime_sn *) NULL){
           /* First Time Manager sees this Service Node */
           sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
           if (sn == (prime_sn *) NULL){
              PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
              break;
           }
        }
        prime_network_mutex_lock();
        sn->state = SN_STATE_SWITCH;
        sn->regEntryLNID  = px_net_event->lnid;       // LNID allocated to this Node
        sn->regEntryLSID  = px_net_event->lsid;       // LSID Allocated to this Nodes
        sn->regEntrySID   = px_net_event->sid;        // SID of Switch through which this Node is connected
        sn->regEntryState = REGISTER_STATE_SWITCH;    // SID of Switch through which this Node is connected
        sn->alvRxcnt      = px_net_event->alvRxcnt;   // Alive Received Counter
        sn->alvTxcnt      = px_net_event->alvTxcnt;   // Alive Transmitted Counter
        sn->alvTime       = px_net_event->alvTime;    // Alive Time
        prime_network_mutex_unlock();
        break;
     case BMNG_NET_EVENT_DEMOTE:
        if (sn == (prime_sn *) NULL){
            /* First Time Manager sees this Service Node */
            sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
            if (sn == (prime_sn *) NULL){
               PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
               break;
            }
        }
        prime_network_mutex_lock();
        sn->state = SN_STATE_TERMINAL;
        sn->regEntryLNID  = px_net_event->lnid;       // LNID allocated to this Node
        sn->regEntryLSID  = px_net_event->lsid;       // LSID Allocated to this Nodes
        sn->regEntrySID   = px_net_event->sid;        // SID of Switch through which this Node is connected
        sn->regEntryState = REGISTER_STATE_TERMINAL;  // Register State Terminal
        sn->alvRxcnt      = px_net_event->alvRxcnt;   // Alive Received Counter
        sn->alvTxcnt      = px_net_event->alvTxcnt;   // Alive Transmitted Counter
        sn->alvTime       = px_net_event->alvTime;    // Alive Time
        prime_network_mutex_unlock();
       break;
     case BMNG_NET_EVENT_ALIVE:
        if (sn == (prime_sn *) NULL){
           /* First Time Manager sees this Service Node */
           sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
           if (sn == (prime_sn *) NULL){
              PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
              break;
           }
           // sn->state = SN_STATE_TERMINAL;
        }
        prime_network_mutex_lock();
        sn->regEntryLNID = px_net_event->lnid;       // LNID allocated to this Node
        sn->regEntryLSID = px_net_event->lsid;       // LSID Allocated to this Nodes
        sn->regEntrySID  = px_net_event->sid;        // SID of Switch through which this Node is connected
        sn->alvRxcnt     = px_net_event->alvRxcnt;   // Alive Received Counter
        sn->alvTxcnt     = px_net_event->alvTxcnt;   // Alive Transmitted Counter
        sn->alvTime      = px_net_event->alvTime;    // Alive Time
        sn->registered   = 1;
        prime_network_mutex_unlock();
        break;
     case BMNG_NET_EVENT_REBOOT:
       /* Base Node Reboot Event -> Reload Base Node Modem Configuration :
          * Enable Sniffer if necessary
          * Set MAC+DUK
					* Remove all the alive information about connections, registered devices...
       */
       // TBD
       break;
     case BMNG_NET_EVENT_NO_DUK:
       if (sn == (prime_sn *) NULL){
          /* First Time Manager sees this Service Node */
          sn = prime_network_add_sn(&prime_network,px_net_event->mac, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
          if (sn == (prime_sn *) NULL){
             PRIME_NETWORK_EVENTS_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
             break;
          }
          prime_network_mutex_lock();
          sn->state = SN_STATE_REGISTERING;
          sn->registered = 0;
          prime_network_mutex_unlock();
       }
       break;
     case BMNG_NET_EVENT_UNKNOWN_NODE:
       /* Registration Message from a SN which is not included in the whitelist */
       break;
     case BMNG_NET_EVENT_ERROR:
       /* ????? */
       break;
   }
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

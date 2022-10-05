/**
 * \file
 *
 * \brief Common Utils
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "prime_log.h"
#include "return_codes.h"

/*
*   An easy way to handle SAMA5D21 gpios using sysfs
*/

/*
 * @brief	Set GPIO direction
 * @param	gpioname   LED name
 * @param	dir        LED trigger [ 0:none, 1:timer, 2:heartbeat, 3:gpio ]
 * @return	0 on success
 * 			   -1 otherwise
 */
 static int set_led_trigger(const char *ledname, int trigger)
 {
 	char file[36];
  int  fd_led;  /* LED File descriptor */
  int  ret = 0;

 	sprintf(file, "/sys/class/leds/%s/trigger", ledname);
 	fd_led = open(file, O_WRONLY | O_SYNC);
 	if (fd_led < 0) {
 		PRIME_LOG(LOG_ERR,"Couldn't open LED %s trigger file: %s\n", ledname, strerror(errno));
 		ret = -1;
 	}

  if ((trigger < 0) || (trigger > 3)){
    PRIME_LOG(LOG_ERR,"Invalid LED trigger %d\n", trigger);
 		ret = -1;
  }

 	if (!ret){
    if (trigger == 3){
   		if (4 != write(fd_led, "gpio", 4)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set LED %s trigger to gpio: %s\n", ledname, strerror(errno));
   			ret = -2;
   		}
   	}else if (trigger == 2){
   		if (10 != write(fd_led, "heartbeat", 10)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set LED %s trigger to heartbeat: %s\n", ledname, strerror(errno));
   			ret = -3;
   		}
   	}else if (trigger == 1){
   		if (5 != write(fd_led, "time", 5)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set LED %s trigger to time: %s\n", ledname, strerror(errno));
   			ret = -4;
   		}
    }else{
   		if(5 != write(fd_led, "none", 5)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set LED %s trigger to none: %s\n", ledname, strerror(errno));
   			ret = -5;
   		}
   	}
  }

 	close(fd_led);
 	return ret;
 }

 /*
 * @brief	Set LED timer trigger delay
 * @param	ledname     LED name
 * @param  delay_on   Time LED on in ms
 * @param  delay_on   Time LED off in ms
 * @return	  0 on success
 * 			   -X otherwise
 */
 int set_led_timer_delays(const char *ledname, int delay_on, int delay_off)
 {
  	char buf[36];
  	int  ret;
   int  fd_led;

   if (set_led_trigger(ledname,1)){ /* Timer */
     PRIME_LOG(LOG_ERR,"Error setting %s trigger Timer\n", ledname);
     return -2;
   }

    sprintf(buf, "/sys/class/leds/%s/delay_on", ledname);
  	fd_led = open(buf, O_RDWR);
  	if (fd_led > 0) {
  		 snprintf(buf, 6, "%d", delay_on);
  		 ret = write(fd_led, buf, 6);
  		 if (ret < 0) {
          close(fd_led);
  			  PRIME_LOG(LOG_ERR,"Failed to set led %s timer delay_on:%s \n", ledname,strerror(errno));
  			  return -1;
  		 }
       close(fd_led);
    }

    sprintf(buf, "/sys/class/leds/%s/delay_off", ledname);
  	fd_led = open(buf, O_RDWR);
    if (fd_led > 0) {
       snprintf(buf, 6, "%d", delay_off);
  		 ret = write(fd_led, buf, 6);
  		 if (ret < 0) {
          close(fd_led);
  			  PRIME_LOG(LOG_ERR,"Failed to set led %s timer delay_off:%s \n", ledname,strerror(errno));
  			  return -1;
  		 }
  		 close(fd_led);
  	}
  	return 0;
 }

/*
* @brief	Set LED Heartbeat trigger invert
* @param	ledname     LED name
* @param  invert      Heartbeat LED ON/OFF or OFF/ON
* @return	  0 on success
* 			   -X otherwise
*/
int set_led_heartbeat_invert(const char *ledname, int invert)
{
 	char buf[36];
 	int  ret;
  int  fd_led;

  if (set_led_trigger(ledname,2)){ /* Heartbeat */
    PRIME_LOG(LOG_ERR,"Error setting %s trigger heartbeat\n", ledname);
    return -2;
  }

  sprintf(buf, "/sys/class/leds/%s/invert", ledname);
 	fd_led = open(buf, O_RDWR);
 	if (fd_led > 0) {
 		 snprintf(buf, 2, "%d", invert ? 1 : 0);
 		 ret = write(fd_led, buf, 2);
 		 if (ret < 0) {
 			  PRIME_LOG(LOG_ERR,"Failed to set led %s heartbeat invert value:%s \n", ledname,strerror(errno));
 			  return -1;
 		 }

 		 close(fd_led);
 		 if (ret == 2)
        return 0;
 	}
 	return -1;
}

 /*
  * @brief	Set LED brightness
  * @param	ledname     LED name
  * @param  brightness  brightness to set [0-255]
  * @return	  0 on success
  * 			   -X otherwise
  */
 int set_led_brightness(const char *ledname, int brightness)
 {
 	char buf[36];
 	int  ret;
  int  fd_led;

  if (set_led_trigger(ledname,0)){ /* None - Not really needed */
    PRIME_LOG(LOG_ERR,"Error setting %s trigger\n", ledname);
    return -2;
  }

  sprintf(buf, "/sys/class/leds/%s/brightness", ledname);
 	fd_led = open(buf, O_RDWR);
 	if (fd_led > 0) {
 		 snprintf(buf, 2, "%d", brightness ? 1 : 0);
 		 ret = write(fd_led, buf, 2);
 		 if (ret < 0) {
 			  PRIME_LOG(LOG_ERR,"Failed to set led %s brightness:%s \n", ledname,strerror(errno));
 			  return -1;
 		 }

 		 close(fd_led);
 		 if (ret == 2)
        return 0;
 	}
 	return -1;
 }

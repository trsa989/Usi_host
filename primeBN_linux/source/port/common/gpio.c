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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "prime_log.h"
#include "return_codes.h"
#include "gpio.h"

/*
*   An easy way to handle SAMA5D21 gpios using sysfs
*/

/*
 * @brief	Set GPIO direction
 * @param	gpioname   GPIO name -> [PAx,PBx,PCx,PDx]
 * @param	dir        GPIO direction [ 0:in, 1:out, 2:HiZ]
 * @return	0 on success
 * 			   -1 otherwise
 */
 static int set_gpio_direction(const char *gpioname, int dir)
 {
 	char file[36];
  int fd_gpio;  /* GPIO File descriptor */
  int ret = 0;

 	sprintf(file, "/sys/class/gpio/%s/direction", gpioname);
 	fd_gpio = open(file, O_WRONLY | O_SYNC);
 	if (fd_gpio < 0) {
 		PRIME_LOG(LOG_ERR,"Couldn't open GPIO %s direction file: %s\n", gpioname, strerror(errno));
 		ret = -1;
 	}

  if ((dir < 0) || (dir > 2)){
    PRIME_LOG(LOG_ERR,"GPIO Invalid direction %d\n", dir);
 		ret = -1;
  }

 	if (!ret){
    if (dir == 2){
   		if (4 != write(fd_gpio, "high", 4)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set GPIO %s direction to out/high: %s\n", gpioname, strerror(errno));
   			ret = -2;
   		}
   	}else if (dir == 1){
   		if (3 != write(fd_gpio, "out", 3)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set GPIO %s direction to out: %s\n", gpioname, strerror(errno));
   			ret = -3;
   		}
   	}else{
   		if(2 != write(fd_gpio, "in", 2)) {
   			PRIME_LOG(LOG_ERR,"Couldn't set GPIO %s direction to in: %s\n", gpioname, strerror(errno));
   			ret = -4;
   		}
   	}
  }

 	close(fd_gpio);
 	return ret;
 }

 /*
  * @brief	Export GPIO
  * @param	gpioname   GPIO name (correspondance bellow)
  *        [PA0...PA31,PB0...PB31,PC0...PC31,PD0...PD31]
  *        [0.......31,32......63,64......95,96.....127]
  * @return	  0 on success
  * 			   -X otherwise
  */
 static int gpio_export(const char *gpioname)
 {
 	int gpionumber = 0;
  char buf[36];
  int fd_gpio;  /* GPIO File descriptor */
  int ret = 0;

 	/* GPIO has already been exported ? */
 	sprintf(buf, "/sys/class/gpio/%s/value", gpioname);
 	fd_gpio = open(buf, O_WRONLY);
 	if (fd_gpio != -1) {
 		 close(fd_gpio);
 		 return 0;
 	}

  /* Translation to gpio number */
  gpionumber=0;
  if ('B' == gpioname[1])
     gpionumber+=32;
  else if ('C' == gpioname[1])
     gpionumber+=64;
  else if ('D' == gpioname[1])
    gpionumber+=96;
  gpionumber+=atoi(&gpioname[2]);

 	fd_gpio = open("/sys/class/gpio/export", O_WRONLY | O_SYNC);
 	if (fd_gpio != -1) {
 		sprintf(buf, "%d", gpionumber);
 		ret = write(fd_gpio, buf, strlen(buf));
 		if(ret < 0) {
 			PRIME_LOG(LOG_ERR,"GPIO Couldn't write to export %s\n", gpioname);
 			ret = -2;
 		}
 		close(fd_gpio);
 	}else {
    PRIME_LOG(LOG_ERR,"Couldn't open export file for GPIO %s\n", gpioname);
 		ret = -1;
 	}
 	return ret;
 }

 /*
  * @brief	Get GPIO value
  * @param	gpioname   GPIO name (correspondance bellow)
  *        [PA0...PA31,PB0...PB31,PC0...PC31,PD0...PD31]
  *        [0.......31,32......63,64......95,96.....127]
  * @return	  GPIO value [0-1]
  * 			   -X otherwise
  */
 int get_gpio_value(const char *gpioname)
 {
 	char in[3] = {0, 0, 0};
 	char file[36];
 	int  nread;
  int  fd_gpio;

  if (gpio_export(gpioname)){
    PRIME_LOG(LOG_ERR, "Error exporting %s\n", gpioname);
    return -1;
  }

  if (set_gpio_direction(gpioname,0)){ /* Input */
    PRIME_LOG(LOG_ERR,"Error setting %s direction\n", gpioname);
    return -2;
  }

 	sprintf(file, "/sys/class/gpio/%s/value", gpioname);
 	fd_gpio = open(file, O_RDWR | O_SYNC);
 	if (fd_gpio < 0) {
 		PRIME_LOG(LOG_ERR,"Failed to get gpio %s value:%s \n", gpioname,strerror(errno));
    return -1;
 	}

 	do {
 		  nread = read(fd_gpio, in, 1);
 	} while (nread == 0);
 	if (nread == -1){
 		PRIME_LOG(LOG_ERR,"Failed to get gpio %s value:%s \n", gpioname,strerror(errno));
 		return -1;
 	}

 	close(fd_gpio);
 	return atoi(in);
 }

 /*
  * @brief	Set GPIO value
  * @param	gpioname   GPIO name (correspondance bellow)
  *        [PA0...PA31,PB0...PB31,PC0...PC31,PD0...PD31]
  *        [0.......31,32......63,64......95,96.....127]
  * @param  value  Value to set [0-1]
  * @return	  0 on success
  * 			   -X otherwise
  */
 int set_gpio_value(const char *gpioname, int value)
 {
 	char buf[36];
 	int  ret;
  int  fd_gpio;

  if (gpio_export(gpioname)){
    PRIME_LOG(LOG_ERR,"Error exporting %s\n", gpioname);
    return -1;
  }

  if (set_gpio_direction(gpioname,1)){ /* Output */
    PRIME_LOG(LOG_ERR,"Error setting %s direction\n", gpioname);
    return -2;
  }

  sprintf(buf, "/sys/class/gpio/%s/value", gpioname);
 	fd_gpio = open(buf, O_RDWR);
 	if (fd_gpio > 0) {
 		 snprintf(buf, 2, "%d", value ? 1 : 0);
 		 ret = write(fd_gpio, buf, 2);
 		 if (ret < 0) {
 			  PRIME_LOG(LOG_ERR,"Failed to set gpio %s value:%s \n", gpioname,strerror(errno));
 			  return -1;
 		 }

 		 close(fd_gpio);
 		 if (ret == 2)
        return 0;
 	}
 	return -1;
 }

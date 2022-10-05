/**
 * \file
 *
 * \brief G3 Coordinator App
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
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>

#include "Config.h"
#include "globals.h"
#include "Logger.h"
#include "tun.h"

/*
 * @brief	Set TUN Queue
 * @param	fd_tun	TUN file descriptor
 * @param	enable	Enable/Disable Queue
 * @return	0 on success
 *                         -1 otherwise
 */
static int tun_set_queue(int fd_tun, int enable)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	if (enable) {
		ifr.ifr_flags = IFF_ATTACH_QUEUE;
	} else {
		ifr.ifr_flags = IFF_DETACH_QUEUE;
	}

	return ioctl(fd_tun, TUNSETQUEUE, (void *)&ifr);
}

/*
 * @brief	Alloc TUN Device
 * @param	dev	TUN device name
 * @param	flags	Tun device flags
 * @return	Tun device file descriptor
 *                         -1 otherwise
 */
int tun_alloc(const char *dev, int flags)
{
	struct ifreq ifr;
	int fd, err;
	char *clonedev = "/dev/net/tun";

	/* open the clone device */
	if ((fd = open(clonedev, O_RDWR | O_SYNC)) < 0) {
		LOG_ERR(Log("Error open TUN clone device"));
		return fd;
	}

	/* preparation of the struct ifr, of type "struct ifreq" */
	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

	if (*dev) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	/* try to create the device */
	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		LOG_ERR(Log("Error creating TUN device"));
		close(fd);
		return err;
	}

	strcpy(dev, ifr.ifr_name);

	if (ioctl(fd, TUNSETPERSIST, 0) < 0) {
		perror("disabling TUNSETPERSIST");
		exit(1);
	}

	tun_set_queue(fd, 0);

	LOG_INFO(Log("Successfully created TUN device"));

	return fd;
}

/*
 * @brief	Configure TUN Device
 * @param	tun_name	   TUN device name
 * @param	panId	       G3 Network PAN ID
 * @param	ipaddr	     G3 Coordinator IP Address
 * @param	prefix_len	 Prefix Lenght
 * @return	Tun device file descriptor
 *                         -1 otherwise
 */
int tun_configure(const char *tun_name, uint16_t panId, const char *ipaddr, uint8_t prefix_len)
{
	char sz_command[512];
	int ret;
	if ((NULL == tun_name) || (NULL == ipaddr)) {
		return -1;
	}

	/* Configure IPv6 Local Link Address regarding RFC4944 */
	sprintf(sz_command, "ip address ad FE80::%X:FF:FE00:0/80 dev %s ", panId, tun_name);
	ret = system(sz_command);
	if (ret != 0) {
		LOG_ERR(Log("Error: Can not configure IPv6 address %s/%d", ipaddr, prefix_len));
		return(ret);
	}

	/* Configure ULA IPv6 Address */
	sprintf(sz_command, "ip address ad %s/%d dev %s ", ipaddr, prefix_len, tun_name);
	ret = system(sz_command);
	if (ret != 0) {
		LOG_ERR(Log("Error: Can not configure IPv6 address %s/%d", ipaddr, prefix_len));
		return(ret);
	}

	/* wake up tun interface */
	sprintf(sz_command, "ifconfig %s up", tun_name);
	ret = system(sz_command);
	if (ret != 0) {
		LOG_ERR(Log("Error: Can not wake up %S interface", tun_name));
		return(ret);
	}

	sprintf(sz_command, "ifconfig %s mtu 1280", tun_name);
	ret = system(sz_command);
	if (ret != 0) {
		LOG_ERR(Log("Error: Can not configure tun interface %s/%d", ipaddr, prefix_len));
		return(ret);
	}

	return 0;
}

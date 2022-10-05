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
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#include "Config.h"
#include "globals.h"
#include "Logger.h"
#include "udp_responder.h"

#ifdef APP_CONFORMANCE_TEST

/**
 * \brief Open UDP Server for Conformance Responder
 * @param const char* iface : Iface name for G3 PLC Network
 */
static int open_udp_server(const char *iface)
{
	struct sockaddr_in6 si_me;
	int s;

	if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin6_family = AF_INET6;
	si_me.sin6_port = htons(CONFORMANCE_SOCKET_PORT);
	si_me.sin6_addr = in6addr_any;
	if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
		return -1;
	}

	struct ipv6_mreq mreq1, mreq2;
	memset(&mreq2, 0, sizeof(mreq2));
	memset(&mreq1, 0, sizeof(mreq1));

	inet_pton(AF_INET6, APP_IPV6_MULTICAST_ADDR_0, &mreq1.ipv6mr_multiaddr);
	mreq1.ipv6mr_interface = if_nametoindex(iface);
	setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq1, sizeof(mreq1));

	inet_pton(AF_INET6, APP_IPV6_MULTICAST_ADDR_1, &mreq2.ipv6mr_multiaddr);
	mreq2.ipv6mr_interface = if_nametoindex(iface);
	setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq2, sizeof(mreq2));

	return s;
}

/**
 * \brief Open UDP Server for Conformance Responder
 * @param unsigned char * data : Data to be sent
 * @param int len : Length of Data to be sent
 * @param struct sockaddr_in6* : IPv6 Destination
 * @param int dest_len : IPv6 Destination Length
 */
static int send_ping_unicast(unsigned char *data, int len, struct sockaddr_in6 *dest, int dest_len)
{
	int sd, sockopt;
	struct icmp6_hdr *pkt;
	unsigned char *pingbuff;

	sd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sd < 0) {
		LOG_ERR(Log("Ping Unicast Error - SOCKET - %d %s\n", errno, strerror(errno)));
		return -1;
	}

	pingbuff = malloc(1500);
	pkt = (struct icmp6_hdr *)pingbuff;
	memset(pingbuff, 0, 1500);

	pkt->icmp6_type = ICMP6_ECHO_REQUEST;
	pkt->icmp6_id = getpid();
	sockopt = offsetof(struct icmp6_hdr, icmp6_cksum);

	if (setsockopt(sd, SOL_RAW, IPV6_CHECKSUM, &sockopt, sizeof(sockopt)) < 0) {
		LOG_ERR(Log("Ping Unicast Error - IPV6_CHECKSUM - %d %s\n", errno, strerror(errno)));
		free(pingbuff);
		return -1;
	}

	sockopt = CONFORMANCE_PING_TTL;
	if (setsockopt(sd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &sockopt, sizeof(sockopt)) < 0) {
		LOG_ERR(Log("Ping Unicast Error - IPV6_UNICAST_HOPS - %d %s\n", errno, strerror(errno)));
		free(pingbuff);
		return -1;
	}

	/* copy data to icmp buufer */
	memcpy(pingbuff + sizeof(struct icmp6_hdr), data, len);
	if (sendto(sd, pingbuff, sizeof(struct icmp6_hdr) + len, 0, (struct sockaddr *)dest, dest_len) < 0) {
		LOG_ERR(Log("Ping Unicast Error - sendto - %d %s\n", errno, strerror(errno)));
		free(pingbuff);
		exit( -1);
	}

	close(sd);
	free(pingbuff);
	return 0;
}

/**
 * \brief UDP Responder Server Thread
 * @param const char * iface : Waiting Messages on this interface
 */
int udp_responder_thread(const char *iface)
{
	int udp_server_fd, len;
	unsigned char buffer[1280];
	struct sockaddr_in6 source;
	socklen_t source_len;

	LOG_INFO(Log("Starting CONFORMANCE G3 app"));
	udp_server_fd = open_udp_server(iface);
	if (udp_server_fd <= 0) {
		pthread_exit(NULL);
		return -1;
	}

	source_len = sizeof(struct sockaddr_in6);
	while (1) {
		len = recvfrom(udp_server_fd, buffer, 1280, 0, (struct sockaddr *)&source, &source_len);
		LOG_DBG(Log("UDP_Responder - Msg received - length=%d\n", len));
		if (len < 0) {
			LOG_ERR(Log("UDP Responder Length Error"));
			pthread_exit(NULL);
			return -1;
		} else {
			if (buffer[0] == 0x01) {
				/*
				 * In order to validate RFC6282 UDP header compression, an exchange of frames transported over UDP is required. For this purpose a very
				 * simple UDP responder needs to be implemented.
				 * - The device listen to port 0xF0BF over UDP.
				 * - The first byte of UDP payload indicate the message type, the rest of the UDP payload correspond to the message data:
				 * - 0x01(UDP request): upon reception, the device must send back an UDP frame to the original sender, using the received frame source
				 * and destination ports for the destination and source
				 * ports (respectively) of the response frame, setting the message type to 0x02 (UDP reply) and copying the message data from the
				 * request;
				 * - 0x02 (UDP reply): this message is dropped upon reception;
				 * - other value: this message is dropped upon reception;
				 */

				/* UDP responder needed for conformance testing */
				/* update UPD payload */
				buffer[0] = 0x02;
				/* Reply back*/
				sendto(udp_server_fd, buffer, len, 0, (struct sockaddr *)&source, source_len);
			} else if (buffer[0] == 0x03) {
				/*
				 * The following extension is added to the UDP responder, in order to make the IUT generate ICMPv6
				 * ECHO Request frames.
				 * The new message type 0x03 (ICMPv6 ECHO request trigger) is added: upon reception, the device
				 * must send back an ICMPv6 ECHO request frame to the original sender. The ICMPv6 Identifier,
				 * Sequence Number and Data fields are filled (in that order) using the received message data.
				 * Example: If an UDP message with a payload of ï¿½03 010203040506070809ï¿½ is received, then an
				 * ICMPv6 echo request is sent back with an ICMPv6 content of ï¿½80 00 xxxx 0102 0304 0506070809ï¿½
				 * (where xxxx correspond to the ICMP checksum).
				 */
				send_ping_unicast(buffer + 1, len - 1, &source, source_len);
			}
		}
	}
}

#endif /* APP_CONFORMANCE_TEST */

#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "Serial.h"
#include "Wiz5100.h"
#include "UDP.h"
#include "Common.h"
#include "DHCP.h"

void start_dhcp(uint8_t *source_mac, UDPSock *socket)
{
	memcpy(dhcp_source_mac, source_mac, 6);
	memset(dhcp_local_ip, 0, 4);
	memset(dhcp_submask, 0, 4);
	memset(dhcp_gw, 0, 4);
	memset(dhcp_server_ip, 0, 4);
	T1 = T2 = lease_time = 0;

	request_lease(socket);
}

int request_lease(UDPSock *socket) 
{
	unsigned char state = STATE_DHCP_START;
	unsigned long start;
	//uint32_t respid;

	id = Millis::millis(); // TODO - fix this
	printf("id=%x\n", (uint16_t)id);
	
	start = Millis::millis();
	
	while (state != STATE_DHCP_LEASED) {
		if (state == STATE_DHCP_START) {
			id++;
			send_dhcp_msg(socket, DHCP_DISCOVER, ((Millis::millis() - start) / 1000));
			state = STATE_DHCP_DISCOVER;
		}
		else if (state == STATE_DHCP_DISCOVER) {
			if (receive_dhcp_msg(socket, DHCP_TIMEOUT, &id) == DHCP_OFFER) {
				printf("received an offer:\n");
				printf("local_ip=%d.%d.%d.%d\nlocal_mask=%d.%d.%d.%d\nlocal_gw=%d.%d.%d.%d\n",
						dhcp_local_ip[0], dhcp_local_ip[1], dhcp_local_ip[2],
						dhcp_local_ip[3], dhcp_submask[0], dhcp_submask[1], 
						dhcp_submask[2], dhcp_submask[3], dhcp_gw[0], 
						dhcp_gw[1], dhcp_gw[2], dhcp_gw[3]);
				send_dhcp_msg(socket, DHCP_REQUEST, ((Millis::millis() - start) / 1000));
				state = STATE_DHCP_REQUEST;
			}
			else 
				state = STATE_DHCP_START;
		}
		else if (state == STATE_DHCP_REQUEST)
		{
			if (receive_dhcp_msg(socket, DHCP_TIMEOUT, &id) == DHCP_ACK) {
				state = STATE_DHCP_LEASED;
				if (lease_time == 0)
					lease_time = DHCP_DEFAULT_LEASE;
				if (T1 == 0)
					T1 = lease_time >> 1;
				if (T2 == 0)
					T2 = lease_time << 1;
			}
			else
				state = STATE_DHCP_START;
		}
	}
	++id;
	Wiz5100::close(DHCP_SOCK);
	Wiz5100::setStatic(dhcp_source_mac, dhcp_local_ip, dhcp_submask, dhcp_gw);
	return 0;
}

void send_dhcp_msg(UDPSock *socket, uint8_t type, uint16_t time_passed)
{
	uint32_t rev_id = htonl(id);
	uint8_t buf[32];
	uint8_t dest_ip[] = {255, 255, 255, 255}; // broadcast
	int i;
	
	memset(buf, 0, 32);

	socket->UDPBegin(dest_ip, DHCP_SERVER_PORT);

	buf[0] = DHCP_BOOTREQUEST;
	buf[1] = DHCP_HTYPE10MB;
	buf[2] = DHCP_HLENETHERNET;
	buf[3] = DHCP_HOPS;
	memcpy(buf + 4, &rev_id, 4);
	buf[8] = (time_passed & 0xFF00) >> 8;
	buf[9] = time_passed & 0x00FF;
	buf[10] = (DHCP_FLAGSBROADCAST & 0xFF00) >> 8;
	buf[11] = DHCP_FLAGSBROADCAST & 0x00FF;
	// ciaddr: already zeroed
	// yiaddr: already zeroed
	// siaddr: already zeroed
	// giaddr: already zeroed
	socket->UDPFillbuf(buf, 28);
	memset(buf, 0, 32);
	memcpy(buf, dhcp_source_mac, 6); // chaddr
	socket->UDPFillbuf(buf, 16);
	memset(buf, 0, 32);
	// fill 192b zeroed out
	for (i = 0; i < 6; ++i)
		socket->UDPFillbuf(buf, 32);
	buf[0] = (uint8_t)((DHCP_MAGIC_COOKIE >> 24) & 0xFF);
	buf[1] = (uint8_t)((DHCP_MAGIC_COOKIE >> 16) & 0xFF);
	buf[2] = (uint8_t)((DHCP_MAGIC_COOKIE >> 8) & 0xFF);
	buf[3] = (uint8_t)(DHCP_MAGIC_COOKIE & 0xFF);
	buf[4] = DHCP_MESSAGE_TYPE;
	buf[5] = 1;
	buf[6] = type;
	buf[7] = DHCP_CLIENT_ID;
	buf[8] = 7;
	buf[9] = 1;
	memcpy(buf + 10, dhcp_source_mac, 6);
	buf[16] = DHCP_HOSTNAME;
	buf[17] = strlen(HOSTNAME); // hostname
	memcpy(buf + 18, HOSTNAME, strlen(HOSTNAME));
	socket->UDPFillbuf(buf, 18 + strlen(HOSTNAME));
	if (type == DHCP_REQUEST) {
		buf[0] = DHCP_REQUESTED_IP;
		buf[1] = 4;
		memcpy(buf + 2, dhcp_local_ip, 4);
		buf[6] = DHCP_SERVER_ID;
		buf[7] = 4;
		memcpy(buf + 8, dhcp_server_ip, 4);
		socket->UDPFillbuf(buf, 12);
	}
	buf[0] = DHCP_PARAM_REQUEST;
	buf[1] = 6;
	buf[2] = DHCP_SUBMASK;
	buf[3] = DHCP_ROUTER;
	buf[4] = DHCP_DNS;
	buf[5] = DHCP_DOMAIN;
	buf[6] = DHCP_T1;
	buf[7] = DHCP_T2;
	buf[8] = DHCP_ENDOPT;
	socket->UDPFillbuf(buf, 9);
	socket->UDPSend();
}

uint8_t receive_dhcp_msg(UDPSock *socket, uint32_t timeout, uint32_t *resid)
{
	uint8_t c, type = 0, optlen;
	int i;
	uint32_t start = Millis::millis();
	uint32_t id;
	RIP_MSG_FIXED rmsg;

	while (socket->UDPStartRecv() == 0) {
		if (Millis::millis() - start > timeout)
			return DHCP_TIMEDOUT;
	}
	socket->UDPRead((uint8_t*)&rmsg, sizeof(RIP_MSG_FIXED));

	if (rmsg.op != DHCP_BOOTREPLY) { // || remote_port != DHCP_SERVER_PORT)
		socket->UDPFlush();
		return 0;
	}

	id = htonl(rmsg.xid);
	if (memcmp(rmsg.chaddr, dhcp_source_mac, 6) != 0 || id != *resid) {
		socket->UDPFlush();
		return 0; // ...
	}
	memcpy(dhcp_local_ip, rmsg.yiaddr, 4);
	// skip until the interesting options
	for (i = 240 - sizeof(RIP_MSG_FIXED); i > 0; i--)
		socket->UDPRead(&c, 1);

	while (socket->UDPRead(&c, 1) > 0) {
		switch (c) {
			case DHCP_ENDOPT:
				break;
			case DHCP_PADOPTION:
				break;
			case DHCP_MESSAGE_TYPE:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead(&type, 1);
				break;
			case DHCP_SUBMASK:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead(dhcp_submask, 4);
				break;
			case DHCP_ROUTER:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead(dhcp_gw, 4);
				for (optlen -= 4; optlen > 0; --optlen)
					socket->UDPRead(&c, 1);
				break;
			case DHCP_DNS:
				socket->UDPRead(&optlen, 1);
				// to be continued...
				while(optlen > 0) {
					socket->UDPRead(&c, 1);
					optlen--;
				}
				break;
			case DHCP_SERVER_ID:
				socket->UDPRead(&optlen, 1);
				if (dhcp_server_ip[0] == 0) // || remote_ip != dhcp_server_ip)
					socket->UDPRead(dhcp_server_ip, 4);
				else
					while(optlen > 0) {
						socket->UDPRead(&c, 1);
						optlen--;
					}
				break;
			case DHCP_T1:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead((uint8_t*)&T1, sizeof(T1));
				T1 = htonl(T1);
				break;
			case DHCP_T2:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead((uint8_t*)&T2, sizeof(T1));
				T2 = htonl(T2);
				break;
			case DHCP_LEASETIME:
				socket->UDPRead(&optlen, 1);
				socket->UDPRead((uint8_t*)&lease_time, sizeof(lease_time));
				break;
			default:
				socket->UDPRead(&optlen, 1);
				while(optlen > 0) {
					socket->UDPRead(&c, 1);
					optlen--;
				}
				break;
		}
	}
	
	return type;
}

int main()
{
	unsigned char mac_addr[] = {0x00, 0x16, 0x36, 0xDE, 0x58, 0xF6};

	Serial::init();
	Serial::attachStdIO();

	Millis::init();

	sei();

	Wiz5100::init();
	Wiz5100::setMAC(mac_addr);

	UDPSock dhcpSock(DHCP_SOCK, DHCP_CLIENT_PORT);

	printf("Starting DHCP...\n");
	start_dhcp(mac_addr, &dhcpSock);

	while (true) {
	}

	return 0;
}

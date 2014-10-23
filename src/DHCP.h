#ifndef __DHCP_H__
#define __DHCP_H__

#include "Wiz5100.h"
#include "UDP.h"
#include "Millis.h"

#define DHCP_SERVER_PORT	67
#define DHCP_CLIENT_PORT	68

#define DHCP_SOCK			2

#define DHCP_TIMEOUT		60000

/* DHCP state machine. */
#define	STATE_DHCP_START	0 
#define	STATE_DHCP_DISCOVER	1
#define STATE_DHCP_REQUEST	2
#define STATE_DHCP_LEASED	3
#define STATE_DHCP_REREQUEST 4
#define	STATE_DHCP_RELEASE	5

#define DHCP_HOPS			0
#define DHCP_BOOTREQUEST	1
#define DHCP_BOOTREPLY		2
#define DHCP_HTYPE10MB		1
#define DHCP_HLENETHERNET	6
#define DHCP_FLAGSBROADCAST 0x8000 
#define DHCP_MAGIC_COOKIE	0x63825363

/* DHCP options */
#define DHCP_PADOPTION		0
#define DHCP_SUBMASK		1
#define DHCP_ROUTER			3
#define DHCP_DNS			6
#define DHCP_HOSTNAME 		12
#define DHCP_DOMAIN			15
#define DHCP_REQUESTED_IP	50
#define DHCP_LEASETIME		51
#define DHCP_MESSAGE_TYPE 	53
#define DHCP_SERVER_ID		54
#define DHCP_PARAM_REQUEST 	55
#define DHCP_T1				58
#define DHCP_T2				59
#define DHCP_CLIENT_ID 		61
#define DHCP_ENDOPT			255

typedef struct 
{
	uint8_t  op; 
	uint8_t  htype; 
	uint8_t  hlen;
	uint8_t  hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint8_t  ciaddr[4];
	uint8_t  yiaddr[4];
	uint8_t  siaddr[4];
	uint8_t  giaddr[4];
	uint8_t  chaddr[6];
} RIP_MSG_FIXED;

#define HOSTNAME "123456"

/* DHCP message type */
#define DHCP_DISCOVER	1
#define DHCP_OFFER		2
#define DHCP_REQUEST	3
#define DHCP_DECLINE	4
#define DHCP_ACK		5
#define DHCP_NAK		6
#define DHCP_RELEASE	7
#define DHCP_INFORM		8

#define DHCP_TIMEDOUT	255
#define	DHCP_DEFAULT_LEASE	900

uint32_t id;
uint8_t dhcp_source_mac[6];
uint8_t dhcp_local_ip[4];
uint8_t dhcp_submask[4];
uint8_t dhcp_gw[4];
uint8_t dhcp_server_ip[4];
uint32_t T1, T2, lease_time;

void start_dhcp(uint8_t *source_mac, UDPSock *socket);
int request_lease(UDPSock *socket);
void send_dhcp_msg(UDPSock *socket, uint8_t type, uint16_t time_passed);
uint8_t receive_dhcp_msg(UDPSock *socket, uint32_t timeout, uint32_t *resid);

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
				   ((x)<< 8 & 0x00FF0000UL) | \
				   ((x)>> 8 & 0x0000FF00UL) | \
				   ((x)>>24 & 0x000000FFUL) )

#endif

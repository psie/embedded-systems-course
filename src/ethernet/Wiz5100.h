#ifndef __ETHERNET_WIZ5100_H__
#define __ETHERNET_WIZ5100_H__

namespace Wiz5100 {
	static const uint8_t writeOpcode = 0xF0;
	static const uint8_t readOpcode = 0x0F;

	static const uint8_t MR_RST = 0b10000000;
	static const uint8_t BUFSIZE_MODE = 0b01010101;

	typedef uint16_t Address;

	/* Wiznet W5100 Register Addresses */
	const Address MR   = 0x0000; // Mode register
	const Address GAR  = 0x0001; // Gateway Address: 0x0001 - 0x0004
	const Address SUBR = 0x0005; // Subnet mask Address: 0x0005 - 0x0008
	const Address SAR  = 0x0009; // Source Hardware Address: 0x0009 - 0x000E
	const Address SIPR = 0x000F; // Source IP Address: 0x000F - 0x0012
	const Address RMSR = 0x001A; // RX Memory Size Register
	const Address TMSR = 0x001B; // TX Memory Size Register

	const Address S_BASE    = 0x0400; // Start of socket0 fields
	const Address S_NEXT    = 0x0100; // Next socket offset
	// Socket fileds offsets
	const Address S_MR      = 0x0000; // Mode Register Address
	const Address S_CR      = 0x0001; // Command Register Address
	const Address S_IR      = 0x0002; // Interrupt Register Address
	const Address S_SR      = 0x0003; // Status Register Address
	const Address S_SPORT   = 0x0004; // Source Port: 0x0404 - 0x0405
	const Address S_DADDR   = 0x000C; // Destination Address: 0x040C - 0x040F
	const Address S_DPORT   = 0x0010; // Destination Port: 0x0410 - 0x0411
	const Address S_PROTO   = 0x0014; // IPRAW protocol
	const Address S_TX_FSR  = 0x0020; // Tx Free Size Register: 0x0420 - 0x0421
	const Address S_TX_RD   = 0x0022; // Tx Read Pointer Register: 0x0422 - 0x0423
	const Address S_TX_WR   = 0x0024; // Tx Write Pointer Register: 0x0424 - 0x0425
	const Address S_RX_RSR  = 0x0026; // Rx Received Size Pointer Register: 0x0425 - 0x0427
	const Address S_RX_RD   = 0x0028; // Rx Read Pointer: 0x0428 - 0x0429

	typedef enum : uint8_t {
		SOCK_CLOSED      = 0x00, // Closed
		SOCK_INIT        = 0x13, // Init state
		SOCK_LISTEN      = 0x14, // Listen state
		SOCK_SYNSENT     = 0x15, // Connection state
		SOCK_SYNRECV     = 0x16, // Connection state
		SOCK_ESTABLISHED = 0x17, // Success - connect
		SOCK_FIN_WAIT    = 0x18, // Closing state
		SOCK_CLOSING     = 0x1A, // Closing state
		SOCK_TIME_WAIT   = 0x1B, // Closing state
		SOCK_CLOSE_WAIT  = 0x1C, // Closing state
		SOCK_LAST_ACK    = 0x1D, // Closing state
		SOCK_UDP         = 0x22, // UDP socket
		SOCK_IPRAW       = 0x32, // IP raw mode socket
		SOCK_MACRAW      = 0x42, // MAC raw mode socket
		SOCK_PPPOE       = 0x5F  // PPPOE socket
	} S_SRVal;

	typedef enum : uint8_t {
		CR_OPEN      = 0x01, // Initialize or open socket
		CR_LISTEN    = 0x02, // Wait connection request in tcp mode(Server mode)
		CR_CONNECT   = 0x04, // Send connection request in tcp mode(Client mode)
		CR_DISCON    = 0x08, // Send closing reqeuset in tcp mode
		CR_CLOSE     = 0x10, // Close socket
		CR_SEND      = 0x20, // Update Tx memory pointer and send data
		CR_SEND_MAC  = 0x21, // Send data with MAC address, so without ARP process
		CR_SEND_KEEP = 0x22, // Send keep alive message
		CR_RECV      = 0x40  // Update Rx memory buffer pointer and receive data
	} S_CRVal;

	typedef enum : uint8_t {
		 MR_CLOSE     = 0x00, // Unused socket
		 MR_TCP       = 0x01, // TCP
		 MR_UDP    	  = 0x02, // UDP
		 MR_IPRAW     = 0x03, // IP LAYER RAW SOCK
		 MR_MACRAW    = 0x04, // MAC LAYER RAW SOCK
		 MR_PPPOE     = 0x05, // PPPoE
		 MR_ND        = 0x20, // No Delayed Ack(TCP) flag
		 MR_MULTI     = 0x80  // support multicating
	} S_MRVal;

	//const uint8_t PROTO_ICMP  = 0x01;

	/* XXX below applies only to RMSR = TMSR = 0x55 (2K per socket) */
	const uint16_t BUF_SIZE = 2048; 
	const Address TX_BUF_MASK = 0x07FF;
	const Address TX_BUF_BASE = 0x4000;
	const Address RX_BUF_MASK = 0x07FF;
	const Address RX_BUF_BASE = 0x6000;

	template<typename T> void write(Address addr, T data);
	void write(Address addr, volatile uint16_t data);
	void write(Address addr, uint8_t *data, int n);
	uint8_t read8(Address addr);
	uint16_t read16(Address addr);
	void read(Address addr, uint8_t *data, int n);
	uint16_t doubleRead(Address addr);

	void init();
	void setMAC(uint8_t *macp);
	void setStatic(uint8_t *macp, uint8_t *ipp,
		uint8_t *subp, uint8_t *gtwp);
	
	void socket(uint8_t sock, uint8_t eth_protocol, uint16_t port);
	void open(uint8_t sock);
	void close(uint8_t sock);
	//void listen(uint8_t sock); 
	void send(uint8_t sock);
	void sockRead(uint8_t sock, uint8_t *buf, unsigned int n);
	void sockWrite(uint8_t sock, uint8_t *buf, uint16_t seek, unsigned int n);
	uint16_t sockRXReceived(uint8_t sock);
};

#define S_FIELD(sock, offset) Wiz5100::S_BASE + offset + Wiz5100::S_NEXT * sock

#endif

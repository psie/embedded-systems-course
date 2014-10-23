#include "UDP.h"

void UDPSock::UDPBegin(uint8_t *ipp, uint16_t dport) {
	txOffset = 0;
	header.length = 0;
	Wiz5100::write(S_FIELD(sock, Wiz5100::S_DADDR), ipp, 4);
	Wiz5100::write(S_FIELD(sock, Wiz5100::S_DPORT), dport);
}

void UDPSock::UDPFillbuf(uint8_t *buf, unsigned int n) {
	Wiz5100::sockWrite(sock, buf, txOffset, n);
	txOffset += n;
}

void UDPSock::UDPSend() {
	Wiz5100::send(sock);
}

unsigned int UDPSock::UDPStartRecv() {
	volatile uint16_t size;

	UDPFlush();

	size = Wiz5100::doubleRead(S_FIELD(sock, Wiz5100::S_RX_RSR));
	if (size == 0)
		return 0;

	Wiz5100::sockRead(sock, (uint8_t*)&header, 8);

	header.length = header.length >> 8 | header.length << 8;
	header.remotePort = header.remotePort >> 8 | header.remotePort << 8;

	return header.length;
}

unsigned int UDPSock::UDPRead(uint8_t *buf, unsigned int n) {
	unsigned int read;

	if (header.length == 0)
		return 0;
	
	if (n > header.length) {
		read = header.length;
		Wiz5100::sockRead(sock, buf, read);
		header.length = 0;
		return read;
	}
	
	Wiz5100::sockRead(sock, buf, n);
	header.length -= n;
	return n;
}

void UDPSock::UDPFlush() {
	uint8_t c;
	while (UDPRead(&c, 1) > 0);
}

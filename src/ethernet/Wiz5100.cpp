#include <util/delay.h>

#include "Pin.h"
#include "SPI.h"
#include "Wiz5100.h"

const static Pin::Pin<SS> slaveSelect;

template<typename T> void Wiz5100::write(Address addr, T data) {
	slaveSelect.low();
	SPI::transmit(writeOpcode);
	SPI::transmit((addr & 0xFF00) >> 8);
	SPI::transmit(addr & 0x00FF);
	SPI::transmit(data);
	slaveSelect.high();
}

void Wiz5100::write(Address addr, uint16_t data) {
	write(addr, (uint8_t)((data & 0xFF00) >> 8));
	write(addr + 1, (uint8_t)(data & 0x00FF));
}

void Wiz5100::write(Address addr, uint8_t *data, int n) {
	int i = 0;

	while (n > 0) {
		write(addr++, data[i++]);
		n--;
	}
}

uint8_t Wiz5100::read8(Address addr) {
	uint8_t r = 0;

	slaveSelect.low();
	SPI::transmit(readOpcode);
	SPI::transmit((addr & 0xFF00) >> 8);
	SPI::transmit(addr & 0x00FF);
	r = SPI::transmit(0);
	slaveSelect.high();
	return r;
}

uint16_t Wiz5100::read16(Address addr) {
	volatile uint16_t r;

	r = (read8(addr) & 0x00FF) << 8;
	r += read8(addr + 1);

	return r;
}

void Wiz5100::read(Address addr, uint8_t *data, int n) {
	volatile unsigned int i = 0;

	while (n > 0) {
		data[i++] = read8(addr++);
		n--;
	}
}

uint16_t Wiz5100::doubleRead(Address addr) {
	volatile uint16_t val1, val2 = 0;

	do {
		val1 = read16(addr);
		if (val1 != 0)
			val2 = read16(addr);
	} while (val1 != val2);

	return val1;
}

/* TODO - implement RX/TX memory size socket options */
void Wiz5100::init() {
	/* Wait a while until the chip "warms up" */
	_delay_ms(300); 

	SPI::init(SPI::MASTER, SPI::CLK_DIV2, SPI::MSBFIRST);
		
	write(MR, MR_RST);
	/* Set RX and TX memory size (2KB per sock each) */
	write(RMSR, BUFSIZE_MODE);
	write(TMSR, BUFSIZE_MODE);
}

void Wiz5100::setMAC(uint8_t *macp) {
	write(SAR, macp, 6);
}

void Wiz5100::setStatic(uint8_t *macp, uint8_t *ipp,
		uint8_t *subp, uint8_t *gtwp)
{
	write(GAR, gtwp, 4);
	write(SAR, macp, 6);
	write(SUBR, subp, 4);
	write(SIPR, ipp, 4);

}

/* TODO - implement IP sub-protocol option */
void Wiz5100::socket(uint8_t sock, uint8_t eth_protocol, uint16_t sport) {
	// Make sure we close the socket first
	if (read8(S_FIELD(sock, S_SR)) == SOCK_CLOSED)
		close(sock); 
	write(S_FIELD(sock, S_MR), eth_protocol);
	write(S_FIELD(sock, S_SPORT), sport);
	open(sock);
}

void Wiz5100::open(uint8_t sock) {
	write(S_FIELD(sock, S_CR), CR_OPEN);
	// Wait for opening 
	while(read8(S_FIELD(sock, S_CR)));
}

void Wiz5100::close(uint8_t sock) {
	write(S_FIELD(sock, S_CR), CR_CLOSE);
	while(read8(S_FIELD(sock, S_CR)));
}

void Wiz5100::send(uint8_t sock) {
	write(S_FIELD(sock, S_CR), CR_SEND);
	// Wait for sending to complete
	while (read8(S_FIELD(sock, S_CR))); 
}

/* TODO - peek option */
void Wiz5100::sockRead(uint8_t sock, uint8_t *buf, unsigned int n) {
	volatile uint16_t offset;
	volatile uint16_t mem_addr;

	offset = read16(S_FIELD(sock, S_RX_RD)) & RX_BUF_MASK;
	mem_addr = offset + RX_BUF_BASE + BUF_SIZE * sock;
	if (offset + n > BUF_SIZE) { // Wrap around
		read(mem_addr, buf, BUF_SIZE - offset);
		read(RX_BUF_BASE + BUF_SIZE * sock,
				buf + BUF_SIZE - offset, n - (BUF_SIZE - offset));
	}
	else
		read(mem_addr, buf, n);
	
	mem_addr += n;
	
	write(S_FIELD(sock, S_RX_RD), mem_addr);
	write(S_FIELD(sock, S_CR), CR_RECV);
	while (read8(S_FIELD(sock, S_CR)));
}

void Wiz5100::sockWrite(uint8_t sock, uint8_t *buf, uint16_t seek,
		unsigned int n) {
	volatile uint16_t freesize, tx_wr, offset, mem_addr;

	do 
		freesize = doubleRead(S_FIELD(sock, S_TX_FSR));
	while (freesize < n);

	tx_wr = read16(S_FIELD(sock, S_TX_WR));
	tx_wr += seek;
	offset = tx_wr & TX_BUF_MASK;
	mem_addr = TX_BUF_BASE + offset + BUF_SIZE * sock;

	if (offset + n > BUF_SIZE) { // Wrap around
		write(mem_addr, buf, BUF_SIZE - offset);
		write(TX_BUF_BASE + BUF_SIZE * sock,
				buf + (BUF_SIZE - offset), n - (BUF_SIZE - offset));
	}
	else
		write(mem_addr, buf, n);

	tx_wr += n;
	write(S_FIELD(sock, S_TX_WR), tx_wr);
}

uint16_t Wiz5100::sockRXReceived(uint8_t sock) {
	return doubleRead(S_FIELD(sock, S_RX_RSR));
}


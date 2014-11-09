#include <util/delay.h>

#include "Pin.h"
#include "SPI.h"
#include "Wiz5100.h"

const static Pin::Pin<SS> slaveSelect;

/* The template type is to achieve polymorphism for this function */
template<typename T> void Wiz5100::write(uint16_t addr, T data) {
	slaveSelect.low();
	SPI::transmit(0xF0); // 0xF0 is the write OP-code
	SPI::transmit((addr & 0xFF00) >> 8);
	SPI::transmit(addr & 0x00FF);
	SPI::transmit(data);
	slaveSelect.high();
}

void Wiz5100::write(uint16_t addr, uint16_t data) {
	write(addr, (uint8_t)((data & 0xFF00) >> 8));
	write(addr + 1, (uint8_t)(data & 0x00FF));
}

void Wiz5100::write(uint16_t addr, uint8_t *data, int n) {
	while (n > 0) {
		write(addr++, *data++);
		n--;
	}
}

uint8_t Wiz5100::read8(uint16_t addr) {
	uint8_t r = 0;

	slaveSelect.low();
	SPI::transmit(0x0F); // 0x0F is the read OP-code
	SPI::transmit((addr & 0xFF00) >> 8);
	SPI::transmit(addr & 0x00FF);
	r = SPI::transmit(0);
	slaveSelect.high();
	return r;
}

uint16_t Wiz5100::read16(uint16_t addr) {
	uint8_t lo = read8(addr++);
	uint8_t hi = read8(addr);

	return ((hi << 8) | lo);
}

void Wiz5100::read(uint16_t addr, uint8_t *data, int n) {
	while (n > 0) {
		*data++ = read8(addr++);
		n--;
	}
}

/* This is to ensure consistent reads */ 
uint16_t Wiz5100::doubleRead(uint16_t addr) {
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

	/* First we read the offset within the chips buffer where our received data
	 * starts (as in the manual, this value has to be truncated to represent an
	 * actual offset by & RX_BUF_MASK). Then we compute an actual address and
	 * either we have to wrap around the buffer boundary, that is read 
	 * everything untill the end of the buffer and the remaining part from the
	 * start, or our data fitted and we read it stright. Then, we store the
	 * full address back from where we obtained the offset. Finally we inform
	 * the chip the we have read the data and wait for the confirmation. 
	 * XXX - I should probably take size of received data (RSR) reported by the
	 * chip into consideration and not allow the user to read past that. */

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

	/* This is somewhat analogous to previous function except that we take
	 * available space to fill the send buffer with reported by the chip into
	 * consideration (as below). The second difference is that we keep a record
	 * in our socket structure of where we ended last time (seek), since I 
	 * encountered issues with the value being updated in the chips register(?).
	 * It is probably not updated until the command to send the data is 
	 * received by the chip and I wanted to allow for filling the buffer
	 * gradually before the actual sending */

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


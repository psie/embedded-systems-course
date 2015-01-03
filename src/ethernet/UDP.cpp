/* 
 * Written by Przemek Sierociński - przsie@gmail.com
 */

#include "UDP.h"

using namespace Wiz5100;

void UDPSock::Begin(uint8_t *ipp, uint16_t dport) {
    txOffset = 0;
    header.length = 0;
    write(S_FIELD(sock, Wiz5100::S_DADDR), ipp, 4);
    write16(S_FIELD(sock, Wiz5100::S_DPORT), dport);
}

void UDPSock::Fillbuf(uint8_t *buf, uint16_t n) {
    sockWrite(sock, buf, txOffset, n);
    txOffset += n;
}

void UDPSock::Send() {
    send(sock);
}

uint16_t UDPSock::StartRecv() {
    volatile uint16_t size;

    Flush();

    size = doubleRead(S_FIELD(sock, Wiz5100::S_RX_RSR));
    if (size == 0)
        return 0;

    sockRead(sock, (uint8_t*)&header, 8);

    header.length = htons(header.length);
    header.remotePort = htons(header.remotePort);

    return header.length;
}

uint16_t UDPSock::Read(uint8_t *buf, uint16_t n) {
    uint16_t read;

    if (header.length == 0)
        return 0;
    
    if (n > header.length) {
        read = header.length;
        sockRead(sock, buf, read);
        header.length = 0;
        return read;
    }
    
    sockRead(sock, buf, n);
    header.length -= n;
    return n;
}

void UDPSock::Flush() {
    uint8_t c;
    while (Read(&c, 1) > 0);
}

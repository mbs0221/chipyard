#include <stdint.h>

#define CRC16_BITS 12
#define CRC16_LEN 4096 // 4KB
#define NAK 0x15
#define ACK 0x06
#define TIMEOUT 30

uint16_t crc16_round(uint16_t crc, uint8_t data);
uint16_t crc16(uint8_t *q);
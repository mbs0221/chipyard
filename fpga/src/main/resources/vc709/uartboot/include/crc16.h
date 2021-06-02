#include <stdint.h>

#define CRC16_BITS 16
#define CRC16_LEN (1<<CRC16_BITS) // 64KiB
#define NUM_BLOCKS 64 // 4MiB
#define NAK 0x15
#define ACK 0x06

uint16_t crc16_round(uint16_t crc, uint8_t data);
uint16_t crc16(uint8_t *q);
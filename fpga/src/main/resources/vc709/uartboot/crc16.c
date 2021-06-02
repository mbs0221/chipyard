#include "include/crc16.h"
 
inline uint16_t crc16_round(uint16_t crc, uint8_t data) {
	crc = (uint8_t)(crc >> 8) | (crc << 8);
	crc ^= data;
	crc ^= (uint8_t)(crc >> 4) & 0xf;
	crc ^= crc << 12;
	crc ^= (crc & 0xff) << 5;
	return crc;
}

uint16_t crc16(uint8_t *q) {
    uint16_t crc = 0;
    for (int i = 0; i < CRC16_LEN; i++) {
        crc = crc16_round(crc, *q++);
    }
	return crc;
}
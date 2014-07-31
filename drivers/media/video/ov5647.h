#ifndef __OV5647_H__
#define __OV5647_H__
struct reginfo
{
    u16 reg;
    u8 val;
};

#define SEQUENCE_INIT        0x00
#define SEQUENCE_NORMAL      0x01
#define SEQUENCE_CAPTURE     0x02
#define SEQUENCE_PREVIEW     0x03

#define SEQUENCE_PROPERTY    0xFFFC
#define SEQUENCE_WAIT_MS     0xFFFD
#define SEQUENCE_WAIT_US     0xFFFE
#define SEQUENCE_END	     0xFFFF
#endif


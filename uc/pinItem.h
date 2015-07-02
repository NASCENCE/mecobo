/* This class uses a lot of bytes.
 *
 * It's to envision shrinking this down to 16 bytes
 * by simply using only pin, start, end, val.  OH WELL!
 * */
#ifndef __PINITEM_H__
#define __PINITEM_H__

#include "../mecoprot.h"

struct pinItem 
{
    FPGA_IO_Pins_TypeDef pin;
    uint32_t startTime;
    uint32_t endTime;
    int type;
    int constantValue;
    int duty;
    int antiDuty;
    int sampleRate;
    int nocCounter;
};

void execute(struct pinItem * item);
void killItem(struct pinItem * item);
#endif

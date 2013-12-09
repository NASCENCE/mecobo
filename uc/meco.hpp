/* This class uses a lot of bytes.
 *
 * It's to envision shrinking this down to 16 bytes
 * by simply using only pin, start, end, val.  OH WELL!
 * */
#include "../mecoprot.h"

class pinItem 
{
  public:
    FPGA_IO_Pins_TypeDef pin;
    int startTime;
    int endTime;
    int type;
    int constantValue;
    int duty;
    int antiDuty;

    void execute();

};


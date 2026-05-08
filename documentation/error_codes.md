## Project AppForFaradayCups
### Error codes in Modbus registers: ErrorCode, Cup1Error, Cup2Error, and Cup3Error


```

In an error code, each bit represents a different type of error.

ErrorCode format:
Bit 0:  Critical error in cup mechanism #1
Bit 1:  Critical error in cup mechanism #2
Bit 2:  Critical error in cup mechanism #3
Bit 3:  reserved
Bit 4:  reserved
Bit 5:  reserved
Bit 6:  reserved
Bit 7:  reserved
Bit 8:  Non-critical error in cup mechanism #1
Bit 9:  Non-critical error in cup mechanism #2
Bit 10: Non-critical error in cup mechanism #3
Bit 11: reserved
Bit 12: reserved
Bit 13: reserved
Bit 14: reserved
Bit 15: reserved

Cup1Error, Cup2Error and Cup2Error format:
Bit 0:  The limit switch signal for switch #1 is abnormal¹;  critical error
Bit 1:  The limit switch signal for switch #2 is abnormal¹²; critical error
Bit 2:  reserved
Bit 3:  reserved
Bit 4:  reserved
Bit 5:  reserved
Bit 6:  reserved
Bit 7:  reserved
Bit 8:  Cup insertion time limit exceeded; non-critical
Bit 9:  Cup ejection time limit exceeded; non-critical
Bit 10: reserved
Bit 11: reserved
Bit 12: reserved
Bit 13: reserved
Bit 14: reserved
Bit 15: reserved

¹ in steady state
² if the switch #2 is installed

```


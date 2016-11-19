#ifndef FlagDefine_h
#define FlagDefine_h

// Input to state machines
#include <bitset>
typedef enum {
  SWITCH_UPPER_FLAG,
  SWITCH_LOWER_FLAG,
  SWITCH_BUMPER_FLAG,
  SENSOR_INRANGE_FLAG,
  COMMAND_OPEN_FLAG,
  COMMAND_CLOSE_FLAG,
  INIT_AUTOCLOSE_FLAG,
  EVENT_OPENED_FLAG,
  EVENT_CLOSED_FLAG,
  FAULT_OPENING_FLAG,
  FAULT_CLOSING_FLAG,
  FAULT_MISSEDCLOSE_FLAG,
  FAULT_DOGSQUISHED_FLAG,
  SIZE_OF_FLAGS_ENUM
} Flag_t;
#endif

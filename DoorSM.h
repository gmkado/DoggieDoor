#ifndef doorSM_h
#define doorSM_h
#include "application.h"//needed for all Particle libraries
#include "FlagDefines.h"
#include "SupervisorSM.h" // need supervisor state
#include <memory>

/****************************************************************
  					States
****************************************************************/
typedef enum {DOOR_CLOSED,
DOOR_OPENING,
DOOR_OPEN,
DOOR_CLOSING,
NO_RECOVER, // fatal state, wait for reset
SIZE_OF_DOORSTATE_ENUM} DoorState_t;

const std::string DoorStateNames[] = { "DOOR_CLOSED", "DOOR_OPENING", "DOOR_OPEN","DOOR_CLOSING", "NO_RECOVER"};

// statically check that the size of DoorStateNames fits the number of DoorStates
static_assert(sizeof(DoorStateNames)/sizeof(char*) == SIZE_OF_DOORSTATE_ENUM, "sizes dont match");
/***************************************************************
 *                  Definitions
 * **************************************************************/
const uint16_t NO_SWITCH_TIMEOUT = 20000; // 20 seconds, this should be longer than the time it would take to unspool + respool the string completely
const uint16_t UPPER_RELEASE_TIMEOUT = 2000;  // 1 second to release the switch, should be plenty
const uint8_t OPENING_SPEED = 70; // 0 to 255
const uint8_t CLOSING_SPEED = 70; // 0 to 255
const uint8_t MAX_OSCILLATIONS = 3;

const int MOTOR_DIR_ADDR = 10; // addresses in EEPROM specifying which direction the motor should run to open/close
/****************************************************************
  					Function Prototypes
****************************************************************/
class DoorSM {
  public:
    DoorSM();
    void init(SupervisorState_t, std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    void runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    DoorState_t getState();

  private:
    DoorState_t currentState;
    bool isEntry;
    bool isExit;
    bool timeoutOccurred;
    uint8_t motorOpenPin;
    uint8_t motorClosePin;
    uint8_t oscCount; // oscillation count, tracks # of times the door switchs directions w/o an event
    bool motorDir; // true => motorOpenPin == MOTOR_DIR1_PIN, false => motorOpenPin == MOTOR_DIR2_PIN
    bool isReleased;

    void transitionTo(DoorState_t to);
    std::unique_ptr<Timer> doorTimer; // use a smart pointer to deal with resource allocation.  This initializes to null, but we reset in DoorSM constructor
    void setMotorDir(bool);
    void onTimeout();
};
#endif

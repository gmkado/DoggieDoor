#ifndef doorSM_h
#define doorSM_h
#include "application.h"//needed for all Particle libraries
#include "FlagDefines.h"
#include <memory>

/****************************************************************
  					States
****************************************************************/
typedef enum {DOOR_CLOSED,
DOOR_OPENING,
DOOR_OPEN,
DOOR_CLOSING,
SIZE_OF_DOORSTATE_ENUM} DoorState_t ;

const std::string DoorStateNames[] = { "DOOR_CLOSED", "DOOR_OPENING", "DOOR_OPEN","DOOR_CLOSING" };

// statically check that the size of DoorStateNames fits the number of DoorStates
static_assert(sizeof(DoorStateNames)/sizeof(char*) == SIZE_OF_DOORSTATE_ENUM, "sizes dont match");
/***************************************************************
 *                  Definitions
 * **************************************************************/
const uint16_t CLOSING_TIME = 5000;
const uint16_t OPENING_TIME = 5000;
const uint8_t OPENING_SPEED = 70; // 0 to 255
const uint8_t CLOSING_SPEED = 70; // 0 to 255
/****************************************************************
  					Function Prototypes
****************************************************************/
class DoorSM {
  public:
    DoorSM();
    void init(std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    void runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    DoorState_t getState();

  private:
    DoorState_t currentState;
    bool isEntry;
    bool isExit;
    bool upperRelease;
    bool timeoutOccurred;
    void transitionTo(DoorState_t to);
    std::unique_ptr<Timer> doorTimer; // use a smart pointer.  This initializes to null, but we reset in DoorSM constructor

    void onTimeout();
};
#endif

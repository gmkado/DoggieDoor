#ifndef autocloseSM_h
#define autocloseSM_h
#include "application.h"//needed for all Particle libraries
#include "DoorSM.h" // this includes <memory>
/****************************************************************
  					States
****************************************************************/
typedef enum {WAITING_FOR_DOG,
WAITING_FOR_DOOR_CLOSED,
WAITING_FOR_DOOR_OPEN,
HOLDING_DOOR_OPEN,
SIZE_OF_AUTOCLOSESTATE_ENUM} AutoCloseState_t ;

const std::string AutoCloseStateNames[] = { "WAITING_FOR_DOG",
"WAITING_FOR_DOOR_CLOSED",
"WAITING_FOR_DOOR_OPEN",
"HOLDING_DOOR_OPEN"};

// statically check that the size of AutoCloseStateNames fits the number of DoorStates
static_assert(sizeof(AutoCloseStateNames)/sizeof(char*) == SIZE_OF_AUTOCLOSESTATE_ENUM, "sizes dont match");
/***************************************************************
 *                  Definitions
 * **************************************************************/
const uint16_t HOLDING_TIME = 5000;

/****************************************************************
  					Function Prototypes
****************************************************************/
class AutoCloseSM {
  public:
    AutoCloseSM();
    void init(DoorState_t, std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    void runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags);

    AutoCloseState_t getState();

  private:
    AutoCloseState_t currentState;
    bool isEntry;
    bool isExit;
    bool timeoutOccurred;
    void transitionTo(AutoCloseState_t);
    std::unique_ptr<Timer> autoCloseTimer; // use a smart pointer.  This initializes to null, but we reset in DoorSM constructor

    void onTimeout();

};
#endif

#ifndef supervisorSM_h
#define supervisorSM_h
#include "application.h"//needed for all Particle libraries
#include "FlagDefines.h"
/****************************************************************
  					States
****************************************************************/
typedef enum {MANUAL_MODE,
AUTO_OPEN,
AUTO_CLOSED,
SIZE_OF_SUPERVISORSTATE_ENUM} SupervisorState_t ;

const std::string SupervisorStateNames[] = { "MANUAL", "AUTO_OPEN", "AUTO_CLOSED"};

// statically check that the size of DoorStateNames fits the number of DoorStates
static_assert(sizeof(SupervisorStateNames)/sizeof(char*) == SIZE_OF_SUPERVISORSTATE_ENUM, "sizes dont match");
/***************************************************************
 *                  Definitions
 * **************************************************************/


/****************************************************************
  					Function Prototypes
****************************************************************/
class SupervisorSM {
  public:
    SupervisorSM(int startHour, int endHour);
    void init();
    void runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags);
    int getStartHour();
    int getEndHour();
    bool getIsAuto();

    void setStartHour(int startHour);
    void setEndHour(int endHour);
    void setIsAuto(bool isauto);

    SupervisorState_t getState();

  private:
    SupervisorState_t currentState;
    bool isEntry;
    bool isExit;
    bool isAuto;
    int startHour;
    int endHour;
    void transitionTo(SupervisorState_t);
    bool isDayTime();

    void (*initAutoCloseSM)(void); // function pointer to call to initialize autoclose state machine
    void (*openDoor)(void);
    void (*closeDoor)(void);
};
#endif

#include "DoorSM.h"


// Constructor
DoorSM::DoorSM() {
    doorTimer.reset(new Timer(1000, &DoorSM::onTimeout, *this, true));
}

void DoorSM::init(std::bitset<SIZE_OF_FLAGS_ENUM> *flags){
    // initialize state
    if(flags->test(SWITCH_UPPER_FLAG)) {
        transitionTo(DOOR_OPEN);
    }else if (flags->test(SWITCH_LOWER_FLAG)) {
        transitionTo(DOOR_CLOSED);
    }else{ // default to door opening
        transitionTo(DOOR_OPENING);
    }

    // consume exit event so we don't do this the first time we enter runSM()
    isExit = false;


}


/*********************** STATE MACHINE **************************************/
void DoorSM::runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags) {
      switch(currentState) {
        case DOOR_CLOSED:
            if(isEntry) {
                flags->set(EVENT_CLOSED_FLAG); // report door closed event
                isEntry = false;
            }

            if(flags->test(COMMAND_OPEN_FLAG)) { // open door command
                transitionTo(DOOR_OPENING);
            }

            break;
        case DOOR_OPENING:
            if(isEntry) {
                // TODO: start motor in the open direction
                doorTimer->changePeriod(OPENING_TIME);
                doorTimer->start();
                isEntry = false;
            }

            if(flags->test(SWITCH_UPPER_FLAG)) {
                transitionTo(DOOR_OPEN);
            } else if(flags->test(COMMAND_CLOSE_FLAG)){ // close door command
                transitionTo(DOOR_CLOSING);
            } else if(timeoutOccurred)  {
                flags->set(FAULT_OPENING_FLAG); // report the timeout
                timeoutOccurred = false; // clear flag
            }

            if(isExit) {
                // TODO:stop motor
                doorTimer->stop();
            }
            break;
        case DOOR_OPEN:
            if(isEntry) {
                flags->set(EVENT_OPENED_FLAG);
                isEntry = false;
            }
            if(flags->test(COMMAND_CLOSE_FLAG)){ // close door command
                transitionTo(DOOR_CLOSING);
            }
            break;
        case DOOR_CLOSING:
            if(isEntry) {
                // TODO: start motor in close direction
                doorTimer->changePeriod(CLOSING_TIME);
                doorTimer->start();
                isEntry = false;
            }
            if(flags->test(SWITCH_LOWER_FLAG)) {
                transitionTo(DOOR_CLOSED);
            }else if(flags->test(COMMAND_OPEN_FLAG)) { // open door command
                transitionTo(DOOR_OPENING);
            }else if(flags->test(SWITCH_BUMPER_FLAG)) {
                flags->set(FAULT_DOGSQUISHED_FLAG);
                transitionTo(DOOR_OPENING); // open the door to prevent further squishing
            }else if(timeoutOccurred) {
                timeoutOccurred = false; // clear flag
                flags->set(FAULT_CLOSING_FLAG); // report timeout
            }
            if(isExit) {
                // TODO: stop motor
                doorTimer->stop();
            }
            break;
    }

    // clear command flags, they should have been dealt with appropriately by this point
    flags->reset(COMMAND_CLOSE_FLAG);
    flags->reset(COMMAND_OPEN_FLAG);

    // ensure that isExit is false, if there were exit actions they should have been resolved by here
    isExit = false;
}


DoorState_t DoorSM::getState() {
    return currentState;
}

void DoorSM::transitionTo(DoorState_t to) {
    Serial.printf("DoorSM: Transitioning from %s to %s\n", DoorStateNames[currentState].c_str(), DoorStateNames[to].c_str());
    currentState = to;
    isEntry = true;
    isExit = true;
}

/*************** TIMER AND INTERRUPT CALLBACKS*********************/
void DoorSM::onTimeout() {
  timeoutOccurred = true;
}

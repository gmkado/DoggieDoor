#include "DoorSM.h"
#include "PinDefines.h"
#include <string>

// Constructor
DoorSM::DoorSM() {
    doorTimer.reset(new Timer(1000, &DoorSM::onTimeout, *this, true)); // Timer(period, callback, oneshot)
}

void DoorSM::init(SupervisorState_t supState,std::bitset<SIZE_OF_FLAGS_ENUM> *flags){
  // read the last motor pins from EEPROM
   EEPROM.get(MOTOR_DIR_ADDR, motorDir);
  if(motorDir == 0xFF) {
    Serial.println("DoorSM: motorDir not in EEPROM, initializing.");
    // EEPROM was empty -> initialize value
    setMotorDir(true);
  } else{
    Serial.println("DoorSM: setting motor direction from EEPROM");
    setMotorDir(motorDir);
  }
    // initialize state
    if(flags->test(SWITCH_UPPER_FLAG)) {
        transitionTo(DOOR_OPEN);
    }else if (flags->test(SWITCH_LOWER_FLAG)) {
        transitionTo(DOOR_CLOSED);
    }else{
      switch(supState){
        case AUTO_OPEN:
          transitionTo(DOOR_OPENING);
          break;
        case AUTO_CLOSED:
          transitionTo(DOOR_CLOSING);
          break;
        case MANUAL_MODE: // Should never get here, always startup in auto mode
          transitionTo(DOOR_CLOSING);
          break;
      }
      oscCount = 0;
    }

    // consume exit event so we don't do this the first time we enter runSM()
    isExit = false;


}


/*********************** STATE MACHINE **************************************/
void DoorSM::runSM(std::bitset<SIZE_OF_FLAGS_ENUM> *flags) {
      switch(currentState) {
        case DOOR_CLOSED:
            if(isEntry) {
              oscCount = 0; // reset oscillation count
              // stop motor in free running mode
              analogWrite(MOTOR_PWM_PIN, 0);

                flags->set(EVENT_CLOSED_FLAG); // report door closed event
                isEntry = false;
            }

            if(flags->test(COMMAND_OPEN_FLAG)) { // open door command
                transitionTo(DOOR_OPENING);
            }

            break;
        case DOOR_OPENING:
            if(isEntry) {
              // start motor in opening direction
              digitalWrite(motorOpenPin, HIGH);
              digitalWrite(motorClosePin, LOW);
              analogWrite(MOTOR_PWM_PIN, OPENING_SPEED);

                doorTimer->changePeriod(NO_SWITCH_TIMEOUT);
                doorTimer->start();
                isEntry = false;
            }
            if(flags->test(SWITCH_UPPER_FLAG)) {                                // door opened
                transitionTo(DOOR_OPEN);
            } else if(flags->test(COMMAND_CLOSE_FLAG)){                         // close door command
                transitionTo(DOOR_CLOSING);
            } else if(timeoutOccurred)  {                                       // NO_SWITCH_TIMEOUT occurred, report the error and switch to closing
              timeoutOccurred = false; // clear flag
                flags->set(FAULT_OPENING_FLAG); // report the timeout

                oscCount++; //  increment oscillation count
                if(oscCount > MAX_OSCILLATIONS) {
                  transitionTo(NO_RECOVER);
                }else{
                  transitionTo(DOOR_CLOSING);
                }
            }

            if(isExit) {
                doorTimer->stop();
            }
            break;
        case DOOR_OPEN:
            if(isEntry) {
              oscCount = 0; // reset oscillation count
              // start motor in brake mode
              digitalWrite(motorOpenPin, LOW);
              digitalWrite(motorClosePin, LOW);
              analogWrite(MOTOR_PWM_PIN, OPENING_SPEED);

                flags->set(EVENT_OPENED_FLAG);
                isEntry = false;
            }
            if(flags->test(COMMAND_CLOSE_FLAG)){ // close door command
                transitionTo(DOOR_CLOSING);
            }
            break;
        case DOOR_CLOSING:
            if(isEntry) {
                // start motor in close direction
                digitalWrite(motorOpenPin, LOW);
                digitalWrite(motorClosePin, HIGH);
                analogWrite(MOTOR_PWM_PIN, CLOSING_SPEED);

                doorTimer->changePeriod(UPPER_RELEASE_TIMEOUT); // timer for releasing the upper pin, will catch if we're going the wrong way
                doorTimer->start();

                isReleased = false;
                isEntry = false;
            }
            if(flags->test(SWITCH_LOWER_FLAG)) {
                transitionTo(DOOR_CLOSED);
            }

            else if(!isReleased && !flags->test(SWITCH_UPPER_FLAG)) {
              Serial.println("DoorSM:upper switch released");
              isReleased = true;
              doorTimer->changePeriod(NO_SWITCH_TIMEOUT); // now change to timing the close
              doorTimer->start();
            }
            else if(timeoutOccurred) {
              timeoutOccurred = false; // clear flag
              if(isReleased) {                                                  // NO_SWITCH_TIMEOUT occurred, report the error and switch to opening
                flags->set(FAULT_CLOSING_FLAG); // report timeout

                oscCount++;
                if(oscCount>MAX_OSCILLATIONS) {
                  transitionTo(NO_RECOVER);
                }else{
                  transitionTo(DOOR_OPENING); // go the other way
                }
              }else{                                                            // UPPER_RELEASE_TIMEOUT occured, we're pulling against the upper limit switch
                flags->set(FAULT_NO_RELEASE_ON_CLOSE_FLAG); // report the error
                setMotorDir(!motorDir); // switch dir pins

                oscCount++; // increment oscillation count TODO: should we count this as an oscillation? when would this happen where reversing wouldn't fix it?
                if(oscCount>MAX_OSCILLATIONS) {
                  transitionTo(NO_RECOVER);
                }else{
                  transitionTo(DOOR_CLOSING); // transition back to this state, which will start motor in opp direction
                }
              }


            }
            else if (isReleased && flags->test(SWITCH_UPPER_FLAG)){             // We wrapped around to the upper pin
                flags->set(FAULT_MISSEDCLOSE_FLAG); // report the error
                setMotorDir(!motorDir); // switch dir pins

                oscCount++;
                if(oscCount>MAX_OSCILLATIONS) {
                  transitionTo(NO_RECOVER);
                }else{
                  transitionTo(DOOR_CLOSING); // transition back to this state, which will start motor in opp sirection
                }
            }else if(flags->test(COMMAND_OPEN_FLAG)) {                          // open door command received
                transitionTo(DOOR_OPENING);
            }else if(flags->test(SWITCH_BUMPER_FLAG)) {                         // dog squished, open the door to prevent further squishing
                flags->set(FAULT_DOGSQUISHED_FLAG);
                transitionTo(DOOR_OPENING);
            }
            if(isExit) {
                doorTimer->stop();
            }
            break;
          case NO_RECOVER:
            if(isEntry) {
              analogWrite(MOTOR_PWM_PIN, 0); // stop the motor in freerunning mode
              flags->set(FAULT_NONRECOVER_FLAG); // report the error
            }
            // No way out
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
  char message[100];
  sprintf(message, "DoorSM: Transitioning from %s to %s", DoorStateNames[currentState].c_str(), DoorStateNames[to].c_str());
  Serial.println(message);
  Particle.publish("Log", message);

  currentState = to;
    isEntry = true;
    isExit = true;

    char publishString[10];
    sprintf(publishString, "%d", to);

    // publish the event
    Particle.publish("DoorState", publishString);
}

void DoorSM::setMotorDir(bool dir) {
    if(dir) {
      motorOpenPin = MOTOR_DIR1_PIN;
      motorClosePin = MOTOR_DIR2_PIN;
    }else{
      motorOpenPin = MOTOR_DIR2_PIN;
      motorClosePin = MOTOR_DIR1_PIN;
    }

    if(dir != motorDir) {
      // we changed the value, so write it to EEPROM
      motorDir = dir;
      EEPROM.put(MOTOR_DIR_ADDR, motorDir);
    }
}
/*************** TIMER AND INTERRUPT CALLBACKS*********************/
void DoorSM::onTimeout() {
  timeoutOccurred = true;
}

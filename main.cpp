
#undef __ARM_FP
#include "mbed.h"
#include "keypad.h"
#include "SLCD.h"
#include "input_module.h"
#include "user_management.h"

//  Configuration  
#define DOOR_OPEN_MS        5000
#define MAX_FAILED_ATTEMPTS 3
#define LOCKOUT_MS          30000

//  Hardware  
Keypad     keypad(PTC8, PTA5, PTA4, PTA12, PTD3, PTA2, PTA1);
SLCD       slcd;
DigitalOut led(LED1);    // green
DigitalOut led2(LED2);   // red
DigitalOut relay(PTE0, 0); // relay — 0 = locked

//  Modules   
InputModule    inputModule(keypad, slcd);
UserManagement userManager(keypad, slcd, led, led2);

//  Forward declaration (Person 2's admin menu function)     
void runAdminMenu(Keypad& keypad, SLCD& slcd, DigitalOut& led, DigitalOut& led2,
                  InputModule& input, UserManagement& userManager);

//  Open door helper              
void openDoor()
{
    relay = 1;
    slcd.clear(); slcd.Home();
    slcd.printf("OPEN");
    led = 0;   // green LED on
    ThisThread::sleep_for(std::chrono::milliseconds(DOOR_OPEN_MS));
    relay = 0;
    led   = 1;
    slcd.clear(); slcd.Home();
    slcd.printf("PIN ");
}

//  Main   
int main()
{
    int  failCount = 0;
    bool lockedOut = false;
    Timer lockTimer;

    slcd.clear(); slcd.Home();
    slcd.printf("PIN ");

    while (true) {

        //  Lockout check             
        if (lockedOut) {
            if (lockTimer.elapsed_time() >=
                    std::chrono::milliseconds(LOCKOUT_MS)) {
                lockTimer.stop();
                lockTimer.reset();
                lockedOut = false;
                failCount = 0;
                slcd.clear(); slcd.Home();
                slcd.printf("PIN ");
                inputModule.reset();
            } else {
                slcd.clear(); slcd.Home();
                slcd.printf("WAIT");
                ThisThread::sleep_for(1s);
                continue;
            }
        }

        //  Collect PIN 
        while (!inputModule.hasPassword()) {
            inputModule.processInput();
        }

        //  Authenticate           
        const char* password = inputModule.getEnteredPassword();
        char auth = userManager.authenticate(password);

        if (auth == 'u') {
            // Normal user — open door
            failCount = 0;
            openDoor();
            inputModule.reset();

        } else if (auth == 'a') {
            // Admin — open door then enter admin menu
            failCount = 0;
            openDoor();
            runAdminMenu(keypad, slcd, led, led2, inputModule, userManager);
            inputModule.reset();
            slcd.clear(); slcd.Home();
            slcd.printf("PIN ");

        } else {
            // Wrong PIN
            failCount++;
            slcd.clear(); slcd.Home();
            slcd.printf("FAIL");
            led2 = 0;   // red LED on
            ThisThread::sleep_for(2s);
            led2 = 1;

            if (failCount >= MAX_FAILED_ATTEMPTS) {
                slcd.clear();
                slcd.Home();
                slcd.printf("LOC");
                lockTimer.start();
                lockedOut = true;
            } else {
                inputModule.reset();
            }
        }

        ThisThread::sleep_for(20ms);
    }
}
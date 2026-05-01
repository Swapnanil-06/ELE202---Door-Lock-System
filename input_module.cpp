#undef __ARM_FP
#include "mbed.h"
#include "input_module.h"
#include <string.h>

InputModule::InputModule(Keypad& keypad, SLCD& slcd)
    : keypad(keypad), slcd(slcd), position(0), PASSWORD_ENTERED(false)
{
    memset(showed_password, '\0', sizeof(showed_password));
    memset(entered_password, '\0', sizeof(entered_password));
    slcd.Home();
    slcd.printf("PIN ");
}

void InputModule::processInput() {
    char key = keypad.ReadKey();
    if (key == NO_KEY) {
        ThisThread::sleep_for(10ms); // match keypad scan rate
        return;
    }

    if (key == '#') {
        if (position >= 4) {
            entered_password[position] = '\0';
            PASSWORD_ENTERED = true;
        } else {
            // too short — flash error
            slcd.clear(); slcd.Home();
            slcd.printf("Err ");
            ThisThread::sleep_for(1000ms);
            slcd.clear(); slcd.Home();
            // redraw current dashes
            char display[5] = "    ";
            for (int i = 0; i < position && i < 4; i++) display[i] = '-';
            slcd.printf("%s", display);
        }
        return;
    }

    if (key == '*') {
        reset();
        return;
    }

    // digit key
    if (position < 8) {
        entered_password[position] = key;
        position++;

        // show up to 4 dashes — one per digit entered
        char display[5] = "    ";
        if (position <= 4) {
            // show all digits entered so far
            strncpy(display, entered_password, position);
        } else {
            // show last 4 digits
            strncpy(display, entered_password + (position - 4), 4);
        }
        display[4] = '\0';
        slcd.Home();
        slcd.printf("%s", display);
    }
}

void InputModule::reset() {
    memset(showed_password, '\0', sizeof(showed_password));
    memset(entered_password, '\0', sizeof(entered_password));
    position = 0;
    PASSWORD_ENTERED = false;
    slcd.clear();
    slcd.Home();
    slcd.printf("PIN ");
}
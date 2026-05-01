/*
 * admin_module.cpp
 * EEE228 ICE Design Exercise
 * Fixed: removed Ticker (not interrupt-safe with LCD)
 * displayMenu() now called directly in main loop
 */

#undef __ARM_FP
#include "mbed.h"
#include "keypad.h"
#include "SLCD.h"
#include "input_module.h"
#include "user_management.h"
#include <cstring>
#include <cstdio>

static const char* menuItems[] = {"USR ", "ADMN", "ADD ", "DEL "};
static const int   MENU_COUNT  = 4;
static int  currentMenuItem = 0;
static int  scrollPosition  = 0;

static SLCD*           g_slcd    = nullptr;
static Keypad*         g_keypad  = nullptr;
static DigitalOut*     g_led     = nullptr;
static DigitalOut*     g_led2    = nullptr;
static InputModule*    g_input   = nullptr;
static UserManagement* g_manager = nullptr;

// Helpers
static void showMsg(const char* msg) {
    char buf[5] = "    ";
    strncpy(buf, msg, 4);
    buf[4] = '\0';
    g_slcd->clear();
    g_slcd->Home();
    g_slcd->printf("%s", buf);
}

static void blinkLed(char type, int times) {
    for (int i = 0; i < times; i++) {
        if (type == 'g') {
            *g_led  = 0; ThisThread::sleep_for(200ms);
            *g_led  = 1; ThisThread::sleep_for(200ms);
        } else {
            *g_led2 = 0; ThisThread::sleep_for(200ms);
            *g_led2 = 1; ThisThread::sleep_for(200ms);
        }
    }
}

// Called directly in the main loop — safe for LCD
static void displayMenu() {
    const char* text = menuItems[currentMenuItem];
    int textLength = strlen(text);

    g_slcd->clear();
    g_slcd->Home();

    if (textLength <= 4) {
        g_slcd->printf("%s", text);
        return;
    }

    char buffer[5];
    for (int i = 0; i < 4; i++) {
        buffer[i] = (scrollPosition + i < textLength)
                    ? text[scrollPosition + i] : ' ';
    }
    buffer[4] = '\0';
    g_slcd->printf("%s", buffer);

    scrollPosition++;
    if (scrollPosition > textLength) scrollPosition = 0;
}

static void nextMenuItem() {
    currentMenuItem = (currentMenuItem + 1) % MENU_COUNT;
    scrollPosition  = 0;
}

static bool collectPin() {
    g_input->reset();
    showMsg("Pass");
    ThisThread::sleep_for(500ms);
    while (!g_input->hasPassword()) {
        g_input->processInput();
    }
    return (g_input->getEnteredPassword()[0] != '\0');
}

static char collectId() {
    char id   = '\0';
    bool done = false;

    g_slcd->clear(); g_slcd->Home();
    g_slcd->printf("ID  ");
    g_slcd->Colon(1);
    ThisThread::sleep_for(300ms);

    while (!done) {
        char key = g_keypad->ReadKey();
        if (key == NO_KEY) { ThisThread::sleep_for(10ms); continue; }

        if (key == '*') {
            id = '\0';
            done = true;
        } else if (key == '#') {
            if (id != '\0') done = true;
        } else if (key >= '0' && key <= '9') {
            id = key;
            g_slcd->clear(); g_slcd->Home();
            g_slcd->printf("ID %c", id);
            g_slcd->Colon(1);
        }
        ThisThread::sleep_for(10ms);
    }

    g_slcd->Colon(0);
    return id;
}

//  Menu actions  
static void executeMenuItem(int menuIndex) {
    switch (menuIndex) {

        case 0: { // USR — change user password
            showMsg("USR ");
            ThisThread::sleep_for(800ms);

            char id = collectId();
            if (id == '\0') { showMsg("Canc"); ThisThread::sleep_for(1000ms); break; }

            char uname[4];
            snprintf(uname, sizeof(uname), "u%c", id);

            if (!collectPin()) { showMsg("Canc"); ThisThread::sleep_for(1000ms); break; }

            char newpass[9];
            strncpy(newpass, g_input->getEnteredPassword(), 8);
            newpass[8] = '\0';

            if (g_manager->change_pswrd(uname, newpass)) {
                showMsg("SAVE"); blinkLed('g', 3);
            } else {
                showMsg("Err "); blinkLed('r', 2);
            }
            ThisThread::sleep_for(1000ms);
            break;
        }

        case 1: { // ADMN — change admin password
            showMsg("ADMN");
            ThisThread::sleep_for(800ms);

            if (!collectPin()) { showMsg("Canc"); ThisThread::sleep_for(1000ms); break; }

            char newpass[9];
            strncpy(newpass, g_input->getEnteredPassword(), 8);
            newpass[8] = '\0';

            if (g_manager->change_pswrd("a0", newpass)) {
                showMsg("SAVE"); blinkLed('g', 3);
            } else {
                showMsg("Err "); blinkLed('r', 2);
            }
            ThisThread::sleep_for(1000ms);
            break;
        }

        case 2: { // ADD — add new user
            showMsg("ADD ");
            ThisThread::sleep_for(800ms);

            if (!collectPin()) { showMsg("Canc"); ThisThread::sleep_for(1000ms); break; }

            char newpass[9];
            strncpy(newpass, g_input->getEnteredPassword(), 8);
            newpass[8] = '\0';

            if (g_manager->add_user(newpass, 'u')) {
                showMsg("Done"); blinkLed('g', 3);
            } else {
                showMsg("FULL"); blinkLed('r', 2);
            }
            ThisThread::sleep_for(1000ms);
            break;
        }

        case 3: { // DEL — delete user
            showMsg("DEL ");
            ThisThread::sleep_for(800ms);

            char id = collectId();
            if (id == '\0') { showMsg("Canc"); ThisThread::sleep_for(1000ms); break; }

            char uname[4];
            snprintf(uname, sizeof(uname), "u%c", id);

            if (g_manager->remove_user(uname)) {
                showMsg("Done"); blinkLed('g', 3);
            } else {
                showMsg("None"); blinkLed('r', 2);
            }
            ThisThread::sleep_for(1000ms);
            break;
        }
    }

    // Reset menu position after action
    currentMenuItem = 0;
    scrollPosition  = 0;
}

// Public entry point 
void runAdminMenu(Keypad& keypad, SLCD& slcd, DigitalOut& led, DigitalOut& led2,
                  InputModule& input, UserManagement& userManager)
{
    g_keypad  = &keypad;
    g_slcd    = &slcd;
    g_led     = &led;
    g_led2    = &led2;
    g_input   = &input;
    g_manager = &userManager;

    currentMenuItem = 0;
    scrollPosition  = 0;

    showMsg("Admn");
    ThisThread::sleep_for(800ms);

    bool exitAdmin = false;
    Timer displayTimer;
    displayTimer.start();

    while (!exitAdmin) {
        // Update display every 400ms directly in loop (no ticker)
        if (displayTimer.elapsed_time() >= 400ms) {
            displayMenu();
            displayTimer.reset();
        }

        char key = g_keypad->ReadKey();
        if (key != NO_KEY) {
            if (key == '0') {
                exitAdmin = true;
            } else if (key == '#') {
                nextMenuItem();
                displayMenu();        // update immediately on keypress
                displayTimer.reset();
            } else if (key == '*') {
                executeMenuItem(currentMenuItem);
                displayTimer.reset();
            } else if (key >= '1' && key <= '4') {
                executeMenuItem(key - '1');
                displayTimer.reset();
            }
        }

        ThisThread::sleep_for(10ms);
    }

    showMsg("Bye ");
    ThisThread::sleep_for(800ms);
}
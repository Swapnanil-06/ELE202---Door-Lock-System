#include "user_management.h"
#include "cstring"
#include "cstdio"
#include <cstddef>
#include <cstdint>
#include <cstring>

UserManagement::UserManagement(Keypad& keypad, SLCD& slcd, DigitalOut& led, DigitalOut& led2)
                 : keypad(keypad), slcd(slcd), led(led), led2(led2),
                  primary_page_w_count(0), mirror_page_w_count(0), id('\0'),
                  position(0), INPUT(false)

        {
                memset(pswrd_entered, 0, sizeof(pswrd_entered));
                memset(pswrd_showed, 0, sizeof(pswrd_showed));
                memset(users, 0, sizeof(users));

                led = 1;
                led2 = 1;

                init_flash();
                load_user_list();
        }


uint32_t UserManagement::page_address(int p_index)
{
    return FLASH_STO_ADD + (p_index * PAGE_SIZE);
}

bool UserManagement::read_page(int p_index, uint8_t* buf)
{
    return (flash.read(buf, page_address(p_index),PAGE_SIZE) == 0);
}

uint8_t UserManagement::checksum(const uint8_t* data, int length)
{
    uint8_t temp_checksum = 0;
    for (int i = 0; i < length; i++) temp_checksum ^= data[i];
    return temp_checksum;
}

bool UserManagement::validate_page(const uint8_t* buf)
{
    if (buf[MAGIC_INDEX] != SLOT_FILL_INDICATOR) return false;

    const uint8_t* data = buf + DATA_START_INDEX;
    uint8_t expected = checksum(data, (int)sizeof(users)*USER_LIMIT);
    return (buf[CHECKSUM_INDEX] == expected);
}

bool UserManagement::write_page(int p_index, uint8_t w_count)
{
    uint8_t buf[PAGE_SIZE];
    memset(buf, 0xFF, PAGE_SIZE);
    buf[MAGIC_INDEX] = SLOT_FILL_INDICATOR;
    buf[WRITE_COUNT_INDEX] = w_count;

    memcpy(buf + DATA_START_INDEX, users, sizeof(users));
    buf[CHECKSUM_INDEX] = checksum(buf + DATA_START_INDEX, (int)sizeof(users));
    
    int write_success = flash.program(buf, page_address(p_index), PAGE_SIZE);
    return(write_success == 0);
}



void UserManagement::page_wear_save_index()
{
    uint8_t buf[PAGE_SIZE];
    memset(buf, 0xFF, PAGE_SIZE);
    buf[MAGIC_INDEX] = SLOT_FILL_INDICATOR;
    buf[DATA_START_INDEX] = primary_page_w_count;
    buf[DATA_START_INDEX + 1] = mirror_page_w_count;
    flash.program(buf, page_address(WEAR_PAGE), PAGE_SIZE);
}

void UserManagement::init_flash()
{
    flash.init();
    page_wear_index();
}

void UserManagement::page_wear_index()
{
    uint8_t buf[PAGE_SIZE];
    if (flash.read(buf, page_address(WEAR_PAGE), PAGE_SIZE) != 0) return;

    if (buf[MAGIC_INDEX] == SLOT_FILL_INDICATOR) {
        primary_page_w_count = buf[DATA_START_INDEX];
        mirror_page_w_count  = buf[DATA_START_INDEX + 1];
    } else {
        primary_page_w_count = 0;
        mirror_page_w_count  = 0;
    }
}

void UserManagement::page_write_defaults()
{
    memset(users, 0, sizeof(users));
    strncpy(users[0].pswrd, "12345678", 8);
    users[0].pswrd[8] = '\0';
    users[0].role = 'a';
    strncpy(users[0].username, "a0", sizeof(users[0].username));

    
    strncpy(users[1].pswrd, "00000000", 8);
    users[1].pswrd[8] = '\0';
    users[1].role = 'u';
    strncpy(users[1].username, "u1", sizeof(users[1].username));

    save_user_list();
}

void UserManagement::load_user_list()
{
    uint8_t buf[PAGE_SIZE];
    
    if (read_page(PRIMARY_PAGE, buf) && validate_page(buf)) {
        memcpy(users, buf + DATA_START_INDEX, sizeof(users));
        primary_page_w_count = buf[WRITE_COUNT_INDEX];
        return;
    }

    if (read_page(MIRROR_PAGE, buf) && validate_page(buf)) {
        memcpy(users, buf + DATA_START_INDEX, sizeof(users));
        mirror_page_w_count = buf[WRITE_COUNT_INDEX];
        save_user_list();   
        return;
    }

    page_write_defaults();
}

void UserManagement::save_user_list()
{
    flash.erase(FLASH_STO_ADD, FLASH_V_SIZE);
    primary_page_w_count++;
    write_page(PRIMARY_PAGE, primary_page_w_count);
    mirror_page_w_count++;
    write_page(MIRROR_PAGE, mirror_page_w_count);
    page_wear_save_index();
}

char UserManagement::authenticate(const char* pswrd)
{
    printf("INPUT: %s\n", pswrd);

    for (int i = 0; i < USER_LIMIT; i++) {
        if (users[i].pswrd[0] != '\0' &&
            users[i].pswrd[0] != (char)0xFF) {
            printf("Stored[%d]: %s | Role: %c\n",
                   i, users[i].pswrd, users[i].role);

            if (strncmp(pswrd, users[i].pswrd, 8) == 0) {
                printf("MATCH FOUND at index %d\n", i);
                return users[i].role;
            }
        }
    }
    printf("NO MATCH\n");
    return 'x';
}

bool UserManagement::add_user(const char* pswrd, char role)
{
    for (int i = 0; i < USER_LIMIT; i++) {
        if (users[i].pswrd[0] == '\0') {
            strncpy(users[i].pswrd, pswrd, 8);
            users[i].pswrd[8] = '\0';
            users[i].role = role;


            if (role == 'a') snprintf(users[i].username, sizeof(users[i].username), "a%d", i);
            else             snprintf(users[i].username, sizeof(users[i].username), "u%d", i);


            save_user_list();

            slcd.clear();slcd.Home();
            slcd.printf("%s", users[i].username);
            ThisThread::sleep_for(1s);
            return true;
        }
    }
    return false;
}

bool UserManagement::remove_user(const char* username)
{
    for (int i = 0; i < USER_LIMIT; i++) {
        if (strcmp(username, users[i].username) == 0) {
            memset(&users[i], 0, sizeof(User));
            save_user_list();
            return true;
        }
    }
    return false;
}


bool UserManagement::change_pswrd(const char* username, const char* new_pswrd)
{
    size_t len = strlen(new_pswrd);
    if (len < 4 || len > 8) return false;


    for (int i = 0; i < USER_LIMIT; i++) {
        if (strcmp(username, users[i].username) == 0) {
            strncpy(users[i].pswrd, new_pswrd, 8);
            users[i].pswrd[8] = '\0';
            save_user_list();
            return true;
        }
    }
    return false;
}

void UserManagement::enter_admin_mode()
{
    display("Admn");

    const char* menu_items[] = { "open_menu", "add.u", "delete.u", "change", "exit" };
    const int   menu_count   = 5;
    int         current_menu = 0;


    const unsigned long SCROLL_MS = 2000;
    unsigned long last_scroll = us_ticker_read() / 1000;


    while (true) {
        unsigned long now = us_ticker_read() / 1000;


        slcd.clear();slcd.Home();
        slcd.printf("%s", menu_items[current_menu]);


        char key = keypad.ReadKey();


        if (key != NO_KEY_PRESS) {
            last_scroll = now;

            if (key == '*') {
                current_menu = (current_menu + 1) % menu_count;

            } else if (key == '#') {
                switch (current_menu) {

                case 0:
                    blink_led(3, 'g', 200ms);
                    return;  

                case 1:
                    input_reset();
                    while (!INPUT) { input_process("password"); }
                    if (add_user(pswrd_entered, 'u')) {
                        blink_led(3, 'g', 200ms);
                    } else {
                        display("FULL");
                    }
                    return;

                case 2: {
                    id = '\0';
                    INPUT = false;
                    position = 0;
                    slcd.clear();
                    slcd.Home();
                    slcd.printf("ID  ");
                    slcd.Colon(1);
                    ThisThread::sleep_for(500ms);
                    while (!INPUT) { input_process("id"); }
                    char user_name[4];
                    snprintf(user_name, sizeof(user_name), "u%c", id);
                    if (remove_user(user_name)) blink_led(3, 'g', 200ms);
                    else                    display("None");
                    return;
                }


               
                case 3: {
                    id = '\0'; INPUT = false; position = 0;
                    slcd.clear();slcd.Home();
                    slcd.printf("ID  "); slcd.Colon(1);
                    ThisThread::sleep_for(500ms);
                    while (!INPUT) { input_process("id"); }


                    if (id == '0') {
                        display("Admin");
                    } else {
                        char msg[5] = "User";
                        msg[3] = id;
                        display(msg);
                    }

                    input_reset();
                    while (!INPUT) { input_process("password"); }

                    char user_name[4];
                    if (id == '0') snprintf(user_name, sizeof(user_name), "a0");
                    else           snprintf(user_name, sizeof(user_name), "u%c", id);

                    if (change_pswrd(user_name, pswrd_entered)) {
                        blink_led(3, 'g', 200ms);
                    } else {
                        display("None");
                        blink_led(2, 'r', 200ms);
                    }
                    return;
                }
                
                case 4:
                    display("Bye ");
                    return;
                }
            }
        } else {
            if (now - last_scroll >= SCROLL_MS) {
                current_menu = (current_menu + 1) % menu_count;
                last_scroll  = now;
            }
        }


        ThisThread::sleep_for(70ms);
    }
}

void UserManagement::display(const char* msg)
{
    char buf[5] = "    ";
    strncpy(buf, msg, 4);
    buf[4] = '\0';
    slcd.clear();
    slcd.Home();
    slcd.printf("%s", buf);
    ThisThread::sleep_for(1s);
    slcd.clear();
    slcd.Home();
}

void UserManagement::blink_led(int times, char type, std::chrono::milliseconds interval)
{
    for (int i = 0; i < times; i++) {
        if (type == 'g')
        {
            led  = 0; ThisThread::sleep_for(interval);
            led  = 1; ThisThread::sleep_for(interval); 
        }
        else             
        {
            led2 = 0; ThisThread::sleep_for(interval);
            led2 = 1; ThisThread::sleep_for(interval); 
        }
    }
}

void UserManagement::blink_time_led(char type, std::chrono::milliseconds duration)
{
    if (type == 'g')
    {
        led  = 0;
        ThisThread::sleep_for(duration);
        led  = 1;
    }
    else
    {
        led2 = 0;
        ThisThread::sleep_for(duration);
        led2 = 1;
    }
}

void UserManagement::input_process(const char* inp_type)
{
    char key = keypad.ReadKey();
    if (key == NO_KEY_PRESS) return;


    if (strcmp(inp_type, "id") == 0) {
        if (position < 1 && key != '*' && key != '#') {
            id = key;
            position++;
            slcd.clear();
            slcd.Home();
            slcd.printf("ID %c", id);
            slcd.Colon(1);
        }
        else if (key == '*') {
            id = '\0'; position = 0; INPUT = false;
            slcd.clear();
            slcd.Home();
            slcd.printf("ID  ");
            slcd.Colon(1);
        }
        else if (key == '#' && position > 0) {
            slcd.clear();
            slcd.Home();
            INPUT = true;
        }


    } else {
    
        if (position < 8 && key != '*' && key != '#') {
            pswrd_entered[position] = key;
            position++;
            pswrd_entered[8] = '\0';

            char shown[5] = {0};
            if (position <= 4) strncpy(shown, pswrd_entered, position);
            else               strncpy(shown, pswrd_entered + (position - 4), 4);
            slcd.clear();
            slcd.Home();
            slcd.printf("%s", shown);


        } else if (key == '*') {
            input_reset();


        } else if (key == '#') {
            slcd.clear();slcd.Home();
            if (position < 4) {
                slcd.printf("INSF");
                ThisThread::sleep_for(2s);
                input_reset();
            } else {
                pswrd_entered[8] = '\0';
                INPUT = true;
            }
        }
    }
}


void UserManagement::input_reset()
{
    INPUT    = false;
    position = 0;
    memset(pswrd_entered, 0, sizeof(pswrd_entered));
    memset(pswrd_showed,  0, sizeof(pswrd_showed));
    slcd.clear();
    slcd.Home();
    slcd.printf("Pass");
    ThisThread::sleep_for(300ms);
}




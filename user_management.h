#ifndef USER_MANAGEMENT_H
#define USER_MANAGEMENT_H

#include <chrono>
#include <cstdint>
#undef __ARM_FP
#include "mbed.h"
#include "FlashIAP.h"
#include "SLCD/SLCD.h"
#include "keypad.h"      
#ifndef NO_KEY
#define NO_KEY 0
#endif
#define NO_KEY_PRESS NO_KEY



typedef struct {
    char pswrd[9];
    char role;
    char username[3];
} User;

#define USER_LIMIT 10
#define USER_SIZE sizeof(User)
#define PAGE_SIZE 256
#define FLASH_V_SIZE (PAGE_SIZE*3)
#define FLASH_STO_ADD 0x0003FC00 

#define PRIMARY_PAGE 0
#define MIRROR_PAGE  1
#define WEAR_PAGE    2

#define MAGIC_INDEX 0
#define WRITE_COUNT_INDEX 1
#define CHECKSUM_INDEX 2
#define DATA_START_INDEX 3

#define SLOT_FILL_INDICATOR 0xAB

class UserManagement{
    public:
    UserManagement(Keypad& keypad, SLCD& slcd, DigitalOut& led, DigitalOut& led2);

    char authenticate(const char* pswrd);

    void enter_admin_mode();

    bool add_user(const char* pswrd, char role);
    bool remove_user(const char* username);
    bool change_pswrd(const char* username, const char* new_pswrd);

    void load_user_list();
    void save_user_list();

private:
    Keypad& keypad;
    SLCD& slcd;
    DigitalOut& led;
    DigitalOut& led2;

    FlashIAP flash;
    User users[USER_LIMIT];
    uint8_t primary_page_w_count;
    uint8_t mirror_page_w_count;

    char pswrd_entered[9];
    char pswrd_showed[9];
    char id = '\0';
    int position = 0;
    bool INPUT;

    void init_flash();
    uint32_t page_address(int p_index);
    uint8_t  checksum(const uint8_t* data, int length);
    bool     read_page(int p_index, uint8_t* buf);
    bool     validate_page(const uint8_t* buf);
    bool     write_page(int p_index, uint8_t write);
    void     page_wear_index();
    void     page_wear_save_index();
    void     page_write_defaults();

    void display(const char* msg);
    void blink_led(int times, char type, std::chrono::milliseconds interval);
    void blink_time_led(char type, std::chrono::milliseconds duration);
    void input_process(const char* inp_type);
    void input_reset();
};
#endif
# ELE202 — Door Lock System

An embedded access control system developed as part of the ELE202 In-Circuit Emulation Design Exercise. The system is built around the NXP MKL46Z256VLL4 microcontroller, featuring a 48 MHz ARM Cortex-M0 processor core, and is implemented in C++.

## Overview

The system allows users to enter an access code via a 3×4 keypad, with feedback provided through an LCD display. Two levels of access are supported:

- **User level** - enter a valid code to trigger the relay and release the door strike
- **Supervisor level** - enter a higher-privilege code to set or change both the user and supervisor codes

Access codes are stored in non-volatile memory (NVM), ensuring they are retained across power cycles. The system is mains operated with a 30-minute battery backup in the event of power failure.

The LCD implementation goes beyond the base specification, supporting an 8-digit rolling display of keystroke history rather than the required 4 digits.

## Hardware

- NXP MKL46Z256VLL4 MCU (ARM Cortex-M0, 48 MHz)
- 3×4 matrix keypad for code entry
- LCD display for user feedback
- Relay output for door-strike actuation
- Mains power supply (230 V AC input, 12 V / 5 V / 3.3 V DC outputs)
- Battery backup circuit for 30-minute operation during power failure



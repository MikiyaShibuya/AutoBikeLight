/*
 * File:     main.c
 * Author: shibuya
 *
 * Created on August 29, 2022, 12:53 PM
 */

#include <xc.h>

// CONFIG1
#pragma config FOSC = INTOSC     // Use internal clock
#pragma config WDTE = OFF            // Disable watch dog timer
#pragma config PWRTE = ON            // Enable power-up timer
#pragma config MCLRE = OFF         // Use MCLR pin as RA3
#pragma config CP = OFF                // Don't protect program memory
#pragma config BOREN = ON            // Enable brown out reset
#pragma config CLKOUTEN = OFF    // Don't use clock out, use as RA4
#pragma config IESO = OFF            // Disable 2-stage clocks start up
#pragma config FCMEN = OFF         // Don't monitor clock

// CONFIG2
#pragma config WRT = OFF             // Don't protect flash memeory
#pragma config PPS1WAY = OFF     // Allow to try to unlock PPSLOCK anytime
#pragma config ZCDDIS = ON         // Disable zero cross detection
#pragma config PLLEN = OFF         // Disable ×4PLL
#pragma config STVREN = ON         // Enable stack overflow reset
#pragma config BORV = HI             // Set brown out trigger voltage as high
#pragma config LPBOR = OFF         // Disable low power brown out reset
#pragma config LVP = OFF             // Disable low voltage programming

#define _XTAL_FREQ 8000000

#define PWR_EN RA5

typedef unsigned char uint8_t;
// typedef unsigned int uint16_t; You can use this type
typedef unsigned long uint32_t;

const static uint32_t COUNT_1DAY = 86400 * 1024;
const static int TMR_CNT_H = (65536 - 235) >> 8;
const static int TMR_CNT_L = (65536 - 235) & 0xff;

// Time to keep powered when not moving
const static uint32_t STATIONAL_COUNT = 3L * 60 * 1024; // 3 min
// const static uint32_t STATIONAL_COUNT = 3L * 1024; // 3 sec

// Duty ratio is [DUTY VALUE] / 124
const static char METER_LAMP_DUTY = 15;
const static char MAIN_LIGHT_DUTY = 80;
const static char TAIL_LAMP_DIM_DUTY = 25;
const static char TAIL_LAMP_FLASH_DUTY = 124;
const static int BLIGHT_DURATION = 60;

// Count 1024th of 1 sec since vibration sensor change, max 86400 * 1024 = 1day
volatile static uint32_t count;
// Count 1024th of 1 sec since chip was powered
volatile static uint16_t millis;

// Get analog voltage as a floating point value in 0 to 1023
int read_adc(int channel) {

    if (channel == 0) {
        ADCON0 = 0b00001101; // Use AN3(RA4)
    }
    else {
        ADCON0 = 0b00011001; // Use AN6(RC2)
    }

    // Start A/D conversion
    ADCON0 = ADCON0 | 0b00000010;

    // Wait for A/D conversion process
    while(1) {
        uint8_t not_done = ADCON0 & 0b00000010;
        if (not_done == 0) {
            break;
        }
    }

    return (ADRESH * (int)0x100 + ADRESL);
}

// Reset timer1 count
void restart_counter() {
    GIE = 0;
    count = 0;
    TMR1H = TMR_CNT_H;
    TMR1L = TMR_CNT_L;
    TMR1IF = 0;
    GIE = 1;
}

// Interrupt function
void __interrupt() isr(){
    GIE = 0;
    if (TMR1IF == 1) {
        if (count < COUNT_1DAY) {
            count ++;
        }
        millis = (millis + 1) & 0x03ff;
        TMR1H = TMR_CNT_H;
        TMR1L = TMR_CNT_L;
        TMR1IF = 0;
    }
    GIE = 1;
}

void main() {

    OSCCON = 0b01110010; // [6:2]=1110 -> 8MHz
    ANSELA = 0b00000000; // Don't use Analog-A
    ANSELC = 0b00000000; // Don't use Analog-C
    TRISA = 0b00010000; // Only RA4 is input
    TRISC = 0b00000100; // Only RC2 is input
    PORTA = 0b00000000; // Initialize A port
    PORTC = 0b00000000; // Initialize C port

    PWR_EN = 1; // Enable power ASAP

    RC5PPS = 0b00001110; // Use RC5 port as PWM3OUT
    PWM3CON = 0b10000000; // Enable PWM3, active high

    RC4PPS = 0b00001111; // Use RC4 port as PWM4OUT
    PWM4CON = 0b10000000; // Enable PWM4, active high

    CCPTMRS = 0b00000000; // All PWM and CCP modules uses TMR2

    ADCON0 = 0b00011001;
    ADCON1 = 0b10110000; // Use internal clock, reference for Vdd and Vss
    ADCON2 = 0b00000000; // Don't use auto-conversion trigger

    T1CON = 0b00110101; // 1/4 clock source, 1:8 prescaler, enable TMR1
    TMR1H = (65536 - 10) >> 8;
    TMR1L = (65536 - 10) & 0xff;
    count = 0;
    millis = 0;
    TMR1IE = 1;
    TMR1IF = 0;
    PEIE = 1;
    GIE = 1;

    PR2 = 124; // 1kHz
    PWM3DCH = 0;
    PWM3DCL = 0;
    PWM4DCH = 0;
    PWM4DCL = 0;

    T2CON = 0b00000110; // Turn on TMR2 with x16 prescaler

    char vib_sns_prev = read_adc(0) > 50 ? 1 : 0; // Read from RA4
    int adc_value_itg = 512;

    // Produce third PWM by simply on/off;
    int pwm_count = 0;
    int pwm_duty = 1;
    int pwm_period = 8;

    while(1) {
        if (pwm_count == 0) {
            // Read analog value from RC2 (0 - 1023)
            int adc_value_read = read_adc(1);
            adc_value_itg = adc_value_itg - (adc_value_itg >> 3) + (adc_value_read >> 3);
        }

        char is_dark = adc_value_itg < 512 ? 1 : 0;

        char vib_sns = read_adc(0) > 50 ? 1 : 0;
        if (count > 102) {
              // Reset counter
              if (vib_sns_prev != vib_sns && count > 102) {
                  vib_sns_prev = vib_sns;
                  restart_counter();
              }
        }

        if (count > STATIONAL_COUNT) {
            PWR_EN = 0;
            PWM3DCH = 0;
            PWM4DCH = 0;
        }
        else {
            PWR_EN = 1;
            if (is_dark) {
                PWM3DCH = MAIN_LIGHT_DUTY;
            }
            else{
                    PWM3DCH = 0;
            }

            if (millis < BLIGHT_DURATION) {
                PWM4DCH = TAIL_LAMP_FLASH_DUTY;
            }
            else if (100 <= millis && millis < 100 + BLIGHT_DURATION) {
                PWM4DCH = TAIL_LAMP_FLASH_DUTY;
            }
            else {
                PWM4DCH = TAIL_LAMP_DIM_DUTY;
            }
        }

        if (pwm_count < pwm_duty && is_dark) {
                RC3 = 1;
        }
        else {
                RC3 = 0;
        }
        pwm_count = (pwm_count + 1) % pwm_period;
    }
    return;
}


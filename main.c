
//*****************************************************************************
//
// Application Name     - TV Remote Decoder (TV Code: Zonda 1355)
// Application Overview - The objective of this application is to demonstrate
//                          GPIO interrupts using SW2 and SW3.
//                          NOTE: the switches are not debounced!
//
//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_nvic.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "prcm.h"
#include "utils.h"
#include "systick.h"
#include "rom_map.h"

#include "spi.h"
#include "uart.h"

// Common interface includes
#include "uart_if.h"
#include "timer_if.h"
#include "timer.h"
#include "gpio.h"


// Common interface includes
#include "pin_mux_config.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
#include "oled_test.h"
#include "Adafruit_GFX.h"

// Pin configurations
#include "pin_mux_config.h"

#define ZERO "1010100000000000"
#define ONE "1010100010000000"
#define TWO "1010100001000000"
#define THREE "1010100011000000"
#define FOUR "1010100000100000"
#define FIVE "1010100010100000"
#define SIX "1010100001100000"
#define SEVEN "1010100011100000"
#define EIGHT "1010100000010000"
#define NINE "1010100010010000"
#define LAST "1010100000110000"
#define MUTE "0010100001001000"




//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

// some helpful macros for systick

// the cc3200's fixed clock frequency of 80 MHz
// note the use of ULL to indicate an unsigned long long constant
#define SYSCLKFREQ 80000000ULL
#define SPI_IF_BIT_RATE         100000
// macro to convert ticks to microseconds
#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

// macro to convert microseconds to ticks
#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

// systick reload value set to 40ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 3200000UL

// track systick counter periods elapsed
// if it is not 0, we know the transmission ended
volatile int systick_cnt = 0;
long ulStatus;
volatile int reset_int = 0;
volatile int repeat = 0;
char input[17] = "";
int bitlen = 0;

char newChar = '0';
char input_str[50] = ""; // Initialize input string
int expired = 0;

int pressCount = 0; // Track the number of presses on the current button
char* currentButton = "0"; // Track the current button being pressed

//volatile uint64_t interval = 0;
int interval = 0;

extern void (* const g_pfnVectors[])(void);

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************


char* decodeInput(char* input) {
    if (strcmp(input, ZERO) == 0) {
        return "0";
    } else if (strcmp(input, ONE) == 0) {
        return "1";
    } else if (strcmp(input, TWO) == 0) {
        return "2";
    } else if (strcmp(input, THREE) == 0) {
        return "3";
    } else if (strcmp(input, FOUR) == 0) {
        return "4";
    } else if (strcmp(input, FIVE) == 0) {
        return "5";
    } else if (strcmp(input, SIX) == 0) {
        return "6";
    } else if (strcmp(input, SEVEN) == 0) {
        return "7";
    } else if (strcmp(input, EIGHT) == 0) {
        return "8";
    } else if (strcmp(input, NINE) == 0) {
        return "9";
    } else if (strcmp(input, MUTE) == 0) {
        return "MUTE";
    } else if (strcmp(input, LAST) == 0) {
        return "LAST";
    } else {
        return "NULL";
    }
}



// Function to get the character corresponding to the button press
char getCharFromButton(const char *button, int pressCount) {
    if (strcmp(button, "2") == 0)
        return "abc"[pressCount % 3];
    else if (strcmp(button, "3") == 0)
        return "def"[pressCount % 3];
    else if (strcmp(button, "4") == 0)
        return "ghi"[pressCount % 3];
    else if (strcmp(button, "5") == 0)
        return "jkl"[pressCount % 3];
    else if (strcmp(button, "6") == 0)
        return "mno"[pressCount % 3];
    else if (strcmp(button, "7") == 0)
        return "pqrs"[pressCount % 4];
    else if (strcmp(button, "8") == 0)
        return "tuv"[pressCount % 3];
    else if (strcmp(button, "9") == 0)
        return "wxyz"[pressCount % 4];
    else
        return '\0'; // Invalid button
}


void display(void)
{

    char* D_input = decodeInput(input);


    if (strcmp(D_input,"NULL") == 0) { // invalid button pressed
        Report("received %s, not valid\n\r", input);
    }
    else{
        Report("Pressed %s\n\r", D_input);
    }



        TimerEnable(TIMERA2_BASE, TIMER_A);


             if(D_input == "MUTE") {
                 // Send the constructed string
                 int len = strlen(input_str);
                 input_str[len + 1] = '\0';
                 Report("Sending: %s\n", input_str);
                 TimerEnable(TIMERA1_BASE, TIMER_A);
                 //break;
             }

             else if(D_input == "LAST") {
                 // Delete the previous character
                 if(strlen(input_str) > 0) {
                     input[strlen(input_str) - 1] = '\0';
                     printf("Deleted: %s\n", D_input);
                 }
             }

             else {

                 newChar = getCharFromButton(D_input, pressCount);
                 Report("Current: %c  ",newChar);

                 // Process button presses
                 if(!strcmp(D_input,currentButton) && newChar != '\0') {
                     // New button pressed, reset press count
                     currentButton = D_input;
                     pressCount = 1;
                 }
                 else {
                   pressCount++;
                 }


             }




}


/**
 * Reset SysTick Counter
 */
static inline void SysTickReset(void) {
    // any write to the ST_CURRENT register clears it
    // after clearing it automatically gets reset without
    // triggering exception logic
    // see reference manual section 3.2.1
    HWREG(NVIC_ST_CURRENT) = 1;

    // clear the global count variable
    systick_cnt = 0;
}

/**
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    // increment every time the systick handler fires
    systick_cnt++;
}


static void GPIOA0IntHandler(void) {

    unsigned long ulStatus;
    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, true);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);        // clear interrupts on GPIOA0


    // read the countdown register and compute elapsed cycles
    uint64_t delta = SYSTICK_RELOAD_VAL - SysTickValueGet();

    // convert elapsed cycles to microseconds
    interval = TICKS_TO_US(delta);


        if (reset_int == 1){

        if (interval <= 2300 && interval >=1500){ // 2.25ms
            strcat(input, "1");
            bitlen++;
        }
        else if (interval < 1500 && interval>800){
            strcat(input, "0");
            bitlen++;
        }
       }

       if (interval <= 15000 && interval >=2800){
           reset_int = 1;
       }




    // reset the countdown register
    SysTickReset();


}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}

static void printLetter()
{
    TimerIntClear(TIMERA1_BASE, TIMER_A);
    TimerDisable(TIMERA1_BASE, TIMER_A);
    fillScreen(BLACK);
   setTextColor(WHITE, BLACK);
   setTextSize(1);
   fillScreen(BLACK);
   setCursor(0, 0); // cursor is outside the screen from previous test
   setTextColor(GREEN, BLACK);
   setTextSize(2);
   Outstr(input_str);
   memset(input_str, 0, sizeof(input_str));
}

static void Timer2IntHandler(void) {
    TimerIntClear(TIMERA2_BASE, TIMER_A);
    TimerDisable(TIMERA2_BASE, TIMER_A);
    expired = 1;
    int len = strlen(input_str);
    pressCount = 0;
    input_str[len] = newChar;
    input_str[len + 1] = '\0'; // Null-terminate the string
    expired = 0;
}


//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//!
//! \return None.
//
//****************************************************************************
int main() {

    BoardInit();
    
    PinMuxConfig();
    
    // Enable SysTick
    SysTickInit();

    // Initialize UART Terminal
    InitTerm();

    // Clear UART Terminal
    ClearTerm();
    unsigned long ulStatus;

    MAP_PRCMPeripheralReset(PRCM_GSPI);

    //
     // Reset SPI
     //
     MAP_SPIReset(GSPI_BASE);

     //
     // Configure SPI interface
     //
     MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                      SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                      (SPI_SW_CTRL_CS |
                      SPI_4PIN_MODE |
                      SPI_TURBO_OFF |
                      SPI_CS_ACTIVEHIGH |
                      SPI_WL_8));

     //
     // Enable SPI for communication
     //
     MAP_SPIEnable(GSPI_BASE);

     Adafruit_Init();
     fillScreen(BLACK);


     Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
     Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, printLetter);
     TimerLoadSet(TIMERA1_BASE, TIMER_A, MILLISECONDS_TO_TICKS(1000));

     Timer_IF_Init(PRCM_TIMERA2, TIMERA2_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
     Timer_IF_IntSetup(TIMERA2_BASE, TIMER_A, Timer2IntHandler);
     TimerLoadSet(TIMERA2_BASE, TIMER_A, MILLISECONDS_TO_TICKS(2000)); // 2 seconds



    MAP_GPIOIntRegister(GPIOA0_BASE, GPIOA0IntHandler);

    MAP_GPIOIntTypeSet(GPIOA0_BASE, 0x1, GPIO_FALLING_EDGE);    // pin50
    ulStatus = MAP_GPIOIntStatus (GPIOA0_BASE, false);
    MAP_GPIOIntClear(GPIOA0_BASE, ulStatus);            // clear interrupts on GPIOA1
    MAP_GPIOIntEnable(GPIOA0_BASE, 0x1); //enable

    Message("\t\t****************************************************\n\r");
    Message("\t\t\tRemote Decode\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");

    // reset the countdown register
    SysTickReset();
    while (1) {

       //Report("%d ", repeat);

        if (bitlen == 16){ // end of signal
            display();
            bitlen = 0;
            reset_int = 0;
            memset(input, 0, sizeof(input));
            MAP_GPIOIntDisable(GPIOA0_BASE, 0x1);
            UtilsDelay(4000000);
            MAP_GPIOIntEnable(GPIOA0_BASE, 0x1);


        }

    }

}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

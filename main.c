
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
#include "gpio_if.h"


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

volatile int reset_int = 0;
char input[17] = "";
int bitlen = 0;
int send_ready = 0;
int receive_ready = 0;



char newChar = '0';
char input_str[50] = ""; // Initialize input string

// Uart Communication
char uart_buffer[50] = "";
volatile int uart_ind = 0;


int pressCount = 0; // Track the number of presses on the current button
char* currentButton = "0"; // Track the current button being pressed

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

static void printLetterDown(char* str)
{
    int cursor_x = 0;

    while (*str != '\0') {
        drawChar(cursor_x, WIDTH*3/4, *str, 0xFFFF, 0xFFFF, 2);
        str++;
        cursor_x += 11;
    }


}

static void printLetterUp(char* str)
{

    int cursor_x = 0;

    while (*str != '\0') {
        drawChar(cursor_x, WIDTH*1/8, *str, 0xFFFF, 0xFFFF, 2);
        str++;
        cursor_x += 11;
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


         if(D_input == "MUTE") {
             // Send the constructed string
             int len = strlen(input_str);
             input_str[len + 1] = '\0';
             send_ready = 1;

         }

         else if(D_input == "LAST") {
             // Delete the previous character
             if(strlen(input_str) > 0) {
                 Report("Deleted: last character\n");
                 input_str[strlen(input_str) - 1] = '\0';
                 newChar = '\0';

             }
         }

         else if (D_input != "NULL"){
             TimerEnable(TIMERA2_BASE, TIMER_A); // begin count for time out for input str

             newChar = getCharFromButton(D_input, pressCount);
             Report("Current: %c\n\r",newChar);

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



static void Timer2IntHandler(void) {
    TimerIntClear(TIMERA2_BASE, TIMER_A);
    TimerDisable(TIMERA2_BASE, TIMER_A);
    int len = strlen(input_str);
    pressCount = 0;
    input_str[len] = newChar;
    input_str[len + 1] = '\0'; // Null-terminate the string
}


static void UART1IntHandler(void) {

    unsigned long ulStatus_u;
    ulStatus_u = MAP_UARTIntStatus(UARTA1_BASE, true);
    MAP_UARTIntClear(UARTA1_BASE, PRCM_UARTA1);
    if (ulStatus_u & UART_INT_RX) {
        int uart_ind = 0;
        while (UARTCharsAvail(UARTA1_BASE)) {
            uart_buffer[uart_ind] = UARTCharGetNonBlocking(UARTA1_BASE);
            //uart_buffer[uart_ind] = UARTCharGet(UARTA1_BASE);
            uart_ind++;
        }
        int len = strlen(uart_buffer);
        uart_buffer[len + 1] = '\0'; // Null-terminate the string
        Report("Received %s\n", uart_buffer);
    }


    receive_ready = 1;


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

     //prcmperipheral reset

     // UART



     MAP_PRCMPeripheralReset(PRCM_UARTA1);


     MAP_UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1),
                             UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));


     MAP_UARTIntRegister(UARTA1_BASE, UART1IntHandler);

     UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

     unsigned long ulStatus_u;
     ulStatus_u = MAP_GPIOIntStatus(UARTA1_BASE, false);
     MAP_GPIOIntClear(UARTA1_BASE, ulStatus_u);

     MAP_UARTIntEnable(UARTA1_BASE, PRCM_UARTA1);



     MAP_UARTEnable(UARTA1_BASE);





/*
     Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
     Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, printLetter);
     TimerLoadSet(TIMERA1_BASE, TIMER_A, MILLISECONDS_TO_TICKS(100));
     */

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

        if (bitlen == 16){

            display();
            bitlen = 0;
            reset_int = 0;
            memset(input, 0, sizeof(input));
            MAP_GPIOIntDisable(GPIOA0_BASE, 0x1);
            UtilsDelay(4000000);
            MAP_GPIOIntEnable(GPIOA0_BASE, 0x1);


        }

        if (send_ready == 1){
            Report("Sending: %s\n", input_str);
            int i = 0;
            //print char for test
            fillScreen(BLACK);
            printLetterUp(input_str);


            while(input_str[i] != '\0'){
                UARTCharPutNonBlocking(UARTA1_BASE, input_str[i]);
                //UARTCharPut(UARTA1_BASE, input_str[i]);
                i++;
            }
            memset(input_str, 0, sizeof(input_str));
            send_ready = 0;
        }


            if (receive_ready == 1){
            //print str
            fillScreen(BLACK);
             printLetterDown(uart_buffer);
             memset(uart_buffer, 0, sizeof(uart_buffer));
            receive_ready = 0;
        }


    }

}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/I2C.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*
 *  ======== UART Driver Stuff ========
 */
#define DISPLAY(x) UART_write(uart, &output, x);

// UART Global Variables
char output[64];
int bytesToSend;

// Driver Handles - Global variables
UART_Handle uart;

void initUART(void)
{
    UART_Params uartParams;
    // Init the driver
    UART_init();
    // Configure the driver
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.baudRate = 115200;

    // Open the driver
    uart = UART_open(CONFIG_UART_0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }
}
// ------------------------------- UART END -----------------------------

/*
 *  ======== I2C Driver Stuff ========
 */
// I2C Global Variables
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
}

sensors[3] = {
    { 0x48, 0x0000, "11X" },
    { 0x49, 0x0000, "116" },
    { 0x41, 0x0001, "006" }
};

uint8_t txBuffer[1];
uint8_t rxBuffer[2];
I2C_Transaction i2cTransaction;

// Driver Handles - Global variables
I2C_Handle i2c;

// Make sure you call initUART() before calling this function.
void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;
    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "));

    // Init the driver
    I2C_init();
    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);

    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"));
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"));

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses
    /* Common I2C transaction setup */
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.slaveAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;
        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id));
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY(snprintf(output, 64, "Found\n\r"));
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"));
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.slaveAddress));
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"));
    }
}

int16_t readTemp(void)
{
    int16_t temperature = 0;
    i2cTransaction.readCount = 2;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;
        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor %d\n\r", i2cTransaction.status));
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"));
    }
    return temperature;
}
// ---------------------------------- I2C END ------------------------------------------

/*
 *  ======== Timer Driver Stuff ========
 */
// Driver Handles - Global variables
Timer_Handle timer0;
volatile unsigned char TimerFlag = 0;

void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
}

void initTimer(void)
{
    Timer_Params params;
    // Init the driver
    Timer_init();
    // Configure the driver
    Timer_Params_init(&params);
    //Every 100 milliseconds
    params.period = 1000000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0,  &params);
    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }
    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }
}
// ---------------------------------- Timer End ------------------------------------------


int leftBtn = 0;
int rightBtn = 0;
/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Toggle an LED */
    //GPIO_toggle(CONFIG_GPIO_LED_0);
    leftBtn = 1;

}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Toggle an LED */
    rightBtn = 1;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{



    /* Call driver init functions */
    GPIO_init();
    initUART(); // The UART must be initialized before calling initI2C()
    initI2C();
    initTimer();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }




/*
    // Loop Forever
    // The student should add flags (similar to the timer flag) to the button handlers
    /*  output to the server (via UART) should be formatted as
     *  AA    = ASCII decimal value of room temperature (00 - 99) degrees Celsius
     *  BB    = ASCII decimal value of set-point temperature (00-99) degrees Celsius
     *  S     = 0 if heat is off, 1 if heat is on
     *  CCCC  = decimal count of seconds since board has been reset
     *  <%02d,%02d,%d,%04d> = temperature, set-point, heat, seconds

    while(1)
    {
      // Every 200ms check the button flags
      // Every 500ms read the temperature and update the LED
      // Every second output the following to the UART
      // <%02d,%02d,%d,%04d> = temperature, setpoint, heat, seconds
      DISPLAY( snprintf(output, 64, "<%02d, %02d, %d, %04d>\n\r", temperature, setpoint_F, heat, seconds ))
      // Refer to zyBooks - "Converting different-period tasks to C"
      // Remember to configure the timer period
      while (!TimerFlag){} // Wait for the timer period
      TimerFlag = 0;       // Lower flag raised by timer
      ++timer;
    }
*/




/*
    if (Btn_elapsedTime >= 200) {
        if(UpPressed() == true) {
            raise();
        }
        if(DownPressed() == true) {
            lower();
        }
        Btn_elapsedTime = 0;

    }

    if (Btn_elapsedTime >= 500) {

   //     roomTemp = roomTemp();
        updateHeater();
        setIndicator(heaterStatus);
        Temp_elapsedTime = 0;


      }
    if (Message_elapsedTime >= 1000) {
        upTime++;
        DISPLAY(snprintf(output, 64, "<%02d, %02d, %d, %04d>\n\r", roomTemp, requestTemp, heaterStatus, upTime));
        Message_elapsedTime = 0;
    }*/



   unsigned long Btn_elapsedTime = 200;
    unsigned long Temp_elapsedTime = 500;
    unsigned long Message_elapsedTime = 1000;
    const unsigned long timerPeriod = 100;

    unsigned long Btn_TimeChecked = 0;
    unsigned long tempCheckedTime = 0;
    unsigned long checkDisplayTime = 0;

    unsigned long setpoint = 25;                                               // Init setpoint (default thermostat value)
    unsigned long heat = 0;                                                   // Init heat (0 is off, 1 is on)
    unsigned long seconds = 0;                                                // Init seconds
    unsigned long temperature = 0;


    while (1)
        {
            readTemp();                                                 // Read for temp changes on board (changes in real time based on ambient temp)
            /* Every 200ms, check button presses */
            if (Btn_TimeChecked >= Btn_elapsedTime)                   // Button check time equals or exceeds period
            {
                if (rightBtn == 1)                                   // Button on right side raises setpoint (thermostat setting) by 1
                {
                    setpoint += 1;                                      // Increment thermostat
                    rightBtn = 0;                                    // Reset button to 0
                }
                if (leftBtn == 1)                                    // Button on left side lowers setpoint (thermostat setting) by 1
                {
                    setpoint -= 1;                                      // Decrement thermostat
                    leftBtn = 0;                                     // Reset button to 0
                }
            }

            /* Every 500ms check temperature and update LED/heat */
           if (tempCheckedTime >= Temp_elapsedTime)                       // Temp check time equals or exceeds period
            {
                temperature = readTemp();                               // Set temperature to equal readTemp
                if (temperature < setpoint)                             // Temp lower than thermostat setting, heat/LED turns on
                {
                    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
                    heat = 1;
                }
                else                                                    // Temp higher than thermostat setting, heat/LED turns off
                {
                    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
                    heat = 0;
                }
            }

            /* Every 1000ms output to UART to display temperature */
           // if (checkDisplayTime >= Message_elapsedTime)                 // Display check time equals or exceeds period
            //{
                DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", // Display to terminal (temperature, setpoint, heat, seconds)
                                temperature, setpoint, heat, seconds));
                ++seconds;                                              // Increment seconds every 1000ms
            //}
            while (!TimerFlag){}                                        // Wait for timer period
            TimerFlag = 0;                                              // Lower flag raised by timer
            Btn_elapsedTime += timerPeriod;                             // Increment buttonCheckTime by 100ms
            Temp_elapsedTime += timerPeriod;                               // Increment tempCheckTime by 100ms
            Message_elapsedTime += timerPeriod;                            // Increment displayCheckTime by 100ms
        }

   /* int16_t input[] = temperature;

    UART_write(uart, &input, 30);*/

    return (NULL);
}

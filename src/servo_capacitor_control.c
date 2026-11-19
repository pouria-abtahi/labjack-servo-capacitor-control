/*
Program: HSI255 - Project
Author: Pouria Abtahi
Date: April 9, 2023
Purpose: Purpose of this program is that interfaces with the motor and LED using a LabJack U3-HV device, based on input
         from a push-button and the voltage of capacitor(AIN1). Your program begins with the servo motor arm (FIO4) all the way to one side and
         the LED off (FIO5 = 0). When the program starts, it will explain the program and ask the user to enter threshold voltage. Also,
         it asks user to choose exit strategy. The user has two choice for exit strategy. 1- how many time in second 2- How many motor activation.  
         After user makes selections, the program will start showing voltage of capacitor. When the user presses the pushbutton,
         the capacitor will charge and when it reaches threshold, the servo motor will move to middle and turn on LED, and alert user
         that the threshold voltage has been reached. When the voltage drops below the threshold, the motor back to its initial position
         and LED turns off (FIO5 = 0).
*/

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "C:\Program Files (x86)\LabJack\Drivers\LabJackUD.h"


int main()

{
                            //// declare variables
    LJ_ERROR lj_cue;
    LJ_HANDLE lj_handle = 0;


    int debounceDelay = 200;  // milliseconds.  when capacitor reaches threshold voltage, it swings
                               // around the threshold, causing multiple activations of the servo motor.
                                // to solve this issue, I add this vaiable and small delay in code to give 
                                // capacitor enough time to stabilize before checking its voltage again.


    double dblAIN1;             // capacitor
    
    double threshold_voltage;

    int counter = 0;

    int currentPosition = -1; // means unknown position

    int exitStrategy = 0;
    int exitCount = 0;

    int programDuration;

    int programCounter = 0;

    int programLimit = 0;


    int motorActivations = 0;
    int previousState = 0;




    lj_cue = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &lj_handle);  //opens communication  with LabJack.

    lj_cue = ePut(lj_handle, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);  // reset LabJack configuration and channels

    // produce 50 Hz PMW signal
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_COUNTER_PIN_OFFSET, 4, 0, 0); // set up FIO4 as PMW output
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_CLOCK_BASE, LJ_tc1MHZ_DIV, 0, 0); // TCB= 1Mhz
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chTIMER_CLOCK_DIVISOR, 78, 0, 0); // DIV=78
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_CONFIG, LJ_chNUMBER_TIMERS_ENABLED, 1, 0, 0); // Enable the timer
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_MODE, 0, LJ_tmPWM8, 0, 0); // set Timer mode to 8-bit



    // set up the servo motor and LED. program starts with the servo motor arm all the way to one side and the LED off
    

    lj_cue = AddRequest(lj_handle, LJ_ioPUT_DIGITAL_BIT, 5, 0, 0, 0); // LED is off; for FIO5 = 0
    lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, 63810, 0, 0); // PWM duty cycle for left side = 0 degree
    lj_cue = Go();


    // explain program to user

    printf("\n\t\t\tThis program controls a servo motor and an LED based on the voltage of a capacitor.\n");
    printf("\t\t\tWhen the capacitor reaches a certain threshold voltage, the servo motor\n");
    printf("\t\t\twill change position to middle and the LED will turn on.\n");
    printf("\t\t\tAlso, it asks user to choose exit strategy. The user has two choice for exit strategy.\n");
    printf("\t\t\t1- how many time in second 2- How many motor activation.\n");



    system("pause");
    system("cls");


    //// getting input from user to choose threshold voltage

    do
    {
        printf("\n\n\t\tPlease enter the threshold voltage (0.1 to 4.45): ");
        scanf("%lf", &threshold_voltage);


        if (threshold_voltage < 0.1 || threshold_voltage > 4.45)

        {
            printf("\n\tInvalid input. Please enter a value between 0.1 and 4.45\n");
            continue;
        }

        printf("\n\n\n\tExit strategy:\n\n");
        printf("\n1. Time (seconds)");
        printf("\n2. Number of motor activations");
        printf("\n\nPlease choose an Exit strategy: ");
        scanf("%d", &exitStrategy);


               //// geting input from user for program duration or motor activations

        if (exitStrategy == 1)
        {
            printf("\n\tEnter the number of seconds you want the program to run: ");
            scanf("%d", &programDuration);
            programLimit = programDuration * 10;       // 10 updates per second
        }
        else if (exitStrategy == 2)
        {
            printf("\n\t\tEnter the number of motor activations: ");
            scanf("%d", &exitCount);
            programLimit = INT_MAX; // set the programLimit to the maximum value of an int
        }
        else
        {
            printf("Invalid input. Please try again.\n");
        }
    } while (exitStrategy != 1 && exitStrategy != 2);


    lj_cue = AddRequest(lj_handle, LJ_ioGET_AIN, 1, 0, 0, 0);             // read capacitor voltage
    lj_cue = Go();
    lj_cue = GetResult(lj_handle, LJ_ioGET_AIN, 1, &dblAIN1);


    while (programCounter < programLimit)

    {
        lj_cue = AddRequest(lj_handle, LJ_ioGET_AIN, 1, 0, 0, 0);             // read AIN1 for capacitor
        lj_cue = Go();
        lj_cue = GetResult(lj_handle, LJ_ioGET_AIN, 1, &dblAIN1);

        printf("\t\tCapacitor voltage = %.2lf\n", dblAIN1);              //report capasior voltage


        if (dblAIN1 >= threshold_voltage && previousState == 0) // check if threshold voltage is reached and the previous state was below threshold
        {
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_DIGITAL_BIT, 5, 1, 0, 0); // LED is on; for FIO5 = 1
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, 60621, 0, 0);
            lj_cue = Go();
            printf("\t\tThreshold voltage reached.\n");
      
            motorActivations++;          // motor activation counter
            previousState = 1;          // set last state to above threshold
            
            Sleep(debounceDelay);       // debounce delay
        }


       else if (dblAIN1 < threshold_voltage)
        {
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_DIGITAL_BIT, 5, 0, 0, 0); // LED is off; for FIO5 = 0
            lj_cue = AddRequest(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, 63810, 0, 0);
            lj_cue = Go();

            previousState = 0;  // set last state to below threshold
        }


        // check exit conditions

        if (exitStrategy == 1 && programCounter >= programLimit)
        {
            break;
        }


        else if (exitStrategy == 2 && motorActivations >= exitCount)

        {
            break;
        }

        Sleep(100);                  // 10 times per second
        programCounter++;
    }


    lj_cue = AddRequest(lj_handle, LJ_ioPUT_DIGITAL_BIT, 5, 0, 0, 0);   // LED is off; for FIO5 = 0
    lj_cue = ePut(lj_handle, LJ_ioPUT_TIMER_VALUE, 0, 63810, 0, 0);   // motor to initial position
    lj_cue = Go();

    lj_cue = ePut(lj_handle, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);  // reset LabJack configuration and channels


    system("pause");   //program ends
    Close();
    return 0;
}
#include <stdio.h>
#include <math.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Atomizer.h>
#include <Button.h>
#include <TimerUtils.h>
#include <Battery.h>

#include "Bitmap_eVicSDK.h"

#define FIRE 0
#define RIGHT 1
#define LEFT 2
volatile uint32_t timerCounter[3] = {0}; //Space for timers of buttons.
volatile uint8_t buttonPressed[3] = {0}; //Determines if button was just pressed
volatile uint8_t timerSpec[3] = {0}; // How many times button has been "tapped"
volatile uint8_t shouldFire = 0;

void timerCallback() {
	// We use the optional parameter as an index
	// counterIndex is the index in timerCounter
    timerCounter[FIRE]++;
    timerCounter[RIGHT]++;
    timerCounter[LEFT]++;
    return;
}

void updateCounter(uint8_t btn) {
    // Handles timer and other logic functions
    // i.e use buttonPressed[button] instead of btnState & BUTTON_MASK_numname
    //
    // Each button handler checks and does atleast this accordingly:
    // - If the button was not pressed in previous iter and is now pressed:
    // -- reset timerCounter[button] and set buttonPressed[button] to True.
    // - If the button is realesed and was pressed in previous iter:
    // -- reset timerCounter[button] and set buttonPressed[button] to False.
    
    // Fire button timer and counter logic
    
    if (btn & BUTTON_MASK_FIRE) {
        if (buttonPressed[FIRE] == 0) {
            timerCounter[FIRE] = 0;
        }
        //This if may be damaging to some functions
        if (timerCounter[FIRE] > 60) {
            timerSpec[FIRE] = 0;
        }
        buttonPressed[FIRE] = 1;

    }
    else if (!(btn & BUTTON_MASK_FIRE)) {
        if (timerCounter[FIRE] < 60 && buttonPressed[FIRE]) {
        timerSpec[FIRE]++;
        timerCounter[FIRE] = 0;
        } else if (buttonPressed[FIRE] || timerCounter[FIRE] > 60) {
            timerSpec[FIRE] = 0;
        }
        buttonPressed[FIRE] = 0;
    }
    
    // Right button timer logic
    if (btn & BUTTON_MASK_RIGHT) {
        if (buttonPressed[RIGHT] == 0) {
            timerCounter[RIGHT] = 0;
        }
        buttonPressed[RIGHT] = 1;
        
        //This if may be damaging to some functions
        if (timerCounter[RIGHT] > 60) {
            timerSpec[RIGHT] = 0;
        }
    }
    else if (!(btn & BUTTON_MASK_RIGHT)) {
        if (timerCounter[RIGHT] < 60 && buttonPressed[RIGHT]) {
        timerSpec[RIGHT]++;
        timerCounter[RIGHT] = 0;
        } else if (buttonPressed[RIGHT] || timerCounter[RIGHT] > 60) {
            timerSpec[RIGHT] = 0;
        }
        buttonPressed[RIGHT] = 0;
    }
    
    // Left button timer logic
    if (btn & BUTTON_MASK_LEFT) {
        if (buttonPressed[LEFT] == 0) {
            timerCounter[LEFT] = 0;
        }
        //This if may be damaging to some functions
        if (timerCounter[LEFT] > 60) {
            timerSpec[LEFT] = 0;
        }
        buttonPressed[LEFT] = 1;
    }
    else if (!(btn & BUTTON_MASK_LEFT)) {
        if (timerCounter[LEFT] < 60 && buttonPressed[LEFT]) {
            timerSpec[LEFT++;
        timerCounter[LEFT] = 0;
        } else if (buttonPressed[LEFT] || timerCounter[LEFT] > 60) {
            timerSpec[LEFT] = 0;
        }
        buttonPressed[LEFT] = 0;
    }
}

uint16_t wattsToVolts(uint32_t watts, uint16_t res) {
	// Units: mV, mW, mOhm
	// V = sqrt(P * R)
	// Round to nearest multiple of 10
	uint16_t volts = (sqrt(watts * res) + 5) / 10;
	return volts * 10;
}

uint16_t correctVoltage(uint16_t currentVolt, uint32_t currentWatt, uint16_t res) {
    // Resistance fluctuates, this corrects voltage to the correct setting.
    uint16_t newVolts = wattsToVolts(currentWatt, res);
    
    if (newVolts != currentVolt) {
        if (Atomizer_IsOn()) {
            // Update output voltage to correct res variations:
            // If the new voltage is lower, we only correct it in
            // 10mV steps, otherwise a flake res reading might
            // make the voltage plummet to zero and stop.
            // If the new voltage is higher, we push it up by 100mV
            // to make it hit harder on TC coils, but still keep it
            // under control.
            if (newVolts < currentVolt) { newVolts = currentVolt - (currentVolt >= 10 ? 10 : 0); }
            else { newVolts = currentVolt + 100; }
        }
    }
    return newVolts;
}

int main(){
    char buf[100];
    uint16_t volts, displayVolts, newVolts/*, battVolts*/;
	uint32_t watts, inc;
	uint8_t btnState/*, battPerc, boardTemp*/;
	Atomizer_Info_t atomInfo;
    
    Atomizer_ReadInfo(&atomInfo);
    
    watts = 20000; // Initial wattage
    volts = wattsToVolts(watts, atomInfo.resistance);
    Atomizer_SetOutputVoltage(volts);
    
    // Display On screen for 0.5 s.
    Display_PutPixels(0, 32, Bitmap_evicSdk, Bitmap_evicSdk_width, Bitmap_evicSdk_height);
    Display_Update();
    Timer_DelayMs(500);
    
    Timer_CreateTimeout(10,1, timerCallback, 0); //Fire
    while(1){
        btnState = Button_GetState();
        updateCounter(btnState);
        Atomizer_ReadInfo(&atomInfo);
        
        // If has tapped and time elapsed is less than 60 10ms, disable firing
        if (timerSpec > 1 && timerCounter[0] < 60) { 
            shouldFire = 0;
        } else {
            shouldFire = 1;   
        }
        // Button logic, "don't" use switch, I've heard it's not efficient.
        //http://embeddedgurus.com/stack-overflow/2010/04/efficient-c-tip-12-be-wary-of-switch-statements/
        //
        
        //If should fire
        if (!Atomizer_IsOn() && (buttonPressed[0]) &&
            (atomInfo.resistance != 0) && shouldFire &&
            (Atomizer_GetError() == OK)) {
                // Take power on
                Atomizer_Control(1);
        } //If not firing
        else if((Atomizer_IsOn() && !(buttonPressed[FIRE])) || !shouldFire) {
			// Take power off
			Atomizer_Control(0);
		}
        
        if (buttonPressed[RIGHT]) {
            if (timerCounter[RIGHT] <= 110) {
                inc = round(0.479332 * exp(timerCounter[RIGHT]*0.0303612)) * 100;
            } else {
                inc = 1800;
            }
            newVolts = wattsToVolts(watts + inc, atomInfo.resistance);
            if (watts + inc >= 75000){
                watts = 75000;
                newVolts = wattsToVolts(watts, atomInfo.resistance);
            }else if(newVolts <= ATOMIZER_MAX_VOLTS) {
                watts += inc;
            }
            volts = newVolts;
            Atomizer_SetOutputVoltage(volts);
        }
        if (buttonPressed[LEFT]) {
            uint32_t deinc;
            
            if (timerCounter[LEFT] <= 110) {
                deinc = round(0.479332 * exp(timerCounter[LEFT]*0.0303612)) * 100;
            } else {
                deinc = 1800;
            }
            newVolts = wattsToVolts(watts - deinc, atomInfo.resistance);
            if (watts - deinc <= 0){
                watts = 0;
                newVolts = wattsToVolts(watts, atomInfo.resistance);
            }else if (newVolts <= ATOMIZER_MAX_VOLTS) {
                watts -= deinc;
            }
            volts = newVolts;
            Atomizer_SetOutputVoltage(volts);
        }
        
        Atomizer_ReadInfo(&atomInfo);
		
        volts = correctVoltage(volts, watts, atomInfo.resistance);
        Atomizer_SetOutputVoltage(volts);
        
        displayVolts = Atomizer_IsOn() ? atomInfo.voltage : volts;
        
        siprintf(buf, "P:%3lu.%luW\nV:%3d.%02d\n",
        watts / 1000, watts % 1000 / 100,
		displayVolts / 1000, displayVolts % 1000 / 10);
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
    }
    
}
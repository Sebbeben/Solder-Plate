#include "./globals.h"
#include "./EEPROMDataManager.h"
#include "Adafruit_ST7789.h"
#include "thermistors/TemperatureController.h"


WrappedState<ReflowProcessState> reflowProcessState = WrappedState<ReflowProcessState>(INITIALIZING);

AnalogRef analogRef(5.0);

ThermistorLookup thermistorLookup = ThermistorLookup();

// Calibration data for 100K thermistors ->https://datasheet.lcsc.com/lcsc/1810161311_Nanjing-Shiheng-Elec-MF58-104F3950_C123399.pdf ->Glass thermistor NTC 3950 100K
// TempCalibration calibration_100K_3950 = {25, 100000, 107, 4957, 167, 1000};
TempCalibration calibration_100K_3950 = {25, 10000, 86, 1032, 169, 118};
// Initalize the 3950 100K thermistors with ACTUAL reference resistor measurnment(Measured between Left pin and GND when the board is powered off) using the default calibration data for 100K thermistor

// You can also make a custom calibration data for your thermistor and use that instead of the default one pass it as shown below --> keep the naming of the thermistor the same as the one you are replacing
// TempCalibration calibration_10K_3950 = {25, 10000, 86, 1032, 169, 118};
// Thermistor thermistor1(THERMISTOR1_PIN, 100000, calibration_10K_3950,ThermistorZ_Placement::TOP,ThermistorXY_Placement::MIDDLE);

//--------------------------------------------------------------------------------------------------------------------------------------

// This is where you set your potentiometer  values  and positioning for the 6 thermistors

// You can tweak them from the datasheet to best fit your thermistor but we reccomend using the default values and setting the potentiometer to these values
//  Does not have to be perfect just set it close to this value and record the measured value and put it for the thermistors
// To measure the resistence turn off the controller completley and measure between GND and the left pin of the connector with the thermistor unplugged

// 2.5k reference = Best accuracy around 138C
Thermistor thermistor1(THERMISTOR1_PIN, 2545, ThermistorZ_Placement::ON_PCB, ThermistorXY_Placement::MIDDLE);          // -> 2545Kohm set refrence -> This thermistor is set directly touching the PCB to be soldered
Thermistor thermistor2(THERMISTOR2_PIN, 9100, ThermistorZ_Placement::BOTTOM, ThermistorXY_Placement::MIDDLE_LOW_TEMP);          //- > 2125Kohm set refrence
Thermistor thermistor3(THERMISTOR3_PIN, 2040, ThermistorZ_Placement::BOTTOM, ThermistorXY_Placement::MIDDLE); // -> 9100Kohm set refrence//// 9k reference = Best accuracy around 90C -> This thermistor is used for the preheat phase if attached
Thermistor thermistor4(THERMISTOR4_PIN, 564, ThermistorZ_Placement::BOTTOM, ThermistorXY_Placement::MIDDLE_HIGH_TEMP); //-> 564Kohm set refrence // 564R reference = Best accuracy around 210C

// Unused by defualt, keep unplugged
Thermistor thermistor5(THERMISTOR5_PIN, 564, ThermistorZ_Placement::BOTTOM, ThermistorXY_Placement::MIDDLE_HIGH_TEMP);
Thermistor thermistor6(THERMISTOR6_PIN, 5727, ThermistorZ_Placement::TOP, ThermistorXY_Placement::MIDDLE_LOW_TEMP);

//---------------------------------------------------------------------------------------------------------------------------------------
// This line is here because TAKUMIII IS a bot and he will not let me remove it
Thermistor thermistors[6] = {thermistor1, thermistor2, thermistor3, thermistor4, thermistor5, thermistor6};

//Define the current sensor 20A version
//ACS712 currentSensor = ACS712(ACS712_PIN, 5.0, 1023,100);

//Define the current sensor 30A version     
ACS712 currentSensor = ACS712(ACS712_PIN, 5.0, 1023,66);        //5.0 is system voltage, 1023 is 0-1023 bits ADC steps, 66 is mV/Amp

// Which Color to use for the reflow process markers
uint16_t preheat_COLOR = 0x6800;
uint16_t soak_COLOR = 0x6b60;
uint16_t reflow_COLOR = 0x0201;
uint16_t cool_COLOR = 0x00a8;

// These are the reflow profiles that you can choose from, you can add more if you want (up to 5) but you will have to change the nReflowProfiles variable to the number of profiles you have

int nReflowProfiles = 4;

ReflowProfile reflowProfiles[] = {
    // 138c profile Sn42Bi58
    ReflowProfile(new ReflowStep[5]{
                      ReflowStep(ReflowProcessState::PREHEAT, 100, 100, EASE_OUT),
                      ReflowStep(ReflowProcessState::SOAK, 90, 140, EASE_IN_OUT),
                      ReflowStep(ReflowProcessState::REFLOW, 90, 170, HALF_SINE),
                      ReflowStep(ReflowProcessState::COOL, 50, 85, EASE_IN),
                      ReflowStep(ReflowProcessState::DONE, 0, 0)},
                  "138c Sn42Bi58\0"),

    // The profile target says to get to 100c in 30 seconds but our hotplate can not do that so we extended the time to 120 seconds and combine the 150 and 183c steps into one step
    // With a 12V PSU it also takes a while to reach these high temps so we increase the times a bit, experemintation is needed for different boards.

    // profile 183C Sn63Pb37
    ReflowProfile(new ReflowStep[5]{
                      ReflowStep(ReflowProcessState::PREHEAT, 120, 100, EASE_OUT), 
                      ReflowStep(ReflowProcessState::SOAK, 160, 183, EASE_IN), 
                      ReflowStep(ReflowProcessState::REFLOW, 110, 235, EASE_OUT), 
                      ReflowStep(ReflowProcessState::COOL, 30, 80, EASE_OUT), 
                      ReflowStep(ReflowProcessState::DONE, 0, 0)}, 
                  "183C Sn63 Pb37 \0"),

    ReflowProfile(new ReflowStep[5]{
                      ReflowStep(ReflowProcessState::PREHEAT, 120, 77, LINEAR), 
                      ReflowStep(ReflowProcessState::SOAK, 180, 135, LINEAR), 
                      ReflowStep(ReflowProcessState::REFLOW, 110, 211, LINEAR), 
                      ReflowStep(ReflowProcessState::COOL, 30, 80, EASE_OUT), 
                      ReflowStep(ReflowProcessState::DONE, 0, 0)}, 
                  "RAMP HOLD 235c\0"),

    ReflowProfile(new ReflowStep[5]{
                      ReflowStep(ReflowProcessState::PREHEAT, 200, 100, MID_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::SOAK, 150, 150, FAST_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::REFLOW, 150, 220, SLOW_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::COOL, 200, 0, LINEAR), 
                      ReflowStep(ReflowProcessState::DONE, 0, 0)}, 
                  "Tuning Profile \0"),

    ReflowProfile(new ReflowStep[5]{
                      ReflowStep(ReflowProcessState::PREHEAT, 200 /*Secounds*/, 100 /*Temperature C*/, MID_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::SOAK, 150 /*Secounds*/, 150 /*Temperature C*/, FAST_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::REFLOW, 300 /*Secounds*/, 220 /*Temperature C*/, SLOW_RAMP_HOLD), 
                      ReflowStep(ReflowProcessState::COOL, 200 /*Secounds*/, 0 /*Temperature C*/, LINEAR), 
                      ReflowStep(ReflowProcessState::DONE, 0 /*Secounds*/, 0 /*Temperature C*/)}, 
                  "Desoldering HOLD \0"),

};

//---------------------------------------------------------------------------------------------------------------------------------------

ReflowProfile chosenReflowProfile = reflowProfiles[0];

// Currently unsued -> can be used to calculate current draw of the system along with system voltage and compared to the current sensor and just be a nice sanity check for the system
uint16_t plateResistanceOhm = 0;

EEPROMDataManager eepromDataManager = EEPROMDataManager();

PidControllerData pidControllerData = {0 /*currentTemp*/, 60 /*TargetTemp*/, 255 /*PWM*/};
PidController pidController = PidController(&pidControllerData);
;


#ifndef __MAIN_H__
#define __MAIN_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

// Button port and pins
#define BUTTON_PORT GPIOA0_BASE
#define BUTTON_PINS 0x8




void SwIntHandler(void);
static void InitializeAppVariables(void);

static int ConfigureMode(int iMode);
long ConnectToNetwork();
static void ReadDeviceConfiguration();
static void OOBTask(void *pvParameters);
void TimerCycleIntHandler(void);
void SwIntHandler(void);
void TimerIntHandler(void);
static void DisplayBanner(char * AppName);
static void BoardInit(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __MAIN_H__




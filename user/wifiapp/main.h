
#ifndef __MAIN_H__
#define __MAIN_H__

#define BUTTON_PORT GPIOA0_BASE
#define BUTTON_PINS 0x8

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

static void BoardInit(void);
void SwIntHandler(void);
static void InitializeAppVariables(void);


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __MAIN_H__




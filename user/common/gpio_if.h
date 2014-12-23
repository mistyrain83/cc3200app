//*****************************************************************************
// gpio_if.h
//
// Defines and Macros for the GPIO interface.
//
//*****************************************************************************

#ifndef __GPIOIF_H__
#define __GPIOIF_H__

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

typedef enum
{
    NO_LED,
    LED_RED = 0x1, /* RED LED GP0/Pin50 */
    LED_GREEN = 0x2, /* GREEN LED GP30/Pin53 */
} ledEnum;

typedef enum
{
    NO_BUTTON,
    BUTTON_PUSH = 0x1, /* BUTTON GP3/Pin58 */
} buttonEnum;

typedef enum
{
    NO_DO,
    DO1 = 0x1, // DO1 GP7/Pin62
    DO2 = 0x2, // DO2 GP9/Pin64
    DO3 = 0x4, // DO3 GP6/Pin61
    DO4 = 0x8, // DO4 GP4/Pin59
} doEnum;

typedef enum
{
    NO_LED_IND = NO_LED,
    MCU_SENDING_DATA_IND = LED_RED,
    MCU_ASSOCIATED_IND, /* Device associated to an AP */
    MCU_IP_ALLOC_IND, /* Device acquired an IP */
    MCU_SERVER_INIT_IND, /* Device connected to remote server */
    MCU_CLIENT_CONNECTED_IND, /* Any client connects to device */
    MCU_ON_IND,              /* Device SLHost invoked successfully */
    MCU_EXECUTE_SUCCESS_IND, /* Task executed sucessfully */
    MCU_EXECUTE_FAIL_IND, /* Task execution failed */
    MCU_RED_LED_GPIO,	/* GP0 for LED RED */
    MCU_GREEN_LED_GPIO, /* GP30 for LED GREEN */
    MCU_LOOP_FOREVER_GPIO,	/* Loop Forever */
    MCU_ALL_LED_IND
} ledNames;

typedef enum
{
    NO_BUTTON_IND = NO_BUTTON,
	MCU_BUTTON_IND = BUTTON_PUSH
} buttonNames;

typedef enum
{
    MCU_DO1_IND = 0,
    MCU_DO2_IND = 1,
    MCU_DO3_IND = 2,
    MCU_DO4_IND = 3,
    MCU_ALL_DO_IND = 4
} doNames;

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void GPIO_IF_GetPortNPin(unsigned char ucPin,
                     unsigned int *puiGPIOPort,
                     unsigned char *pucGPIOPin);

extern void GPIO_IF_ConfigureNIntEnable(unsigned int uiGPIOPort,
                                  unsigned char ucGPIOPin,
                                  unsigned int uiIntType,
                                  void (*pfnIntHandler)(void));					 
extern void GPIO_IF_Set(unsigned char ucPin,
             unsigned int uiGPIOPort,
             unsigned char ucGPIOPin,
             unsigned char ucGPIOValue);

extern unsigned char GPIO_IF_Get(unsigned char ucPin,
             unsigned int uiGPIOPort,
             unsigned char ucGPIOPin);
extern void GPIO_IF_LedConfigure(unsigned char ucPins);
extern void GPIO_IF_ButtonConfigure(unsigned char ucPins);
extern void GPIO_IF_DOConfigure(unsigned char ucPins);
extern void GPIO_IF_LedOn(char ledNum);
extern void GPIO_IF_DOOn(char doNum);
extern void GPIO_IF_LedOff(char ledNum);
extern void GPIO_IF_DOOff(char doNum);
extern unsigned char GPIO_IF_LedStatus(unsigned char ucGPIONum);
extern unsigned char GPIO_IF_ButtonStatus(unsigned char ucGPIONum);
extern unsigned char GPIO_IF_DOStatus(unsigned char ucGPIONum);
extern void GPIO_IF_LedToggle(unsigned char ucLedNum);
extern void GPIO_IF_DOToggle(unsigned char ucDONum);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __GPIOIF_H__


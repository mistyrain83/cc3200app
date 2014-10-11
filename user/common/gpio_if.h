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
    LED1 = 0x1, /* RED LED GP0/Pin50 */
    LED2 = 0x2, /* GREEN LED GP30/Pin53 */
} ledEnum;

typedef enum
{
    NO_LED_IND = NO_LED,
    MCU_SENDING_DATA_IND = LED1,
    MCU_ASSOCIATED_IND, /* Device associated to an AP */
    MCU_IP_ALLOC_IND, /* Device acquired an IP */
    MCU_SERVER_INIT_IND, /* Device connected to remote server */
    MCU_CLIENT_CONNECTED_IND, /* Any client connects to device */
    MCU_ON_IND,              /* Device SLHost invoked successfully */
    MCU_EXECUTE_SUCCESS_IND, /* Task executed sucessfully */
    MCU_EXECUTE_FAIL_IND, /* Task execution failed */
    MCU_RED_LED_GPIO,	/* GP0 for LED RED */
    MCU_GREEN_LED_GPIO, /* GP30 for LED GREEN */
    MCU_ALL_LED_IND
} ledNames;

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
extern void GPIO_IF_LedOn(char ledNum);
extern void GPIO_IF_LedOff(char ledNum);
extern unsigned char GPIO_IF_LedStatus(unsigned char ucGPIONum);
extern void GPIO_IF_LedToggle(unsigned char ucLedNum);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __GPIOIF_H__


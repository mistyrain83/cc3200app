//*****************************************************************************
//
// Application Name     - cc3200app
// Application Overview - this is cc3200 main app
// 
// author  - Qixb
// date    - 2014-09-23
//
//*****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Simplelink includes
#include "simplelink.h"
#include "netcfg.h"

// Driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "utils.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "pin.h"
#include "gpio.h"
#include "timer.h"

// OS includes
#include "osi.h"

// Common interface includes
#include "gpio_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "timer_if.h"
#include "common.h"

// App Includes
//#include "device_status.h"
#include "smartconfig.h"
#include "pinmux.h"
#include "communication.h"
#include "do.h"
#include "pwmout.h"

#include "main.h"

#define APPLICATION_VERSION              "0.1.0"
#define APP_NAME                         "Wifi App"
#define OOB_TASK_PRIORITY                1
#define SPAWN_TASK_PRIORITY              9
#define OOB_STACK_SIZE                   2048
#define SH_GPIO_3                       3       /* P58 GPIO_03 - Device Mode - Button Pin */
#define AUTO_CONNECTION_TIMEOUT_COUNT   50      /* 5 Sec */
#define SL_STOP_TIMEOUT                 200

#define LED_TASK_PRIORITY              2
#define LED_STACK_SIZE                 1024

#define BUF_SIZE            14
#define BUF_RECV_SIZE            1024

#define MYIP_LEN_MAX  16
#define MYPORT_LEN_MAX  5

#define TCP_LISTEN_PORT 5001

#define MSG_SEND_LOOP_NUM 20 // 2s =  20*100ms

#define SWITCH_BOUNCE_TIME 40000  // ~50ms for UtilsDelay function parameter


typedef enum
{
  LED_OFF = 0,
  LED_ON,
  LED_BLINK
}eLEDStatus;

// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    TCP_CLIENT_FAILED = -0x7D0,
    TCP_SERVER_FAILED = TCP_CLIENT_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = TCP_SERVER_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
static const char pcDigits[] = "0123456789";
static unsigned char POST_token[] = "__SL_P_ULD";
static unsigned char GET_token_TEMP[]  = "__SL_G_UTP";
static unsigned char GET_token_ACC[]  = "__SL_G_UAC";
static unsigned char GET_token_UIC[]  = "__SL_G_UIC";
static int g_iInternetAccess = -1;

static unsigned int g_uiDeviceModeConfig = ROLE_STA; // default is STA mode
static unsigned long  g_ulStatus = 0; // SimpleLink Status
static unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
static unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

static int g_iTcpSocketID = -1;

unsigned char g_cBsdBuf[BUF_SIZE];
unsigned char g_ucBsdRecvBuf[BUF_RECV_SIZE];

unsigned char g_cServerIP[MYIP_LEN_MAX];
unsigned char g_cServerPort[MYPORT_LEN_MAX];

char g_cCurrentIP[MYIP_LEN_MAX];
unsigned int g_iIP[4];
unsigned int g_uiServerIP = 0;
unsigned short g_usServerPort = 0;

unsigned int g_iSaveFlag = FAILURE;
unsigned int g_iIPLen = 0;
unsigned int g_iPortLen = 0;
unsigned int g_iCurrentIPLen = 0;

S_DO_CMD g_sRedLed;
S_DO_CMD g_sDO[DO_CHN_NUM];
unsigned int g_iSendLoopNum = 0;

unsigned int g_iSwPressedNum = 0; // Button pressed number


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif


//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


#ifdef USE_FREERTOS
//*****************************************************************************
//
//! Application defined hook (or callback) function - the tick hook.
//! The tick interrupt can optionally call this
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationTickHook( void )
{
}

//*****************************************************************************
//
//! Application defined hook (or callback) function - assert
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    while(1)
    {

    }
}

//*****************************************************************************
//
//! Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void )
{

}

//*****************************************************************************
//
//! Application provided stack overflow hook function.
//!
//! \param  handle of the offending task
//! \param  name  of the offending task
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationStackOverflowHook( OsiTaskHandle *pxTask, signed char *pcTaskName)
{
    ( void ) pxTask;
    ( void ) pcTaskName;

    for( ;; );
}

void vApplicationMallocFailedHook()
{
    while(1)
  {
    // Infinite loop;
  }
}
#endif


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] Device Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

			// RED LED ON if Disconnect from AP
			if(GPIO_IF_LedStatus(MCU_RED_LED_GPIO) == 1)
			{
				GPIO_IF_LedOn(MCU_RED_LED_GPIO);
			}

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT] Device disconnected from the AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on application's "
                           "request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR] Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
				UART_PRINT("pEventData->reason_code = 0x%x \n\r", pEventData->reason_code);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
            // when device is in AP mode and any client connects to device cc3xxx
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
            //

            UART_PRINT("[WLAN EVENT] Station connected to device\n\r");
        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            // when client disconnects from device (AP)
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
            //
            UART_PRINT("[WLAN EVENT] Station disconnected from device\n\r");
        }
        break;

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
            //SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, SSID,
            // Token etc) will be available in 'slSmartConfigStartAsyncResponse_t'
            // - Applications can use it if required
            //
            //  slSmartConfigStartAsyncResponse_t *pEventData = NULL;
            //  pEventData = &pSlWlanEvent->EventData.smartConfigStartResponse;
            //

        }
        break;

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
        {
            // SmartConfig operation finished
            //CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, padding
            // etc) will be available in 'slSmartConfigStopAsyncResponse_t' -
            // Applications can use it if required
            //
            // slSmartConfigStopAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.smartConfigStopResponse;
            //
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

			// RED LED Off Indicate acquired IP address
			if(GPIO_IF_LedStatus(MCU_RED_LED_GPIO) == 0)
			{
				GPIO_IF_LedOff(MCU_RED_LED_GPIO);
			}

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
			g_iCurrentIPLen = sprintf(g_cCurrentIP, "%d.%d.%d.%d", 
			SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0));
 //UART_PRINT("current ip = %s \n", inet_ntoa(pNetAppEvent->EventData.ipAcquiredV4.ip))
			//memcpy(g_cCurrentIP, inet_ntoa(pNetAppEvent->EventData.ipAcquiredV4.ip), sizeof(pNetAppEvent->EventData.ipAcquiredV4.ip));

            UNUSED(pEventData);
        }
        break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Leased details(like IP-Leased,lease-time,
            // mac etc) will be available in 'SlIpLeasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpLeasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipLeased;
            //

        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Released details (like IP-address, mac
            // etc) will be available in 'SlIpReleasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpReleasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipReleased;
            //
        }
		break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, 
                                SlHttpServerResponse_t *pSlHttpServerResponse)
{
    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            unsigned char *ptr;

            ptr = pSlHttpServerResponse->ResponseData.token_value.data;
            pSlHttpServerResponse->ResponseData.token_value.len = 0;
            

           

		if (0== memcmp (pSlHttpServerEvent->EventData.httpTokenName.data, \
                                  "__SL_G_UCI", \
                                  pSlHttpServerEvent->EventData.httpTokenName.len))
              {
                  
                  // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                  memcpy (pSlHttpServerResponse->ResponseData.token_value.data, \
                          g_cCurrentIP,g_iCurrentIPLen);
                  pSlHttpServerResponse->ResponseData.token_value.len = g_iCurrentIPLen;
                     
              }
		else if (0== memcmp (pSlHttpServerEvent->EventData.httpTokenName.data, \
                                  "__SL_G_UIP", \
                                  pSlHttpServerEvent->EventData.httpTokenName.len))
              {
                  
                  // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                  //memcpy (pSlHttpServerResponse->ResponseData.token_value.data, \
                  //        g_cServerIP,strlen(g_cServerIP));
                  //pSlHttpServerResponse->ResponseData.token_value.len = strlen(g_cServerIP);
              }
		else if (0== memcmp (pSlHttpServerEvent->EventData.httpTokenName.data, \
                                  "__SL_G_UPT", \
                                  pSlHttpServerEvent->EventData.httpTokenName.len))
              {
                  
                  // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                  //memcpy (pSlHttpServerResponse->ResponseData.token_value.data, \
                  //        g_cServerPort,strlen(g_cServerPort));
                  //pSlHttpServerResponse->ResponseData.token_value.len = strlen(g_cServerPort);
              }
			  

        }
            break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            unsigned char led;
            unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_name.data;
			
			/* Post request - print post values */
              DBG_PRINT("Post request\n\r");

              if ((0 == memcmp (pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                                "__SL_P_USC", \
                pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
                (0 == memcmp (pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                                  "Apply", \
                   pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
              {
                  UART_PRINT("Catch Apply Button\n\r");
				  g_iSaveFlag = SUCCESS;
              }
              
              
              
              
              if (0 == memcmp (pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                               "__SL_P_UIP", \
                               pSlHttpServerEvent->EventData.httpPostData.token_name.len))
              {
                  memcpy (g_cServerIP,pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                          pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                  
                  g_cServerIP[pSlHttpServerEvent->EventData.httpPostData.token_value.len] = 0;
                  
				  UART_PRINT("len %d Server IP: %s\n\r", pSlHttpServerEvent->EventData.httpPostData.token_value.len, g_cServerIP);
				  sscanf (g_cServerIP,"%d.%d.%d.%d", &g_iIP[0], &g_iIP[1], &g_iIP[2], &g_iIP[3]);
				  UART_PRINT("ip = %d.%d.%d.%d\n", g_iIP[3],g_iIP[2],g_iIP[1],g_iIP[0]);
				  UART_PRINT("ip = 0x%x\n", htonl(SL_IPV4_VAL(g_iIP[3],g_iIP[2],g_iIP[1],g_iIP[0])));
				  UART_PRINT("ip3 = %s\n",g_cServerIP);
				  g_uiServerIP = htonl(SL_IPV4_VAL(g_iIP[3],g_iIP[2],g_iIP[1],g_iIP[0]));
              }
              
              if (0 == memcmp (pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                                "__SL_P_UPT", \
                               pSlHttpServerEvent->EventData.httpPostData.token_name.len))
              {
                 memcpy (g_cServerPort,pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                          pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                  
                  g_cServerPort[pSlHttpServerEvent->EventData.httpPostData.token_value.len] = 0;
                  
				  UART_PRINT("len %d Server Port: %s\n\r", pSlHttpServerEvent->EventData.httpPostData.token_value.len, g_cServerPort);
					UART_PRINT("port1 = %d\n", (atoi(g_cServerPort)));
				  UART_PRINT("port2 = %d\n", htons(atoi(g_cServerPort)));
				  g_usServerPort = atoi(g_cServerPort);
              }
              
              
			
			

            //g_ucLEDStatus = 0;
            if(memcmp(ptr, POST_token, strlen((const char *)POST_token)) == 0)
            {
                ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
                if(memcmp(ptr, "LED", 3) != 0)
                    break;
                ptr += 3;
                led = *ptr;
                ptr += 2;
                if(led == '1')
                {
                    if(memcmp(ptr, "ON", 2) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);

                    }
                    else if(memcmp(ptr, "Blink", 5) == 0)
                    {
                        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
                    }
                    else
                    {
                        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
                    }
                }
                else if(led == '2')
                {
                    
                }

            }
          
        }
            break;
        default:
            break;
    }
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    if(pDevEvent == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
    {
        UART_PRINT("Null pointer\n\r");
        LOOP_FOREVER();
    }
    //
    // This application doesn't work w/ socket - Events are not expected
    //
       switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->EventData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                               "failed to transmit all queued packets\n\n",
                               pSock->EventData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , "
                               "reason (%d) \n\n",
                               pSock->EventData.sd, pSock->EventData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", \
                        pSock->Event);
    }
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void 
InitializeAppVariables(void)
{
	int i;
    g_ulStatus = 0;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    g_uiDeviceModeConfig = ROLE_STA; // default is STA mode
	g_sRedLed.cmd = DO_CMD_DEFAULT;
	g_sRedLed.flag = FALSE;
	g_sRedLed.loopnum = 0;
	for(i = 0; i < DO_CHN_NUM; i++)
	{
		g_sDO[i].cmd = DO_CMD_OFF;
		g_sDO[i].flag = FALSE;
		g_sDO[i].loopnum = 0;
	}
}


//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//!                        
//
//****************************************************************************
static int ConfigureMode(int iMode)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}


//****************************************************************************
//
//!    \brief Connects to the Network in AP or STA Mode - If Button is
//!                                             Pressed, Force it to AP mode
//!
//! \return  0 - Success
//!          -1 - Failure
//
//****************************************************************************
long ConnectToNetwork()
{
    long lRetVal = -1;
    unsigned int uiConnectTimeoutCnt = 0;

    // staring simplelink
    lRetVal =  sl_Start(NULL,NULL,NULL);
    ASSERT_ON_ERROR( lRetVal);

    // Device is in AP Mode and Button is not Pressed
    if(ROLE_STA != lRetVal && g_uiDeviceModeConfig == ROLE_STA )
    {
        if (ROLE_AP == lRetVal)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
            #ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
            #endif
            }
        }
        // Switch to STA Mode
        lRetVal = ConfigureMode(ROLE_STA);
        ASSERT_ON_ERROR( lRetVal);
    }

    //Device is in STA Mode and Button is Pressed
    if(ROLE_AP != lRetVal && g_uiDeviceModeConfig == ROLE_AP )
    {
         // Switch to AP Mode
         lRetVal = ConfigureMode(ROLE_AP);
         ASSERT_ON_ERROR( lRetVal);

    }

    // No Mode Change Required
    if(lRetVal == ROLE_AP)
    {
        // waiting for the AP to acquire IP address from Internal DHCP Server
        // If the device is in AP mode, we need to wait for this event 
        // before doing anything 
        while(!IS_IP_ACQUIRED(g_ulStatus))
        {
        #ifndef SL_PLATFORM_MULTI_THREADED
            _SlNonOsMainLoopTask(); 
        #endif
        }
        // Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        // Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

       char cCount=0;
       
       // Blink LED 3 times to Indicate AP Mode
       for(cCount = 0;cCount<3;cCount++)
       {
           //Turn RED LED On
           GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           osi_Sleep(400);
           
           //Turn RED LED Off
           GPIO_IF_LedOff(MCU_RED_LED_GPIO);
           osi_Sleep(400);
       }

       char ssid[32];
	   unsigned short len = 32;
	   unsigned short config_opt = WLAN_AP_OPT_SSID;
	   sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len, (unsigned char* )ssid);
	   UART_PRINT("\n\r Connect to : \'%s\'\n\r\n\r", ssid);
    }
    else
    {
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

    	//waiting for the device to Auto Connect
        while(uiConnectTimeoutCnt<AUTO_CONNECTION_TIMEOUT_COUNT &&
            ((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))) 
        {
            //Turn RED LED On
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            //Turn RED LED Off
            GPIO_IF_LedOff(MCU_RED_LED_GPIO);
            osi_Sleep(50);
            
            uiConnectTimeoutCnt++;
        }
        //Couldn't connect Using Auto Profile
        if(uiConnectTimeoutCnt == AUTO_CONNECTION_TIMEOUT_COUNT)
        {
            //Blink Red LED to Indicate Connection Error
            GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            
            CLR_STATUS_BIT_ALL(g_ulStatus);

            //Connect Using Smart Config
            lRetVal = SmartConfigConnect();
            ASSERT_ON_ERROR(lRetVal);

            //Waiting for the device to Auto Connect
            while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
            {
                MAP_UtilsDelay(500);              
            }
    }
    //Turn RED LED Off
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);

    //g_iInternetAccess = ConnectionTest();

    }
    return SUCCESS;
}


//****************************************************************************
//
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void ReadDeviceConfiguration()
{
    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    unsigned char ucPinValue;
        
    // Read GPIO
    GPIO_IF_GetPortNPin(SH_GPIO_3, &uiGPIOPort, &pucGPIOPin);
    ucPinValue = GPIO_IF_Get(SH_GPIO_3, uiGPIOPort, pucGPIOPin);
        
    // If Button be pressed, Mode is AP
    if(ucPinValue == 0)
    {
        //AP Mode
        g_uiDeviceModeConfig = ROLE_AP;
    }
    else
    {
        //STA Mode
        g_uiDeviceModeConfig = ROLE_STA;
    }

}

//****************************************************************************
//
//!    \brief OOB Application Main Task - Initializes SimpleLink Driver and
//!                                              Handles HTTP Requests
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void OOBTask(void *pvParameters)
{
	int i;
    long   lRetVal = -1;
	SlSockAddrIn_t  sAddr;
	int             iAddrSize;
    int             iSockID;
    int             iStatus;
	unsigned short usLoopNum;
	long            lNonBlocking = 1;
	int             iCounter;

    //Read Device Mode Configuration
    ReadDeviceConfiguration();

    //Connect to Network
    lRetVal = ConnectToNetwork();
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

	// filling the buffer
    for (iCounter=0 ; iCounter<BUF_SIZE ; iCounter++)
    {
        g_cBsdBuf[iCounter] = (char)(iCounter);
    }

	lRetVal = BsdTcpServer(&iSockID, TCP_LISTEN_PORT);
    if(lRetVal < 0)
    {
        UART_PRINT("TCP Server create failed\n\r");
        LOOP_FOREVER();
    }
	//UART_PRINT("before accept\n\r");
    //g_iTcpSocketID = SL_EAGAIN;

	
    // waiting for an incoming TCP connection
    while( 1 )
    {
		// GREEN LED ON Indicate NO Communication with Client
		if(GPIO_IF_LedStatus(MCU_GREEN_LED_GPIO) == 1)
		{
			GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
		}
		
        // accepts a connection form a TCP client, if there is any
        // otherwise returns SL_EAGAIN
        g_iTcpSocketID = sl_Accept(iSockID, ( struct SlSockAddr_t *)&sAddr, 
                                (SlSocklen_t*)&iAddrSize);
        if( g_iTcpSocketID == SL_EAGAIN )
        {
           MAP_UtilsDelay(10000);
        }
        else if( g_iTcpSocketID < 0 )
        {
            // error
            UART_PRINT("[WARN] Accept Error!\n");
            sl_Close(g_iTcpSocketID);
            sl_Close(iSockID);
			//break;
        }
		else
		{
			// setting socket option to make the socket as non blocking
			iStatus = sl_SetSockOpt(g_iTcpSocketID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, 
									&lNonBlocking, sizeof(lNonBlocking));
			if( iStatus < 0 )
			{
				UART_PRINT("[WARN] SetSockOpt Error!\n");
				sl_Close(g_iTcpSocketID);
			}
		}

		

		while(1)
		{
			// GREEN LED OFF Indicate YES Communication with Client
			if(GPIO_IF_LedStatus(MCU_GREEN_LED_GPIO) == 0)
			{
				GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
			}
			
			iStatus = sl_Recv(g_iTcpSocketID, g_ucBsdRecvBuf, BUF_RECV_SIZE, 0);
	        if( iStatus == 0 )
	        {
	          // error
	          UART_PRINT("[WARN] Client Closed!\n");
	          sl_Close(g_iTcpSocketID);
	          //sl_Close(iSockID);
			  //g_iTcpSocketID = SL_SOC_ERROR;  // re-connect
			  break;
	        }

			for(i = 0; i < iStatus; i++)
			{
				UART_PRINT("0x%x\n\r", g_ucBsdRecvBuf[i]);
			}
			

			// parse receive buf
			if(iStatus > 0)
			{
				parseDOCmd(g_ucBsdRecvBuf, iStatus);
			}

			// send buf
			if(g_iSendLoopNum >= MSG_SEND_LOOP_NUM)
			{
				g_iSendLoopNum = 0;
				
				g_cBsdBuf[0] = MSG_TYPE_SEND;
				g_cBsdBuf[1] = MSG_VER_CONTROL4;
				g_cBsdBuf[2] = CMD_TYPE_DO;
				g_cBsdBuf[3] = DO_CHN_NUM;
				g_cBsdBuf[4] = GPIO_IF_DOStatus(MCU_DO1_IND);
				g_cBsdBuf[5] = GPIO_IF_DOStatus(MCU_DO2_IND);
				g_cBsdBuf[6] = GPIO_IF_DOStatus(MCU_DO3_IND);
				g_cBsdBuf[7] = GPIO_IF_DOStatus(MCU_DO4_IND);
				//g_cBsdBuf[8] = 0x00;
				//sTestBufLen = 6;
		        // sending packet
		        iStatus = sl_Send(g_iTcpSocketID, g_cBsdBuf, 8, 0 );
		        if( iStatus <= 0 )
		        {
					UART_PRINT("[WARN] Send ERROR!\n");
		          sl_Close(g_iTcpSocketID);
		            // error
		          
				  LOOP_FOREVER();
		        }
			}

			g_iSendLoopNum++;
			osi_Sleep(100);
		}
    }	

}

//****************************************************************************
//
//! Process the main async event
//!
//! \param  none
//!
//! \return none
//
//****************************************************************************
void TimerCycleIntHandler(void)
{
	int i = 0;
	//
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(TIMERA1_BASE);

	// Process event
	for(i = 0; i < DO_CHN_NUM; i++)
	{
		switch(g_sDO[i].cmd)
		{
			case DO_CMD_OFF:
				GPIO_IF_DOOff(i);
				g_sDO[i].cmd = DO_CMD_DEFAULT;
				break;
			case DO_CMD_ON:
				GPIO_IF_DOOn(i);
				g_sDO[i].cmd = DO_CMD_DEFAULT;
				break;
			case DO_CMD_TOGGLE:
				GPIO_IF_DOToggle(i);
				g_sDO[i].cmd = DO_CMD_DEFAULT;
				break;
			default:
				break;
			
		}
		
		if(g_sDO[i].flag == TRUE)
		{
			g_sDO[i].loopnum--;
			
			if(g_sDO[i].loopnum == 0)
			{
				g_sDO[i].flag = FALSE;
				GPIO_IF_DOToggle(i);
				g_sDO[i].cmd = DO_CMD_DEFAULT;
			}
		}
	}
    
}

//*****************************************************************************
//
//! This function is button (switch) interrupt handler, also process the
//! switch bounce. the delay timer is SWITCH_BOUNCE_TIME
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void SwIntHandler(void)
{
	static long L_prevButtonState = 0x00;    
	static long L_buttonState     = 0x00;

	if(MAP_GPIOIntStatus(BUTTON_PORT, true) & BUTTON_PINS)
	{
		MAP_GPIOIntClear(BUTTON_PORT, BUTTON_PINS);
	}
	
	L_buttonState = GPIOPinRead(BUTTON_PORT, BUTTON_PINS);
	
	if( !(L_prevButtonState & BUTTON_PINS) && (L_buttonState & BUTTON_PINS) )
	{
		g_iSwPressedNum++;
		if(g_iSwPressedNum == 1)
		{
			//
		    // Turn on the button interrupt timers
		    //
		    Timer_IF_Start(TIMERA0_BASE, TIMER_A,
		                  PERIODIC_TEST_CYCLES * PERIODIC_TEST_LOOPS);
		}
		UART_PRINT("[INFO] BUTTON pressed\n\r");
	}
	
	L_prevButtonState = L_buttonState;
	MAP_UtilsDelay(SWITCH_BOUNCE_TIME);
}

//*****************************************************************************
//
//! This function handles the times of button pressed
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void TimerIntHandler(void)
{
	//
    // Clear the timer interrupt.
    //
    Timer_IF_InterruptClear(TIMERA0_BASE);
	Timer_IF_Stop(TIMERA0_BASE, TIMER_A);
	switch(g_iSwPressedNum)
	{
	case 4:
		UART_PRINT("Button pressed 4 times\n");
		break;
	case 6:
		UART_PRINT("Button pressed 6 times\n");
		break;
	default:
		UART_PRINT("Button pressed %d times\n", g_iSwPressedNum);
		break;
	}
	g_iSwPressedNum = 0;
	UART_PRINT("Clear button pressed number\n");
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char * AppName)
{
    UART_PRINT("\n\n\n\r");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t     CC3200 %s Application       \n\r", AppName);
	UART_PRINT("\t  Application Version  %s\n\r", APPLICATION_VERSION);
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\n\n\n\r");
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
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif  //ccs
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif  //ewarm
    
#endif  //USE_TIRTOS
    
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
void main()
{
    long   lRetVal = -1;

    //
    // Board Initilization
    //
    BoardInit();
    
    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

	// Init button timer interrupt handler
	//
    // Configuring the timers
    //
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
	//
    // Setup the interrupts for the timer timeouts.
    //
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, TimerIntHandler);

	//
    // Init button interrupt handler
    // BUTTON - PIN_58 - GPIO_03
    //
    GPIO_IF_ConfigureNIntEnable(BUTTON_PORT, BUTTON_PINS, GPIO_BOTH_EDGES, SwIntHandler);

	// Init cycle timer interrupt handler
	//
    // Configuring the timers
    //
    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
	//
    // Setup the interrupts for the timer timeouts.
    //
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, TimerCycleIntHandler);
	//
    // Turn on the timers
    //
	Timer_IF_Start(TIMERA1_BASE, TIMER_A,
                  PERIODIC_TEST_CYCLES * PERIODIC_TEST_LOOPS / 50);

#if defined(P_DIMMER_BOARD)
	// Init timer pwm
	//
    // TIMERA3 (TIMER A) as GREEN of RGB light. GPIO 11 --> PWM_7
    //
    SetupTimerPWMMode(TIMERA3_BASE, TIMER_B, 
            (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM), 1);

    MAP_TimerEnable(TIMERA3_BASE,TIMER_B);
	UpdateDutyCycle(TIMERA3_BASE, TIMER_B, 0);
#endif // P_DIMMER_BOARD

    // Initialize Global Variables
    InitializeAppVariables();
    
    //
    // UART Init
    //
    #ifndef NOTERM
    InitTerm();
	#endif
    
    DisplayBanner(APP_NAME);
	
	//
	// DO Init
	//
	GPIO_IF_DOConfigure(DO1|DO2|DO3|DO4);
    GPIO_IF_DOOff(MCU_ALL_DO_IND);
	
    //
    // LED Init
    //
    GPIO_IF_LedConfigure(LED1|LED2);
    //Turn On and Off the LEDs
    GPIO_IF_LedOn(MCU_ALL_LED_IND);
	UART_PRINT("ON %d, should be 0\n", GPIO_IF_LedStatus(MCU_GREEN_LED_GPIO));
	MAP_UtilsDelay(8000000);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
	UART_PRINT("OFF %d, shoule be 1\n", GPIO_IF_LedStatus(MCU_GREEN_LED_GPIO));
    
    //
    // Simplelinkspawntask
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
		GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        LOOP_FOREVER();
    } 

    //
    // Create OOB Task
    //
    lRetVal = osi_TaskCreate(OOBTask, (signed char*)"OOBTask", \
                                OOB_STACK_SIZE, NULL, \
                                OOB_TASK_PRIORITY, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
		GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        LOOP_FOREVER();
    }
	
    //
    // Start OS Scheduler
    //
    osi_start();

    while (1)
    {

    }

}

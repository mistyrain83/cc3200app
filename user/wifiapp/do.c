//*****************************************************************************
// do.c - DO Device
//
//****************************************************************************

#include "simplelink.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "do.h"

extern S_DO_CMD g_sRedLed;
extern S_DO_CMD g_sDO[DO_CHN_NUM];

int parseDOCmd(unsigned char ucBuf[], int uiLen)
{
	int i;
	unsigned int iChn;
	unsigned short usLoopNum;
	// parse receive buf
	if(uiLen >= DO_MSG_LEN_MIN)
	{
		// Check message type and message version
		if((ucBuf[0] == MSG_TYPE_RECV) 
			&& (ucBuf[1] == MSG_VER_CONTROL4) )
		{
			for(i = 0; i < DO_CHN_NUM; i++)
			{
				// Check next message len
				if(uiLen < ((i+1)*MSG_LEN_DO + 2))
				{
					return 0;
				}
				// Check command type DO
				if(ucBuf[i*MSG_LEN_DO + 2] == CMD_TYPE_DO)
				{
					// DO channel
					iChn = ucBuf[i*MSG_LEN_DO + 3];
					UART_PRINT("Chn = %d!\n", iChn);
					if(iChn > DO_CHN_NUM)
					{
						UART_PRINT("[WARN] DO Chn Error!\n");
						return -1;
					}
					
					// Command payload
					usLoopNum = ((ucBuf[i*MSG_LEN_DO + 5] << 8) + ucBuf[i*MSG_LEN_DO + 6])/100;
					g_sDO[iChn].loopnum = usLoopNum;
					
					// Command type
					switch(ucBuf[i*MSG_LEN_DO + 4])
					{
						case DO_CMD_OFF:
							g_sDO[iChn].cmd = DO_CMD_OFF;
							if(usLoopNum > 0)
							{
								g_sDO[iChn].flag = TRUE;
							}
							UART_PRINT("Chn %d Off!\n", iChn);
							break;
						case DO_CMD_ON:
							g_sDO[iChn].cmd = DO_CMD_ON;
							if(usLoopNum > 0)
							{
								g_sDO[iChn].flag = TRUE;
							}
							UART_PRINT("Chn %d On!\n", iChn);
							break;
						case DO_CMD_TOGGLE:
							g_sDO[iChn].cmd = DO_CMD_TOGGLE;
							if(usLoopNum > 0)
							{
								g_sDO[iChn].flag = TRUE;
							}
							break;
						default:
							UART_PRINT("[WARN] DO Cmd ERROR!\n");
							break;
					}
				}
				else if(ucBuf[2] == 0x41)  // PWM
				{
					UART_PRINT("Update PWM %d\n", ((ucBuf[5] << 8) + ucBuf[6]));
					//UpdateDutyCycle(TIMERA3_BASE, TIMER_B, ((ucBuf[5] << 8) + ucBuf[6]));
				}
				else
				{
					UART_PRINT("[WARN] Not Know Device!\n");
				}
			}
		}
		else
		{
			UART_PRINT("[WARN] Received Message Type or Version error!\n");
			return -1;
		}
	}
	else
	{
		UART_PRINT("[WARN] Received Message len error!\n");
		return -1;
	}
}



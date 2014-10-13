#ifndef __DO_H__
#define __DO_H__

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

#define DO_CHN_NUM 		4	// DO channel number
#define DO_MSG_LEN_MIN 	7	// Minimum message length

// message type
#define MSG_TYPE_RECV  		0x20  // control4 -> device
#define MSG_VER_CONTROL4    0x01  // control4
#define MSG_DO_CONTROL4     0x20  // DO control4
// DO message include: Type, ChannelNo, Command, Data(Payload, 2 Bytes)
#define MSG_LEN_DO			0x05  // DO command mssage len

#define MSG_TYPE_SEND		0x10 // device -> control4

// command type
#define CMD_TYPE_DO			0x21  // digital output

// DO command
#define DO_CMD_OFF     0x00
#define DO_CMD_ON      0x01
#define DO_CMD_TOGGLE  0x02
#define DO_CMD_DEFAULT 0xFF

// DO command structure
typedef struct 
{
	unsigned char 	chn;    	// channel number
	unsigned char 	cmd;    	// command
	int           	flag;   	// whether toggle flag
	unsigned short 	loopnum; 	// delay Cycle(ms)
}S_DO_CMD;

// Function declaration
int parseDOCmd(unsigned char ucBuf[], int iLen);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __DO_H__
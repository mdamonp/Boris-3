#ifndef GALIL_COMMAND

#define GALIL_COMMAND

#include "windows.h"
//#include "dmcdrc.h"

//#define DISABLE_GALIL

// GALIL Function Status Values
// NOTE:If the returned error code is -1>code>-99
// then this is a DMC level error

#define GALIL_CMD_OK										0
#define GALIL_CMD_NOT_CONNECTED 							-100
#define GALIL_CMD_OPEN_ERROR								-101
#define GALIL_CMD_COMM_ERROR								-102
#define GALIL_CMD_NOT_READY									-103
#define GALIL_CMD_LOCK_ERROR								-104
#define GALIL_CMD_RESPONSE_TIMEOUT_ERROR					-105
#define GALIL_CMD_ERROR										-106
#define GALIL_CMD_SEND_RETRY_ERROR							-107
#define GALIL_CMD_PARSE_RETRY_ERROR							-108
#define GALIL_CMD_LOCAL_LOCK_TIMEOUT_ERROR					-109
#define GALIL_CMD_CLEAR_COMM_ERROR							-110
#define GALIL_DATA_SOCKET_ERROR								-111
#define GALIL_CMD_CLOSE_ERROR								-112
#define GALIL_CMD_TLV_ERROR									-113

// Values for cmdStat

#define GALIL_CMD_STAT_NO_ERROR								0
//#define GALIL_CMD_STAT_TMSTAT_ERROR						-1
#define GALIL_CMD_STAT_NOT_HOMED							-2
#define GALIL_CMD_STAT_ARGUMENT_OUT_OF_RANGE				-3
#define GALIL_CMD_STAT_INVALID_COMMAND						-4
#define GALIL_CMD_STAT_MACHINE_ALREADY_MOVING				-5
//#define GALIL_CMD_STAT_MACHINE_LOOPBACK_ERROR				-6
#define GALIL_CMD_STAT_INVALID_ARGUMENT_COUNT				-7
#define GALIL_CMD_STAT_MACHINE_RUNTIME_ERROR				-8
#define GALIL_CMD_STAT_MACHINE_NOT_POWERED_ERROR			-9
#define GALIL_CMD_STAT_POSITION_OUT_OF_RANGE				-10
#define GALIL_CMD_STAT_NO_RESPONSE							-100
#define GALIL_CMD_NO_CONNECTION								-200

// Galil Command Definitions

#define TRANSLATE_FIXED_DISTANCE_COMMAND					1
#define TRANSLATE_LOAD_LIMIT_COMMAND						2

// Galil State Definitions

#define GALIL_CMD_POWER_OFF									0
#define GALIL_CMD_POWER_ON_NOT_HOMED						1
#define GALIL_CMD_POWER_ON_HOMED							2
#define GALIL_CMD_POWER_ON									3
#define GALIL_CMD_AT_PARK									4
#define GALIL_CMD_MOTION_HALTED								5
#define GALIL_CMD_TRANSLATE_COMPLETE						6
#define GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP		7
#define GALIL_CMD_SETTING_POWER_STATE						101
#define GALIL_CMD_HOMING									102
#define GALIL_CMD_MOVING_TO_POSITION						103
#define GALIL_CMD_JOGGING									104
#define GALIL_CMD_TRANSLATING								105
#define GALIL_CMD_PENDANT_ACTIVE							201
#define GALIL_CMD_TRANSLATE_READY							301
#define GALIL_CMD_SYSTEM_ERROR								-1			
#define GALIL_CMD_POSITION_FOLLOWING_ERROR					-2
#define GALIL_CMD_COMM_ERROR								-3
#define GALIL_CMD_TRANSLATOR_READY_NOT_DETECTED				-4

// Miscellaneus Definitions

#define GALIL_CMD_TRUE										1
#define GALIL_CMD_FALSE										0
#define GALIL_CMD_DEFAULT_GALIL_CMD_RESPONSE_TIMEOUT		1.0
#define GALIL_CMD_RESET_GALIL_CMD_RESPONSE_TIMEOUT			10.0
#define GALIL_CMD_GALIL_CMD_RESPONSE_NO_TIMEOUT				0.0

#define GALIL_CMD_ERRORINFO_SRC_BUF_SIZE      				256     // including NULL
#define GALIL_CMD_ERRORINFO_DESC_BUF_SIZE     				1024    // including NULL

typedef struct _GALIL_CMD_ERROR_INFO
{
    int errCode;                       						// An error code describing the error.
    char function[GALIL_CMD_ERRORINFO_SRC_BUF_SIZE];		// Source of the exception.
	unsigned int line;
    char errorMsg[GALIL_CMD_ERRORINFO_DESC_BUF_SIZE];		// Textual description of the error.
} GALIL_CMD_ERROR_INFO;

// Function Prototypes

int DLLEXPORT DLLSTDCALL GALIL_CMD_Open(const USHORT controllerId);
int DLLEXPORT DLLSTDCALL GALIL_CMD_Close(void);
int DLLEXPORT DLLSTDCALL GALIL_CMD_TranslateCmd(int mode, 
												const double velocityInIPM, 
												const double distanceInInches, 
												const double loadInPounds, 
												const const double timeout, 
												int* const cmdStat);
int DLLEXPORT DLLSTDCALL GALIL_CMD_ReadMachineData(int* const state,  
									double* const aPosInInches, 
									double* const loadInPounds,
									double* const timeStamp,
									const double timeout, 
									int* const cmdStat);
int DLLEXPORT DLLSTDCALL GALIL_CMD_VersionInfo(char* const version,  
									const double timeout, 
									int* const cmdStat);
int DLLEXPORT DLLSTDCALL GALIL_CMD_SystemErrorInfo(int* const errorNum,
									int* const errorLine,
									int* const errorThread,
									const double timeout, 
									int* const cmdStat);
void DLLEXPORT DLLSTDCALL GALIL_CMD_GetLastErrorInfo(GALIL_CMD_ERROR_INFO* const errorInfo);
//int DLLEXPORT DLLSTDCALL GALIL_CMD_ReadDataRecord(DMCDATARECORDQR const dataRecord, const double timeout, int* const cmdStat);

#endif

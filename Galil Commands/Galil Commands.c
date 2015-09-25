#include "dmccom.h"
#include <Windows.h>
//#include "dataskt.h"
//#include <cvinetv.h>
//#include <userint.h>
//#include <formatio.h>
//#include <utility.h>
//#include <ansi_c.h>
#include <toolbox.h>
#include "Galil Commands.h"
#include <tcpsupp.h>
//#include "EventLog.h"

GALIL_CMD_ERROR_INFO gsLastErrorInfo;

// local defines

#define DMC_CMD_LEN						256
#define DMC_RESP_LEN					256
#define DMC_MSG_DELAY					0.025
#define DMC_LOCK_RETRY_DELAY			0.025
#define MAX_GALIL_ARG_CNT				20
#define GALIL_MAX_MSG_RETRY_CNT			3

// local function prototypes

int _GALIL_SendCmdMsg(int cmd, double* const arg, const USHORT argCnt, double* const result, const USHORT resultCnt, const double timeout, int* const cmdStat);
int _GALIL_ReadArray(char* const arrayName, double* const arg, const USHORT argCnt, const double timeout);
int _GALIL_WriteArray(char* const arrayName, double* const arg, const USHORT argCnt, const double timeout);
void _GALIL_CMD_QueueErrorInfo(const char* const function, const int line, const int errorCode, const char* const errorMsg);
void _GALIL_CMD_QueueErrorInfoVarArgs(const char* const function, const int line, const int errorCode, char* fmt, ...);
void _GALIL_Delay(const double delayTime);
void _GALIL_FilterMsg(char *input, char *output);

#define GALIL_CMD_QUEUE_ERROR_INFO(errorCode,errorMsg) _GALIL_CMD_QueueErrorInfo(__func__,__LINE__,errorCode,errorMsg)
#define GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(errorCode,fmt,...) _GALIL_CMD_QueueErrorInfoVarArgs(__func__,__LINE__,errorCode,fmt,__VA_ARGS__)

/* This is the callback function for the DataSocket */
void CVICALLBACK DSCallback (DSHandle dsHandle, int event, void *callbackData);

// localized globals

static int gsDmcHnd = 0;
static ssize_t gsGalilLockHnd = 0;
static int gsTlvLockHnd = 0;
static int *gsTlvLockPtr = NULL;
static int gsGalilCommOpenned = GALIL_CMD_FALSE;

/*
int DLLEXPORT DLLSTDCALL DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	//int stat;
	
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			if (InitCVIRTE (hinstDLL, 0, 0) == 0) 
			{
				return 0;
			}
			
			break;

		case DLL_PROCESS_DETACH:
			if (!CVIRTEHasBeenDetached ())
			{
				CloseCVIRTE ();
			}
		   
			break;
	}
	
	return 1;
}
*/

int DLLEXPORT DLLSTDCALL GALIL_CMD_Open(const USHORT controllerId)
{
	int stat = CmtNewLock (NULL, OPT_TL_PROCESS_EVENTS_WHILE_WAITING , &gsGalilLockHnd);
	
	if (stat != 0)    // stat equal zero if all went well
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to create thread lock - CmtNewLock Error = %d", stat);
		return GALIL_CMD_LOCK_ERROR;
	}

	stat = CmtNewThreadLocalVar (sizeof(int), FALSE, NULL, NULL,
								 &gsTlvLockHnd);
	
	if (stat != 0)    // stat equal zero if all went well
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_TLV_ERROR, "Unable to create thread local variable - CmtNewThreadLocalVar Error = %d", stat);
		return GALIL_CMD_TLV_ERROR;
	}

	stat = CmtGetThreadLocalVar (gsTlvLockHnd, &gsTlvLockPtr);
	
	if (stat != 0)    // stat equal zero if all went well
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_TLV_ERROR, "Unable to get thread local variable - CmtGetThreadLocalVar Error = %d", stat);
		return GALIL_CMD_TLV_ERROR;
	}

	// Open communications with the DMC Controller
	
	long int dmcStat = DMCOpen(controllerId, 0, &gsDmcHnd); 
	
	if(dmcStat != DMCNOERROR)    // dmcStat equal zero if all went well
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to perform DMCOpen for controller ID = %d - dmcStat Error = %d", controllerId, dmcStat);
		return GALIL_CMD_OPEN_ERROR;
	}
	
	gsGalilCommOpenned = GALIL_CMD_TRUE;
	return(GALIL_CMD_OK);
}

int DLLEXPORT DLLSTDCALL GALIL_CMD_Close(void)
{
	int stat = GALIL_CMD_OK;
	
	gsGalilCommOpenned = GALIL_CMD_FALSE;
	
	// Close communications with the DMC Controller
	
	if (gsDmcHnd != 0)
	{
		long int dmcStat = DMCClose(gsDmcHnd); 
	
		if (dmcStat != DMCNOERROR)    // dmcStat equal zero if all went well
		{
			GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_CLOSE_ERROR, "Unable to perform DMCClose - dmcStat Error = %d", dmcStat);
			stat = GALIL_CMD_CLOSE_ERROR;
		}
		
		gsDmcHnd = 0;
	}
	
	if (gsTlvLockHnd != 0)
	{
		stat = CmtDiscardThreadLocalVar (gsTlvLockHnd);
	
		if (stat != 0)    // stat equal zero if all went well
		{
			GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_TLV_ERROR, "Unable to discard thread local variable - CmtDiscardThreadLocalVar Error = %d", stat);
			stat = GALIL_CMD_TLV_ERROR;
		}
		
		gsTlvLockHnd = 0;
	}

	gsTlvLockPtr = NULL;
	
	if (gsGalilLockHnd != 0)
	{
		int lockStat = CmtDiscardLock (gsGalilLockHnd);
	
		if (lockStat != 0)    // stat equal zero if all went well
		{
			GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to discard thread lock - CmtDiscardLock Error = %d", stat);
			stat = GALIL_CMD_LOCK_ERROR;
		}
		
		gsGalilLockHnd = 0;
	}
	
	return(stat);
}

int DLLEXPORT DLLSTDCALL GALIL_CMD_TranslateCmd(int mode, const double velocityInIPM, const double distanceInInches, const double loadInPounds, const const double timeout, int* const cmdStat)
{
	if (!gsGalilCommOpenned)
	{
		GALIL_CMD_QUEUE_ERROR_INFO(GALIL_CMD_LOCK_ERROR, "Connection to Galil not established");
		return(GALIL_CMD_NO_CONNECTION);
	}

	int dmcCmd = 0;
	
	switch (mode)
	{
		case TRANSLATE_LOAD_LIMIT_COMMAND:
			dmcCmd = TRANSLATE_LOAD_LIMIT_COMMAND;
			break;
		
		default:
			dmcCmd = TRANSLATE_FIXED_DISTANCE_COMMAND;;
			break;
	}
	
	double dArgArray[3] = {0.0};
	dArgArray[0] = velocityInIPM;
	dArgArray[1] = distanceInInches;
	dArgArray[2] = loadInPounds;
	
	int stat = _GALIL_SendCmdMsg(dmcCmd,
							dArgArray,
							sizeof(dArgArray)/sizeof(double),
							NULL,
							0,
							timeout,
							cmdStat);

	if (stat != GALIL_CMD_OK)
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to send message - _GALIL_SendTranslateCmdMsg = %d", stat);
		return(stat);
	}

	return(GALIL_CMD_OK);
}

int DLLEXPORT DLLSTDCALL GALIL_CMD_ReadMachineData(int* const state,  
									double* const aPosInInches, 
									double* const loadInPounds,
									double* const timeStamp,
									const double timeout, 
									int* const cmdStat)
{
	if (!gsGalilCommOpenned)
	{
		GALIL_CMD_QUEUE_ERROR_INFO(GALIL_CMD_LOCK_ERROR, "Connection to Galil not established");
		return(GALIL_CMD_NO_CONNECTION);
	}
	
	double dValArray[3] = {0.0};

	int stat =  _GALIL_ReadArray("MCHDAT[]", dValArray, sizeof(dValArray)/sizeof(double), timeout);

	if (stat != GALIL_CMD_OK)
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to read GALIL array data - _GALIL_ReadArray Error = %d", stat);
		return stat;
	}
	
	*cmdStat = GALIL_CMD_OK;

	*state = (int) dValArray[0];
	*aPosInInches = dValArray[1];
	*loadInPounds = dValArray[2];
	*timeStamp = Timer();
	
	return(GALIL_CMD_OK);
}

int DLLEXPORT DLLSTDCALL GALIL_CMD_VersionInfo(char* const version,  
									const double timeout, 
									int* const cmdStat)
{
	if (!gsGalilCommOpenned)
	{
		GALIL_CMD_QUEUE_ERROR_INFO(GALIL_CMD_LOCK_ERROR, "Connection to Galil not established");
		return(GALIL_CMD_NO_CONNECTION);
	}
	
	double dValArray[4] = {0.0};

	int stat =  _GALIL_ReadArray("SOFTVER[]", dValArray, sizeof(dValArray)/sizeof(double), timeout);

	if (stat != GALIL_CMD_OK)
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to read GALIL array data - _GALIL_ReadArray Error = %d", stat);
		return stat;
	}
	
	*cmdStat = GALIL_CMD_OK;

	int ver[4] = {0};
	
	ver[0] = (int) dValArray[0];
	ver[1] = (int) dValArray[1];
	ver[2] = (int) dValArray[2];
	ver[3] = (int) dValArray[3];
	
	sprintf(version, "%d.%d.%d.%d", ver[0], ver[1], ver[2], ver[3]);
	return(GALIL_CMD_OK);
}

int DLLEXPORT DLLSTDCALL GALIL_CMD_SystemErrorInfo(int* const errorNum,
									int* const errorLine,
									int* const errorThread,
									const double timeout, 
									int* const cmdStat)
{
	if (!gsGalilCommOpenned)
	{
		GALIL_CMD_QUEUE_ERROR_INFO(GALIL_CMD_LOCK_ERROR, "Connection to Galil not established");
		return(GALIL_CMD_NO_CONNECTION);
	}
	
	double dValArray[3] = {0.0};

	int stat =  _GALIL_ReadArray("ERRINFO[]", dValArray, sizeof(dValArray)/sizeof(double), timeout);

	if (stat != GALIL_CMD_OK)
	{
		GALIL_CMD_QUEUE_ERROR_INFO_VAR_ARGS(GALIL_CMD_LOCK_ERROR, "Unable to read GALIL array data - _GALIL_ReadArray Error = %d", stat);
		return stat;
	}
	
	*cmdStat = GALIL_CMD_OK;

	*errorNum = (int) dValArray[0];
	*errorLine = (int) dValArray[1];
	*errorThread = (int) dValArray[2];
	
	return(GALIL_CMD_OK);
}

void DLLEXPORT DLLSTDCALL GALIL_CMD_GetLastErrorInfo(GALIL_CMD_ERROR_INFO* const errorInfo)
{
	memcpy(errorInfo, &gsLastErrorInfo, sizeof(GALIL_CMD_ERROR_INFO));
	return;
}

/*
int DLLEXPORT DLLSTDCALL GALIL_CMD_ReadDataRecord(DMCDATARECORDQR const dataRecord, const double timeout, int* const cmdStat)
{
	int stat = CmtGetLock (gsGalilLockHnd);
	
	if (stat != 0)
	{
		return(GALIL_CMD_LOCK_ERROR);
	}

	stat = GALIL_CMD_OK;
	double baseTime = Timer();
	
	while (*gsTlvLockPtr == TRUE)
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogError(g//EventLogHandle, "SendMsg Command Lock Timout Error");
				stat = GALIL_CMD_LOCAL_LOCK_TIMEOUT_ERROR;
				goto LOCAL_LOCK_ERROR;
			}
		}
		
		 _GALIL_Delay(DMC_LOCK_RETRY_DELAY);
	}
	
	*gsTlvLockPtr = TRUE;
	
	int retryCnt = 0;
	int retry = FALSE;
	
	do
	{
		retry = FALSE;
		long int dmcStat = DMCCommand(gsDmcHnd, "QR", (LPCHAR)&dataRecord, sizeof(DMCDATARECORDQR));

		if (dmcStat != DMCNOERROR)
		{
			//EventLogVError(g//EventLogHandle, "DMCArrayUpload Send Error %d - retry %d", dmcStat, (retryCnt+1));

			retry = TRUE;
			retryCnt++;
		
			if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
			{
				//EventLogError(g//EventLogHandle, "DMCArrayUpload Send Error max retry limit error");
				stat = GALIL_CMD_SEND_RETRY_ERROR;
				goto DONE;
			}
			
			 _GALIL_Delay(DMC_MSG_DELAY);
			continue;
		}
	}
	while (retry);
	
DONE:
	*gsTlvLockPtr = FALSE;
LOCAL_LOCK_ERROR:
	CmtReleaseLock (gsGalilLockHnd);
	 _GALIL_Delay(DMC_MSG_DELAY);
	return(stat);
}
//*/

// Local Functions

int _GALIL_SendCmdMsg(int cmd, double* const arg, const USHORT argCnt, double* const result, const USHORT resultCnt, const double timeout, int* const cmdStat)
{
	char dmcCmdMsg[DMC_CMD_LEN];
	char dmcRespMsg[DMC_RESP_LEN];
	unsigned long int ulVal;
	double dVal;
	int i;
	char *token;
	char config[DMC_CMD_LEN];
	USHORT base;
	USHORT argRemaining;
	USHORT argToSend;
	USHORT argToRead;
	int retry;
	int parseStat;
	
	int stat = GALIL_CMD_OK;
	int returnCmd = 0;
	int returnCnt = 0;

	int lockStat = CmtGetLock (gsGalilLockHnd);
	
	if (lockStat != 0)
	{
		return(GALIL_CMD_LOCK_ERROR);
	}
	
	double baseTime = Timer();
	
	while (*gsTlvLockPtr == TRUE)
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogError(g//EventLogHandle, "SendMsg Command Lock Timout Error");
				stat = GALIL_CMD_LOCAL_LOCK_TIMEOUT_ERROR;
				goto LOCAL_LOCK_ERROR;
			}
		}
		
		 _GALIL_Delay(DMC_LOCK_RETRY_DELAY);
	}
	
	*gsTlvLockPtr = TRUE;
	baseTime = Timer();
	int cmdPend = -1;
	
	do
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogVError(g//EventLogHandle, "GALIL CMDPEND Access Timout Error (loc 1) - last CMDPEND = %d", cmdPend);
				stat = GALIL_CMD_RESPONSE_TIMEOUT_ERROR;
				goto DONE;
			}
		}
		
		sprintf(dmcCmdMsg, "CMDPEND=");

		int retryCnt = 0;
	
		do
		{
			retry = FALSE;
			
			long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

			if (dmcStat != DMCNOERROR)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d", dmcCmdMsg, dmcStat);

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
					stat = GALIL_CMD_SEND_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			parseStat = sscanf(dmcRespMsg,"%lf",&dVal);

			if (parseStat != 1)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Parse Error - loc 1 (%s) %d - retry = %d ", dmcCmdMsg, stat, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Parse (%s) max retry limit error", dmcCmdMsg);
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
		
			cmdPend = (int)dVal;
		}
		while (retry);
	}
	while (cmdPend != 0);
	
	if (argCnt == 0)
	{
		sprintf(dmcCmdMsg, "CMDARG[0]=%d;CMDARG[1]=0;CMDPEND=%d", cmd, 1);

		int retryCnt = 0;
	
		do
		{
			retry = FALSE;

			long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

			if (dmcStat != DMCNOERROR)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d", dmcCmdMsg, dmcStat);

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
					stat = GALIL_CMD_SEND_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
		}
		while (retry);

	}
	else
	{
		if (argCnt <= MAX_GALIL_ARG_CNT)
		{
			sprintf(config, "%d,%d", cmd, argCnt);
	
			for (i=0; i<argCnt; i++)
			{
				sprintf(config, "%s,%.4lf", config, arg[i]);
			}

			int retryCnt = 0;
	
			do
			{
				retry = FALSE;

				long int dmcStat = DMCArrayDownload(gsDmcHnd, 
										"CMDARG[]", 
										0, 
										argCnt+1, 
										config, 
										(ULONG)strlen(config),
										&ulVal);

				if (dmcStat != DMCNOERROR)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) Send Error %d", dmcStat);

					retry = TRUE;
					retryCnt++;
			
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) max retry limit error");
						stat = GALIL_CMD_SEND_RETRY_ERROR;
						goto DONE;
					}
				
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
			}
			while (retry);

			sprintf(dmcCmdMsg, "CMDPEND=%d", 1);

			retryCnt = 0;
	
			do
			{
				retry = FALSE;

				long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

				if (dmcStat != DMCNOERROR)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d", dmcCmdMsg, dmcStat);

					retry = TRUE;
					retryCnt++;
			
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
						stat = GALIL_CMD_SEND_RETRY_ERROR;
						goto DONE;
					}
				
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
			}
			while (retry);
		}
		else
		{
			sprintf(config, "%d,%d", cmd, argCnt);
	
			for (i=0; i<MAX_GALIL_ARG_CNT; i++)
			{
				sprintf(config, "%s,%.4lf", config, arg[i]);
			}

			int retryCnt = 0;
	
			do
			{
				long int dmcStat = DMCArrayDownload(gsDmcHnd, 
										"CMDARG[]", 
										0, 
										MAX_GALIL_ARG_CNT+1, 
										config, 
										(ULONG)strlen(config),
										&ulVal);

				if (dmcStat != DMCNOERROR)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) Send Error %d", dmcStat);

					retry = TRUE;
					retryCnt++;
			
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) max retry limit error");
						stat = GALIL_CMD_SEND_RETRY_ERROR;
						goto DONE;
					}
				
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
			}
			while (retry);

			base = MAX_GALIL_ARG_CNT+1;
			argRemaining = argCnt-MAX_GALIL_ARG_CNT;
			
			while (argRemaining > 0)
			{
				argToSend = (argRemaining >= MAX_GALIL_ARG_CNT) ? MAX_GALIL_ARG_CNT : argRemaining;
				
				config[0] = 0;
				
				for (i=base; i<(argToSend+base); i++)
				{
					sprintf(config, "%s,%.4lf", config, arg[i]);
				}

				retryCnt = 0;
	
				do
				{
					retry = FALSE;

					long int dmcStat = DMCArrayDownload(gsDmcHnd, 
											"CMDARG[]", 
											base, 
											base+argToSend-1, 
											config, 
											(ULONG)strlen(config),
											&ulVal);

					if (dmcStat != DMCNOERROR)
					{
						//EventLogVError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) Send Error %d", dmcStat);

						retry = TRUE;
						retryCnt++;
			
						if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
						{
							//EventLogError(g//EventLogHandle, "DMCArrayDownload (CMDARG[]) max retry limit error");
							stat = GALIL_CMD_SEND_RETRY_ERROR;
							goto DONE;
						}
				
						 _GALIL_Delay(DMC_MSG_DELAY);
						continue;
					}
				}
				while (retry);

				if (argRemaining >= MAX_GALIL_ARG_CNT)
				{
					base += MAX_GALIL_ARG_CNT;
					argRemaining -= MAX_GALIL_ARG_CNT;
				}
				else
				{
					argRemaining = 0;
					break;
				}
			}
			
			sprintf(dmcCmdMsg, "CMDPEND=%d", 1);

			retryCnt = 0;
	
			do
			{
				retry = FALSE;
				
				long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

				if (dmcStat != DMCNOERROR)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d", dmcCmdMsg, dmcStat);

					retry = TRUE;
					retryCnt++;
			
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
						stat = GALIL_CMD_SEND_RETRY_ERROR;
						goto DONE;
					}
				
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
			}
			while (retry);
		}
	}

	baseTime = Timer();
	
	do
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogVError(g//EventLogHandle, "GALIL CMDPEND Access Timout Error (loc 2) - last CMDPEND = %d", cmdPend);
				stat = GALIL_CMD_RESPONSE_TIMEOUT_ERROR;
				goto DONE;
			}
		}
		
		 _GALIL_Delay(DMC_MSG_DELAY);
		sprintf(dmcCmdMsg, "CMDPEND=");

		int retryCnt = 0;
	
		do
		{
			retry = FALSE;
			
			long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

			if (dmcStat != DMCNOERROR)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d", dmcCmdMsg, dmcStat);

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
					stat = GALIL_CMD_SEND_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			parseStat = sscanf(dmcRespMsg,"%lf",&dVal);

			if (parseStat != 1)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Parse Error - loc 2 (%s) %d - retry = %d ", dmcCmdMsg, stat, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCCommand Parse (%s) max retry limit error", dmcCmdMsg);
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
		
			cmdPend = (int)dVal;
		}
		while (retry);
	}
	while (cmdPend == 1);
	
	if ((resultCnt+3) <= MAX_GALIL_ARG_CNT)
	{
		int retryCnt = 0;
	
		do
		{
			retry = FALSE;
			
			long int dmcStat = DMCArrayUpload(gsDmcHnd, 
									"CMDRES[]", 
									0, 
									resultCnt+3-1, 
									dmcRespMsg, 
									(ULONG)sizeof(dmcRespMsg),
									&ulVal,
									1);

			if (dmcStat != DMCNOERROR)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Send Error %d - retry %d", dmcStat, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayDownload (CMDRES[]) max retry limit error");
					stat = GALIL_CMD_SEND_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			token = strtok (dmcRespMsg, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmd id token Error - retry %d ", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmd id token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				returnCmd = atoi(token);
			}

			if (returnCmd != cmd)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid cmd id Error - expected %d - received %d - retry %d", cmd, returnCmd, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid cmd id Error max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			token = strtok (NULL, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmdStat token Error - retry %d ", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmdStat token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				*cmdStat = atoi(token);
			}

			token = strtok (NULL, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse returnCnt token Error - retry %d ", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse returnCnt token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				returnCnt = atoi(token);
			}

			if (returnCnt != resultCnt)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid resultCnt Error - expected %d - received %d - retry %d", resultCnt, returnCnt, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid resultCnt Error max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			if (resultCnt > 0)
			{
				for (i=0; i<resultCnt; i++)
				{
					token = strtok (NULL, ",");

					if (token == NULL)
					{
						//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token Error - retry %d", i, (retryCnt+1));

						retry = TRUE;
						retryCnt++;
			
						if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
						{
							//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token max retry limit error", i);
							stat = GALIL_CMD_PARSE_RETRY_ERROR;
							goto DONE;
						}
				
						 _GALIL_Delay(DMC_MSG_DELAY);
						continue;
					}
					else
					{
						result[i] = atof(token);
					}
				}
			}
		}
		while (retry);
	}
	else
	{
		base = 0;
		argRemaining = resultCnt+3;
		argToRead = (argRemaining > MAX_GALIL_ARG_CNT) ? MAX_GALIL_ARG_CNT : argRemaining;

		int retryCnt = 0;
	
		do
		{
			retry = FALSE;
			
			long int dmcStat = DMCArrayUpload(gsDmcHnd, 
									"CMDRES[]", 
									base, 
									argToRead-1, 
									dmcRespMsg, 
									(ULONG)sizeof(dmcRespMsg),
									&ulVal,
									1);

			if (dmcStat != DMCNOERROR)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Send Error %d - retry %d", dmcStat, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayDownload (CMDRES[]) max retry limit error");
					stat = GALIL_CMD_SEND_RETRY_ERROR;
					goto DONE;
				}
				
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			token = strtok (dmcRespMsg, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmd id token Error - retry %d", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
	
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmd id token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
		
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				returnCmd = atoi(token);
			}

			if (returnCmd != cmd)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid cmd id Error - expected %d - received %d - retry %d", cmd, returnCmd, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
	
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid cmd id Error max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
		
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			token = strtok (NULL, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmdStat token Error - retry %d", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
	
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse cmdStat token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
		
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				*cmdStat = atoi(token);
			}

			token = strtok (NULL, ",");

			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse returnCnt token Error - retry %d", (retryCnt+1));

				retry = TRUE;
				retryCnt++;
	
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse returnCnt token max retry limit error");
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
		
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}
			else
			{
				returnCnt = atoi(token);
			}

			if (returnCnt != resultCnt)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) invalid resultCnt Error - expected %d - received %d - retry %d", resultCnt, returnCnt, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
	
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
		
				 _GALIL_Delay(DMC_MSG_DELAY);
				continue;
			}

			argToRead -= 3;
		
			for (i=0; i<argToRead; i++)
			{
				token = strtok (NULL, ",");

				if (token == NULL)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token Error - retry %d", i, (retryCnt+1));

					retry = TRUE;
					retryCnt++;
	
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token max retry limit error", i);
						stat = GALIL_CMD_PARSE_RETRY_ERROR;
						goto DONE;
					}
		
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
				else
				{
					result[i] = atof(token);
				}
			}
		}
		while (retry);

		base += argToRead;
		argRemaining -= (argToRead+3);
		argToRead = (argRemaining > MAX_GALIL_ARG_CNT) ? MAX_GALIL_ARG_CNT : argRemaining;

		while (argRemaining > 0)
		{
			retryCnt = 0;
	
			do
			{
				retry = FALSE;

				long int dmcStat = DMCArrayUpload(gsDmcHnd, 
										"CMDRES[]", 
										base+3, 
										base+3+argToRead-1, 
										dmcRespMsg, 
										(ULONG)sizeof(dmcRespMsg),
										&ulVal,
										1);

				if (dmcStat != DMCNOERROR)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Send Error %d - retry %d", dmcStat, (retryCnt+1));

					retry = TRUE;
					retryCnt++;
			
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) max retry limit error");
						stat = GALIL_CMD_SEND_RETRY_ERROR;
						goto DONE;
					}
				
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}

				token = strtok (dmcRespMsg, ",");

				if (token == NULL)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token Error - retry %d", base, (retryCnt+1));

					retry = TRUE;
					retryCnt++;
	
					if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
					{
						//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token max retry limit error", base);
						stat = GALIL_CMD_PARSE_RETRY_ERROR;
						goto DONE;
					}
		
					 _GALIL_Delay(DMC_MSG_DELAY);
					continue;
				}
				else
				{
					result[base] = atoi(token);
				}

				if (argRemaining > 1)
				{
					for (i=base+1; i<(base+1+argToRead-1); i++)
					{
						token = strtok (NULL, ",");
	
						if (token == NULL)
						{
							//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token Error - retry %d", i, (retryCnt+1));

							retry = TRUE;
							retryCnt++;
	
							if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
							{
								//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[]) Parse result[%d] token max retry limit error", i);
								stat = GALIL_CMD_PARSE_RETRY_ERROR;
								goto DONE;
							}
		
							 _GALIL_Delay(DMC_MSG_DELAY);
							continue;
						}
						else
						{
							result[i] = atof(token);
						}
					}
				}
			
				if (argRemaining > MAX_GALIL_ARG_CNT)
				{
					base += MAX_GALIL_ARG_CNT;
					argRemaining -= MAX_GALIL_ARG_CNT;
				}
				else
				{
					argRemaining = 0;
					break;
				}
			}
			while (retry);
		}
	}

DONE:
	sprintf(dmcCmdMsg, "CMDPEND=%d", 0);

	do
	{
		retry = FALSE;
		int retryCnt = 0;

		long int dmcStat = DMCCommand(gsDmcHnd, dmcCmdMsg, dmcRespMsg, (ULONG)sizeof(dmcRespMsg)); 

		if (dmcStat != DMCNOERROR)
		{
			//EventLogVError(g//EventLogHandle, "DMCCommand Send Error (%s) %d - retry %d", dmcCmdMsg, dmcStat, (retryCnt+1));

			retry = TRUE;
			retryCnt++;
	
			if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
			{
				//EventLogVError(g//EventLogHandle, "DMCCommand Send (%s) max retry limit error", dmcCmdMsg);
				stat = GALIL_CMD_SEND_RETRY_ERROR;
				//goto DONE;
				break;
			}
		
			 _GALIL_Delay(DMC_MSG_DELAY);
			continue;
		}
	}
	while (retry);
	
	//stat = GALIL_CMD_OK;

	*gsTlvLockPtr = FALSE;
LOCAL_LOCK_ERROR:
	CmtReleaseLock (gsGalilLockHnd);
	 _GALIL_Delay(DMC_MSG_DELAY);
	return(stat);
}

int _GALIL_ReadArray(char* const arrayName, double* const arg, const USHORT argCnt, const double timeout)
{
	long dmcStat;
	char dmcRespMsg[DMC_RESP_LEN];
	char dmcFilteredMsg[DMC_RESP_LEN];
	unsigned long int ulVal;
	int i;
	char *token;
	int gotLock = 0;
	
	//int stat = CmtGetLock (gsGalilLockHnd);
	int stat = CmtGetLockEx (gsGalilLockHnd, 1, CMT_WAIT_FOREVER, &gotLock);
	
	if (stat != 0)
	{
		char errMsg[CMT_MAX_MESSAGE_BUF_SIZE] = {0};
		
		CmtGetErrorMessage(stat, errMsg);
		return(GALIL_CMD_LOCK_ERROR);
	}

	if (!gotLock)
	{
		return(GALIL_CMD_LOCK_ERROR);
	}

	stat = GALIL_CMD_OK;
	double baseTime = Timer();
	
	while (*gsTlvLockPtr == TRUE)
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogError(g//EventLogHandle, "SendMsg Command Lock Timout Error");
				stat = GALIL_CMD_LOCAL_LOCK_TIMEOUT_ERROR;
				goto LOCAL_LOCK_ERROR;
			}
		}
		
		 _GALIL_Delay(DMC_LOCK_RETRY_DELAY);
	}
	
	*gsTlvLockPtr = TRUE;
	
	int retryCnt = 0;
	int retry = FALSE;
	
	do
	{
		retry = FALSE;
		
		dmcStat = DMCArrayUpload(gsDmcHnd, 
								arrayName, 
								0, 
								argCnt-1, 
								dmcRespMsg, 
								(ULONG)sizeof(dmcRespMsg),
								&ulVal,
								1);

		if (dmcStat != DMCNOERROR)
		{
			//EventLogVError(g//EventLogHandle, "DMCArrayUpload Send Error %d - retry %d", dmcStat, (retryCnt+1));

			retry = TRUE;
			retryCnt++;
		
			if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
			{
				//EventLogError(g//EventLogHandle, "DMCArrayUpload Send Error max retry limit error");
				stat = GALIL_CMD_SEND_RETRY_ERROR;
				goto DONE;
			}
			
			 _GALIL_Delay(DMC_MSG_DELAY);
			continue;
		}
	
		_GALIL_FilterMsg(dmcRespMsg, dmcFilteredMsg);
		token = strtok (dmcFilteredMsg, ",");

		for (i=0; i<argCnt; i++)
		{
			if (token == NULL)
			{
				//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[%d]) Parse cmd id token Error - retry %d", i, (retryCnt+1));

				retry = TRUE;
				retryCnt++;
			
				if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
				{
					//EventLogVError(g//EventLogHandle, "DMCArrayUpload (CMDRES[%d]) Parse cmd id token max retry limit error", i);
					stat = GALIL_CMD_PARSE_RETRY_ERROR;
					goto DONE;
				}
			
				 _GALIL_Delay(DMC_MSG_DELAY);
				break;
			}
			else
			{
				arg[i] = atof(token);
			}

			token = strtok (NULL, ",");
		}
	}
	while (retry);
	
DONE:
	*gsTlvLockPtr = FALSE;
LOCAL_LOCK_ERROR:
	CmtReleaseLock (gsGalilLockHnd);
	 _GALIL_Delay(DMC_MSG_DELAY);
	return(stat);
}

int _GALIL_WriteArray(char* const arrayName, double* const arg, const USHORT argCnt, const double timeout)
{
	long dmcStat;
	char dmcMsg[DMC_CMD_LEN];
	unsigned long int ulVal;
	int i;
	int gotLock = 0;
	
	if (argCnt == 0)
	{
		return(GALIL_CMD_OK);
	}
	
//	int stat = CmtGetLock (gsGalilLockHnd);
	int stat = CmtGetLockEx (gsGalilLockHnd, 1, CMT_WAIT_FOREVER, &gotLock);
	
	if (stat != 0)
	{
		char errMsg[CMT_MAX_MESSAGE_BUF_SIZE] = {0};
		
		CmtGetErrorMessage(stat, errMsg);
		return(GALIL_CMD_LOCK_ERROR);
	}

	if (!gotLock)
	{
		return(GALIL_CMD_LOCK_ERROR);
	}

	stat = GALIL_CMD_OK;
	double baseTime = Timer();
	
	while (*gsTlvLockPtr == TRUE)
	{
		if (timeout > 0.0)
		{
			double deltaTime = Timer() - baseTime;
		
			if (deltaTime > timeout)
			{
				//EventLogError(g//EventLogHandle, "SendMsg Command Lock Timout Error");
				stat = GALIL_CMD_LOCAL_LOCK_TIMEOUT_ERROR;
				goto LOCAL_LOCK_ERROR;
			}
		}
		
		 _GALIL_Delay(DMC_LOCK_RETRY_DELAY);
	}
	
	*gsTlvLockPtr = TRUE;
	stat = GALIL_CMD_OK;
	
	sprintf(dmcMsg, "%.4lf", arg[0]);;
	
	if (argCnt > 1)
	{
		for (i=1; i<argCnt; i++)
		{
			sprintf(dmcMsg, "%s,%.4lf", dmcMsg, arg[i]);
		}
	}
	
	int retryCnt = 0;
	int retry = FALSE;
	
	do
	{
		retry = FALSE;
		
		dmcStat = DMCArrayDownload(gsDmcHnd, 
								arrayName, 
								0, 
								argCnt-1, 
								dmcMsg, 
								(ULONG)strlen(dmcMsg),
								&ulVal);

		if (dmcStat != DMCNOERROR)
		{
			//EventLogVError(g//EventLogHandle, "DMCArrayDownload Send Error %d - retry %d", dmcStat, (retryCnt+1));

			retry = TRUE;
			retryCnt++;
			
			if (retryCnt >= GALIL_MAX_MSG_RETRY_CNT)
			{
				//EventLogError(g//EventLogHandle, "DMCArrayDownload Send Error max retry limit error");
				stat = GALIL_CMD_SEND_RETRY_ERROR;
				goto DONE;
			}
			
			 _GALIL_Delay(DMC_MSG_DELAY);
			continue;
		}
	}
	while (retry);
	
DONE:
	*gsTlvLockPtr = FALSE;
LOCAL_LOCK_ERROR:
	CmtReleaseLock (gsGalilLockHnd);
	 _GALIL_Delay(DMC_MSG_DELAY);
	return(GALIL_CMD_OK);
}

void _GALIL_CMD_QueueErrorInfo(const char* const function, const int line, const int errorCode, const char* const errorMsg)
{
	gsLastErrorInfo.errCode = errorCode;
	gsLastErrorInfo.line = line;
	strncpy(gsLastErrorInfo.function, function, GALIL_CMD_ERRORINFO_SRC_BUF_SIZE-1);
	strncpy(gsLastErrorInfo.errorMsg, errorMsg, GALIL_CMD_ERRORINFO_DESC_BUF_SIZE-1);
}

void _GALIL_CMD_QueueErrorInfoVarArgs(const char* const function, const int line, const int errorCode, char* fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vsprintf(gsLastErrorInfo.errorMsg, fmt, ap);
	
	gsLastErrorInfo.errCode = errorCode;
	gsLastErrorInfo.line = line;
	strncpy(gsLastErrorInfo.function, function, GALIL_CMD_ERRORINFO_SRC_BUF_SIZE-1);
	va_end(ap);
}

void _GALIL_Delay(const double delayTime)
{
	double baseTime = Timer();
	double deltaTime = 0.0;
	
	while(deltaTime < delayTime)
	{
		ProcessTCPEvents ();
		Sleep(5);
		deltaTime = Timer() - baseTime;
	}
}

void _GALIL_FilterMsg(char *input, char *output)
{
	size_t inputLen = strlen(input);
	size_t index = 0;
	
	for (size_t i=0; i<inputLen; i++)
	{
		if (input[i] != ':')
		{
			output[index++] = input[i];
		}
	}
	
	output[index] = 0;
	return;
}

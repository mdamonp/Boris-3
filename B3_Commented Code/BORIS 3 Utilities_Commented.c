#include <windows.h>
#include <stdint.h>
#include <formatio.h>
#include <userint.h>
#include "BORIS 3.h"
#include "inifile.h"
#include "BORIS 3 Utilities.h"
#include "BORIS 3 Definitions.h"

extern B3_LAST_ERROR gLastError;

#define LOG_ERROR(error, msg) _LogError(error, __func__, __LINE__, msg)
#define LOG_ERROR_WITH_VARGS(error, fmt,...) _LogErrorWithVargs(error, __func__, __LINE__, fmt, __VA_ARGS__)

int CVICALLBACK CheckSumCalc(void *outputDest, char *outputString);

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//  FUNCTION NAME: Get Last Error
//	DESCRIPTION: Simply copies te last error in the
//			global last error variable to the pointer.
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
void B3_GetLastError(B3_LAST_ERROR *lastError)
{
	*lastError = gLastError;
	return;
}

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//  FUNCTION NAME: Load Ini File
//	DESCRIPTION: Load the user-editable INI file
//      into appropriate system variables according
//			to the 'tag' name of the line in ini file.
//			The order of items loaded in:
//				- 
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
int B3_LoadIni(char *path, B3_SYSTEM_INFO *system)
{
	IniText iniText;
	int funcStat = B3_OK;
	int stat = 0;

	iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory to load file");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	stat =  (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI read from file error");
		funcStat = stat;
		goto EXIT;
	}

	char section[64];
	char tag[128];

	strcpy(section, "GENERAL");
	strcpy(tag, "VERSION");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->version, sizeof(system->version));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "PROCEDURE_DIRECTORY");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->procedureDir, sizeof(system->procedureDir));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

/*
	if (!strncmp(system->version, "1.0", strlen("1.0")))
	{
		for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
		{
			sprintf(tag, "PANEL_%d_ACTIVE", panelId);

			int panelIndex = panelId-1;

			stat = Ini_GetBoolean (iniText, section, tag, &system->panelInstalled[panelIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		}
	}
//*/

	strcpy(section, "GRAPHS");
	strcpy(tag, "ENABLED");

	stat = Ini_GetBoolean (iniText, section, tag, &system->graphsEnabled);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "LOGGING");
	strcpy(tag, "DIRECTORY");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->logging.logDir, sizeof(system->logging.logDir));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "TRANSLATOR");
	strcpy(tag, "GALIL_DEVICE_ID");

	unsigned int galilDeviceId = 0;

	stat = Ini_GetUInt (iniText, section, tag, &galilDeviceId);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	system->translateInfo.galilMotionControllerDeviceId = (unsigned short int)galilDeviceId;

	strcpy(tag, "MAX_LOAD_FOR_TRANSLATOR_COMPLETE_IN_POUNDS");

	stat = Ini_GetDouble (iniText, section, tag, &system->translateInfo.maxLimitForTranslatorCompleteInPounds);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

/*
	if (!strncmp(system->version, "1.1", strlen("1.1")))
	{
		sprintf(section, "PANELS");

		for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
		{
			sprintf(tag, "PANEL_%d_ACTIVE", panelId);

			int panelIndex = panelId-1;

			stat = Ini_GetBoolean (iniText, section, tag, &system->panelInstalled[panelIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		}
	}
//*/

	sprintf(section, "PANELS");

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		sprintf(tag, "PANEL_%d_ACTIVE", panelId);

		int panelIndex = panelId-1;

		stat = Ini_GetBoolean (iniText, section, tag, &system->panelInstalled[panelIndex]);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		if (system->panelInstalled[panelIndex])
		{
			sprintf(section, "PANEL_%d", panelId);

			strcpy(tag, "ACTIVE_PROCEDURE");

			char procPath[MAX_PATHNAME_LEN];

			stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, procPath, sizeof(procPath));

			if (stat == 0)
			{
				strcpy(procPath, "");
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			if (!strcmp(procPath, ""))
			{
				strcpy(system->panel[panelIndex].activeProcedure, "");
			}
			else
			{
				strcpy(system->panel[panelIndex].activeProcedure, procPath);
				//sprintf(system->panel[panelIndex].activeProcedure, "%s%s", system->procedureDir, fileName);
			}

			for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
			{
				int stationIndex = stationId-1;

				sprintf(tag, "STATION_%d_ACTIVE", stationId);

				stat = Ini_GetBoolean (iniText, section, tag, &system->panel[panelIndex].stationInstalled[stationIndex]);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}
			}
		}
	}

	strcpy(section, "TEMPERATURE_DAQ");
	strcpy(tag, "SAMPLE_RATE_PER_CHANNEL_IN_HZ");

	stat = Ini_GetUInt64 (iniText, section, tag, &system->temp.sampleRateIPerChannelnHz);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}


	strcpy(tag, "FILTER_LEVEL");

	stat = Ini_GetDouble (iniText, section, tag, &system->temp.filterLevel);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

/*
	if (!strncmp(system->version, "1.0", strlen("1.0")))
	{
		strcpy(tag, "MIN_TEMP_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &system->temp.minTempInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "MAX_TEMP_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &system->temp.maxTempInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}
	else
	{
		system->temp.  = B3_MIN_TEMP_IN_F;
		system->temp.maxTempInF = B3_MAX_TEMP_IN_F;
	}
//*/

	strcpy(section, "TEMPERATURE_IO_MAP");
	strcpy(tag, "TEMPERATURE_DEVICE");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->temp.device, sizeof(system->temp.device));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "ROOM_TEMP_INDEX");

	stat = Ini_GetInt (iniText, section, tag, &system->temp.roomTempIndex);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		sprintf(tag, "PANEL_%d_BOX_TEMP_INDEX", panelId);

		stat = Ini_GetInt (iniText, section, tag, &system->temp.boxTempIndex[panelIndex]);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
		{
			int stationIndex = stationId - 1;

			sprintf(tag, "PANEL_%d-STATION_%d_TEMP_INDEX", panelId, stationId);

			stat = Ini_GetInt (iniText, section, tag, &system->temp.anchorTempIndex[panelIndex][stationIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		}
	}

	strcpy(section, "DIGITAL_OUT_IO_MAP");
	strcpy(tag, "STATION_ON");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.systemOnDevice, sizeof(system->control.systemOnDevice));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STATION_ALERT");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.systemAlertDevice, sizeof(system->control.systemAlertDevice));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STATION_ERROR");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.systemErrorDevice, sizeof(system->control.systemErrorDevice));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		sprintf(tag, "PANEL_%d-HEATING", panelId);

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.panelHeatingDevice[panelIndex], sizeof(system->control.panelHeatingDevice[panelIndex]));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		sprintf(tag, "PANEL_%d-COOLING", panelId);

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.panelCoolingDevice[panelIndex], sizeof(system->control.panelCoolingDevice[panelIndex]));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
		// ######################### Using Station For-Loop ################################
		// #################################################################################
		// #########################  Add Actuators Here! ##################################
		// #################################################################################
		// #################################################################################
	}

	strcpy(section, "DIGITAL_IN_IO_MAP");
	strcpy(tag, "DEVICE");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, system->control.digitalInDevice, sizeof(system->control.digitalInDevice));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
		{
			int stationIndex = stationId - 1;

			sprintf(tag, "PANEL_%d-STATION_%d_TRANSLATOR_INSTALLED_INDEX", panelId, stationId);

			stat = Ini_GetInt (iniText, section, tag,
								&system->control.translatorInPlaceIndex[panelIndex][stationIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
			// ######################### Using Station For-Loop ################################
			// #################################################################################
			// #########################  Add Switches Here! ###################################
			// #################################################################################
			// #################################################################################
		}
	}

	strcpy(section, "TEMPERATURE_CALIBRATION");

	for (int deviceId=1; deviceId<=B3_SCB_68A_COUNT; deviceId++)
	{
		int deviceIndex = deviceId - 1;

		sprintf(tag, "SCB_68A_%d_OFFSET_IN_F", deviceId);

		stat = Ini_GetDouble (iniText, section, tag,
							&system->temp.tempCalOffset[deviceIndex]);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}

int B3_SaveIni(char *path, B3_SYSTEM_INFO *system)
{
	IniText iniText;
	int funcStat = B3_OK;
	int stat = 0;

	iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory to load file");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	char section[64];
	char tag[128];

	strcpy(section, "GENERAL");
	strcpy(tag, "VERSION");

	stat = Ini_PutRawString (iniText, section, tag, system->version);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "PROCEDURE_DIRECTORY");

	stat = Ini_PutRawString (iniText, section, tag, system->procedureDir);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

/*
	if (!strncmp(system->version, "1.0", strlen("1.0")))
	{
		for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
		{
			sprintf(tag, "PANEL_%d_ACTIVE", panelId);

			int panelIndex = panelId-1;

			stat = Ini_PutBoolean (iniText, section, tag, system->panelInstalled[panelIndex]);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		}
	}
//*/

	strcpy(section, "GRAPHS");
	strcpy(tag, "ENABLED");

	stat = Ini_PutBoolean (iniText, section, tag, system->graphsEnabled);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "LOGGING");
	strcpy(tag, "DIRECTORY");

	stat = Ini_PutRawString (iniText, section, tag, system->logging.logDir);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "TRANSLATOR");
	strcpy(tag, "GALIL_DEVICE_ID");

	stat = Ini_PutInt (iniText, section, tag, system->translateInfo.galilMotionControllerDeviceId);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MAX_LOAD_FOR_TRANSLATOR_COMPLETE_IN_POUNDS");

	stat = Ini_PutDouble (iniText, section, tag, system->translateInfo.maxLimitForTranslatorCompleteInPounds);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

/*
	if (!strncmp(system->version, "1.1", strlen("1.1")))
	{
	sprintf(section, "PANELS");

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		sprintf(tag, "PANEL_%d_ACTIVE", panelId);

		int panelIndex = panelId-1;

		stat = Ini_PutBoolean (iniText, section, tag, system->panelInstalled[panelIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}
	}
//*/

	sprintf(section, "PANELS");

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		sprintf(tag, "PANEL_%d_ACTIVE", panelId);

		int panelIndex = panelId-1;

		stat = Ini_PutBoolean (iniText, section, tag, system->panelInstalled[panelIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		if (system->panelInstalled[panelIndex])
		{
			sprintf(section, "PANEL_%d", panelId);

			strcpy(tag, "ACTIVE_PROCEDURE");

			stat = Ini_PutRawString (iniText, section, tag, system->panel[panelIndex].activeProcedure);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
			{
				int stationIndex = stationId-1;

				sprintf(tag, "STATION_%d_ACTIVE", stationId);

				stat = Ini_PutBoolean (iniText, section, tag, system->panel[panelIndex].stationInstalled[stationIndex]);

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}
			}
		}
	}

	strcpy(section, "TEMPERATURE_DAQ");
	strcpy(tag, "SAMPLE_RATE_PER_CHANNEL_IN_HZ");

	stat = Ini_PutDouble (iniText, section, tag, system->temp.sampleRateIPerChannelnHz);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "FILTER_LEVEL");

	stat = Ini_PutDouble (iniText, section, tag, system->temp.filterLevel);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	if (!strncmp(system->version, "1.0", strlen("1.0")))
	{
		strcpy(tag, "MIN_TEMP_IN_F");

		stat = Ini_PutDouble (iniText, section, tag, B3_MIN_TEMP_IN_F);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "MAX_TEMP_IN_F");

		stat = Ini_PutDouble (iniText, section, tag, B3_MAX_TEMP_IN_F);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

	strcpy(section, "TEMPERATURE_IO_MAP");
	strcpy(tag, "TEMPERATURE_DEVICE");

	stat = Ini_PutRawString (iniText, section, tag, system->temp.device);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "ROOM_TEMP_INDEX");

	stat = Ini_PutInt (iniText, section, tag, system->temp.roomTempIndex);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		sprintf(tag, "PANEL_%d_BOX_TEMP_INDEX", panelId);

		stat = Ini_PutInt (iniText, section, tag, system->temp.boxTempIndex[panelIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
		{
			int stationIndex = stationId - 1;

			sprintf(tag, "PANEL_%d-STATION_%d_TEMP_INDEX", panelId, stationId);

			stat = Ini_PutInt (iniText, section, tag, system->temp.anchorTempIndex[panelIndex][stationIndex]);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		}
	}

	strcpy(section, "DIGITAL_OUT_IO_MAP");
	strcpy(tag, "STATION_ON");

	stat = Ini_PutRawString (iniText, section, tag, system->control.systemOnDevice);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STATION_ALERT");

	stat = Ini_PutRawString (iniText, section, tag, system->control.systemAlertDevice);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STATION_ERROR");

	stat = Ini_PutRawString (iniText, section, tag, system->control.systemErrorDevice);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId - 1;

		sprintf(tag, "PANEL_%d-HEATING", panelId);

		stat = Ini_PutRawString (iniText, section, tag, system->control.panelHeatingDevice[panelIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		sprintf(tag, "PANEL_%d-COOLING", panelId);

		stat = Ini_PutRawString (iniText, section, tag, system->control.panelCoolingDevice[panelIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	  // ######################### Using Station For-Loop ################################
		// #################################################################################
		// #########################  Add Actuators Here! ##################################
		// #################################################################################
		// #################################################################################
	}

	strcpy(section, "DIGITAL_IN_IO_MAP");
	strcpy(tag, "DEVICE");

	stat = Ini_PutRawString (iniText, section, tag, system->control.digitalInDevice);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (int panelId=1; panelId<=B3_MAX_PANEL_COUNT; panelId++)
	{
		int panelIndex = panelId-1;

		for (int stationId=1; stationId<=B3_MAX_STATION_COUNT; stationId++)
		{
			int stationIndex = stationId-1;

			sprintf(tag, "PANEL_%d-STATION_%d_TRANSLATOR_INSTALLED_INDEX", panelId, stationId);

			stat = Ini_PutInt (iniText, section, tag, system->control.translatorInPlaceIndex[panelIndex][stationIndex]);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}
		  // #################################################################################
			// #################################################################################
			// #########################  Add Switches Here! ###################################
			// #################################################################################
			// #################################################################################
		}
	}

	strcpy(section, "TEMPERATURE_CALIBRATION");

	for (int deviceId=1; deviceId<=B3_SCB_68A_COUNT; deviceId++)
	{
		int deviceIndex = deviceId - 1;

		sprintf(tag, "SCB_68A_%d_OFFSET_IN_F", deviceId);

		stat = Ini_PutDouble (iniText, section, tag, system->temp.tempCalOffset[deviceIndex]);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error saving [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

	stat = Ini_WriteToFile (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI write to file error");
		funcStat = stat;
		goto EXIT;
	}

/***
	stat = Ini_WriteToRegistry (iniText, 2, "SOFTWARE\\Boris 3");

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI write to registry error");
		funcStat = stat;
		goto EXIT;
	}
//**/

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}

int B3_LoadProcedure(char *path, B3_PROCEDURE_INFO *procedure)
{
	int funcStat = B3_OK;
	int stat = 0;

	IniText iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	stat = Ini_ReadFromFile (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI read from file error");
		funcStat = stat;
		goto EXIT;
	}

	char section[64];
	char tag[128];

	strcpy(section, "GENERAL");
	strcpy(tag, "VERSION");

	stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, procedure->version, sizeof(procedure->version));

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	if (!strncmp(procedure->version, "1.1", strlen("1.1")) || !strncmp(procedure->version, "1.2", strlen("1.2")))
	{
		strcpy(tag, "CHECKSUM");

		stat = Ini_GetUInt (iniText, section, tag, &procedure->checksum);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		unsigned int checkSum = 0;

		Ini_WriteGeneric (iniText, CheckSumCalc, &checkSum);

		if (checkSum != procedure->checksum)
		{
			LOG_ERROR_WITH_VARGS(stat, "Checksum error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "NAME");

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, procedure->name, sizeof(procedure->name));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "CREATED");

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, procedure->created, sizeof(procedure->created));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(section, "DISPLAY");
		strcpy(tag, "STRIPCHART_MIN_TEMP_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &procedure->stripChartMinTempInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if ((procedure->stripChartMinTempInF < B3_MIN_TEMP_IN_F) ||
			(procedure->stripChartMinTempInF > B3_MAX_TEMP_IN_F))
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(tag, "STRIPCHART_MAX_TEMP_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &procedure->stripChartMaxTempInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if ((procedure->stripChartMaxTempInF <= procedure->stripChartMinTempInF) ||
			(procedure->stripChartMaxTempInF > B3_MAX_TEMP_IN_F))
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/
		strcpy(tag, "STRIPCHART_SPAN_IN_MINUTES");

		unsigned int spanInMinutes;

		stat = Ini_GetUInt (iniText, section, tag, &spanInMinutes);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if (spanInMinutes > 480)
		{
			spanInMinutes = 480;
		}
		else if (spanInMinutes < 12)
		{
			spanInMinutes = 12;
		}
//*/

		procedure->stripChartSpanInMinutes = spanInMinutes;

		strcpy(section, "LOGGING");
		strcpy(tag, "TEMPERATURE_LOG_PERIOD_IN_SECONDS_PER_LOG_ITEM");

		stat = Ini_GetUInt (iniText, section, tag, &procedure->temperatureLogPeriodInSecPerLog);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if ((procedure->temperatureLogPeriodInSecPerLog == 0) ||
			(procedure->temperatureLogPeriodInSecPerLog > 3600))
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(section, "BOX");
		strcpy(tag, "SET_POINT_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &procedure->setPointInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if ((procedure->setPointInF < procedure->stripChartMinTempInF) ||
			(procedure->setPointInF > procedure->stripChartMaxTempInF))
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(tag, "SET_POINT_TOLERANCE_IN_F");

		stat = Ini_GetDouble (iniText, section, tag, &procedure->setPointToleranceInF);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if (procedure->setPointToleranceInF <= 0.0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(tag, "SET_POINT_ADJUSTABLE");

		stat = Ini_GetBoolean (iniText, section, tag, &procedure->setPointAdjustable);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(section, "STABILIZATION");
		strcpy(tag, "STABILIZATION_READY_DELAY_IN_SECONDS");

		stat = Ini_GetDouble (iniText, section, tag, &procedure->stabilizationReadyDelayInSec);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

/*
		if (procedure->stabilizationReadyDelayInSec < 0.0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(tag, "MAX_TIME_TO_STABILIZATION_IN_MINUTES");
		double valInMinutes = 0;

		stat = Ini_GetDouble (iniText, section, tag, &valInMinutes);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		procedure->maxTimeToStabilizationInSeconds = valInMinutes * B3_MINUTES_TO_SECONDS;

/*
		if (procedure->maxTimeToStabilizationInSeconds < 0.0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
		else if ((procedure->maxTimeToStabilizationInSeconds > 0.0) &&
			(procedure->maxTimeToStabilizationInSeconds <= procedure->stabilizationReadyDelayInSec))
		{
			LOG_ERROR_WITH_VARGS(stat, "Error in value with [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
//*/

		strcpy(section, "TRANSLATION_DETECTION");
		strcpy(tag, "DETECTION_METHOD");
		char detectMethod[64];

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, detectMethod, sizeof(detectMethod));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		if (stricmp("PEAK", detectMethod) == 0)
		{
			procedure->translationDetectionMethod = B3_TRANSLATE_DETECTION_PEAK_METHOD;
		}
		else if (stricmp("FIXED_TIME", detectMethod) == 0)
		{
			procedure->translationDetectionMethod = B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD;
		}
		else
		{
			LOG_ERROR_WITH_VARGS(B3_ARGUMENT_ERROR, "Error in value [%s] %s tag - Invalid method (%s)", section, tag, detectMethod);
			funcStat = B3_ARGUMENT_ERROR;
			goto EXIT;
		}

		switch(procedure->translationDetectionMethod)
		{
			case B3_TRANSLATE_DETECTION_PEAK_METHOD:
				strcpy(tag, "HEATING_DETECTION_THRESHOLD_IN_F");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->peakTemperatureTranslationDetectionMethodInfo.heatingDetectionThresholdInF);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "POST_PEAK_TEMP_DROP_IN_PERCENT");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetection);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "POST_PEAK_TEMP_DROP_ERROR_IN_PERCENT");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "DETECTION_DEADZONE_IN_SECONDS");

				stat = Ini_GetDouble (iniText, section, tag,
								&procedure->peakTemperatureTranslationDetectionMethodInfo.deadZoneDelayInSeconds);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "MAX_TIME_TO_TRANSLATION_READY_IN_MINUTES");

				stat = Ini_GetDouble (iniText, section, tag, &valInMinutes);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->peakTemperatureTranslationDetectionMethodInfo.maxTimeForThresholdDetectionInSeconds = valInMinutes * B3_MINUTES_TO_SECONDS;

				break;

			case B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD:
				strcpy(tag, "DELAY_IN_MINUTES");

				double delayInMinutes = 0;

				stat = Ini_GetDouble (iniText, section, tag, &delayInMinutes);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->fixedTimeTranslationDetectionMethodInfo.delayInSeconds = delayInMinutes * 60.0;

				if (!strncmp(procedure->version, "1.2", strlen("1.2")))
				{
					strcpy(tag, "MAX_DELAY_FOR_READY_ACK_IN_MINUTES");

					delayInMinutes = 0;

					stat = Ini_GetDouble (iniText, section, tag, &delayInMinutes);

					if (stat == 0)
					{
						LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
						funcStat = B3_INI_FILE_ERROR;
						goto EXIT;
					}

					if (stat < 0)
					{
						LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
						funcStat = stat;
						goto EXIT;
					}

					procedure->fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds =
						delayInMinutes * 60.0;
				}
				else
				{
					procedure->fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds = 0.0;
				}

				break;
		}

		strcpy(section, "POST_TRANSLATION_HEATING");
		strcpy(tag, "ENABLED");

		stat = Ini_GetBoolean (iniText, section, tag, &procedure->postTranslationHeatingEnabled);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		if (procedure->postTranslationHeatingEnabled)
		{
			strcpy(tag, "SET_POINT_IN_F");

			stat = Ini_GetDouble (iniText, section, tag, &procedure->postTranslationHeatingSetPointInF);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "SET_POINT_TOLERANCE_IN_F");

			stat = Ini_GetDouble (iniText, section, tag, &procedure->postTranslationHeatingSetPointToleranceInF);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "SET_POINT_TRIGGER_DELAY_IN_SECONDS");

			stat = Ini_GetDouble (iniText, section, tag, &procedure->postTranslationHeatingSetPointTriggerDelayInSec);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "POST_PEAK_TEMP_DROP_IN_PERCENT");

			stat = Ini_GetDouble (iniText, section, tag, &procedure->postTranslationHeatingPeakTemperatureDropInPercent);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "HEATING_TIME_IN_MINUTES");

			stat = Ini_GetDouble (iniText, section, tag, &valInMinutes);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			procedure->postTranslationHeatingTimeInSeconds = valInMinutes * B3_MINUTES_TO_SECONDS;
			procedure->maxCoolDownTimeInSeconds = 0.0;
		}
		else
		{
			procedure->postTranslationHeatingSetPointInF = 0.0;
			procedure->postTranslationHeatingSetPointToleranceInF = 0.0;
			procedure->postTranslationHeatingSetPointTriggerDelayInSec = 0.0;
			procedure->postTranslationHeatingPeakTemperatureDropInPercent = 0.0;
			procedure->postTranslationHeatingTimeInSeconds = 0.0;

			strcpy(section, "COOL_DOWN");
			strcpy(tag, "MAX_COOL_DOWN_TIME_IN_MINUTES");

			stat = Ini_GetDouble (iniText, section, tag, &valInMinutes);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			procedure->maxCoolDownTimeInSeconds = valInMinutes * B3_MINUTES_TO_SECONDS;
		}

		strcpy(section, "TRANSLATION");
		strcpy(tag, "METHOD");
		char method[64];

		stat = Ini_GetRawStringIntoBuffer (iniText, section, tag, method, sizeof(method));

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		if (stricmp("FIXED_DISTANCE", method) == 0)
		{
			procedure->translationMethod = B3_TRANSLATE_FIXED_LENGHT_METHOD;
		}
		else if (stricmp("LOAD_LIMIT", method) == 0)
		{
			procedure->translationMethod = B3_TRANSLATE_LOAD_LIMIT_METHOD;
		}
		else if (stricmp("MANUAL", method) == 0)
		{
			procedure->translationMethod = B3_TRANSLATE_MANUAL_METHOD;
		}
		else
		{
			LOG_ERROR_WITH_VARGS(B3_ARGUMENT_ERROR, "Error parsing [%s] %s tag - Invalid method (%s)", section, tag, method);
			funcStat = B3_ARGUMENT_ERROR;
			goto EXIT;
		}

		double velocityInIPM = 0.0;

		switch(procedure->translationMethod)
		{
			case B3_TRANSLATE_FIXED_LENGHT_METHOD:
				strcpy(tag, "VELOCITY_IN_INCHES_PER_MINUTE");

				stat = Ini_GetDouble (iniText, section, tag,
								&velocityInIPM);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->fixedLengthTranslationMethodInfo.velocityInIPS =
					velocityInIPM / B3_MINUTES_TO_SECONDS;

				strcpy(tag, "DISTANCE_IN_INCHES");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->fixedLengthTranslationMethodInfo.displacementInInches);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "MAX_LOAD_LIMIT_IN_POUNDS");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->loadLimitTranslateMethodInfo.velocityInIPS = 0.0;
				procedure->loadLimitTranslateMethodInfo.loadLimitInPounds = 0.0;
				procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches = 0.0;
				break;

			case B3_TRANSLATE_LOAD_LIMIT_METHOD:
				strcpy(tag, "VELOCITY_IN_INCHES_PER_MINUTE");

				stat = Ini_GetDouble (iniText, section, tag, &velocityInIPM);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->loadLimitTranslateMethodInfo.velocityInIPS =
								velocityInIPM / B3_MINUTES_TO_SECONDS;

				strcpy(tag, "LOAD_LIMIT_IN_POUNDS");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->loadLimitTranslateMethodInfo.loadLimitInPounds);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				strcpy(tag, "MAX_DISTANCE_IN_INCHES");

				stat = Ini_GetDouble (iniText, section, tag, &procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				procedure->fixedLengthTranslationMethodInfo.displacementInInches = 0.0;
				procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts = 0.0;
				procedure->fixedLengthTranslationMethodInfo.velocityInIPS = 0.0;
				break;

			case B3_TRANSLATE_MANUAL_METHOD:
				procedure->fixedLengthTranslationMethodInfo.displacementInInches = 0.0;
				procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts = 0.0;
				procedure->fixedLengthTranslationMethodInfo.velocityInIPS = 0.0;
				procedure->loadLimitTranslateMethodInfo.velocityInIPS = 0.0;
				procedure->loadLimitTranslateMethodInfo.loadLimitInPounds = 0.0;
				procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches = 0.0;
				break;

		}
	}
	else
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - invalid version %s", section, tag, procedure->version);
		funcStat = stat;
		goto EXIT;
	}

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}

int B3_SaveProcedure(char *path, B3_PROCEDURE_INFO *procedure)
{
	int funcStat = B3_OK;
	int stat = 0;

	IniText iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	char section[64];
	char tag[128];

	strcpy(section, "GENERAL");
	strcpy(tag, "VERSION");

	stat = Ini_PutString(iniText, section, tag, procedure->version);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "CHECKSUM");

	stat = Ini_PutUInt(iniText, section, tag, procedure->checksum);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "NAME");

	stat = Ini_PutString(iniText, section, tag, procedure->name);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "CREATED");

	stat = Ini_PutString(iniText, section, tag, procedure->created);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "DISPLAY");
	strcpy(tag, "STRIPCHART_MIN_TEMP_IN_F");

	stat = Ini_PutDouble (iniText, section, tag, procedure->stripChartMinTempInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STRIPCHART_MAX_TEMP_IN_F");

	stat = Ini_PutDouble (iniText, section, tag, procedure->stripChartMaxTempInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "STRIPCHART_SPAN_IN_MINUTES");

	stat = Ini_PutUInt (iniText, section, tag, procedure->stripChartSpanInMinutes);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "LOGGING");
	strcpy(tag, "TEMPERATURE_LOG_PERIOD_IN_SECONDS_PER_LOG_ITEM");

	stat = Ini_PutUInt (iniText, section, tag, procedure->temperatureLogPeriodInSecPerLog);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "BOX");
	strcpy(tag, "SET_POINT_IN_F");

	stat = Ini_PutDouble (iniText, section, tag, procedure->setPointInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "SET_POINT_TOLERANCE_IN_F");

	stat = Ini_PutDouble (iniText, section, tag, procedure->setPointToleranceInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "SET_POINT_ADJUSTABLE");

	stat = Ini_PutBoolean (iniText, section, tag, procedure->setPointAdjustable);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "STABILIZATION");
	strcpy(tag, "STABILIZATION_READY_DELAY_IN_SECONDS");

	stat = Ini_PutDouble (iniText, section, tag, procedure->stabilizationReadyDelayInSec);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MAX_TIME_TO_STABILIZATION_IN_MINUTES");
	double valInMinutes = procedure->maxTimeToStabilizationInSeconds / B3_MINUTES_TO_SECONDS;

	stat = Ini_PutDouble (iniText, section, tag, valInMinutes);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "TRANSLATION_DETECTION");
	strcpy(tag, "DETECTION_METHOD");

	switch(procedure->translationDetectionMethod)
	{
		case B3_TRANSLATE_DETECTION_PEAK_METHOD:
			stat = Ini_PutString(iniText, section, tag, "PEAK");
			break;

		case B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD:
			stat = Ini_PutString(iniText, section, tag, "FIXED_TIME");
			break;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	switch(procedure->translationDetectionMethod)
	{
		case B3_TRANSLATE_DETECTION_PEAK_METHOD:
			strcpy(tag, "HEATING_DETECTION_THRESHOLD_IN_F");

			stat = Ini_PutDouble (iniText, section, tag, procedure->peakTemperatureTranslationDetectionMethodInfo.heatingDetectionThresholdInF);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "POST_PEAK_TEMP_DROP_IN_PERCENT");

			stat = Ini_PutDouble (iniText, section, tag, procedure->peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetection);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "POST_PEAK_TEMP_DROP_ERROR_IN_PERCENT");

			stat = Ini_PutDouble (iniText, section, tag, procedure->peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "DETECTION_DEADZONE_IN_SECONDS");

			stat = Ini_PutDouble (iniText, section, tag,
							procedure->peakTemperatureTranslationDetectionMethodInfo.deadZoneDelayInSeconds);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "MAX_TIME_TO_TRANSLATION_READY_IN_MINUTES");
			valInMinutes = procedure->peakTemperatureTranslationDetectionMethodInfo.maxTimeForThresholdDetectionInSeconds /
						   B3_MINUTES_TO_SECONDS;

			stat = Ini_PutDouble (iniText, section, tag, valInMinutes);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			break;

		case B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD:
			double delayInMinutes = procedure->fixedTimeTranslationDetectionMethodInfo.delayInSeconds / 60.0;

			strcpy(tag, "DELAY_IN_MINUTES");

			stat = Ini_PutDouble (iniText, section, tag, delayInMinutes);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			delayInMinutes =
					procedure->fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds / 60.0;

			strcpy(tag, "MAX_DELAY_FOR_READY_ACK_IN_MINUTES");

			stat = Ini_PutDouble (iniText, section, tag, delayInMinutes);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			break;
	}

	strcpy(section, "POST_TRANSLATION_HEATING");
	strcpy(tag, "ENABLED");

	stat = Ini_PutBoolean (iniText, section, tag, procedure->postTranslationHeatingEnabled);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	if (procedure->postTranslationHeatingEnabled)
	{
		strcpy(tag, "SET_POINT_IN_F");

		stat = Ini_PutDouble (iniText, section, tag, procedure->postTranslationHeatingSetPointInF);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "SET_POINT_TOLERANCE_IN_F");

		stat = Ini_PutDouble (iniText, section, tag, procedure->postTranslationHeatingSetPointToleranceInF);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "SET_POINT_TRIGGER_DELAY_IN_SECONDS");

		stat = Ini_PutDouble (iniText, section, tag, procedure->postTranslationHeatingSetPointTriggerDelayInSec);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "POST_PEAK_TEMP_DROP_IN_PERCENT");

		stat = Ini_PutDouble (iniText, section, tag, procedure->postTranslationHeatingPeakTemperatureDropInPercent);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		strcpy(tag, "HEATING_TIME_IN_MINUTES");
		valInMinutes = procedure->postTranslationHeatingTimeInSeconds / B3_MINUTES_TO_SECONDS;

		stat = Ini_PutDouble (iniText, section, tag, valInMinutes);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}
	else
	{
		strcpy(section, "COOL_DOWN");
		strcpy(tag, "MAX_COOL_DOWN_TIME_IN_MINUTES");
		valInMinutes = procedure->maxCoolDownTimeInSeconds / B3_MINUTES_TO_SECONDS;

		stat = Ini_PutDouble (iniText, section, tag, valInMinutes);

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

	strcpy(section, "TRANSLATION");
	strcpy(tag, "METHOD");

	switch(procedure->translationMethod)
	{
		case B3_TRANSLATE_FIXED_LENGHT_METHOD:
			stat = Ini_PutString(iniText, section, tag, "FIXED_DISTANCE");
			break;

		case B3_TRANSLATE_LOAD_LIMIT_METHOD:
			stat = Ini_PutString(iniText, section, tag, "LOAD_LIMIT");
			break;

		case B3_TRANSLATE_MANUAL_METHOD:
			stat = Ini_PutString(iniText, section, tag, "MANUAL");
			break;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	double velocityInIPM = 0.0;

	switch(procedure->translationMethod)
	{
		case B3_TRANSLATE_FIXED_LENGHT_METHOD:
			strcpy(tag, "VELOCITY_IN_INCHES_PER_MINUTE");
			velocityInIPM = procedure->fixedLengthTranslationMethodInfo.velocityInIPS * B3_MINUTES_TO_SECONDS;
			stat = Ini_PutDouble (iniText, section, tag, velocityInIPM);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "DISTANCE_IN_INCHES");

			stat = Ini_PutDouble (iniText, section, tag, procedure->fixedLengthTranslationMethodInfo.displacementInInches);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "MAX_LOAD_LIMIT_IN_POUNDS");

			stat = Ini_PutDouble (iniText, section, tag, procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			procedure->loadLimitTranslateMethodInfo.velocityInIPS = 0.0;
			procedure->loadLimitTranslateMethodInfo.loadLimitInPounds = 0.0;
			procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches = 0.0;
			break;

		case B3_TRANSLATE_LOAD_LIMIT_METHOD:
			strcpy(tag, "VELOCITY_IN_INCHES_PER_MINUTE");
			velocityInIPM = procedure->loadLimitTranslateMethodInfo.velocityInIPS * B3_MINUTES_TO_SECONDS;
			stat = Ini_PutDouble (iniText, section, tag, velocityInIPM);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "LOAD_LIMIT_IN_POUNDS");

			stat = Ini_PutDouble (iniText, section, tag, procedure->loadLimitTranslateMethodInfo.loadLimitInPounds);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "MAX_DISTANCE_IN_INCHES");

			stat = Ini_PutDouble (iniText, section, tag, procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches);

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			procedure->fixedLengthTranslationMethodInfo.displacementInInches = 0.0;
			procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts = 0.0;
			procedure->fixedLengthTranslationMethodInfo.velocityInIPS = 0.0;
			break;

		case B3_TRANSLATE_MANUAL_METHOD:
			procedure->fixedLengthTranslationMethodInfo.displacementInInches = 0.0;
			procedure->fixedLengthTranslationMethodInfo.maxLoadLimitInPounts = 0.0;
			procedure->fixedLengthTranslationMethodInfo.velocityInIPS = 0.0;
			procedure->loadLimitTranslateMethodInfo.velocityInIPS = 0.0;
			procedure->loadLimitTranslateMethodInfo.loadLimitInPounds = 0.0;
			procedure->loadLimitTranslateMethodInfo.maxDisplacementInInches = 0.0;
			break;
	}

	unsigned int checkSum = 0;

	Ini_WriteGeneric (iniText, CheckSumCalc, &checkSum);

	strcpy(section, "GENERAL");
	strcpy(tag, "CHECKSUM");

	stat = Ini_PutUInt(iniText, section, tag, checkSum);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error writing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	stat = Ini_WriteToFile (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI read from file error");
		funcStat = stat;
		goto EXIT;
	}

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}

int B3SystemLog(char *path, char *logMsg)
{
	int funcStat = B3_OK;

	FILE *stream = NULL;

	stream = fopen (path, "a");

	if (stream == NULL)
	{
		LOG_ERROR_WITH_VARGS(B3_SYSTEM_ERROR, "Unable to open system log at %s", path);
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	char *dateStr = NULL;
	char *timeStr = NULL;

	dateStr = DateStr();
	timeStr = TimeStr();

	int stat = fprintf (stream, "%s %s - \"%s\"\n", dateStr, timeStr, logMsg);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

EXIT:
	if (stream != NULL)
	{
		fclose(stream);
	}

	return(funcStat);
}

int B3SystemLogWithVargs(char *path, const char* const fmt, ...)
{
	char msg[512] = {0};
	va_list ap;

	va_start(ap,fmt);
	vsnprintf(msg,sizeof(msg)-1, fmt,ap);
	va_end(ap);

	int stat = B3SystemLog(path, msg);
	return(stat);
}

int B3SystemErrorLog(char *path, B3_LAST_ERROR *lastError)
{
	char msg[1024] = {0};

	sprintf(msg, "ERROR %d\"\n\tFunction: %s\n\tLine: %d\n\tError Msg: \"%s",
			lastError->error,
			lastError->func,
			lastError->line,
			lastError->errorMsg);

	int stat = B3SystemLog(path, msg);
	return(stat);
}

int B3StationLog(char *path, char *elapsedTimeStr, double roomTempInF, double setPointInF, double boxTempInF, double anchorTempInF, int state, int subState)
{
	int funcStat = B3_OK;

	FILE *stream = NULL;

	stream = fopen (path, "a");

	if (stream == NULL)
	{
		LOG_ERROR_WITH_VARGS(B3_SYSTEM_ERROR, "Unable to open system log at %s", path);
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	char *dateStr = NULL;
	char *timeStr = NULL;

	dateStr = DateStr();
	timeStr = TimeStr();

	int stat = fprintf (stream, "%s %s %s %.1lf %.1lf %.1lf %.1lf %d %d\n",
						dateStr,
						timeStr,
						elapsedTimeStr,
						roomTempInF,
						setPointInF,
						boxTempInF,
						anchorTempInF,
						state,
						subState);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

EXIT:
	if (stream != NULL)
	{
		fclose(stream);
	}

	return(funcStat);
}

int B3TranslationLog(char *path, double loadInPounds, double displacementInInches)
{
	int funcStat = B3_OK;

	FILE *stream = NULL;

	stream = fopen (path, "a");

	if (stream == NULL)
	{
		LOG_ERROR_WITH_VARGS(B3_SYSTEM_ERROR, "Unable to open system log at %s", path);
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	char *dateStr = NULL;
	char *timeStr = NULL;

	dateStr = DateStr();
	timeStr = TimeStr();

	int stat = fprintf (stream, "%s %s - %.1lf %.4lf\n",
						dateStr,
						timeStr,
						loadInPounds,
						displacementInInches);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

EXIT:
	if (stream != NULL)
	{
		fclose(stream);
	}

	return(funcStat);
}

int B3StationSummaryLog(const char const *path, const B3_SYSTEM_INFO const *system, const int panelIndex, const int stationIndex)
{
	int funcStat = B3_OK;
	int stat = 0;
	CVIAbsoluteTime nullTime = {{0}, {0}};

	FILE *stream = NULL;

	stream = fopen (path, "a");

	if (stream == NULL)
	{
		LOG_ERROR_WITH_VARGS(B3_SYSTEM_ERROR, "Unable to open system log at %s", path);
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	stat = fprintf (stream, "Serial Number: %s\n", system->panel[panelIndex].station[stationIndex].serialNum);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Procedure Path: %s\n", system->panel[panelIndex].activeProcedure);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Procedure Name: %s\n", system->panel[panelIndex].procedure.name);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Procedure Checksum: %d\n", system->panel[panelIndex].procedure.checksum);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Panel: %d\n", panelIndex+1);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Station: %d\n", stationIndex+1);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	char timeStr[256];
	double localizedTimeInSec;
	int isValidTime = 0;

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.startTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.startTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Start: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.stabilizationCompleteTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.stabilizationCompleteTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Stabilization Complete: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.opAckOfInjectionReadyTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.opAckOfInjectionReadyTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Operator ACK of Stabilization Complete: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.peakTempTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.peakTempTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Peak Temp Detected: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Peak Temp in F: %.1lf\n", system->panel[panelIndex].station[stationIndex].summary.peakTempInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.translationReadyTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.translationReadyTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Translation Ready: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

/*
	stat = fprintf (stream, "Translation Ready Temp in F: %.1lf\n", system->panel[panelIndex].station[stationIndex].summary.translationDetectTempInF);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.opAckOfTranslationReadyTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.opAckOfTranslationReadyTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Operator ACK of Translation Ready: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}
//*/

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.translationStartTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.translationStartTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Translation Start: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.translationCompleteTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.translationCompleteTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Translation Complete: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.completionTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.completionTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Completion: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	CompareCVIAbsoluteTimes (system->panel[panelIndex].station[stationIndex].summary.opAckOfCompletionTime, nullTime, &isValidTime);

	if (isValidTime)
	{
		CVIAbsoluteTimeToCVIUILTime (system->panel[panelIndex].station[stationIndex].summary.opAckOfCompletionTime, &localizedTimeInSec);
		FormatDateTimeString (localizedTimeInSec, "%c", timeStr, (int)sizeof(timeStr)-1);
	}
	else
	{
		strcpy(timeStr, "");
	}

	stat = fprintf (stream, "Operator ACK of Completion: %s\n", timeStr);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

	stat = fprintf (stream, "Completion State: %d\n", system->panel[panelIndex].station[stationIndex].summary.completionState);

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Unable to write to system summary log at %s", path);
		funcStat = stat;
		goto EXIT;
	}

EXIT:
	if (stream != NULL)
	{
		fclose(stream);
	}

	return(funcStat);
}

// Utility functions

void _LogError(int error, char* func, int line, char* errorMsg)
{
	gLastError.error = error;
	strcpy(gLastError.errorMsg, errorMsg);
	strcpy(gLastError.func, func);
	gLastError.line = line;
	GetCurrentCVIAbsoluteTime(&gLastError.errTime);
	return;
}

void _LogErrorWithVargs(int error, char* func, int line, const char* const fmt, ...)
{
	char msg[512] = {0};
	va_list ap;

	va_start(ap,fmt);
	vsnprintf(msg,sizeof(msg)-1, fmt,ap);
	va_end(ap);

	_LogError(error, func, line, msg);
	return;
}

uInt32 B3_CalcProcedureChecksum(char *path)
{
	uInt32 checkSum = 0;
	int fileHnd = 0;
	int line = 1;

	fileHnd = OpenFile (path, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);

	if (fileHnd < 0)
	{
		LOG_ERROR_WITH_VARGS(fileHnd, "Unable to open %s for to calc check sum", path);
		goto EXIT;
	}

	int foundCheckSumLine = 0;
	int endOfFile = 0;
	char lineStr[256] = {0};
	int bytesRead = 0;

	while (!foundCheckSumLine && !endOfFile)
	{
		bytesRead = ReadLine (fileHnd, lineStr, sizeof(lineStr)-1);

		if (bytesRead > 0)
		{
			if (!strnicmp(lineStr, "CHECKSUM", strlen("CHECKSUM")))
			{
				foundCheckSumLine = 1;
			}
		}
		else if (bytesRead == -2)
		{
			endOfFile = 1;
		}
		else if (bytesRead < 0)
		{
			LOG_ERROR_WITH_VARGS(fileHnd, "Unable to read %s at line %d for to calc check sum", path, line);
			checkSum = 0;
			goto EXIT; // error
		}

		line++;
	}

	if (!foundCheckSumLine || endOfFile)
	{
		return(checkSum);
	}

	while (!endOfFile)
	{
		bytesRead = ReadLine (fileHnd, lineStr, sizeof(lineStr)-1);

		if (bytesRead > 0)
		{
			for (int i=0; i<bytesRead; i++)
			{
				checkSum += lineStr[i];
			}
		}
		else if (bytesRead == -2)
		{
			endOfFile = 1;
		}
		else if (bytesRead < 0)
		{
			LOG_ERROR_WITH_VARGS(fileHnd, "Unable to read %s at line %d for to calc check sum", path, line);
			checkSum = 0;
			goto EXIT;
		}

		line++;
	}

EXIT:
	if (fileHnd > 0)
	{
		CloseFile(fileHnd);
		fileHnd = 0;
	}

	return(checkSum);
}

int CVICALLBACK CheckSumCalc(void *outputDest, char *outputString)
{
	static int addToCheckSum = 0;
	unsigned int *checkSumPtr = (unsigned int*)outputDest;

	if (!stricmp(outputString, "[GENERAL]"))
	{
		addToCheckSum = 0;
		return(0);
	}
	else if (!strnicmp(outputString, "CHECKSUM", strlen("CHECKSUM")))
	{
		addToCheckSum = 1;
		return(0);
	}
	else
	{
		if (addToCheckSum)
		{
			size_t lineLen = strlen(outputString);

			for (size_t i=0; i<lineLen; i++)
			{
				*checkSumPtr += outputString[i];
			}

		}
	}

	return(0);
}

int B3_LoadSimTranslateData(char *path, B3_SIM_TRANSLATE_DATA *data)
{
	int funcStat = B3_OK;
	int stat = 0;

	IniText iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	stat = Ini_ReadFromFile (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI read from file error");
		funcStat = stat;
		goto EXIT;
	}

	char section[64];
	char tag[128];

	strcpy(section, "SETUP_DATA");
	strcpy(tag, "HOME_IN_INCHES");

	stat = Ini_GetDouble (iniText, section, tag, &data->homePositionInInches);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "PARK_IN_INCHES");

	stat = Ini_GetDouble (iniText, section, tag, &data->parkPositionInInches);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "TRANSLATE_START_IN_INCHES");

	stat = Ini_GetDouble (iniText, section, tag, &data->translateStartPositionInInches);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MAX_VELOCITY_IN_IPS");

	stat = Ini_GetDouble (iniText, section, tag, &data->maxVelocityInIPS);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "HOME_VELOCITY_IN_IPS");

	stat = Ini_GetDouble (iniText, section, tag, &data->homeVelocityInIPS);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "JOG_VELOCITY_IN_IPS");

	stat = Ini_GetDouble (iniText, section, tag, &data->jogVelocityInIPS);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MIN_POSITION_IN_INCHES");

	stat = Ini_GetDouble (iniText, section, tag, &data->minPositionInInches);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MAX_POSITION_IN_INCHES");

	stat = Ini_GetDouble (iniText, section, tag, &data->maxPositionInInches);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "MAX_LOAD_IN_LBS");

	stat = Ini_GetDouble (iniText, section, tag, &data->maxLoadInLBS);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "SAFE_MOVE_MAX_LOAD_LIMIT_IN_LBS");

	stat = Ini_GetDouble (iniText, section, tag, &data->safeMaxLoadInLBS);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "AUTO_POSITION_TO_TRANSLATE_READY");

	stat = Ini_GetBoolean (iniText, section, tag, &data->autoPositionToTranslateReady);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(section, "TRANSLATE_DATA");
	strcpy(tag, "DATA_COUNT");

	stat = Ini_GetUInt (iniText, section, tag, &data->dataCount);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (unsigned int index=0; index<data->dataCount; index++)
	{
		unsigned int id = index + 1;

		sprintf(tag, "DATA_%d_IN_INCHES", id);

		stat = Ini_GetDouble (iniText, section, tag, &data->translatePositionInInches[index]);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}

		sprintf(tag, "DATA_%d_IN_LBS", id);

		stat = Ini_GetDouble (iniText, section, tag, &data->translateLoadInLBS[index]);

		if (stat == 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
			funcStat = B3_INI_FILE_ERROR;
			goto EXIT;
		}

		if (stat < 0)
		{
			LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
			funcStat = stat;
			goto EXIT;
		}
	}

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}


int B3_LoadSimThermalData(char *path, B3_SIM_THERMAL_DATA *data)
{
	int funcStat = B3_OK;
	int stat = 0;

	IniText iniText = Ini_New(1);

	if (iniText == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Not enough memory");
		funcStat = B3_SYSTEM_ERROR;
		goto EXIT;
	}

	Ini_DisableSorting (iniText);

	stat = Ini_ReadFromFile (iniText, path);

	if (stat != 0)
	{
		LOG_ERROR(stat, "INI read from file error");
		funcStat = stat;
		goto EXIT;
	}

	char section[64];
	char tag[128];

	strcpy(section, "GENERAL");
	strcpy(tag, "INITIAL_ROOM_TEMP_IN_F");

	stat = Ini_GetDouble (iniText, section, tag, &data->initRoomTempInF);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "INITIAL_BOX_TEMP_IN_F");

	stat = Ini_GetDouble (iniText, section, tag, &data->initBoxTempInF);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "HEATING_RATE_PER_ANCHOR_IN_F_PER_SEC");

	stat = Ini_GetDouble (iniText, section, tag, &data->anchorHeatingRateInFPerSec);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "VORTEX_HEATING_RATE_N_F_PER_SEC");

	stat = Ini_GetDouble (iniText, section, tag, &data->vortexHeatingRateInFPerSec);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "VORTEX_COOLING_RATE_N_F_PER_SEC");

	stat = Ini_GetDouble (iniText, section, tag, &data->vortexCoolingRateInFPerSec);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	strcpy(tag, "TEMP_CHANGE_RATE_IN_F_PER_SEC2");

	stat = Ini_GetDouble (iniText, section, tag, &data->tempChangeRateInFPerSec2);

	if (stat == 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
		funcStat = B3_INI_FILE_ERROR;
		goto EXIT;
	}

	if (stat < 0)
	{
		LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
		funcStat = stat;
		goto EXIT;
	}

	for (unsigned int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		unsigned int panelId = panelIndex + 1;

		for (unsigned int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
		{
			unsigned int anchorId = stationIndex + 1;

			sprintf(section, "PANEL_%d_ANCHOR_%d_TEMPERATURE_PROFILE", panelId, anchorId);
			strcpy(tag, "PRE_RESIN_TEMP_IN_F");

			stat = Ini_GetDouble (iniText, section, tag, &data->anchorPreResinTempInF[panelIndex][stationIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			strcpy(tag, "POST_RESIN_THERMAL_DATA_COUNT");

			stat = Ini_GetUInt (iniText, section, tag, &data->anchorPostResinDataCount[panelIndex][stationIndex]);

			if (stat == 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
				funcStat = B3_INI_FILE_ERROR;
				goto EXIT;
			}

			if (stat < 0)
			{
				LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
				funcStat = stat;
				goto EXIT;
			}

			for (unsigned int dataIndex=0; dataIndex<data->anchorPostResinDataCount[panelIndex][stationIndex]; dataIndex++)
			{
				unsigned int dataId = dataIndex + 1;

				sprintf(tag, "POST_RESIN_THERMAL_DATA_%d_TIME_IN_MIN", dataId);

				stat = Ini_GetDouble (iniText, section, tag, &data->anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex]);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}

				data->anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex] *= 60.0;

				sprintf(tag, "POST_RESIN_THERMAL_DATA_%d_TEMP_IN_F", dataId);

				stat = Ini_GetDouble (iniText, section, tag, &data->anchorPostResinTempInF[panelIndex][stationIndex][dataIndex]);

				if (stat == 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag - not found", section, tag);
					funcStat = B3_INI_FILE_ERROR;
					goto EXIT;
				}

				if (stat < 0)
				{
					LOG_ERROR_WITH_VARGS(stat, "Error parsing [%s] %s tag", section, tag);
					funcStat = stat;
					goto EXIT;
				}
			}

		}
	}

EXIT:

	Ini_Dispose(iniText);
	return(funcStat);
}

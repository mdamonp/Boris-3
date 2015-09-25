// BORIS
// VERSION 3.0
// APPLIED FIBER TECHNOLOGY

//   ___________________________
//  |   SIMULATOR DEFINITIONS: |
//  | Remove these to compile  |
//  | the full Boris Program   |
//  |  If not removed, code    |
//  | will compile for use as  |
//  | a system simulator.      |
//  |__________________________|
//
#define INSTALL_THERMAL_SIMULATOR
#define INSTALL_TRANSLATOR_SIMULATOR
#define SIM_ASYNC_TIMER_PERIOD	0.1

		// External Libraries and Header files:
#include "Galil Commands.h"
#include <formatio.h>
#include "asynctmr.h"
#include "toolbox.h"
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>
#include <stdlib.h>
#include <userint.h>
#include <NIDAQmx.h>
#include "BORIS 3.h"
#include "BORIS 3 Definitions.h"
#include "BORIS 3 Utilities.h"
#include "DSUtilities.h"

// GLOBAL DEFINITIONS
#define LOG_ERROR(error, msg) _LogError(error, __func__, __LINE__, msg)
#define LOG_ERROR_WITH_VARGS(error, fmt,...) _LogErrorWithVargs(error, __func__, __LINE__, fmt, __VA_ARGS__)

#define _DAQmxErrChk(functionCall) if( DAQmxFailed(daqError=(functionCall)) ) goto EXIT; else

#define PANEL_SPACING			32
#define TOOLTIP_DELAY			500
#define INITIALIZE_STATE_ENGINE	0
#define RUN_STATE_ENGINE		1
#define SECONDS_PER_PLOT_POINT	5
#define POINTS_PER_MINUTE		60/SECONDS_PER_PLOT_POINT

#define BORIS_3_LOG_FILE		"BORIS 3.log"

#define MAX_ANALOG_IN_CHANNEL_COUNT		32
#define MAX_DIGITAL_IN_CHANNEL_COUNT	24
#define MAX_DIGITAL_OUT_CHANNEL_COUNT	24

// Global Static Variables (mostly panel handles)
static char gsAppLogPath[MAX_PATHNAME_LEN];

static int gsMainPnlHndl = 0;
static int gsMainPnlMenu = 0;

static int gsSystemPnlHndl = 0;

static int gsTranslatePnlHndl = 0;

static int gsStationPanelSelectPnlHnd = 0;

static int gsB3PanelPnlHndl[B3_MAX_PANEL_COUNT] = {0};
static int gsB3PanelPnlMenu[B3_MAX_PANEL_COUNT] = {0};

static int gsManagePanelHnd = 0;

static int gsB3GraphPnlHndl[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {{0},{0}};

static int gsAboutPnlHnd = 0;

static int gsDisclaimerPnlHnd = 0;

static int gsGalilErrorPnlHnd = 0;

static int gsChartScalePnlHnd = 0;

static int gsErrorBypassAuthorizationPnlHnd = 0;

static int gsGalilAsyncTimerId = 0;
static CmtThreadLockHandle gsGalilAsyncThreadLock = 0;

static CmtThreadLockHandle gsDAQmxDOutLock = 0;

#ifdef INSTALL_THERMAL_SIMULATOR
#define THERMAL_SIMULATOR_DEFAULT_DATA_PATH "Sim Thermal.thd"
static char gsThermalSimulatorDataPath[MAX_PATHNAME_LEN] = THERMAL_SIMULATOR_DEFAULT_DATA_PATH;
static int gsThermalSimulatorPanelHnd;
static int gsThermalSimulatorAsyncTimerId = 0;
int CVICALLBACK ThermalSimulatorAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2);
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
#define TRANSLATOR_SIMULATOR_DEFAULT_DATA_PATH "Sim Translate.trd"
static char gsTranslatorSimulatorDataPath[MAX_PATHNAME_LEN] = TRANSLATOR_SIMULATOR_DEFAULT_DATA_PATH;
static int gsTranslatorSimulatorPanelHnd;
static int gsSimJog = 0;
static double gsSimTargetPosition = 0.0;
static int gsTranslatorSimulatorAsyncTimerId = 0;
int CVICALLBACK TranslatorSimulatorAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2);
#endif

static B3_SYSTEM_INFO gsSystem;

B3_LAST_ERROR gLastError = {0};

static TaskHandle gsDAQmxTempTaskHnd=0;
static float64 *gsTemperatureData=NULL;
static uInt32 gsNumTempDAQmxChannels;

static TaskHandle gsDinTaskHnd = 0;
static int gsNumDinDAQmxChannels = 0;

static char gsLogDir[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT][MAX_PATHNAME_LEN] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)][0 ... (MAX_PATHNAME_LEN-1)] = 0};
static char gsLogPath[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT][MAX_PATHNAME_LEN] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)][0 ... (MAX_PATHNAME_LEN-1)] = 0};

static int gsAppSetupChanged = 0;

#define UI_ELEMENT_COUNT		6
#define UI_PANEL_TOP			0
#define UI_PANEL_LEFT			1
#define UI_PANEL_HEIGHT			2
#define UI_PANEL_WIDTH			3
#define UI_PANEL_TOTAL_HEIGHT	4
#define UI_PANEL_TOTAL_WIDTH	5

static int gsDefaultSystemPanelPosition[UI_ELEMENT_COUNT] = {0};
static int gsDefaultTranslatorPanelPosition[UI_ELEMENT_COUNT] = {0};
static int gsDefaultB3PanelPosition[B3_MAX_PANEL_COUNT][UI_ELEMENT_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (UI_ELEMENT_COUNT-1)] = 0};
static int gsDefaultB3GraphPosition[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT][UI_ELEMENT_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)][0 ... (UI_ELEMENT_COUNT-1)] = 0};

static char gsProductVersion[64] = {0};
static int gsLogLoaded = 0;

typedef struct deferredCallData
{
	int panel;
	int control;
} DEFERRED_CALL_DATA;

// ******** Function Declarations ***********
//
//  - - - - - - - - - - - - - - - - - - - - -
// Function: DaqMXTempDeviceReadCallback
//
// Description: Callback for the Register
//     every n samples event from the
//     gsDAQmxTempTaskHnd Task.
//       Corresponds to DoneCallback.
//     See [L739] in Version 1.2
//  - - - - - - - - - - - - - - - - - - - - -
int32 CVICALLBACK DaqMXTempDeviceReadCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);

//  - - - - - - - - - - - - - - - - - - - - -
// Function: DoneCallback
//
// Description: Callback when finished with
//     gsDAQmxTempTaskHnd.
//     See [L758]&[L2013] in Version 1.2
//  - - - - - - - - - - - - - - - - - - - - -
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Function: TempControlStateEngine
//
// Description: A few nested Switch
//	 statements with all temperature-based
//   control functions included. The function
//	 is called when the "DaqMXTempDeviceReadCallback"
//	 ocurrs based on its initial setup in the
//	 main function. Cases include:
//
//		-	switch(operation)
//			- INITIALIZE_STATE_ENGINE
//			- RUN_STATE_ENGINE
//
//			- switch(gsSystem.panel[panelIndex].station[stationIndex].state)
//				- B3_STATION_STATE_OFF
//				- B3_STATION_STABILIZATION_READY_DELAY_ERROR
//				- B3_STATION_TRANSLATION_READY_DELAY_ERROR
//				- B3_STATION_TRANSLATION_START_DELAY_ERROR
//				- B3_STATION_COOLDOWN_READY_DELAY_ERROR
//				- B3_STATION_STATE_OFF
//				- B3_STATION_STATE_STABILIZING
//				- B3_STATION_STATE_STABILIZING_EACG
//				- B3_STATION_STATE_READY_FOR_RESIN
//				- B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY
//				- B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG
//
//			- switch(gsSystem.panel[panelIndex].procedure.translationDetectionMethod)
//				- B3_TRANSLATE_DETECTION_PEAK_METHOD
//				- switch(translationDetectionState[panelIndex][stationIndex])
//					- B3_TRANS_DETECT_IDLE
//					- B3_PEAK_DETECTION
//					- B3_TRANS_WAITING_FOR_COMPLETION
//					- B3_TRANS_COMPLETE
//				- B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD
//			- B3_STATION_STATE_READY_FOR_TRANSLATION
//			- B3_STATION_STATE_READY_FOR_TRANSLATION_EACG
//			- B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
//			- B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION_EACG
//			- B3_STATION_STATE_TRANSLATING
//			- B3_STATION_STATE_TRANSLATING_COMPLETE
//			- B3_STATION_STATE_WAITING_FOR_HEATING_READY
//			- B3_STATION_STATE_READY_FOR_HEATING
//			- B3_STATION_STATE_HEATING
//			- B3_STATION_STATE_COOL_DOWN
//			- B3_STATION_STATE_COOL_DOWN_EACG
//			- B3_STATION_STATE_COMPLETE
//		- default
//
//		inputs: operation, panelIndex, stationIndex
//		returns : 0
//
//     See [L2512] in Version 1.2
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int32 TempControlStateEngine(int operation, int panelIndex, int stationIndex);

//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Function: TempControlStateEngine
//
// Description:
//
//		inputs: operation, panelIndex, state
//		returns : daqError
//		Called From:
//				- Main(All Vortex Off)
//				- Main(All lights off)
//				- Main Exit:(All Vortex Off)
//				- Main Exit:(All Lights Off)
//				- B3StationControl(Lights for Running, off, etc.)
//				- TempControl State Engine (Lights for Error)
//				- GalilAsyncCallback (Lights for Error)
//
//     See [L2512] in Version 1.2
//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int32 B3SystemControl(int operation, int panelIndex, int state);
int CVICALLBACK GalilAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2);
void PanelAutoArrange(void);
void PanelStatusUpdate(int panelIndex);
void BuildRunTimeStr(int panelIndex, int stationIndex, char* timeStr);
int32 CVICALLBACK DinChangeDetectionCallback(TaskHandle taskHandle, int32 signalID, void *callbackData);
void  DeferredCallToB3StationControl(void *data);

int main (int argc, char *argv[])
{
	int stat = 0;

	// check to see if this app is already running

	int running = 0;

	CheckForDuplicateAppInstance (ACTIVATE_OTHER_INSTANCE, &running);

	if (running)
	{
		return(0);
	}

	if (argc > 1)
	{
		for (int i=1; i<argc; i++)
		{
#ifdef INSTALL_THERMAL_SIMULATOR
			if (stricmp(argv[i], "-selectThermalSimData") == 0)
			{
				int ans = 0;
				char iniDataPath[MAX_PATHNAME_LEN] = {0};

				ans = FileSelectPopupEx ("", "*.thd", "", "Select Simulator Thermal data file",
										 VAL_SELECT_BUTTON, B3_FALSE, B3_TRUE, iniDataPath);

				if (ans == 1)
				{
					strcpy(gsThermalSimulatorDataPath, iniDataPath);
				}
				else
				{
					strcpy(gsThermalSimulatorDataPath, THERMAL_SIMULATOR_DEFAULT_DATA_PATH);
				}
			}
#endif
#ifdef INSTALL_TRANSLATOR_SIMULATOR
			if (stricmp(argv[i], "-selectTranslatorSimData") == 0)
			{
				int ans = 0;
				char iniDataPath[MAX_PATHNAME_LEN] = {0};

				ans = FileSelectPopupEx ("", "*.trd", "", "Select Simulator Translator data file",
										 VAL_SELECT_BUTTON, B3_FALSE, B3_TRUE, iniDataPath);

				if (ans == 1)
				{
					strcpy(gsTranslatorSimulatorDataPath, iniDataPath);
				}
				else
				{
					strcpy(gsTranslatorSimulatorDataPath, TRANSLATOR_SIMULATOR_DEFAULT_DATA_PATH);
				}
			}
#endif
		}
	}
	// initialize CVIRTE

	if (InitCVIRTE (0, argv, 0) == 0)
	{
		LOG_ERROR(B3_SYSTEM_ERROR, "Unable initialize CVIRTE - exiting app");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable initialize CVIRTE - exiting app");
		return -1;	/* out of memory */
	}

	memset(&gsSystem, 0, sizeof(gsSystem));

	// load the app ini file - this includes information on what other child panels to load

	stat = B3_LoadIni("BORIS 3.ini", &gsSystem);

	if (stat != B3_OK)
	{
		gsLogLoaded = 0;
		B3_GetLastError(&gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load ini file BORIS 3.ini - exiting app");
		goto EXIT;
	}

	gsLogLoaded = 1;

	// log app is starting

	sprintf(gsAppLogPath, "%s%s", gsSystem.logging.logDir, BORIS_3_LOG_FILE);

	stat = B3SystemLog(gsAppLogPath, "Starting BORIS 3");

	if (stat != B3_OK)
	{
		B3_GetLastError(&gLastError);
		char errorMsg[1024];
		sprintf(errorMsg, "Error %d %s - exiting app", gLastError.error, gLastError.errorMsg);
		Beep();
		MessagePopup ("FATAL ERROR", errorMsg);
		return -1;	/* out of memory */
	}

	char version[64] = {0};

	DS_UTIL_GetVersionInfo (argv[0], version, gsProductVersion);
	B3SystemLogWithVargs(gsAppLogPath, "Product Version: %s", gsProductVersion);

	DS_UTIL_ProductVersionInfo(version);
	B3SystemLogWithVargs(gsAppLogPath, "DSUtilities.dll Product Version: %s", version);

#ifdef INSTALL_THERMAL_SIMULATOR
	B3SystemLogWithVargs(gsAppLogPath, "Thermal Simulator activated using <%s>", gsThermalSimulatorDataPath);
#endif
#ifdef INSTALL_TRANSLATOR_SIMULATOR
	B3SystemLogWithVargs(gsAppLogPath, "Translator Simulator activated using <%s>", gsTranslatorSimulatorDataPath);
#endif

	// load the MAIN panel

	if ((gsMainPnlHndl = LoadPanel (0, "BORIS 3.uir", MAIN)) < 0)
	{
		LOG_ERROR(gsMainPnlHndl, "Unable to load MAIN Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load MAIN Panel - exiting app");
		return -1;
	}

	// get the menu bar handle the MAIN panel

	gsMainPnlMenu = GetPanelMenuBar (gsMainPnlHndl);

	if (gsMainPnlMenu <= 0)
	{
		LOG_ERROR(gsMainPnlMenu, "Unable to load MAIN Panel menu bar");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load MAIN menu bar - exiting app");
		goto EXIT;
	}

	// display the panel

	DisplayPanel (gsMainPnlHndl);

	// maximize the MAIN panel

	SetPanelAttribute (gsMainPnlHndl, ATTR_WINDOW_ZOOM, VAL_MAXIMIZE);

	// load the SYSTEM panel

	if ((gsSystemPnlHndl = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", MACHINE)) < 0)
	{
		LOG_ERROR(gsMainPnlHndl, "Unable to load MACHINE Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load MACHINE Panel - exiting app");
		return -1;
	}

	// load the TRANSL (translation) panel

	if ((gsTranslatePnlHndl = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", TRANSL)) < 0)
	{
		LOG_ERROR(gsTranslatePnlHndl, "Unable to load TRANSL Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load TRANSL panel menu bar");
		return -1;
	}

	// load the GALIL_ERR panel

	if ((gsGalilErrorPnlHnd = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", GALIL_ERR)) < 0)
	{
		LOG_ERROR(gsGalilErrorPnlHnd, "Unable to load GALIL_ERR Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load GALIL_ERR panel menu bar");
		return -1;
	}

	// load the CHRT_SETUP panel

	if ((gsChartScalePnlHnd = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", CHRT_SETUP)) < 0)
	{
		LOG_ERROR(gsChartScalePnlHnd, "Unable to load CHRT_SETUP Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load CHRT_SETUP panel menu bar");
		return -1;
	}

	// load the EAC panel

	if ((gsErrorBypassAuthorizationPnlHnd = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", EAC)) < 0)
	{
		LOG_ERROR(gsErrorBypassAuthorizationPnlHnd, "Unable to load EAC Panel");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load EAC panel menu bar");
		return -1;
	}

	for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		int panelId = panelIndex+1;

		if (gsSystem.panelInstalled[panelIndex])	// if panel was installed
		{
			// load panel #

			if ((gsB3PanelPnlHndl[panelIndex] = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", B3_PANEL)) < 0)
			{
				LOG_ERROR_WITH_VARGS(gsB3PanelPnlHndl[panelIndex], "Unable to load B3_PANEL_%d panel", panelId);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL ERROR", "Unable to load B3_PANEL panel - exiting app");
				return -1;
			}

			// load the panel # menu bar

			gsB3PanelPnlMenu[panelIndex] = LoadMenuBar(gsB3PanelPnlHndl[panelIndex], "BORIS 3.uir", PANEL_MENU);

			if (gsB3PanelPnlMenu[panelIndex] <= 0)
			{
				LOG_ERROR_WITH_VARGS(gsB3PanelPnlMenu[panelIndex], "Unable to load B3_PANEL_%d panel menu bar", panelId);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL ERROR", "Unable to load B3_PANEL panel menu bar");
				goto EXIT;
			}

			// initialize the panel appearance

			char panelName[64];

			sprintf(panelName, "PANEL %d", panelId);
			SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_TITLE, panelName);

			int ctrlArrayHnd = 0;
			int controlId = 0;

			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_TOOLTIP_TEXT, "Click on icon to enter procedure data");
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_TOOLTIP_TEXT, "Click on icon to remove procedure data");
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_TEXT, "Click on icon to enter serial # information");
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_TEXT, "Click on icon to remove serial # information");
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_TEXT, "Click on icon to clear alert");
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_TEXT, "Click on icon to clear alert");
			SetCtrlArrayAttribute(ctrlArrayHnd, ATTR_TOOLTIP_DELAY, TOOLTIP_DELAY);

			int activeStationCount = 0;
			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					activeStationCount++;
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

					if (gsSystem.graphsEnabled)
					{
						gsB3GraphPnlHndl[panelIndex][stationIndex] = LoadPanel(gsMainPnlHndl, "BORIS 3.uir", GRAPH);
						sprintf(panelName, "PANEL %d - STATION %d", panelIndex+1, stationIndex+1);
						SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TITLE, panelName);
						SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_DIMMED, 1);
					}
				}
				else
				{
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId, ATTR_FRAME_COLOR, VAL_GRAY);
				}
			}

			if (activeStationCount > 1)
			{
				SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_REMOVE, ATTR_DIMMED, 0);
			}

			if (activeStationCount > 3)
			{
				SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_ADD, ATTR_DIMMED, 1);
			}

			if (strcmp(gsSystem.panel[panelIndex].activeProcedure, ""))
			{
				stat = B3_LoadProcedure(gsSystem.panel[panelIndex].activeProcedure, &gsSystem.panel[panelIndex].procedure);

				if (stat != B3_OK)
				{
					B3_GetLastError(&gLastError);
					B3SystemErrorLog(gsAppLogPath, &gLastError);
					LOG_ERROR_WITH_VARGS(gsB3PanelPnlMenu[panelIndex], "Unable to load active procedure %s for panel %d",
										 	gsSystem.panel[panelIndex].activeProcedure, panelId);
					B3SystemErrorLog(gsAppLogPath, &gLastError);
					Beep();
					MessagePopup ("SYSTEM ERROR", "Unable to load active procedure");
					goto CLEAR_PROCEDURE_DISPLAY;
				}
				else
				{
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, gsSystem.panel[panelIndex].procedure.name);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, gsSystem.panel[panelIndex].procedure.setPointInF);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 0);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 1);

					for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
					{
						if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
						{

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
							SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
						}
					}

					gsSystem.panel[panelIndex].setPointInF = gsSystem.panel[panelIndex].procedure.setPointInF;

					if (gsSystem.panel[panelIndex].procedure.setPointAdjustable)
					{
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
										  ATTR_CTRL_MODE, VAL_HOT);
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
										  ATTR_SHOW_INCDEC_ARROWS, 1);
//						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
//						                  ATTR_MIN_VALUE, gsSystem.panel[panelIndex].procedure.stripChartMinTempInF);
//						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
//						                  ATTR_MAX_VALUE, gsSystem.panel[panelIndex].procedure.stripChartMaxTempInF);
					}
					else
					{
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
										  ATTR_CTRL_MODE, VAL_INDICATOR);
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
										  ATTR_SHOW_INCDEC_ARROWS, 0);
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
						                  ATTR_MIN_VALUE, B3_MIN_TEMP_IN_F);
						SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
						                  ATTR_MAX_VALUE, B3_MAX_TEMP_IN_F);
					}
				}
			}
			else
			{
CLEAR_PROCEDURE_DISPLAY:
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, "Select Procedure");

				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 1);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
				SetCtrlArrayVal (ctrlArrayHnd, "");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
			}
		}
	}

	PanelAutoArrange();

#ifdef INSTALL_THERMAL_SIMULATOR
	gsThermalSimulatorPanelHnd = LoadPanel(gsMainPnlHndl, "Boris 3.uir", TEMP_SIM);
	//DisplayPanel(gsThermalSimulatorPanelHnd);

	gsThermalSimulatorAsyncTimerId = NewAsyncTimer (SIM_ASYNC_TIMER_PERIOD, -1, 1, ThermalSimulatorAsyncCallback, NULL);

	if (gsThermalSimulatorAsyncTimerId < 0)
	{
		LOG_ERROR(gsGalilAsyncTimerId, "Unable to create Thermal Simulator async timer");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to create Thermal Simulator async timer - exiting app");
		goto EXIT;
	}

	stat = B3_LoadSimThermalData(gsThermalSimulatorDataPath, &gsSystem.control.simThermalData);

	if (stat < 0)
	{
		LOG_ERROR(gsGalilAsyncTimerId, "Unable to load Thermal Simulator data");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load Thermal Simulator data - exiting app");
		goto EXIT;
	}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
	gsTranslatorSimulatorPanelHnd = LoadPanel(gsMainPnlHndl, "Boris 3.uir", TRANSL_SIM);
	SetActiveCtrl(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_TRANS_AT_STATION);
	DisplayPanel(gsTranslatorSimulatorPanelHnd);
	SetPanelAttribute (gsTranslatorSimulatorPanelHnd, ATTR_TOP, gsDefaultTranslatorPanelPosition[UI_PANEL_TOP] +
					  gsDefaultTranslatorPanelPosition[UI_PANEL_TOTAL_HEIGHT]);
	SetPanelAttribute (gsTranslatorSimulatorPanelHnd, ATTR_LEFT, gsDefaultTranslatorPanelPosition[UI_PANEL_LEFT]);

	gsTranslatorSimulatorAsyncTimerId = NewAsyncTimer (SIM_ASYNC_TIMER_PERIOD, -1, 1, TranslatorSimulatorAsyncCallback, NULL);

	if (gsTranslatorSimulatorAsyncTimerId < 0)
	{
		LOG_ERROR(gsGalilAsyncTimerId, "Unable to create Translate Simulator async timer");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to create Translate Simulator async timer - exiting app");
		goto EXIT;
	}

	stat = B3_LoadSimTranslateData(gsTranslatorSimulatorDataPath, &gsSystem.control.simTranslatorData);

	if (stat < 0)
	{
		LOG_ERROR(gsGalilAsyncTimerId, "Unable to load Translate Simulator data");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to load Translate Simulator data - exiting app");
		goto EXIT;
	}
#endif

#ifndef INSTALL_TRANSLATOR_SIMULATOR
	stat = GALIL_CMD_Open(gsSystem.translateInfo.galilMotionControllerDeviceId);

	if (stat < GALIL_CMD_OK)
	{
		LOG_ERROR(stat, "Unable to connect with the Galil controller");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to connect with the Galil controller - exiting app");
		goto EXIT;
	}

	int cmdStat = 0;

	stat = GALIL_CMD_VersionInfo(version, 0.1, &cmdStat);

	if (stat != GALIL_CMD_OK)
	{
		GALIL_CMD_ERROR_INFO errorInfo;
		GALIL_CMD_GetLastErrorInfo(&errorInfo);
		B3SystemLog(gsAppLogPath, "Galil ERROR: Status error with the Galil controller for Get Version Info - exiting app");
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to read the Galil software version information - exiting app");
		goto EXIT;
	}
	else if (cmdStat < GALIL_CMD_POWER_OFF)
	{
		GALIL_CMD_ERROR_INFO errorInfo;
		GALIL_CMD_GetLastErrorInfo(&errorInfo);

		if (cmdStat == GALIL_CMD_COMM_ERROR)
		{
			B3SystemLog(gsAppLogPath, "Galil ERROR: Comm error with the Galil controller for Get Version Info - exiting app");
		}
		else if (cmdStat == GALIL_CMD_POSITION_FOLLOWING_ERROR)
		{
			B3SystemLog(gsAppLogPath, "Galil ERROR: Position following error detected on the Galil controller for Get Version Info - exiting app");
		}
		else if (cmdStat == GALIL_CMD_SYSTEM_ERROR)
		{
			B3SystemLog(gsAppLogPath, "Galil ERROR: System error detected on the Galil controller for Get Version Info - exiting app");

			int errorNum = 0;
			int errorLine = 0;
			int errThread = 0;

			GALIL_CMD_SystemErrorInfo(&errorNum, &errorLine, &errThread, 0.1, &cmdStat);

			B3SystemLogWithVargs(gsAppLogPath, "Galil System Error Info: Error:%d at Line:%d in Thread:%d", errorNum, errorLine, errThread);
		}

		Beep();
		MessagePopup ("FATAL ERROR", "Unable to read the Galil software version information - exiting app");
		goto EXIT;
	}

	B3SystemLogWithVargs(gsAppLogPath, "Galil Software Version: %s", version);

#endif

	CmtNewLock (NULL, OPT_TL_PROCESS_EVENTS_WHILE_WAITING, &gsGalilAsyncThreadLock);

	gsGalilAsyncTimerId = NewAsyncTimer (1.0, -1, 1, GalilAsyncCallback, 0);

	if (gsGalilAsyncTimerId < 0)
	{
		LOG_ERROR(gsGalilAsyncTimerId, "Unable to create Galil async timer");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to create Galil async timer - exiting app");
		goto EXIT;
	}

	CmtNewLock (NULL, OPT_TL_PROCESS_EVENTS_WHILE_WAITING, &gsDAQmxDOutLock);

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	int32 daqError = 0;
	double ratePerChan = gsSystem.temp.sampleRateIPerChannelnHz;
	uInt64 samplesPerChan = gsSystem.temp.sampleRateIPerChannelnHz;

	daqError = DAQmxCreateTask("TempTask", &gsDAQmxTempTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCreateAIThrmcplChan (gsDAQmxTempTaskHnd, gsSystem.temp.device,
                          "", B3_MIN_TEMP_IN_F, B3_MAX_TEMP_IN_F, DAQmx_Val_DegF,
                          DAQmx_Val_J_Type_TC, DAQmx_Val_BuiltIn, 0.0, "");

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateAIThrmcplChan Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateAIThrmcplChan Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCfgSampClkTiming (gsDAQmxTempTaskHnd, "", ratePerChan, DAQmx_Val_Rising,
										DAQmx_Val_ContSamps, samplesPerChan);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCfgSampClkTiming Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCfgSampClkTiming Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxRegisterEveryNSamplesEvent (gsDAQmxTempTaskHnd,
												  DAQmx_Val_Acquired_Into_Buffer,
												  (uInt32)samplesPerChan, 0, DaqMXTempDeviceReadCallback,
												  NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxRegisterEveryNSamplesEvent Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxRegisterEveryNSamplesEvent Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxRegisterDoneEvent(gsDAQmxTempTaskHnd,0,DoneCallback,NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxRegisterDoneEvent Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxRegisterDoneEvent Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxGetTaskAttribute(gsDAQmxTempTaskHnd,DAQmx_Task_NumChans,&gsNumTempDAQmxChannels);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxGetTaskAttribute Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxGetTaskAttribute Error (see app log for details) - exiting app");
		goto EXIT;
	}

	gsTemperatureData=(float64*)malloc(samplesPerChan*gsNumTempDAQmxChannels*sizeof(float64));

	if( gsTemperatureData == NULL )
	{
		LOG_ERROR(daqError, "Unable to allocate memory for temperature measuements");
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL ERROR", "Unable to allocate memory for temperature measuements - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCreateTask("DinInitialReadTask", &gsDinTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCreateDIChan(gsDinTaskHnd, gsSystem.control.digitalInDevice, "", DAQmx_Val_ChanPerLine);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateDIChan Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateDIChan Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxStartTask(gsDinTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxGetTaskAttribute(gsDinTaskHnd, DAQmx_Task_NumChans, &gsNumDinDAQmxChannels);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxGetTaskAttribute Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxGetTaskAttribute Error (see app log for details) - exiting app");
		goto EXIT;
	}

	uInt8 data[64]={0};
	int32 numReadPerChan;
	int32 bytesPerSamp;

	int translatorInstalled = 0;
	int translatorPanelIndex = 0;
	int translatorStationIndex = 0;

	daqError = DAQmxReadDigitalLines (gsDinTaskHnd, 1, 0.0, DAQmx_Val_GroupByChannel,
										 data, gsNumDinDAQmxChannels, &numReadPerChan,
										 &bytesPerSamp, NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxReadDigitalLines Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxReadDigitalLines Error (see app log for details) - exiting app");
		goto EXIT;
	}

	for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		if (gsSystem.panelInstalled[panelIndex])
		{
			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					uInt8 dioState = data[gsSystem.control.translatorInPlaceIndex[panelIndex][stationIndex]];
					gsSystem.control.translatorInPlace[panelIndex][stationIndex] = dioState;

					if (gsSystem.panelInstalled[panelIndex])
					{
						if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
						{
							int ctrlArrayHnd = 0;
							int controlId = 0;

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, dioState);

							if (dioState == B3_DIN_LOGIC_ON)
							{
								translatorInstalled = 1;
								translatorPanelIndex = panelIndex;
								translatorStationIndex = stationIndex;
							}
						}
					}
				}
			}
		}
	}

	if (translatorInstalled)
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, translatorPanelIndex+1);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, translatorStationIndex+1);
		gsSystem.control.translatorPanelIndex = translatorPanelIndex;
		gsSystem.control.translatorStationIndex = translatorStationIndex;
	}
	else
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, 0);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, 0);
		gsSystem.control.translatorPanelIndex = -1;
		gsSystem.control.translatorStationIndex = -1;
	}

	DAQmxStopTask(gsDinTaskHnd);
	DAQmxClearTask(gsDinTaskHnd);
	gsDinTaskHnd = 0;

	daqError = DAQmxCreateTask("DinTask", &gsDinTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCreateDIChan(gsDinTaskHnd, gsSystem.control.digitalInDevice, "", DAQmx_Val_ChanPerLine);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateDIChan Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateDIChan Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxCfgChangeDetectionTiming(gsDinTaskHnd, gsSystem.control.digitalInDevice, gsSystem.control.digitalInDevice, DAQmx_Val_ContSamps, 1);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxCfgChangeDetectionTiming Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxCfgChangeDetectionTiming Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxRegisterSignalEvent(gsDinTaskHnd, DAQmx_Val_ChangeDetectionEvent, 0, DinChangeDetectionCallback, NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxRegisterSignalEvent Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxRegisterSignalEvent Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxGetTaskAttribute(gsDinTaskHnd, DAQmx_Task_NumChans, &gsNumDinDAQmxChannels);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxGetTaskAttribute Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxGetTaskAttribute Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxStartTask(gsDAQmxTempTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	daqError = DAQmxStartTask(gsDinTaskHnd);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
		goto EXIT;
	}

	for (int panelIndex = 0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		if (gsSystem.panelInstalled[panelIndex])
		{
			stat = B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);

			if (stat != B3_OK)
			{
				B3_GetLastError(&gLastError);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL ERROR", "Unable to set panel control - exiting app");
				goto EXIT;
			}
		}
	}

	B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_RUNNING, 0);
	B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
	B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);

	SetActivePanel(gsSystemPnlHndl);

	RunUserInterface ();

EXIT:

	if (gsLogLoaded)
	{
		if (DAQmxFailed(daqError))
		{
			int32 bufSize = 0;

			bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

			char daqErrorStr[bufSize];

			DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
			LOG_ERROR_WITH_VARGS(daqError, "DAQmx Error: %s", daqErrorStr);
			B3SystemErrorLog(gsAppLogPath, &gLastError);
			Beep();
			MessagePopup ("DAQmx ERROR", "See app log for details - exiting app");
		}

		if (gsDAQmxTempTaskHnd != 0)
		{
			DAQmxStopTask(gsDAQmxTempTaskHnd);
			DAQmxClearTask(gsDAQmxTempTaskHnd);
			gsDAQmxTempTaskHnd = 0;
		}

		if( gsTemperatureData )
		{
			free(gsTemperatureData);
			gsTemperatureData = NULL;
		}

		if (gsDAQmxDOutLock)
		{
			for (int panelIndex = 0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
			{
				if (gsSystem.panelInstalled[panelIndex])
				{
					stat = B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);

					if (stat != B3_OK)
					{
						B3_GetLastError(&gLastError);

						if (gsLogLoaded)
						{
							B3SystemErrorLog(gsAppLogPath, &gLastError);
						}

						Beep();
						MessagePopup ("SHUTDOWN ERROR", "Unable to set panel control - exiting app");
					}
				}
			}

			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_RUNNING, 0);
			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);
		}

		if (gsGalilAsyncTimerId)
		{
			DiscardAsyncTimer(gsGalilAsyncTimerId);
		}

		if (gsGalilAsyncThreadLock)
		{
			CmtDiscardLock(gsGalilAsyncThreadLock);
			gsGalilAsyncThreadLock = 0;
		}

		if (gsDAQmxDOutLock)
		{
			CmtDiscardLock(gsDAQmxDOutLock);
			gsDAQmxDOutLock = 0;
		}

#ifndef INSTALL_TRANSLATOR_SIMULATOR
		GALIL_CMD_Close();
#endif

#ifdef INSTALL_THERMAL_SIMULATOR
		if (gsThermalSimulatorAsyncTimerId)
		{
			DiscardAsyncTimer(gsThermalSimulatorAsyncTimerId);
		}

		if (gsThermalSimulatorPanelHnd)
		{
			DiscardPanel(gsThermalSimulatorPanelHnd);
			gsThermalSimulatorPanelHnd = 0;
		}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
		if (gsTranslatorSimulatorAsyncTimerId)
		{
			DiscardAsyncTimer(gsTranslatorSimulatorAsyncTimerId);
		}

		if (gsTranslatorSimulatorPanelHnd)
		{
			DiscardPanel(gsTranslatorSimulatorPanelHnd);
			gsTranslatorSimulatorPanelHnd = 0;
		}
#endif

		if (gsDisclaimerPnlHnd)
		{
			DiscardPanel(gsDisclaimerPnlHnd);
			gsDisclaimerPnlHnd = 0;
		}

		if (gsAboutPnlHnd)
		{
			DiscardPanel(gsAboutPnlHnd);
			gsAboutPnlHnd = 0;
		}

		for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
		{
			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				if (gsB3GraphPnlHndl[panelIndex][stationIndex])
				{
					DiscardPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
					gsB3GraphPnlHndl[panelIndex][stationIndex] = 0;
				}
			}
		}

		if (gsStationPanelSelectPnlHnd)
		{
			DiscardPanel (gsStationPanelSelectPnlHnd);
			gsStationPanelSelectPnlHnd = 0;
		}

		if (gsTranslatePnlHndl)
		{
			DiscardPanel (gsTranslatePnlHndl);
			gsTranslatePnlHndl = 0;
		}

		if (gsGalilErrorPnlHnd)
		{
			DiscardPanel (gsGalilErrorPnlHnd);
			gsGalilErrorPnlHnd = 0;
		}

		if (gsChartScalePnlHnd)
		{
			DiscardPanel (gsChartScalePnlHnd);
			gsChartScalePnlHnd = 0;
		}

		if (gsErrorBypassAuthorizationPnlHnd)
		{
			DiscardPanel (gsErrorBypassAuthorizationPnlHnd);
			gsErrorBypassAuthorizationPnlHnd = 0;
		}

		if (gsSystemPnlHndl)
		{
			DiscardPanel (gsSystemPnlHndl);
			gsSystemPnlHndl = 0;
		}

		if (gsMainPnlHndl)
		{
			DiscardPanel (gsMainPnlHndl);
			gsMainPnlHndl = 0;
			gsMainPnlMenu = 0;
		}

		if (gsAppSetupChanged)
		{
			stat = B3_SaveIni("BORIS 3.ini", &gsSystem);

			if (stat != B3_OK)
			{
				B3_GetLastError(&gLastError);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL ERROR", "Unable to save ini file BORIS 3.ini");
			}

			gsAppSetupChanged = 0;
		}


		B3SystemLog(gsAppLogPath, "Quitting BORIS 3");
		gsLogLoaded = 0;
	}

	return 0;
}

void CVICALLBACK MAIN_MENU_SystemExit (int menuBar, int menuItem, void *callbackData,
									 int panel)
{
	for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		if (gsSystem.panel[panelIndex].enabled)
		{
			Beep();
			MessagePopup ("Notice", "Cannot exit while existing panels are active");
			return;
		}
	}

	int ans = 0;

	ans = ConfirmPopup ("Exiting Application", "Are you sure you want to exit Boris 3?");

	if (ans)
	{
		QuitUserInterface(0);
	}

	return;
}

int CVICALLBACK ExitApp (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			Beep();
			MessagePopup ("Notice",
              "Please use System->Exit menu item on the main panel to exit the program");
			break;
	}

	return 0;
}

int CVICALLBACK EnterSerialNumberOk (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	char serialNum[18] = {0};
	char serialNumVerify[18] = {0};
	int panelIndex = 0;
	int stationIndex = 0;

	int ctrlArrayHnd = 0;
	int controlId = 0;

	int revisionCount = 0;

	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal(panel, SERIAL_NUM_VAL, serialNum);
			GetCtrlVal(panel, SERIAL_NUM_VERIFY, serialNumVerify);
			GetCtrlVal(panel, SERIAL_NUM_PANEL_INDEX, &panelIndex);
			GetCtrlVal(panel, SERIAL_NUM_STATION_INDEX, &stationIndex);

			int validSerialNumEntered = 0;

			if (stricmp(serialNum, serialNumVerify) == 0)
			{
				char newDir[MAX_PATHNAME_LEN] = {0};
				int dirCreated = 0;

				do
				{
					if (revisionCount == 0)
					{
						sprintf(newDir, "%s%s\\", gsSystem.logging.logDir, serialNum);
					}
					else
					{
						sprintf(newDir, "%s%s - rev %d\\", gsSystem.logging.logDir, serialNum, revisionCount);
					}

					int stat = MakeDir(newDir);

					if (stat == 0)
					{
						dirCreated = 1;
						validSerialNumEntered = 1;

						if (revisionCount == 0)
						{
							sprintf(gsLogDir[panelIndex][stationIndex], "%s%s\\", gsSystem.logging.logDir, serialNum);
							sprintf(gsLogPath[panelIndex][stationIndex], "%s%s\\%s", gsSystem.logging.logDir, serialNum, serialNum);
						}
						else
						{
							sprintf(gsLogDir[panelIndex][stationIndex], "%s%s - rev %d\\", gsSystem.logging.logDir, serialNum, revisionCount);
							sprintf(gsLogPath[panelIndex][stationIndex], "%s%s - rev %d\\%s - rev %d", gsSystem.logging.logDir, serialNum, revisionCount, serialNum, revisionCount);
						}
					}
					else if (stat == -9)	// dir already exists
					{
						char sNum[64] = {0};
						char msg[256] = {0};

						if (revisionCount == 0)
						{
							sprintf(sNum, "%s", serialNum);
						}
						else
						{
							sprintf(sNum, "%s - rev %d", serialNum, revisionCount);
						}

						sprintf(msg, "Serial # <%s> has been previously used\n\nDo you want to proceed?", sNum);

						int ans = ConfirmPopup ("Warning", msg);

						if (ans == 0)
						{
							SetCtrlVal(panel, SERIAL_NUM_VAL, "");
							SetCtrlVal(panel, SERIAL_NUM_VERIFY, "");

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");
							strcpy(gsSystem.panel[panelIndex].station[stationIndex].serialNum, "");

							break;
						}
						else
						{
							revisionCount++;
						}

					}
					else if (stat)
					{
						char msg[256];

						sprintf(msg, "ERROR %d - Unable to create log directory %s ", newDir);
						Beep();
						MessagePopup ("System Error", msg);
						SetCtrlVal(panel, SERIAL_NUM_VAL, "");
						SetCtrlVal(panel, SERIAL_NUM_VERIFY, "");
					}
				} while (dirCreated == 0);

				if (dirCreated)
				{
					B3SystemLogWithVargs(gsAppLogPath, "Operator entered serial # <%s> for panel %d - station %d",
										serialNum, panelIndex+1, stationIndex+1);

					//gsLogPath[panelIndex][stationIndex]
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, serialNum);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

					strcpy(gsSystem.panel[panelIndex].station[stationIndex].serialNum, serialNum);
				}

				DiscardPanel(panel);

			}
			else
			{
				Beep();
				MessagePopup ("Entry Error", "Serial Numbers did not match");
				SetCtrlVal(panel, SERIAL_NUM_VAL, "");
				SetCtrlVal(panel, SERIAL_NUM_VERIFY, "");
			}

			SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_DIMMED, 0);
			break;
	}

	return 0;
}

int CVICALLBACK EnterSerialNumberCancel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	char title[64] = {0};
	int panelId = 0;
	int panelIndex = 0;
	int stationId = 0;
	int stationIndex = 0;
	int ctrlArrayHnd = 0;
	int controlId = 0;

	switch (event)
	{
		case EVENT_COMMIT:

			GetPanelAttribute (panel, ATTR_TITLE, title);

			sscanf(title, "PANEL %d - STATION %d", &panelId, &stationId);
			panelIndex = panelId - 1;
			stationIndex = stationId - 1;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

			strcpy(gsSystem.panel[panelIndex].station[stationIndex].serialNum, "");

			DiscardPanel(panel);
			SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_DIMMED, 0);
			break;
	}

	return 0;
}

int CVICALLBACK B3StationControl (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	int stat = 0;
	int panelIndex = -1;
	int panelId = 0;
	int stationIndex = -1;
	int stationId = 0;
	int ctrlArrayHnd = 0;
	int controlId = 0;

	switch (event)
	{
		case EVENT_COMMIT:
			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
					break;
				}
			}

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, CONTROL_CA);
			GetCtrlArrayIndex (ctrlArrayHnd, panel, control, &stationIndex);
			stationId = stationIndex+1;

			int controlState = 0;

			GetCtrlVal(panel, control, &controlState);
			//gsSystem.panel[panelIndex].station[stationIndex].enabled = controlState;

			if (controlState)
			{
				B3SystemLogWithVargs(gsAppLogPath, "Operator enabled control for panel %d - station %d",
									panelId, stationId);

				SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION, ATTR_DIMMED, 1);

				if (!gsSystem.panel[panelIndex].enabled)
				{
					gsSystem.panel[panelIndex].enabled = controlState;
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, gsSystem.panel[panelIndex].setPointInF);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 0);

					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_TEMP, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, ATTR_DIMMED, 0);

					for (int sIndex=0; sIndex<B3_MAX_STATION_COUNT; sIndex++)
					{
						if (gsSystem.panel[panelIndex].stationInstalled[sIndex])
						{
							int visible = 0;

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
							GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &visible);

							if (visible)
							{
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
								SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "");
							}
							else
							{
								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
							}
						}
					}
				}

				CVIAbsoluteTime nullTime = {{0, 0}, 0};
				gsSystem.panel[panelIndex].station[stationIndex].summary.startTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.stabilizationCompleteTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.opAckOfInjectionReadyTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.translationReadyTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.opAckOfTranslationReadyTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.translationStartTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.translationCompleteTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.startOfPostBakeHeatingTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.completionTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.opAckOfCompletionTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempTime = nullTime;
				gsSystem.panel[panelIndex].station[stationIndex].summary.completionState = B3_STATION_STATE_OFF;
				gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempInF = 0.0;
				gsSystem.panel[panelIndex].station[stationIndex].summary.translationDetectTempInF = 0.0;

				GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.startTime);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], RUN_TIME_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, B3_STATION_STATE_STABILIZING);

				int pointsPerChart = (gsSystem.panel[panelIndex].procedure.stripChartSpanInMinutes * POINTS_PER_MINUTE) +1;

				if (gsSystem.graphsEnabled)
				{
					SetCtrlAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex],
									  GRAPH_STRIPCHART, ATTR_POINTS_PER_SCREEN,
									  pointsPerChart);
					SetAxisScalingMode (gsB3GraphPnlHndl[panelIndex][stationIndex],
										GRAPH_STRIPCHART, VAL_LEFT_YAXIS, VAL_MANUAL,
										gsSystem.panel[panelIndex].procedure.stripChartMinTempInF,
										gsSystem.panel[panelIndex].procedure.stripChartMaxTempInF);
					ClearStripChart (gsB3GraphPnlHndl[panelIndex][stationIndex], GRAPH_STRIPCHART);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_DIMMED, 0);
				}

				gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_STABILIZING;
				gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_RUNNING, 1);
			}
			else
			{
				char *callBackData = (char*)callbackData;

				if ((callBackData != NULL) &&
				   (!stricmp(callBackData, "AUTO")))
				{
					B3SystemLogWithVargs(gsAppLogPath, "System auto disabled control for panel %d - station %d",
										panelId, stationId);
				}
				else
				{
					int ans = 0;
					char msg[512];

					sprintf(msg, "Are you sure you want to stop control \nof Panel %d - Station %d?", panelId, stationId);
					ans = ConfirmPopup ("Stopping Control", msg);

					if (ans == 0)
					{
						SetCtrlVal(panel, control, 1);
						return(0);
					}

					B3SystemLogWithVargs(gsAppLogPath, "Operator disabled control for panel %d - station %d",
										panelId, stationId);
				}

				if (gsSystem.panel[panelIndex].station[stationIndex].summary.completionState != B3_STATION_STATE_OFF)
				{
					GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.completionTime);
					gsSystem.panel[panelIndex].station[stationIndex].summary.completionState =
										gsSystem.panel[panelIndex].station[stationIndex].state;
				}

				char logPath[MAX_PATHNAME_LEN];

				strcpy(logPath, gsLogPath[panelIndex][stationIndex]);
				strcat(logPath, ".sum");

				stat = B3StationSummaryLog(logPath,
									&gsSystem,
									panelIndex,
									stationIndex);

				int panelState = 0;
				int stationControlState = 0;

				for (int i=0; i<B3_MAX_STATION_COUNT; i++)
				{
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, i);
					GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &stationControlState);

					if (stationControlState)
					{
						panelState = 1;
						break;
					}
				}

				if (!panelState)
				{
					gsSystem.panel[panelIndex].enabled = 0;
					SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION, ATTR_DIMMED, 0);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, 0.0);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 1);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_TEMP, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, ATTR_DIMMED, 1);

					for (int sIndex=0; sIndex<B3_MAX_STATION_COUNT; sIndex++)
					{
						if (gsSystem.panel[panelIndex].stationInstalled[sIndex])
						{
							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, sIndex);
							SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");
						}
					}
				}

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], RUN_TIME_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "00:00:00");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				//SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, B3_STATION_STATE_OFF);
				gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_OFF;
				gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

/*
				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
				//B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);
				//SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
				//SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
//*/

				if (gsSystem.graphsEnabled)
				{
					ClearStripChart (gsB3GraphPnlHndl[panelIndex][stationIndex], GRAPH_STRIPCHART);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_DIMMED, 1);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TOP, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_LEFT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH]);
				}

				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_RUNNING, 0);
				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);
				B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);
			}

			break;
	}
	return 0;
}

int CVICALLBACK LoadProcedure(int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			char path[MAX_PATHNAME_LEN];
			int stat = 0;
			int panelIndex = -1;
			int panelId = 0;

			SetCtrlAttribute(panel, control, ATTR_VISIBLE, 0);

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
				}
			}

			char title[64] = {0};

			sprintf(title, "Load Boris 3 Procedure for Panel %d", panelId);
			SetPanelAttribute (panel, ATTR_DIMMED, 1);

			Beep();
			stat = FileSelectPopupEx (gsSystem.procedureDir, "*.pdr", "*.pdr", title, VAL_LOAD_BUTTON,
									  1, 1, path);

			SetPanelAttribute (panel, ATTR_DIMMED, 0);

			if (stat != 1)
			{
				SetCtrlAttribute(panel, control, ATTR_VISIBLE, 1);
				return(0);
			}

			stat = B3_LoadProcedure(path, &gsSystem.panel[panelIndex].procedure);

			if (stat != B3_OK)
			{
				B3SystemLogWithVargs(gsAppLogPath, "FATAL ERROR %d - Unable to load active procedure %s for panel %d", stat, gsSystem.panel[panelIndex].activeProcedure, panelId);
				Beep();
				MessagePopup ("File ERROR", "Unable to load procedure");
				SetCtrlVal(panel, B3_PANEL_PROCEDURE, "Select Procedure");
				SetCtrlAttribute(panel, control, ATTR_VISIBLE, 1);
				return(0);
			}

			gsAppSetupChanged = 1;

			B3SystemLogWithVargs(gsAppLogPath, "Operator selected procedure %s for panel %d",
								path, panelIndex+1);

			strcpy(gsSystem.panel[panelIndex].activeProcedure, path);
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, gsSystem.panel[panelIndex].procedure.name);
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, gsSystem.panel[panelIndex].procedure.setPointInF);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 0);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 1);

			int ctrlArrayHnd = 0;
			int controlId = 0;

			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
				}
			}

			gsSystem.panel[panelIndex].setPointInF = gsSystem.panel[panelIndex].procedure.setPointInF;

			if (gsSystem.panel[panelIndex].procedure.setPointAdjustable)
			{
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_CTRL_MODE, VAL_HOT);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_SHOW_INCDEC_ARROWS, 1);
//				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
//				                  ATTR_MIN_VALUE, gsSystem.panel[panelIndex].procedure.stripChartMinTempInF);
//				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
//				                  ATTR_MAX_VALUE, gsSystem.panel[panelIndex].procedure.stripChartMaxTempInF);
			}
			else
			{
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_SHOW_INCDEC_ARROWS, 0);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_CTRL_MODE, VAL_INDICATOR);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
								  ATTR_SHOW_INCDEC_ARROWS, 0);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
				                  ATTR_MIN_VALUE, B3_MIN_TEMP_IN_F);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP,
				                  ATTR_MAX_VALUE, B3_MAX_TEMP_IN_F);
			}
			break;
	}
	return 0;
}

int CVICALLBACK EnterSerialNumber (int panel, int control, int event,
								   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = -1;
			int panelId = 0;
			int stationIndex = -1;
			int stationId = 0;

			SetCtrlAttribute(panel, control, ATTR_VISIBLE, 0);

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
					break;
				}
			}

			int ctrlArrayHnd = 0;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, SERIAL_ALERT_CA);
			GetCtrlArrayIndex (ctrlArrayHnd, panel, control, &stationIndex);
			stationId = stationIndex+1;

			int serialNumPnlHnd = 0;

			if ((serialNumPnlHnd = LoadPanel (gsB3PanelPnlHndl[panelIndex], "BORIS 3.uir", SERIAL_NUM)) < 0)
			{
				B3SystemLogWithVargs(gsAppLogPath, "RUN-TIME ERROR %d - Unable to load SERIAL_NUM Panel", gsMainPnlHndl);
				Beep();
				MessagePopup ("RUN-TIME ERROR", "Unable to load SERIAL_NUM Panel");
				return(0);
			}

			char title[64];
			sprintf(title, "PANEL %d - STATION %d", panelId, stationId);
			SetPanelAttribute (serialNumPnlHnd, ATTR_TITLE, title);
			SetCtrlVal(serialNumPnlHnd, SERIAL_NUM_VAL, "");
			SetCtrlVal(serialNumPnlHnd, SERIAL_NUM_VERIFY, "");
			SetCtrlVal(serialNumPnlHnd, SERIAL_NUM_PANEL_INDEX, panelIndex);
			SetCtrlVal(serialNumPnlHnd, SERIAL_NUM_STATION_INDEX, stationIndex);

			SetPanelAttribute (panel, ATTR_DIMMED, 1);
			Beep();
			DisplayPanel(serialNumPnlHnd);

			break;
	}
	return 0;
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	if ( gsTemperatureData )
	{
		free(gsTemperatureData);
		gsTemperatureData = NULL;
	}

	DAQmxClearTask(gsDAQmxTempTaskHnd);
	gsDAQmxTempTaskHnd = 0;

	// Check to see if an error stopped the task.

	if (status < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(status, "DAQmx ERROR: %s", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmx Error (see app log for details) - exiting app");
		QuitUserInterface(status);
		return(status);
	}

	return 0;
}

void CVICALLBACK PANEL_MENU_STATION_Add (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	gsAppSetupChanged = 1;

	int panelIndex = 0;
	int panelId = 0;

	for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
	{
		if (panel == gsB3PanelPnlHndl[i])
		{
			panelIndex = i;
			break;
		}
	}

	panelId = panelIndex+1;

	int stationsCurrentlyInstalled = 0;

	for (int i=0; i<B3_MAX_STATION_COUNT; i++)
	{
		if (gsSystem.panel[panelIndex].stationInstalled[i])
		{
			stationsCurrentlyInstalled++;
		}
	}

	B3SystemLogWithVargs(gsAppLogPath, "Operator selected add station for panel %d", panelId);

	if (stationsCurrentlyInstalled == 4)
	{
		SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_ADD, ATTR_DIMMED, 1);
		return;
	}

	if (stationsCurrentlyInstalled == 3)
	{
		SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_ADD, ATTR_DIMMED, 1);
	}

	SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_REMOVE, ATTR_DIMMED, 0);

	int stationIndex = stationsCurrentlyInstalled;
	gsSystem.panel[panelIndex].stationInstalled[stationIndex] = 1;

	int procedureLoaded = 0;

	if (stricmp(gsSystem.panel[panelIndex].activeProcedure, ""))
	{
		procedureLoaded = 1;
	}

	int ctrlArrayHnd = 0;
	int controlId = 0;

	if (procedureLoaded)
	{
		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
		SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
		SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
		SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
		SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
	}

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

	int frameColor;

	GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_PANEL, ATTR_FRAME_COLOR, &frameColor);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId,
					  ATTR_FRAME_COLOR, frameColor);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

	if (gsSystem.graphsEnabled)
	{
		gsB3GraphPnlHndl[panelIndex][stationIndex] = LoadPanel(gsMainPnlHndl, "BORIS 3.uir", GRAPH);

		char panelName[64];

		sprintf(panelName, "PANEL %d - STATION %d", panelIndex+1, stationIndex+1);
		SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TITLE, panelName);
		SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_DIMMED, 1);
	}

	PanelAutoArrange();
	return;
}

void CVICALLBACK PANEL_MENU_STATION_Remove (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	gsAppSetupChanged = 1;

	int panelIndex = 0;
	int panelId = 0;

	for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
	{
		if (panel == gsB3PanelPnlHndl[i])
		{
			panelIndex = i;
			break;
		}
	}

	panelId = panelIndex+1;

	B3SystemLogWithVargs(gsAppLogPath, "Operator selected remove station for panel %d", panelId);

	int stationsCurrentlyInstalled = 0;

	for (int i=0; i<B3_MAX_STATION_COUNT; i++)
	{
		if (gsSystem.panel[panelIndex].stationInstalled[i])
		{
			stationsCurrentlyInstalled++;
		}
	}

	if (stationsCurrentlyInstalled == 2)
	{
		SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_REMOVE, ATTR_DIMMED, 1);
	}

	SetMenuBarAttribute (gsB3PanelPnlMenu[panelIndex], PANEL_MENU_STATION_ADD, ATTR_DIMMED, 0);

	int stationIndex = stationsCurrentlyInstalled-1;
	gsSystem.panel[panelIndex].stationInstalled[stationIndex] = 0;

	int ctrlArrayHnd = 0;
	int controlId = 0;

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
	SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "");

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId,
					  ATTR_FRAME_COLOR, VAL_GRAY);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
	SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0.0);

	ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
	controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
	SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

	if (gsB3GraphPnlHndl[panelIndex][stationIndex])
	{
		DiscardPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
		gsB3GraphPnlHndl[panelIndex][stationIndex] = 0;
	}

	PanelAutoArrange();
	return;
}

void CVICALLBACK MAIN_MENU_PANEL_Manage (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	if (gsManagePanelHnd)
	{
		Beep();
		DisplayPanel(gsManagePanelHnd);
		return;
	}

	if ((gsManagePanelHnd = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", PANEL_MGMT)) < 0)
	{
		B3SystemLogWithVargs(gsAppLogPath, "RUN-TIME ERROR %d - Unable to load PANEL_MGMT Panel", gsManagePanelHnd);
		Beep();
		MessagePopup ("RUN-TIME ERROR", "Unable to load SERIAL_NUM Panel");
		return;
	}

	int ctrlArrayHnd = 0;
	int controlId = 0;

	for (int panelIndex = 0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsManagePanelHnd, PANEL_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, panelIndex);
		SetCtrlVal(gsManagePanelHnd, controlId, gsSystem.panelInstalled[panelIndex]);
	}

	Beep();
	DisplayPanel(gsManagePanelHnd);
}

int CVICALLBACK ClosePanelMgmtPanel (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			DiscardPanel(panel);
			gsManagePanelHnd = 0;
			break;
	}
	return 0;
}

int CVICALLBACK ManagePanels (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int ctrlArrayHnd = 0;
			int panelIndex = 0;
			int panelId = 0;

			gsAppSetupChanged = 1;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, PANEL_CA);
			GetCtrlArrayIndex (ctrlArrayHnd, panel, control, &panelIndex);
			panelId = panelIndex+1;

			int activePanelCount = 0;

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsSystem.panelInstalled[i])
				{
					activePanelCount++;
				}
			}

			B3SystemLog(gsAppLogPath, "Operator selected manage panels");

			int panelSelectionState = 0;

			GetCtrlVal(panel, control, &panelSelectionState);

			if (!panelSelectionState)
			{
				if (activePanelCount == 1)
				{
					SetCtrlVal(panel, control, 1);
					Beep();
					MessagePopup ("Panel Removal", "At least one panel must be installed");
					return(0);
				}

				if (gsSystem.panel[panelIndex].enabled)
				{
					SetCtrlVal(panel, control, 1);
					Beep();
					MessagePopup ("Panel Control Error",
					  "Panel must not be active in order to remove it");
					return(0);
				}

				B3SystemLogWithVargs(gsAppLogPath, "Operator removed panel %d", panelId);

				gsSystem.panelInstalled[panelIndex] = 0;

				DiscardPanel(gsB3PanelPnlHndl[panelIndex]);
				gsB3PanelPnlHndl[panelIndex] = 0;

				for (int stationIndex=0; stationIndex-B3_MAX_STATION_COUNT; stationIndex++)
				{
					if (gsB3GraphPnlHndl[panelIndex][stationIndex])
					{
						DiscardPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
						gsB3GraphPnlHndl[panelIndex][stationIndex] = 0;
					}
				}
			}
			else
			{
				if (activePanelCount == B3_MAX_PANEL_COUNT)
				{
					Beep();
					MessagePopup ("Panel Addition", "No more than 5 panels can be installed");
					SetCtrlVal(panel, control, 1);
					return(0);
				}

				if ((gsB3PanelPnlHndl[panelIndex] = LoadPanel (gsMainPnlHndl, "BORIS 3.uir", B3_PANEL)) < 0)
				{
					B3SystemLogWithVargs(gsAppLogPath, "FATAL ERROR %d - Unable to load B3_PANEL_%d panel", panelId, gsTranslatePnlHndl);
					Beep();
					MessagePopup ("FATAL ERROR", "Unable to load B3_PANEL panel menu bar");
					return(0);
				}

				gsB3PanelPnlMenu[panelIndex] = LoadMenuBar(gsB3PanelPnlHndl[panelIndex], "BORIS 3.uir", PANEL_MENU);

				B3SystemLogWithVargs(gsAppLogPath, "Operator added panel %d", panelId);

				char panelName[64];

				sprintf(panelName, "PANEL %d", panelId);
				SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_TITLE, panelName);

				//int ctrlArrayHnd = 0;
				int controlId = 0;

				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, "Select Procedure");
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 1);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
				SetCtrlArrayVal (ctrlArrayHnd, "");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, 0);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				int frameColor;

				GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_PANEL, ATTR_FRAME_COLOR, &frameColor);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_FRAME_COLOR, VAL_GRAY);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, 0);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId, ATTR_FRAME_COLOR, frameColor);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, 0);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
				SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, 0);

				DisplayPanel (gsB3PanelPnlHndl[panelIndex]);

				gsSystem.panelInstalled[panelIndex] = 1;
				gsSystem.panel[panelIndex].enabled = 0;

				gsSystem.panel[panelIndex].stationInstalled[0] = 1;

				for (int stationIndex=1; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
				{
					gsSystem.panel[panelIndex].stationInstalled[stationIndex] = 0;
				}

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
				SetCtrlArrayVal(ctrlArrayHnd, 0);

				for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
				{
					if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
					{
						if (gsSystem.graphsEnabled)
						{
							gsB3GraphPnlHndl[panelIndex][stationIndex] = LoadPanel(gsMainPnlHndl, "BORIS 3.uir", GRAPH);

							sprintf(panelName, "PANEL %d - STATION %d", panelIndex+1, stationIndex+1);
							SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TITLE, panelName);
							SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_DIMMED, 1);
						}
					}
				}
			}

			PanelAutoArrange();
			DisplayPanel(panel);
			break;
	}
	return 0;
}

int CVICALLBACK ProcessStatusAlert (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = 0;
			int panelId = 0;
			int stationIndex = 0;
			int stationId = 0;

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
					break;
				}
			}

			switch(control)
			{
				case B3_PANEL_STATUS_ALERT_1:
					stationIndex = 0;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ALERT_2:
					stationIndex = 1;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ALERT_3:
					stationIndex = 2;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ALERT_4:
					stationIndex = 3;
					stationId = stationIndex+1;
					break;

			}

			B3SystemLogWithVargs(gsAppLogPath, "Operator cleared alert for panel %d - station %d",
								panelId, stationId);

			SetCtrlAttribute(panel, control, ATTR_VISIBLE, 0);
			break;
	}

	return 0;
}
//
// ** See Function definition for Summary comments **
int32 TempControlStateEngine(int operation, int panelIndex, int stationIndex)
{
	// * * * * * * * * * * * *  Initializing Variables for use in State Engine  * * * * * * * * * * * * (This only happens once)
	static int lastState[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0};
	static double baseTimeInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static int translationDetectionState[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = {B3_TRANS_DETECT_IDLE}};
	static int lastTranslationDetectionState[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = {B3_TRANS_DETECT_IDLE}};
	static double peakTempInF[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double peakTimeInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double baseTimeForStabilizationToCompleteInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double baseTimeForTranslationReadyInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double baseTimeForTranslationStartedInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double baseTimeForStartOfHeatingInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static double baseTimeForCoolDownCompleteInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0.0};
	static int translationComplete[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 0};
	static DEFERRED_CALL_DATA deferredCallData[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = {0, 0}};
	static int lastStateBeforeError[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = B3_STATION_STATE_OFF};
	static int lastSubStateBeforeError[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = B3_SUB_STATE_NOT_USED};
	static int lastTranslationDetectionStateBeforeError[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = {B3_TRANS_DETECT_IDLE}};
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

	int ctrlArrayHnd = 0;
	int controlId = 0;
	double anchorTempInF = gsSystem.panel[panelIndex].station[stationIndex].anchorTempInF;
	double setPointInF = gsSystem.panel[panelIndex].setPointInF;
	double setPointToleranceInF = gsSystem.panel[panelIndex].procedure.setPointToleranceInF;
	int alertControlVisible = 0;
	int errorControlVisible = 0;
	double deltaTimeInSec = 0.0;
#ifndef INSTALL_TRANSLATOR_SIMULATOR
	int stat = 0;				// Initialized if NOT simulating Translator
#endif
	int cmdStat = 0;

// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
// * First Level State Machine to either INITIALIZE or RUN System										*
// *  Cases:																																				*
// *			- INITIALIZE_STATE_ENGINE																									*
// *			- RUN_STATE_ENGINE																												*
// *	 Contains:    (Other Switch Statements)																				*
// *			- switch(gsSystem.panel[panelIndex].station[stationIndex].state)					*
// * 			- switch(gsSystem.panel[panelIndex].procedure.translationDetectionMethod)	*
// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
	switch(operation)
	{
//			PRIMARY INITIALIZATION:
		case INITIALIZE_STATE_ENGINE:
			for (int pIndex=0; pIndex<B3_MAX_PANEL_COUNT; pIndex++)
			{
				for (int sIndex=0; sIndex<B3_MAX_STATION_COUNT; sIndex++)
				{
					lastState[pIndex][sIndex] = B3_STATION_STATE_OFF;
				}
			}

			return(0);

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//			MAIN STATE ENGINE TO RUN THE STATION. ALSO CONTROLS TRANSLATION.
//			Contains States:
//				- B3_STATION_STATE_OFF
//				- B3_STATION_STABILIZATION_READY_DELAY_ERROR
//				- B3_STATION_TRANSLATION_READY_DELAY_ERROR
//				- B3_STATION_TRANSLATION_START_DELAY_ERROR
//				- B3_STATION_COOLDOWN_READY_DELAY_ERROR
//				- B3_STATION_STATE_OFF 	*Duplicate? @michaelp
//				- B3_STATION_STATE_STABILIZING
//				- B3_STATION_STATE_STABILIZING_EACG
//				- B3_STATION_STATE_READY_FOR_RESIN
//				- B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY
//				- B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG
//				- B3_STATION_STATE_READY_FOR_TRANSLATION
//				- B3_STATION_STATE_READY_FOR_TRANSLATION_EACG
//				- B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
//				- B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION_EACG
//				- B3_STATION_STATE_TRANSLATING
//				- B3_STATION_STATE_TRANSLATING_COMPLETE
//				- B3_STATION_STATE_WAITING_FOR_HEATING_READY
//				- B3_STATION_STATE_READY_FOR_HEATING
//				- B3_STATION_STATE_HEATING
//				- B3_STATION_STATE_COOL_DOWN
//				- B3_STATION_STATE_COOL_DOWN_EACG
//				- B3_STATION_STATE_COMPLETE
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
		case RUN_STATE_ENGINE:
			switch(gsSystem.panel[panelIndex].station[stationIndex].state)
			{
				case B3_STATION_STATE_ERROR:                      // General Error.
				case B3_STATION_STABILIZATION_READY_DELAY_ERROR:  // Stabilization Delay Error.
				case B3_STATION_TRANSLATION_READY_DELAY_ERROR:    // Translation Delay Error.
				case B3_STATION_TRANSLATION_START_DELAY_ERROR:		// Translation Delay Error.
				case B3_STATION_COOLDOWN_READY_DELAY_ERROR:				// Cooldown Delay Error.
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{  //Records state and substate, and last states before error if they differ
						lastStateBeforeError[panelIndex][stationIndex] = lastState[panelIndex][stationIndex];
						lastSubStateBeforeError[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].subState;
						lastTranslationDetectionStateBeforeError[panelIndex][stationIndex] = lastTranslationDetectionState[panelIndex][stationIndex];
						// Update lastState to current
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						// update Panel States
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						// update Panel Alert Status (as not visible)
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
						// update Error status(as Visible)
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);
						// Set Error Light on, Alert Light Off
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
					}

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &errorControlVisible);

					if (!errorControlVisible)
					{
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);
					}

					break;

				case B3_STATION_STATE_OFF:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);

						int state = 0;
						GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &state);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0);

						if (state)
						{
							deferredCallData[panelIndex][stationIndex].panel = gsB3PanelPnlHndl[panelIndex];
							deferredCallData[panelIndex][stationIndex].control = controlId;
							PostDeferredCall (DeferredCallToB3StationControl, &deferredCallData[panelIndex][stationIndex]);
						}

						gsSystem.panel[panelIndex].postTranslationHeating = 0;
						ProcessSystemEvents();
					}

					// do nothing
					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Stabilizing
//	DESCRIPTION:	This state ocurrs while waiting for temperature
//		to stabilize.
//		  The system sets a timer for delay time error based on the
//		procedure variable maxTimeToStabilizationInSeconds. If this
//		time is reached, the state is set to error.
//			 The primary function of this state is to wait until the temp
//		has been within tolerance(set in procedure) for the required
//		delay time set in procedure. If it has, the state is changed
//		to B3_STATION_STATE_READY_FOR_RESIN. Otherwise timer continues.
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_STABILIZING:
				case B3_STATION_STATE_STABILIZING_EACG:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)  // if just entering stabilizing,
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						baseTimeInSec[panelIndex][stationIndex] = Timer();  //

						if (lastStateBeforeError[panelIndex][stationIndex] != B3_STATION_STATE_STABILIZING)
						{
							baseTimeForStabilizationToCompleteInSec[panelIndex][stationIndex] = Timer();  // if Stabilizing for first time, set timer base time
						}

						lastStateBeforeError[panelIndex][stationIndex] = B3_STATION_STATE_OFF;  // *How does this work @tomdoyle ?
						lastSubStateBeforeError[panelIndex][stationIndex] = B3_SUB_STATE_NOT_USED;
						lastTranslationDetectionStateBeforeError[panelIndex][stationIndex] = B3_TRANS_DETECT_IDLE;

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
					}

					deltaTimeInSec = Timer() - baseTimeForStabilizationToCompleteInSec[panelIndex][stationIndex];

					if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_STABILIZING) &&
						(gsSystem.panel[panelIndex].procedure.maxTimeToStabilizationInSeconds > 0.0) &&
						(deltaTimeInSec > gsSystem.panel[panelIndex].procedure.maxTimeToStabilizationInSeconds)) // If Delay Error, Set State/lights/msg Error and break
					{
						B3SystemLogWithVargs(gsAppLogPath, "Stabilization ready delay error detected for panel %d - station %d",
											panelIndex+1, stationIndex+1);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STABILIZATION_READY_DELAY_ERROR;
						break;
					}

					if ((anchorTempInF <= (setPointInF + setPointToleranceInF)) &&
						(anchorTempInF >= (setPointInF - setPointToleranceInF)))
					{
						deltaTimeInSec = Timer() - baseTimeInSec[panelIndex][stationIndex];

						if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.stabilizationReadyDelayInSec)
						{
							// transition to B3_STATION_STATE_STABILIZING

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_RESIN;
							break;
						}

					}
					else
					{
						baseTimeInSec[panelIndex][stationIndex] = Timer();
					}

					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Ready for Resin:
//	DESCRIPTION:  This state ocurrs when stabilization paramaters have
//						been met.
//			  On first call, the user is alerted with light and UI indicator
//      and the indicator is the translation ready button.
//			  The state checks if values are still stabilized for station,
// 			(if not ready, returns to stabilizing case). If User has
// 			hidden alert with a click, continue to wait for translate.
//
//			Next State: B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY.
//			Optional (error) State: B3_STATION_STATE_STABILIZING
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_READY_FOR_RESIN:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);  // Make alert visible to user

						GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.stabilizationCompleteTime);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 1);

						B3SystemLogWithVargs(gsAppLogPath, "Ready for resin alert posted for panel %d - station %d",
											panelIndex+1, stationIndex+1);
					}

					if ((anchorTempInF > (setPointInF + setPointToleranceInF)) ||
						(anchorTempInF < (setPointInF - setPointToleranceInF)))
					{
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						B3SystemLogWithVargs(gsAppLogPath, "Ready for resin alert cleared for panel %d - station %d",
											panelIndex+1, stationIndex+1);
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_STABILIZING;
						break;
					}

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &alertControlVisible);

					if (!alertControlVisible)  // User has clicked alert control  and is ready for translation
					{
						// transition to B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.opAckOfInjectionReadyTime);
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY;    // READY FOR TRANSLATION STATE CHANGE
						break;
					}

					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Waiting for Translation Ready:
//	DESCRIPTION:	 This state is used when the station is being infused with
//		resin and is being monitored for translation. The states first
//    use initializes it. After this, the case contains two nested
//    switch statements:
//			switch(gsSystem.panel[panelIndex].procedure.translationDetectionMethod)
//				which includes cases: {
//					B3_TRANSLATE_DETECTION_PEAK_METHOD
//						which contains switch statement:
//							switch(translationDetectionState[panelIndex][stationIndex])
//								which contains cases: {
//									B3_TRANS_DETECT_IDLE
//									B3_PEAK_DETECTION
//									B3_TRANS_WAITING_FOR_COMPLETION
//									B3_TRANS_COMPLETE
//									}
//					B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD }
//
//		Next State: B3_STATION_STATE_READY_FOR_TRANSLATION
//		Optional Next state: B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
//
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY:
				case B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)   // state initialization
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_TRANS_DETECT_IDLE;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						if (lastStateBeforeError[panelIndex][stationIndex] == B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY)
						{
							translationDetectionState[panelIndex][stationIndex] = lastTranslationDetectionStateBeforeError[panelIndex][stationIndex];
						}
						else
						{
							translationDetectionState[panelIndex][stationIndex] = B3_TRANS_DETECT_IDLE;
						}

						lastTranslationDetectionState[panelIndex][stationIndex] = B3_TRANS_DETECT_OFF;
						baseTimeForTranslationReadyInSec[panelIndex][stationIndex] = Timer();									// Start Translation (resin exotherm reaction) Timer

						lastStateBeforeError[panelIndex][stationIndex] = B3_STATION_STATE_OFF;
						lastSubStateBeforeError[panelIndex][stationIndex] = B3_SUB_STATE_NOT_USED;
						lastTranslationDetectionStateBeforeError[panelIndex][stationIndex] = B3_TRANS_DETECT_IDLE;


/*
						for (int sIndex=0; sIndex<B3_MAX_STATION_COUNT; sIndex++)
						{
							if (gsSystem.panel[panelIndex].stationInstalled[sIndex])
							{
								int active = 0;

								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
								GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &active);

								if (!active)
								{
									SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
									break;
								}
							}
						}
//*/
					}

					deltaTimeInSec = Timer() - baseTimeForTranslationReadyInSec[panelIndex][stationIndex];

// *  *  *  *  *  *  *  *  * Detection Method switch: *  *  *  *  *  *  *  *
// *			- Peak Detection Method  (contains additional switch)  			     *
// * 	  	- Fixed Time Method  (Time Set in Procedure)    			           *
// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
					switch(gsSystem.panel[panelIndex].procedure.translationDetectionMethod)
					{
			// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
			//	CASE TYPE: B3_TRANSLATE
			//	STATE TITLE:Translate Detection Peak Method:
			//	DESCRIPTION:	This case, defined as translationDetectionMethod
			//		in the procedure, searches for the peak and triggers
			//		translation based on a predefined equation per resin.
			//			This case first determines if the maxTime for
			//		detection has been reached and raises an Error if so.
			//   		In the case of no error, another switch statement
			// 		is triggered: switch(translationDetectionState[panelIndex][stationIndex])
			//
			// 			Defined within:       B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG
			//			Next State(s):        B3_STATION_STATE_READY_FOR_TRANSLATION
			//										        B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
			//			Optional Error state: B3_STATION_TRANSLATION_READY_DELAY_ERROR
			// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
						case B3_TRANSLATE_DETECTION_PEAK_METHOD:
							if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY) &&
								(gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.maxTimeForThresholdDetectionInSeconds > 0.0) &&
								(deltaTimeInSec > gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.maxTimeForThresholdDetectionInSeconds))   // If peak has not been reached witin time => error
							{
								B3SystemLogWithVargs(gsAppLogPath, "Translation ready delay error detected for panel %d - station %d",
													panelIndex+1, stationIndex+1);

								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

								gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_TRANSLATION_READY_DELAY_ERROR;
								break;
							}
							// *  *  *  *  *  *   Detection States: *  *  *  *  *  *  *
							// *			- B3_TRANS_DETECT_IDLE   			     						  *
							// * 	  	- B3_PEAK_DETECTION														  *
							// *      - B3_TRANS_WAITING_FOR_COMPLETION						    *
							// *      - B3_TRANS_COMPLETE             						    *
							// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *   *
							switch(translationDetectionState[panelIndex][stationIndex])
							{
								// Begin Timer and Keek Timer going to calculate delta.
								// If Temp > setpoint and threshold, and delta>dead_time,
								//		go to B3_PEAK_DETECTION. Else, keep running timer.
								case B3_TRANS_DETECT_IDLE:
									if (lastTranslationDetectionState[panelIndex][stationIndex] != translationDetectionState[panelIndex][stationIndex])
									{
										lastTranslationDetectionState[panelIndex][stationIndex] = translationDetectionState[panelIndex][stationIndex];
										gsSystem.panel[panelIndex].station[stationIndex].subState = translationDetectionState[panelIndex][stationIndex];
										baseTimeInSec[panelIndex][stationIndex] = Timer();
									}

									if ((anchorTempInF > (setPointInF + gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.heatingDetectionThresholdInF)))
									{
										deltaTimeInSec = Timer() - baseTimeInSec[panelIndex][stationIndex];

										if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.deadZoneDelayInSeconds)
										{
											translationDetectionState[panelIndex][stationIndex] = B3_PEAK_DETECTION;
										}

										break;
									}
									else
									{
										baseTimeInSec[panelIndex][stationIndex] = Timer();
									}


									break;

								// Upon detection of peak, continue tracking peak upward,
								//   re-starting timer every time peak is surpassed. Record Peaks.
								// When temp is no longer climbing, go to  B3_TRANS_WAITING_FOR_COMPLETION
								case B3_PEAK_DETECTION:
									if (lastTranslationDetectionState[panelIndex][stationIndex] != translationDetectionState[panelIndex][stationIndex])
									{
										lastTranslationDetectionState[panelIndex][stationIndex] = translationDetectionState[panelIndex][stationIndex];
										gsSystem.panel[panelIndex].station[stationIndex].subState = translationDetectionState[panelIndex][stationIndex];
										baseTimeInSec[panelIndex][stationIndex] = Timer();
										peakTempInF[panelIndex][stationIndex] = anchorTempInF;
										peakTimeInSec[panelIndex][stationIndex] = Timer();
										GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempTime);
										gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempInF = peakTempInF[panelIndex][stationIndex];
									}

									if (anchorTempInF > peakTempInF[panelIndex][stationIndex])  // Peak not reached yet. Keep resetting timer until reached.
									{
										baseTimeInSec[panelIndex][stationIndex] = Timer();
										peakTempInF[panelIndex][stationIndex] = anchorTempInF;
										peakTimeInSec[panelIndex][stationIndex] = Timer();
										GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempTime);
										gsSystem.panel[panelIndex].station[stationIndex].summary.peakTempInF = anchorTempInF;
									}
									else  // Temp is no longer climbing.
									{
										deltaTimeInSec = Timer() - baseTimeInSec[panelIndex][stationIndex];

										if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.deadZoneDelayInSeconds)
										{
											translationDetectionState[panelIndex][stationIndex] = B3_TRANS_WAITING_FOR_COMPLETION;
										}
									}

									break;

								// While waiting for completion, continue to monitor for peaks.
								//   If additional peaks found, return to B3_PEAK_DETECTION State.
								// Calculate tempDropforTranDetectInF, and wait for anchorTemp to
								//    reach appropriate value. When reached, go to state B3_TRANS_COMPLETE
								//
								case B3_TRANS_WAITING_FOR_COMPLETION:
									if (lastTranslationDetectionState[panelIndex][stationIndex] != translationDetectionState[panelIndex][stationIndex])  // Initialize state.
									{
										lastTranslationDetectionState[panelIndex][stationIndex] = translationDetectionState[panelIndex][stationIndex];
										gsSystem.panel[panelIndex].station[stationIndex].subState = translationDetectionState[panelIndex][stationIndex];
										baseTimeInSec[panelIndex][stationIndex] = Timer();
									}

									if (anchorTempInF > peakTempInF[panelIndex][stationIndex])
									{
											translationDetectionState[panelIndex][stationIndex] = B3_PEAK_DETECTION;
									}
									else
									{
										double tempDropforTranDetectInF = (peakTempInF[panelIndex][stationIndex] - setPointInF) *
																   (gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetection/100.0);

										if (anchorTempInF < (peakTempInF[panelIndex][stationIndex] - tempDropforTranDetectInF))
										{
											//deltaTimeInSec = Timer() - peakTimeInSec[panelIndex][stationIndex];
											deltaTimeInSec = Timer() - baseTimeInSec[panelIndex][stationIndex];

											if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.deadZoneDelayInSeconds)
											{
												translationDetectionState[panelIndex][stationIndex] = B3_TRANS_COMPLETE;
											}
										}
										else
										{
											baseTimeInSec[panelIndex][stationIndex] = Timer();
										}
									}

									break;

								// B3_TRANS_COMPLETE Records final Time, temp, and changes state.
								//   Sets last state and basetimeinsec, then moves to next state,
								// 	 based on translation method in Procedure.
								//  Next State(s): B3_STATION_STATE_READY_FOR_TRANSLATION
								//								 B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
								//
								case B3_TRANS_COMPLETE:
									if (lastTranslationDetectionState[panelIndex][stationIndex] != translationDetectionState[panelIndex][stationIndex])
									{
										lastTranslationDetectionState[panelIndex][stationIndex] = translationDetectionState[panelIndex][stationIndex];
										gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
										baseTimeInSec[panelIndex][stationIndex] = Timer();
									}
									// Record Time and Final Temp
									GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.translationReadyTime);
									gsSystem.panel[panelIndex].station[stationIndex].summary.translationDetectTempInF = anchorTempInF;

									if (gsSystem.panel[panelIndex].procedure.translationMethod == B3_TRANSLATE_MANUAL_METHOD)
									{
										gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION;
									}
									else
									{
										gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_TRANSLATION;
									}

									break;
							}

							break;

						// In this case, if delta time equals the delay as defined
						//    in the procedure, then translation is ready.
						//  Next State(s): B3_STATION_STATE_READY_FOR_TRANSLATION
						//								 B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION
						//
						case B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD:
							if (deltaTimeInSec >= gsSystem.panel[panelIndex].procedure.fixedTimeTranslationDetectionMethodInfo.delayInSeconds)
							{
								GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.translationReadyTime);
								gsSystem.panel[panelIndex].station[stationIndex].summary.translationDetectTempInF = anchorTempInF;

								if (gsSystem.panel[panelIndex].procedure.translationMethod == B3_TRANSLATE_MANUAL_METHOD)
								{
									gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION;
								}
								else
								{
									gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_TRANSLATION;
								}
							}

							break;
					}

					break;
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	Station Ready for Translation:
//			 The defined station has been prepared for translation. This state
//			determines whether the translator is in position for translation & checks
//			with the Galil motion controller to ensure it is ready. The state will
//			result in an error if the system times out (maxTimeForTranslationReadyAckInSeconds)
//			or if the temperature of the anchor drops below (tempDropforTranErrorDetectInF)
//			the max drop value as described in the procedure.
//
//			Next State:  B3_STATION_STATE_TRANSLATING
//			Error State: B3_STATION_TRANSLATION_START_DELAY_ERROR
//
// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_READY_FOR_TRANSLATION:
				case B3_STATION_STATE_READY_FOR_TRANSLATION_EACG:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)  // If first time at state...
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						if ((gsSystem.control.translatorInPlace) &&
							(gsSystem.control.translatorPanelIndex == panelIndex) &&
							(gsSystem.control.translatorStationIndex == stationIndex) &&
						    (gsSystem.translateInfo.state == GALIL_CMD_TRANSLATE_READY))  //Check to see if ready for translation
						{
							B3SystemLogWithVargs(gsAppLogPath, "Translation automatically started alert posted for panel %d - station %d",
												panelIndex+1, stationIndex+1);
							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_TRANSLATING;  // Go to translation state if ready. else...
							break;
						}
						else
						{
							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);  // Translation system is not ready, start timer...
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							baseTimeForTranslationStartedInSec[panelIndex][stationIndex] = Timer();

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 1);																		// ...set alert and wait.

							B3SystemLogWithVargs(gsAppLogPath, "Ready for translation alert posted for panel %d - station %d",
												panelIndex+1, stationIndex+1);
						}
					}

					if ((gsSystem.control.translatorInPlace) &&
						(gsSystem.control.translatorPanelIndex == panelIndex) &&
						(gsSystem.control.translatorStationIndex == stationIndex) &&
					    (gsSystem.translateInfo.state == GALIL_CMD_TRANSLATE_READY))  // This is NOT first time around. If station is ready for translation...
					{
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);  // Post "started" Alert

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);

						B3SystemLogWithVargs(gsAppLogPath, "Translation automatically started alert posted for panel %d - station %d",
											panelIndex+1, stationIndex+1);
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_TRANSLATING;  // ... Remove errors and auto-start translation.
					}
					else
					{
						if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_READY_FOR_TRANSLATION) &&
							(gsSystem.panel[panelIndex].procedure.translationDetectionMethod == B3_TRANSLATE_DETECTION_PEAK_METHOD) &&
							(gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError > 0.0))
						{
							double tempDropforTranErrorDetectInF = (peakTempInF[panelIndex][stationIndex] - setPointInF) *
													   (gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError/100.0);

							if (anchorTempInF < (peakTempInF[panelIndex][stationIndex] - tempDropforTranErrorDetectInF))  // Temperature has dropped too far for translation.
							{																																															// ERROR ***
								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);

								B3SystemLogWithVargs(gsAppLogPath, "Translation start delay error detected for panel %d - station %d",
													panelIndex+1, stationIndex+1);

								gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_TRANSLATION_START_DELAY_ERROR;
								break;
							}
						}
						else if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_READY_FOR_TRANSLATION) &&
							(gsSystem.panel[panelIndex].procedure.translationDetectionMethod == B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD) &&
							(gsSystem.panel[panelIndex].procedure.fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds > 0.0))
						{
							double startDelayTimeInSec = Timer() - baseTimeForTranslationStartedInSec[panelIndex][stationIndex];

							if (startDelayTimeInSec >
								gsSystem.panel[panelIndex].procedure.fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds)  // Translation not started in timely manner.
							{																																																												// ERROR ***
								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

								ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
								controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

								B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);

								B3SystemLogWithVargs(gsAppLogPath, "Translation start delay error detected for panel %d - station %d",
													panelIndex+1, stationIndex+1);

								gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_TRANSLATION_START_DELAY_ERROR;
								break;
							}
						}

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &alertControlVisible);  // Post alerts for translation "ready."

						if (!alertControlVisible)
						{
							B3SystemLogWithVargs(gsAppLogPath, "Operator cleared alert for panel %d - station %d",
												panelIndex+1, stationIndex+1);
						}
					}

					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Ready for Manual Translation:
//	DESCRIPTION:	  The defined station has been prepared for translation.
//			This state is used when manual translation is defined by the procedure.
//			  The state waits for user to click the alert control on the screen
//			to indicate translation completion. It will then move to heating or
//			cool down depending on procedure.
//				The state will also go to error modes if the time surpasses the
//			delay time predefined in the procedure, or if the temperature of
//			the anchor drops too far below the peak value.
//
//			Next State(s):  B3_STATION_STATE_WAITING_FOR_HEATING_READY
//										  B3_STATION_STATE_COOL_DOWN
//			Error State(s): B3_STATION_TRANSLATION_START_DELAY_ERROR
//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION:
				case B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION_EACG:

					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

						baseTimeForTranslationStartedInSec[panelIndex][stationIndex] = Timer();  // Start Timer for manual translation alert

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 1);  // alert posted on screen and light

						B3SystemLogWithVargs(gsAppLogPath, "Ready for manual translation alert posted for panel %d - station %d",
											panelIndex+1, stationIndex+1);
					}

					if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION) &&
						(gsSystem.panel[panelIndex].procedure.translationDetectionMethod == B3_TRANSLATE_DETECTION_PEAK_METHOD) &&
						(gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError > 0.0))
					{
						double tempDropforTranErrorDetectInF = (peakTempInF[panelIndex][stationIndex] - setPointInF) *
												   (gsSystem.panel[panelIndex].procedure.peakTemperatureTranslationDetectionMethodInfo.percentTempDropForThresholdDetectionError/100.0);

						if (anchorTempInF < (peakTempInF[panelIndex][stationIndex] - tempDropforTranErrorDetectInF)) // if temp error...
						{
							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);

							B3SystemLogWithVargs(gsAppLogPath, "Translation start delay error detected for panel %d - station %d",
												panelIndex+1, stationIndex+1);

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_TRANSLATION_START_DELAY_ERROR;  // If temp has dropped too much, ERROR
							break;
						}
					}
					else if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION) &&
						(gsSystem.panel[panelIndex].procedure.translationDetectionMethod == B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD) &&
						(gsSystem.panel[panelIndex].procedure.fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds > 0.0))  //
					{
						double startDelayTimeInSec = Timer() - baseTimeForTranslationStartedInSec[panelIndex][stationIndex];

						if (startDelayTimeInSec >
							gsSystem.panel[panelIndex].procedure.fixedTimeTranslationDetectionMethodInfo.maxTimeForTranslationReadyAckInSeconds)  // If timing delay has occured
						{
							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);

							B3SystemLogWithVargs(gsAppLogPath, "Translation start delay error detected for panel %d - station %d",
												panelIndex+1, stationIndex+1);

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_TRANSLATION_START_DELAY_ERROR;  // Delay error for station start
							break;
						}
					}

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &alertControlVisible);

					if (!alertControlVisible)
					{
						B3SystemLogWithVargs(gsAppLogPath, "Operator cleared manual translation alert for panel %d - station %d",
											panelIndex+1, stationIndex+1);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
						B3SystemLogWithVargs(gsAppLogPath, "Translator support installed by operator for panel %d - station %d",
											panelIndex+1, stationIndex+1);

						if (gsSystem.panel[panelIndex].procedure.postTranslationHeatingEnabled)
						{
							// transition to B3_STATION_STATE_WAITING_FOR_HEATING_READY

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_WAITING_FOR_HEATING_READY;
						}
						else
						{
							// transition to B3_STATION_STATE_COOL_DOWN

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_COOL_DOWN;
						}
					}

					break;

			// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
			//	CASE TYPE: B3_STATION
			//	STATE TITLE: Translating:
			//	DESCRIPTION:			Begins translation with variables set from procedure.TranslationMethod.
			//			Upon first call of the function, laststate is changed to current, times
			//			are recorded and station index values are defined. Either length (LENGHT)
			//			or Load Limit methods are used in a switch statement, then the variables
			//			"translationcomplete" is set to 0 for current station. <<*** Important***
			//				Errors and alerts are cleared and the translation command is sent to
			//			Galil MC if not simulating.
			//				Galil returns a "stat" variable, which will define success or failure
			//			of the translation process by MC. Further info about the error can be
			//			found in the cmdStat variable (Values for error codes may be found in
			//		  the Galil\ Commands(gclib).h file)  Errors from Galil include:
			//			(Error if cmdStat is less than GALIL_CMD_POWER_OFF = 0)
			//					GALIL_CMD_COMM_ERROR = -102
			//					GALIL_CMD_POSITION_FOLLOWING_ERROR = -2
			//					GALIL_CMD_SYSTEM_ERROR = -1
			//						* Other errors exist but are not used here (see Galil Commands.h)
			//				If there are no errors (stat = GALIL_CMD_OK)  the function exits
			//			on first loop (?) and then returns to move to The final state case.
			//
			//			Next State(s):  B3_STATION_STATE_TRANSLATING_COMPLETE
			//
			//			Error State(s): (None)
			//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_TRANSLATING:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.translationStartTime);

						int mode = 0;
						double velocityInIPS = 0.0;
						double distanceInInches = 0.0;
						double loadInPounds = 0.0;

						switch (gsSystem.panel[panelIndex].procedure.translationMethod)
						{
							// Fixed Length Translation Method. Defined in procedure editor.
							// ***Note: Incorrect spelling of "LENGHT" in state name and
							//		MaxLoadLimitInPounts for loadInPounds Variable.
							case B3_TRANSLATE_FIXED_LENGHT_METHOD:
								mode = B3_TRANSLATE_FIXED_LENGHT_METHOD;
								velocityInIPS = gsSystem.panel[panelIndex].procedure.fixedLengthTranslationMethodInfo.velocityInIPS;
								distanceInInches = gsSystem.panel[panelIndex].procedure.fixedLengthTranslationMethodInfo.displacementInInches;
								loadInPounds = gsSystem.panel[panelIndex].procedure.fixedLengthTranslationMethodInfo.maxLoadLimitInPounts;
								break;

							default:
								// Default Translation Method is "B3_TRANSLATE_LOAD_LIMIT_METHOD"
								//   All Values defined in procedure editor process.
								mode = B3_TRANSLATE_LOAD_LIMIT_METHOD;
								velocityInIPS = gsSystem.panel[panelIndex].procedure.loadLimitTranslateMethodInfo.velocityInIPS;
								distanceInInches = gsSystem.panel[panelIndex].procedure.loadLimitTranslateMethodInfo.maxDisplacementInInches;
								loadInPounds = gsSystem.panel[panelIndex].procedure.loadLimitTranslateMethodInfo.loadLimitInPounds;
								break;

						}

						translationComplete[panelIndex][stationIndex] = 0;    // IMPORTANT: This defines the translation process as Complete.

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						B3SystemLogWithVargs(gsAppLogPath, "Translation started for panel %d - station %d",
											panelIndex+1, stationIndex+1);

#ifndef INSTALL_TRANSLATOR_SIMULATOR
						//CmtGetLock (gsGalilAsyncThreadLock);    // Threadlock used in _GALIL_SendCmdMsg function
						{
							stat = GALIL_CMD_TranslateCmd(mode,  // See Galil Commands(gclib).c line 174 [V1.2] for further Information
												   velocityInIPS,					 //    ***Note: contains functions that utilize the Galil ThreadLock.
												   distanceInInches,
												   loadInPounds,
												   0.1,
												   &cmdStat);	// Always returns status of Galil.
						}
						//CmtReleaseLock (gsGalilAsyncThreadLock);

						if (stat != GALIL_CMD_OK)  // Skip if stat = 0
						{
							GALIL_CMD_ERROR_INFO errorInfo;
							GALIL_CMD_GetLastErrorInfo(&errorInfo);
							B3SystemLog(gsAppLogPath, "Galil ERROR: Status error with the Galil controller for Translate Cmd");

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
							SetAsyncTimerAttribute (gsGalilAsyncTimerId, ASYNC_ATTR_ENABLED, 0);
							DisplayPanel(gsGalilErrorPnlHnd);
							Beep();
						}
						else if (cmdStat < GALIL_CMD_POWER_OFF)  // skip if cmdStat >= 0
						{
							GALIL_CMD_ERROR_INFO errorInfo;
							GALIL_CMD_GetLastErrorInfo(&errorInfo);

							if (cmdStat == GALIL_CMD_COMM_ERROR)
							{
								B3SystemLog(gsAppLogPath, "Galil ERROR: Comm error with the Galil controller for Translate Cmd");
							}
							else if (cmdStat == GALIL_CMD_POSITION_FOLLOWING_ERROR)
							{
								B3SystemLog(gsAppLogPath, "Galil ERROR: Position following error detected on the Galil controller for Translate Cmd");
							}
							else if (cmdStat == GALIL_CMD_SYSTEM_ERROR)
							{
								B3SystemLog(gsAppLogPath, "Galil ERROR: System error detected on the Galil controller for Translate Cmd");

								int errorNum = 0;
								int errorLine = 0;
								int errThread = 0;

								GALIL_CMD_SystemErrorInfo(&errorNum, &errorLine, &errThread, 0.1, &cmdStat);

								B3SystemLogWithVargs(gsAppLogPath, "Galil System Error Info: Error:%d at Line:%d in Thread:%d", errorNum, errorLine, errThread);
							}

							B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
							SetAsyncTimerAttribute (gsGalilAsyncTimerId, ASYNC_ATTR_ENABLED, 0);
							DisplayPanel(gsGalilErrorPnlHnd);
							Beep();  // End Error Code
						}
#else  // Simulator CtrlVal Code.
						SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATING);
						//double timeout = 0.0;
						cmdStat = 0;
#endif
					}  // End of "first use IF" Statement

					if (translationComplete[panelIndex][stationIndex] == 0) // This is set on first use. It should stay true until next state.
					{
						if ((gsSystem.translateInfo.state == GALIL_CMD_TRANSLATE_COMPLETE) || (gsSystem.translateInfo.state == GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP)) // May require additional code to be set. or, set in Sim Function
						{
							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_TRANSLATING_COMPLETE;  // Change state automatically.
							break;
						}
					}

					break;

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	//	CASE TYPE: B3_STATION
	//	STATE TITLE: Translation Complete:
	//	DESCRIPTION:			Post Translation, The Anchor must be supported by an actuator
	//			before the translation device can be removed. This state knows that
	//			the actuator is in position when the loadcell on the translation
	//			device reads less than 50lbs.
	//				The Anchor may or may not need post-translation heating. This
	//			requirement is defined in the procedure. If heating is required,
	//			the next state goes to the waiting for heating state. If not
	//			required, the program moves to the cooldown state.
	//				The program will not allow the operator to move on until the
	//			translator support actuator is installed.
	//
	//			Next State(s):  B3_STATION_STATE_WAITING_FOR_HEATING_READY
	//											B3_STATION_STATE_COOL_DOWN
	//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_TRANSLATING_COMPLETE:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						translationComplete[panelIndex][stationIndex] = 1;
						GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.translationCompleteTime);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 1);

						B3SystemLogWithVargs(gsAppLogPath, "Translation completed for panel %d - station %d",
											panelIndex+1, stationIndex+1);

					}

//					if (gsSystem.translateInfo.loadInLBS <= gsSystem.control.simTranslatorData.safeMaxLoadInLBS)
					if (gsSystem.translateInfo.loadInLBS <= 50)	// This verifies that the translator support actuator is installed
					{
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);
						B3SystemLogWithVargs(gsAppLogPath, "Translator support installed by operator for panel %d - station %d",
											panelIndex+1, stationIndex+1);

						if (gsSystem.panel[panelIndex].procedure.postTranslationHeatingEnabled)
						{
							// transition to B3_STATION_STATE_WAITING_FOR_HEATING_READY

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_WAITING_FOR_HEATING_READY;
						}
						else
						{
							// transition to B3_STATION_STATE_COOL_DOWN

							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_COOL_DOWN;
						}
					}
					else
					{
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &alertControlVisible);

						if (!alertControlVisible)
						{
//							if (gsSystem.translateInfo.loadInLBS > gsSystem.control.simTranslatorData.safeMaxLoadInLBS)
							if (gsSystem.translateInfo.loadInLBS > 50)
							{
								B3SystemLogWithVargs(gsAppLogPath, "Operator attempted to clear alert for panel %d - station %d but post translation support was not installed",
													panelIndex+1, stationIndex+1);
								Beep();
								//MessagePopup ("Translator Warning", "The translator load is above the safe move load limit. \n\nInstall post translation support then clear the alert");
								SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);
							}
						}
					}

					break;


// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE:  Waiting for Heating Ready:
//				The Anchor must cool to a particular temperature before being
//			heated. The set point for this is calculated based on a percentage
//			of the peak temperature. A timer runs while waiting for this temperature
//			to be read.
//
//			Next State(s):  B3_STATION_STATE_WAITING_FOR_HEATING_READY
//											B3_STATION_STATE_COOL_DOWN
//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_WAITING_FOR_HEATING_READY:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						baseTimeInSec[panelIndex][stationIndex] = Timer();
					}

					double tempDropforHeatingDetectInF = (peakTempInF[panelIndex][stationIndex] - setPointInF) *
											   (gsSystem.panel[panelIndex].procedure.postTranslationHeatingPeakTemperatureDropInPercent/100.0);

					if (anchorTempInF < (peakTempInF[panelIndex][stationIndex] - tempDropforHeatingDetectInF))
					{
						deltaTimeInSec = Timer() - peakTimeInSec[panelIndex][stationIndex];

						if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.postTranslationHeatingSetPointTriggerDelayInSec)
						{
							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_HEATING;
						}
					}
					else
					{
						baseTimeInSec[panelIndex][stationIndex] = Timer();
					}

					break;

				case B3_STATION_STATE_READY_FOR_HEATING:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
					}

					int readyForHeating = 1;

					for (int i=0; i<B3_MAX_STATION_COUNT; i++)  // Iterate through all stations installed in panel to check if every one is ready to heat.
					{
						if (i == stationIndex)
						{
							continue;
						}

						if (gsSystem.panel[panelIndex].stationInstalled)
						{
							if (gsSystem.panel[panelIndex].station[i].state == B3_STATION_STATE_OFF)  // B3_STATION_STATE_OFF = 0
							{
								continue;
							}
							else if ((gsSystem.panel[panelIndex].station[i].state < B3_STATION_STATE_OFF) ||          // If state is a negative number (ERROR)
									 (gsSystem.panel[panelIndex].station[i].state >= B3_STATION_STATE_STABILIZING_EACG))  // If state is greater than 101
							{
								readyForHeating = 0;
								break;
							}
							else if (gsSystem.panel[panelIndex].station[i].state < B3_STATION_STATE_READY_FOR_HEATING) // If state is Less  than 9 (anything other than heating already, cooling, or complete)
							{
								readyForHeating = 0;
								break;
							}
							else if (gsSystem.panel[panelIndex].station[i].state == B3_STATION_STATE_HEATING) // If state = 11, ready for heating.
							{
								readyForHeating = 1;
								break;
							}
						}
					}

					if (readyForHeating)
					{
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_HEATING;  // Change station state to heating.
					}

					break;

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	//	CASE TYPE: B3_STATION
	//	STATE TITLE: Heating
	//	DESCRIPTION:		The heating process is strictly time based. The heating
	//			process duration is set during the procedure editing process. When
	//			heating is complete, the process is complete. The program goes to
	//			the final state.
	//
	//			Next State(s):  B3_STATION_STATE_COMPLETE
	//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_HEATING:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						baseTimeInSec[panelIndex][stationIndex] = Timer();
						gsSystem.panel[panelIndex].postTranslationHeating = 1;
						baseTimeForStartOfHeatingInSec[panelIndex][stationIndex] = Timer();
					}

					deltaTimeInSec = Timer() - baseTimeForStartOfHeatingInSec[panelIndex][stationIndex];

					if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.postTranslationHeatingTimeInSeconds)
					{
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_COMPLETE;
					}

					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Cool Down
//	DESCRIPTION:		If post-translation heating is not required by the
//			procedure, the anchor continues through a cool down cycle. The state
//			involves the use of a timer which tracks the length of the cool down.
//				Target temperature for cool down is to be within the tolerance of
//			the "set point" and the maximum cool down length is defined in the
//			procedure. If cool down takes too long, a delay error is raised.
//			If cool down occurs within defined time, goes to Completion state.
//
//			Next State(s):  B3_STATION_STATE_COMPLETE
//
//			Error State(s): B3_STATION_COOLDOWN_READY_DELAY_ERROR
//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_COOL_DOWN:
				case B3_STATION_STATE_COOL_DOWN_EACG:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
						baseTimeInSec[panelIndex][stationIndex] = Timer();

						if (lastStateBeforeError[panelIndex][stationIndex] != B3_STATION_STATE_COOL_DOWN)
						{
							baseTimeForCoolDownCompleteInSec[panelIndex][stationIndex] = Timer();
						}

						lastStateBeforeError[panelIndex][stationIndex] = B3_STATION_STATE_OFF;
						lastSubStateBeforeError[panelIndex][stationIndex] = B3_SUB_STATE_NOT_USED;
						lastTranslationDetectionStateBeforeError[panelIndex][stationIndex] = B3_TRANS_DETECT_IDLE;
					}

					deltaTimeInSec = Timer() - baseTimeForCoolDownCompleteInSec[panelIndex][stationIndex];

					if ((gsSystem.panel[panelIndex].station[stationIndex].state == B3_STATION_STATE_COOL_DOWN) &&
						(gsSystem.panel[panelIndex].procedure.maxCoolDownTimeInSeconds > 0.0) &&
						(deltaTimeInSec > gsSystem.panel[panelIndex].procedure.maxCoolDownTimeInSeconds))
					{
						B3SystemLogWithVargs(gsAppLogPath, "Cool down ready delay error detected for panel %d - station %d",
											panelIndex+1, stationIndex+1);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_COOLDOWN_READY_DELAY_ERROR;
						break;
					}

					if (anchorTempInF < (setPointInF + setPointToleranceInF))
					{
						deltaTimeInSec = Timer() - baseTimeInSec[panelIndex][stationIndex];

						if (deltaTimeInSec > gsSystem.panel[panelIndex].procedure.coolDownReadyDelayInSec)
						{
							GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.completionTime);
							gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_COMPLETE;
						}
					}
					else
					{
						baseTimeInSec[panelIndex][stationIndex] = Timer();
					}

					break;

// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
//	CASE TYPE: B3_STATION
//	STATE TITLE: Complete
//	DESCRIPTION:		When an anchor is complete, Alerts are set to notify
//			the operator of completion. When the operator clicks the alert to
//			hide it, the system changes to the OFF state.
//
//			Next State(s):  B3_STATION_STATE_OFF
//  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
				case B3_STATION_STATE_COMPLETE:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 1);

						gsSystem.panel[panelIndex].postTranslationHeating = 0;

						B3SystemLogWithVargs(gsAppLogPath, "Station complete alert posted for panel %d - station %d",
											panelIndex+1, stationIndex+1);
					}

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &alertControlVisible);

					if (!alertControlVisible)
					{
						B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ALERT, 0);

						GetCurrentCVIAbsoluteTime(&gsSystem.panel[panelIndex].station[stationIndex].summary.opAckOfCompletionTime);
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_OFF;
					}

					break;

				default:
					if (lastState[panelIndex][stationIndex] != gsSystem.panel[panelIndex].station[stationIndex].state)
					{
						lastState[panelIndex][stationIndex] = gsSystem.panel[panelIndex].station[stationIndex].state;
						gsSystem.panel[panelIndex].station[stationIndex].subState = B3_SUB_STATE_NOT_USED;
						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, gsSystem.panel[panelIndex].station[stationIndex].state);
					}

					// transition to B3_STATION_STATE_ERROR

					gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_ERROR;
					break;
			}

			break;
	}

	return(0);
}
//-------------------------------------------------------------------------
//	FUNCTION: B3SystemControl
//
//  DESCRIPTION:
//
//  INPUT: int operation, int opId, int state
//-------------------------------------------------------------------------
int32 B3SystemControl(int operation, int opId, int state)
{
	uInt8 logicState = 0;
	int daqError = 0;
	TaskHandle taskHandle=0;

	if (state)
	{
		logicState = B3_DOUT_LOGIC_ON;
	}
	else
	{
		logicState = B3_DOUT_LOGIC_OFF;
	}

	if (operation == B3_SYSTEM_LIGHTS)
	{
		char systemChan[B3_MAX_DEVICE_NAME_LENGTH] = {0};
		uInt8 systemData = (uInt8)logicState;

		switch(opId)
		{
			case B3_SYSTEM_RUNNING:
				strcpy(systemChan, gsSystem.control.systemOnDevice);

				if (state)
				{
					gsSystem.control.systemOn = state;
					systemData = logicState;
					SetCtrlVal(gsSystemPnlHndl, MACHINE_RUNNING_LED, state);
				}
				else
				{
					int controlActive = 0;

					for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
					{
						if (gsSystem.panelInstalled[panelIndex])
						{
							for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
							{
								if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
								{
									int ctrlArrayHnd = 0;
									int controlId = 0;
									int active = 0;

									ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
									controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
									GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &active);

									if (active)
									{
										controlActive = 1;
										break;
									}
								}
							}
						}

						if (controlActive)
						{
							break;
						}
					}

					if (!controlActive)
					{
						gsSystem.control.systemOn = 0;
						systemData = B3_DOUT_LOGIC_OFF;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_RUNNING_LED, 0);
					}
					else
					{
						gsSystem.control.systemOn = 1;
						systemData = B3_DOUT_LOGIC_ON;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_RUNNING_LED, 1);
					}
				}

				break;

			case B3_SYSTEM_ALERT:
				strcpy(systemChan, gsSystem.control.systemAlertDevice);

				if (state)
				{
					gsSystem.control.systemAlert = state;
					systemData = logicState;
					SetCtrlVal(gsSystemPnlHndl, MACHINE_ALERT_LED, state);
					Beep();
				}
				else
				{
					int alertActive = 0;

					for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
					{
						if (gsSystem.panelInstalled[panelIndex])
						{
							for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
							{
								if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
								{
									int ctrlArrayHnd = 0;
									int controlId = 0;
									int visible = 0;

									ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
									controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
									GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &visible);

									if (visible)
									{
										alertActive = 1;
										break;
									}
								}
							}
						}

						if (alertActive)
						{
							break;
						}
					}

					if (!alertActive)
					{
						gsSystem.control.systemAlert = 0;
						systemData = B3_DOUT_LOGIC_OFF;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_ALERT_LED, 0);
					}
					else
					{
						gsSystem.control.systemAlert = 1;
						systemData = B3_DOUT_LOGIC_ON;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_ALERT_LED, 1);
					}
				}

				break;

			case B3_SYSTEM_ERROR:
				strcpy(systemChan, gsSystem.control.systemErrorDevice);

				if (state)
				{
					gsSystem.control.systemError = state;
					systemData = logicState;
					SetCtrlVal(gsSystemPnlHndl, MACHINE_ERROR_LED, state);
					Beep();
				}
				else
				{
					int errorActive = 0;

					for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
					{
						if (gsSystem.panelInstalled[panelIndex])
						{
							for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
							{
								if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
								{
									int ctrlArrayHnd = 0;
									int controlId = 0;
									int visible = 0;

									ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
									controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
									GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &visible);

									if (visible)
									{
										errorActive = 1;
										break;
									}
								}
							}
						}

						if (errorActive)
						{
							break;
						}
					}

					if (gsSystem.translateInfo.state < 0)
					{
						errorActive = 1;
					}

					if (!errorActive)
					{
						gsSystem.control.systemError = 0;
						systemData = B3_DOUT_LOGIC_OFF;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_ERROR_LED, 0);
					}
					else
					{
						gsSystem.control.systemError = 1;
						systemData = B3_DOUT_LOGIC_ON;
						SetCtrlVal(gsSystemPnlHndl, MACHINE_ERROR_LED, 1);
					}
				}

				break;

			default:
				break;
		}

		CmtGetLock (gsDAQmxDOutLock);
		{
			daqError = DAQmxCreateTask("DOutTaskForSystemLights",&taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			daqError = DAQmxCreateDOChan (taskHandle, systemChan, "", DAQmx_Val_ChanPerLine);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateDOChan Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateDOChan Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Start Code
			/*********************************************/
			daqError = DAQmxStartTask(taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Write Code
			/*********************************************/
			daqError = DAQmxWriteDigitalLines (taskHandle, 1, 1, 0, DAQmx_Val_GroupByChannel,
												 &systemData, NULL, NULL);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxWriteDigitalLines Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxWriteDigitalLines Error (see app log for details) - exiting app");
				goto EXIT;
			}

			if( taskHandle!=0 )
			{
				/*********************************************/
				// DAQmx Stop Code
				/*********************************************/
				DAQmxStopTask(taskHandle);
				DAQmxClearTask(taskHandle);
				taskHandle = 0;
			}
		}
		CmtReleaseLock (gsDAQmxDOutLock);

		return(daqError);
	}
	else
	{
		int panelIndex = opId;

		char coolingChan[B3_MAX_DEVICE_NAME_LENGTH] = {0};
		char heatingChan[B3_MAX_DEVICE_NAME_LENGTH] = {0};
		uInt8 coolingData = 0;
		uInt8 heatingData = 0;

		strcpy(coolingChan, gsSystem.control.panelCoolingDevice[panelIndex]);
		strcpy(heatingChan, gsSystem.control.panelHeatingDevice[panelIndex]);

		switch(operation)
		{
			case B3_PANEL_COOLING:
				coolingData = B3_DOUT_LOGIC_ON;
				heatingData = B3_DOUT_LOGIC_OFF;
				break;

			case B3_PANEL_HEATING:
				coolingData = B3_DOUT_LOGIC_OFF;
				heatingData = B3_DOUT_LOGIC_ON;
				break;

			case B3_PANEL_VORTEX_OFF:
			default:
				coolingData = B3_DOUT_LOGIC_OFF;
				heatingData = B3_DOUT_LOGIC_OFF;
				break;
		}

		CmtGetLock (gsDAQmxDOutLock);
		{
			daqError = DAQmxCreateTask("DOutTaskForCoolingControl",&taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			daqError = DAQmxCreateDOChan (taskHandle, coolingChan, "", DAQmx_Val_ChanPerLine);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateDOChan Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateDOChan Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Start Code
			/*********************************************/
			daqError = DAQmxStartTask(taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Write Code
			/*********************************************/
			daqError = DAQmxWriteDigitalLines (taskHandle, 1, 1, 0, DAQmx_Val_GroupByChannel,
												 &coolingData, NULL, NULL);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxWriteDigitalLines Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxWriteDigitalLines Error (see app log for details) - exiting app");
				goto EXIT;
			}

			if( taskHandle!=0 ) {
				/*********************************************/
				// DAQmx Stop Code
				/*********************************************/
				DAQmxStopTask(taskHandle);
				DAQmxClearTask(taskHandle);
				taskHandle = 0;
			}

			daqError = DAQmxCreateTask("DOutTaskForHeatingControl",&taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			daqError = DAQmxCreateDOChan (taskHandle, heatingChan, "", DAQmx_Val_ChanPerLine);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxCreateDOChan Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxCreateDOChan Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Start Code
			/*********************************************/
			daqError = DAQmxStartTask(taskHandle);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxStartTask Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxStartTask Error (see app log for details) - exiting app");
				goto EXIT;
			}

			/*********************************************/
			// DAQmx Write Code
			/*********************************************/
			daqError = DAQmxWriteDigitalLines (taskHandle, 1, 1, 0, DAQmx_Val_GroupByChannel,
												 &heatingData, NULL, NULL);

			if (daqError < 0)
			{
				int32 bufSize = 0;

				bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

				char daqErrorStr[bufSize];

				DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
				LOG_ERROR_WITH_VARGS(daqError, "DAQmxWriteDigitalLines Error: <%s>", daqErrorStr);
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("FATAL DAQmx ERROR", "DAQmxWriteDigitalLines Error (see app log for details) - exiting app");
				goto EXIT;
			}

			if( taskHandle!=0 ) {
				/*********************************************/
				// DAQmx Stop Code
				/*********************************************/
				DAQmxStopTask(taskHandle);
				DAQmxClearTask(taskHandle);
				taskHandle = 0;
			}
		}
		CmtReleaseLock (gsDAQmxDOutLock);
	}

EXIT:
	int getLock = 0;

	CmtTryToGetLock (gsDAQmxDOutLock, &getLock);
	CmtReleaseLock (gsDAQmxDOutLock);

	return(daqError);
}

int CVICALLBACK GalilAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2)
{
	if (event == EVENT_TIMER_TICK)
	{
		int stat = 0;
		int state = 0;
		double absolutePositionInInches = 0.0;
		double loadInPounds = 0.0;
		double timeStampInSeconds = 0.0;
		int cmdStat = 0;
		static lastState = GALIL_CMD_POWER_OFF;
		int panelId = 0;
		int stationId = 0;
		int panelIndex = 0;
		int stationIndex = 0;

#ifdef INSTALL_TRANSLATOR_SIMULATOR
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PANEL_ID, &panelId);
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, &stationId);
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, &state);
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, &loadInPounds);
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, &absolutePositionInInches);
		stat = GALIL_CMD_OK;
		timeStampInSeconds = Timer();
		cmdStat = GALIL_CMD_OK;
#else
		//CmtGetLock (gsGalilAsyncThreadLock);
		{
			stat = GALIL_CMD_ReadMachineData(&state,
											&absolutePositionInInches,
											&loadInPounds,
											&timeStampInSeconds,
											0.1,
											&cmdStat);
		}
		//CmtReleaseLock (gsGalilAsyncThreadLock);

		if (stat != GALIL_CMD_OK)
		{
			SetAsyncTimerAttribute (gsGalilAsyncTimerId, ASYNC_ATTR_ENABLED, 0);
			GALIL_CMD_ERROR_INFO errorInfo;
			GALIL_CMD_GetLastErrorInfo(&errorInfo);
			B3SystemLog(gsAppLogPath, "Galil ERROR: Status error with the Galil controller for Read Machine Data");
			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
			B3SystemLog(gsAppLogPath, "Galil ERROR: Unable to read the machine data");
			DisplayPanel(gsGalilErrorPnlHnd);
			Beep();
		}
		else if (cmdStat < GALIL_CMD_POWER_OFF)
		{
			GALIL_CMD_ERROR_INFO errorInfo;
			GALIL_CMD_GetLastErrorInfo(&errorInfo);

			if (cmdStat == GALIL_CMD_COMM_ERROR)
			{
				B3SystemLog(gsAppLogPath, "Galil ERROR: Comm error  with the Galil controller for Read Machine Data Cmd");
			}
			else if (cmdStat == GALIL_CMD_POSITION_FOLLOWING_ERROR)
			{
				B3SystemLog(gsAppLogPath, "Galil ERROR: Position following error detected on the Galil controller for Read Machine Data Cmd");
			}
			else if (cmdStat == GALIL_CMD_SYSTEM_ERROR)
			{
				B3SystemLog(gsAppLogPath, "Galil ERROR: System error detected on the Galil controller for Read Machine Data Cmd");

				int errorNum = 0;
				int errorLine = 0;
				int errThread = 0;

				GALIL_CMD_SystemErrorInfo(&errorNum, &errorLine, &errThread, 0.1, &cmdStat);

				B3SystemLogWithVargs(gsAppLogPath, "Galil System Error Info: Error:%d at Line:%d in Thread:%d", errorNum, errorLine, errThread);
			}

			state = cmdStat;
		}

		GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
		GetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, &stationId);
#endif

		panelIndex = panelId-1;
		stationIndex = stationId-1;

		gsSystem.translateInfo.state = state;
		gsSystem.translateInfo.loadInLBS = loadInPounds;
		gsSystem.translateInfo.displacementInInches = absolutePositionInInches;

		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATUS, state);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_LOAD, loadInPounds);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_DISP, absolutePositionInInches);

		if (state != lastState)
		{
			if (state < GALIL_CMD_POWER_OFF)
			{
				SetAsyncTimerAttribute (gsGalilAsyncTimerId, ASYNC_ATTR_ENABLED, 0);
				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 1);
				B3SystemLogWithVargs(gsAppLogPath, "Galil ERROR: Galil State = %d", state);
				DisplayPanel(gsGalilErrorPnlHnd);
				Beep();
			}
			else if (lastState < GALIL_CMD_POWER_OFF)
			{
				B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);
			}
		}

		switch(state)
		{
			case GALIL_CMD_POWER_OFF:
			case GALIL_CMD_POWER_ON_NOT_HOMED:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute (gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_WHITE);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 1);
				}

				break;

			case GALIL_CMD_POWER_ON_HOMED:
			case GALIL_CMD_POWER_ON:
			case GALIL_CMD_AT_PARK:
			case GALIL_CMD_MOTION_HALTED:
			case GALIL_CMD_SETTING_POWER_STATE:
			case GALIL_CMD_PENDANT_ACTIVE:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_WHITE);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 0);
				}

				GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
				break;

			case GALIL_CMD_HOMING:
			case GALIL_CMD_MOVING_TO_POSITION:
			case GALIL_CMD_JOGGING:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_YELLOW);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 0);
				}

				GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
				break;

			case GALIL_CMD_TRANSLATE_READY:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_GREEN);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 0);

					B3SystemLogWithVargs(gsAppLogPath, "Translator ready for panel %d - station %d",
										panelIndex+1, stationIndex+1);
				}

				GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
				GetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, &stationId);
				panelIndex = panelId-1;
				stationIndex = stationIndex-1;
				break;

			case GALIL_CMD_TRANSLATING:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_YELLOW);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 0);
				}

				{
					char logPath[MAX_PATHNAME_LEN];

					strcpy(logPath, gsLogPath[panelIndex][stationIndex]);
					strcat(logPath, ".tlg");

					GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
					GetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, &stationId);
					panelIndex = panelId-1;
					stationIndex = stationIndex-1;

					B3TranslationLog(logPath,
									 loadInPounds,
									 absolutePositionInInches);
				}

				break;

			case GALIL_CMD_TRANSLATE_COMPLETE:
			case GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_WHITE);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 0);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 0);

					char logPath[MAX_PATHNAME_LEN];

					strcpy(logPath, gsLogPath[panelIndex][stationIndex]);
					strcat(logPath, ".tlg");

					GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
					GetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, &stationId);
					panelIndex = panelId-1;
					stationIndex = stationIndex-1;

					B3TranslationLog(logPath,
									 loadInPounds,
									 absolutePositionInInches);
				}

				GetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, &panelId);
				break;

			case GALIL_CMD_SYSTEM_ERROR:
			case GALIL_CMD_POSITION_FOLLOWING_ERROR:
			case GALIL_CMD_COMM_ERROR:
			default:
				if (lastState != state)
				{
					lastState = state;
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATUS, ATTR_TEXT_BGCOLOR, VAL_RED);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_PANEL_ID, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_STATION_ID, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_LOAD, ATTR_DIMMED, 1);
					SetCtrlAttribute(gsTranslatePnlHndl, TRANSL_DISP, ATTR_DIMMED, 1);
				}

				break;
		}
	}

	return(0);
}

void PanelAutoArrange(void)
{
	int mainTop = 0;
	int mainLeft = 0;
	int mainTitleBarHeight = 0;
	int mainMenuHeight = 0;
	int mainFrameHeight = 0;
	int mainFrameWidth = 0;

	int titleBarHeight = 0;
	int menuHeight = 0;
	int frameHeight = 0;
	int frameWidth = 0;

	// get panel attribute data to be used for panel positioning

	GetPanelAttribute (gsMainPnlHndl, ATTR_TOP, &mainTop);
	GetPanelAttribute (gsMainPnlHndl, ATTR_LEFT, &mainLeft);
	GetPanelAttribute (gsMainPnlHndl, ATTR_TITLEBAR_ACTUAL_THICKNESS, &mainTitleBarHeight);
	GetPanelAttribute (gsMainPnlHndl, ATTR_MENU_HEIGHT, &mainMenuHeight);
	GetPanelAttribute (gsMainPnlHndl, ATTR_FRAME_ACTUAL_HEIGHT, &mainFrameHeight);
	GetPanelAttribute (gsMainPnlHndl, ATTR_FRAME_ACTUAL_WIDTH, &mainFrameWidth);

	// position the SYSTEM panel to the default location

	GetPanelAttribute (gsSystemPnlHndl, ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
	GetPanelAttribute (gsSystemPnlHndl, ATTR_MENU_HEIGHT, &menuHeight);
	GetPanelAttribute (gsSystemPnlHndl, ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
	GetPanelAttribute (gsSystemPnlHndl, ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

	gsDefaultSystemPanelPosition[UI_PANEL_TOP] = mainTop+mainFrameHeight+mainTitleBarHeight+mainMenuHeight+PANEL_SPACING;
	gsDefaultSystemPanelPosition[UI_PANEL_LEFT] = mainLeft+mainFrameWidth+PANEL_SPACING;

	if (gsDefaultSystemPanelPosition[UI_PANEL_HEIGHT] == 0 &&
		gsDefaultSystemPanelPosition[UI_PANEL_WIDTH] == 0)
	{
		GetPanelAttribute (gsSystemPnlHndl, ATTR_HEIGHT, &gsDefaultSystemPanelPosition[UI_PANEL_HEIGHT]);
		gsDefaultSystemPanelPosition[UI_PANEL_TOTAL_HEIGHT] = gsDefaultSystemPanelPosition[UI_PANEL_HEIGHT] +
																titleBarHeight +
																menuHeight +
																(2 * frameHeight);
		GetPanelAttribute (gsSystemPnlHndl, ATTR_WIDTH, &gsDefaultSystemPanelPosition[UI_PANEL_WIDTH]);
		gsDefaultSystemPanelPosition[UI_PANEL_TOTAL_WIDTH] = gsDefaultSystemPanelPosition[UI_PANEL_WIDTH] +
																(2 * frameWidth);
	}

	SetPanelAttribute (gsSystemPnlHndl, ATTR_TOP, gsDefaultSystemPanelPosition[UI_PANEL_TOP]);
	SetPanelAttribute (gsSystemPnlHndl, ATTR_LEFT, gsDefaultSystemPanelPosition[UI_PANEL_LEFT]);
	DisplayPanel (gsSystemPnlHndl);

	// position the TRANSL panel to the default location

	GetPanelAttribute (gsTranslatePnlHndl, ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_MENU_HEIGHT, &menuHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

	gsDefaultTranslatorPanelPosition[UI_PANEL_TOP] = gsDefaultSystemPanelPosition[UI_PANEL_TOP]+gsDefaultSystemPanelPosition[UI_PANEL_TOTAL_HEIGHT]+PANEL_SPACING;
	gsDefaultTranslatorPanelPosition[UI_PANEL_LEFT] = gsDefaultSystemPanelPosition[UI_PANEL_LEFT];

	GetPanelAttribute (gsTranslatePnlHndl, ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_MENU_HEIGHT, &menuHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
	GetPanelAttribute (gsTranslatePnlHndl, ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

	if (gsDefaultTranslatorPanelPosition[UI_PANEL_HEIGHT] == 0 &&
		gsDefaultTranslatorPanelPosition[UI_PANEL_WIDTH] == 0)
	{
		GetPanelAttribute (gsTranslatePnlHndl, ATTR_HEIGHT, &gsDefaultTranslatorPanelPosition[UI_PANEL_HEIGHT]);
		gsDefaultTranslatorPanelPosition[UI_PANEL_TOTAL_HEIGHT] = gsDefaultTranslatorPanelPosition[UI_PANEL_HEIGHT] +
																titleBarHeight +
																menuHeight +
																(2 * frameHeight);
		GetPanelAttribute (gsTranslatePnlHndl, ATTR_WIDTH, &gsDefaultTranslatorPanelPosition[UI_PANEL_WIDTH]);
		gsDefaultTranslatorPanelPosition[UI_PANEL_TOTAL_WIDTH] = gsDefaultTranslatorPanelPosition[UI_PANEL_WIDTH] +
																(2 * frameWidth);
	}

	SetPanelAttribute (gsTranslatePnlHndl, ATTR_TOP, gsDefaultTranslatorPanelPosition[UI_PANEL_TOP]);
	SetPanelAttribute (gsTranslatePnlHndl, ATTR_LEFT, gsDefaultTranslatorPanelPosition[UI_PANEL_LEFT]);
	DisplayPanel (gsTranslatePnlHndl);

	for (int panelIndex=0, panelCount=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		if (gsSystem.panelInstalled[panelIndex])
		{
			GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
			GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_MENU_HEIGHT, &menuHeight);
			GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
			GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

			if (gsDefaultB3PanelPosition[panelIndex][UI_PANEL_HEIGHT] == 0 &&
				gsDefaultB3PanelPosition[panelIndex][UI_PANEL_WIDTH] == 0)
			{
				GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_HEIGHT, &gsDefaultB3PanelPosition[panelIndex][UI_PANEL_HEIGHT]);
				gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_HEIGHT] = gsDefaultB3PanelPosition[panelIndex][UI_PANEL_HEIGHT] +
																			menuHeight +
																			(2 * frameHeight);
				GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_WIDTH, &gsDefaultB3PanelPosition[panelIndex][UI_PANEL_WIDTH]);
				gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_WIDTH] = gsDefaultB3PanelPosition[panelIndex][UI_PANEL_WIDTH] +
																			(2 * frameWidth);
			}

			gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOP] = gsDefaultSystemPanelPosition[UI_PANEL_TOP];
			gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT] =
										gsDefaultTranslatorPanelPosition[UI_PANEL_LEFT] +
										gsDefaultTranslatorPanelPosition[UI_PANEL_TOTAL_WIDTH] +
										PANEL_SPACING +
										(panelCount * (PANEL_SPACING + gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_WIDTH]));
			SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_TOP, gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOP]);
			SetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_LEFT, gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT]);
			DisplayPanel(gsB3PanelPnlHndl[panelIndex]);

			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT;	stationIndex++)
			{
				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex] && gsSystem.graphsEnabled)
				{
					GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
					GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_MENU_HEIGHT, &menuHeight);
					GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
					GetPanelAttribute (gsB3PanelPnlHndl[panelIndex], ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

					if (gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT] == 0 &&
						gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH] == 0)
					{
						GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, &gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT]);
						gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOTAL_HEIGHT] = gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT] +
																									 + titleBarHeight +
																									(2 * frameHeight);
						gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH] = (gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_WIDTH]/2)-(2*frameWidth);
						gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOTAL_WIDTH] = gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH] +
																									(2 * frameWidth);
					}

					switch(stationIndex)
					{
						case 0:
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOP] +
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_HEIGHT];
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT];
							break;

						case 1:
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOP] +
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_TOTAL_HEIGHT];
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT] +
										gsDefaultB3GraphPosition[panelIndex][1][UI_PANEL_TOTAL_WIDTH];
						break;

						case 2:
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP] =
										gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_TOP] +
										gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_TOTAL_HEIGHT];
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT];
							break;

						case 3:
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP] =
										gsDefaultB3GraphPosition[panelIndex][1][UI_PANEL_TOP] +
										gsDefaultB3GraphPosition[panelIndex][1][UI_PANEL_TOTAL_HEIGHT];
							gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT] =
										gsDefaultB3PanelPosition[panelIndex][UI_PANEL_LEFT] +
										gsDefaultB3GraphPosition[panelIndex][2][UI_PANEL_TOTAL_WIDTH];
							break;
					}

					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TOP, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_LEFT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT]);
					SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH]);
					DisplayPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
				}
				else
				{
					if (gsB3GraphPnlHndl[panelIndex][stationIndex])
					{
						DiscardPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
						gsB3GraphPnlHndl[panelIndex][stationIndex] = 0;
					}
				}
			}

			panelCount++;
		}
	}

	return;
}

int CVICALLBACK ResizeStripChart (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	int panelIndex = -1;
	int stationIndex = -1;
	int panelWidth = 0;
	int panelHeight = 0;

	switch (event)
	{
		case EVENT_LEFT_DOUBLE_CLICK:
			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				for (int j=0; j<B3_MAX_STATION_COUNT; j++)
				{
					if (panel == gsB3GraphPnlHndl[i][j])
					{
						panelIndex = i;
						stationIndex = j;
						break;
					}
				}
			}

			int mainMenuHeight = 0;
			int titleBarHeight = 0;
			int frameHeight = 0;
			int frameWidth = 0;

			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TITLEBAR_ACTUAL_THICKNESS, &titleBarHeight);
			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_MENU_HEIGHT, &mainMenuHeight);
			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_FRAME_ACTUAL_HEIGHT, &frameHeight);
			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_FRAME_ACTUAL_WIDTH, &frameWidth);

			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, &panelHeight);
			panelHeight += mainMenuHeight;

			GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, &panelWidth);

			if (panelWidth == gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH])
			{
				int top = gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_TOP];
				int left = gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_LEFT];
				int height = (2 * gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_TOTAL_HEIGHT]) - (2 * frameHeight) - titleBarHeight - mainMenuHeight;
				int width = (2 * gsDefaultB3GraphPosition[panelIndex][0][UI_PANEL_TOTAL_WIDTH]) - (2 * frameWidth);

				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TOP, top);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_LEFT, left);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, height);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, width);

			}
			else
			{
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_TOP, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_TOP]);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_LEFT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_LEFT]);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_HEIGHT, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_HEIGHT]);
				SetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH]);
			}

			DisplayPanel(gsB3GraphPnlHndl[panelIndex][stationIndex]);
			HidePanel(gsChartScalePnlHnd);
			break;

		case EVENT_RIGHT_DOUBLE_CLICK:
			int visible = 0;

			GetPanelAttribute (gsChartScalePnlHnd, ATTR_VISIBLE, &visible);

			if (!visible)
			{
				for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
				{
					for (int j=0; j<B3_MAX_STATION_COUNT; j++)
					{
						if (panel == gsB3GraphPnlHndl[i][j])
						{
							panelIndex = i;
							stationIndex = j;
							break;
						}
					}
				}

				GetPanelAttribute (gsB3GraphPnlHndl[panelIndex][stationIndex], ATTR_WIDTH, &panelWidth);

				if (panelWidth != gsDefaultB3GraphPosition[panelIndex][stationIndex][UI_PANEL_WIDTH])
				{
					int mode = 0;
					double minTempInF = 0.0;
					double maxTempInF = 0.0;

					GetAxisScalingMode (gsB3GraphPnlHndl[panelIndex][stationIndex],
										GRAPH_STRIPCHART, VAL_LEFT_YAXIS, &mode,
										&minTempInF,
										&maxTempInF);

					SetCtrlVal(gsChartScalePnlHnd, CHRT_SETUP_PANEL, panelIndex+1);
					SetCtrlVal(gsChartScalePnlHnd, CHRT_SETUP_STATION, stationIndex+1);
					SetCtrlVal(gsChartScalePnlHnd, CHRT_SETUP_MIN_TEMP, minTempInF);
					SetCtrlVal(gsChartScalePnlHnd, CHRT_SETUP_MAX_TEMP, maxTempInF);

					DisplayPanel(gsChartScalePnlHnd);
				}
			}

			break;
	}

	return 0;
}

int CVICALLBACK CloseAbout (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if (gsDisclaimerPnlHnd)
			{
				DiscardPanel(gsDisclaimerPnlHnd);
				gsDisclaimerPnlHnd = 0;
			}

			DiscardPanel(gsAboutPnlHnd);
			gsAboutPnlHnd = 0;
			break;
	}
	return 0;
}

void CVICALLBACK DisplayAbout (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	if (gsAboutPnlHnd == 0)
	{
		gsAboutPnlHnd = LoadPanel(gsMainPnlHndl,  "BORIS 3.uir", ABOUT);
		SetCtrlVal(gsAboutPnlHnd, ABOUT_VERSION, gsProductVersion);
	}

	Beep();
	DisplayPanel(gsAboutPnlHnd);
	return;
}

int CVICALLBACK ShowDisclaimer (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if (gsDisclaimerPnlHnd == 0)
			{
				gsDisclaimerPnlHnd = LoadPanel(gsMainPnlHndl,  "BORIS 3.uir", DISCLAIMER);
			}

			Beep();
			DisplayPanel(gsDisclaimerPnlHnd);
			break;
	}
	return 0;
}

int CVICALLBACK CloseDisclaimer (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			DiscardPanel(gsDisclaimerPnlHnd);
			gsDisclaimerPnlHnd = 0;
			break;
	}
	return 0;
}

void PanelStatusUpdate(int panelIndex)
{
	if (!gsSystem.panelInstalled[panelIndex])
	{
		if (gsB3PanelPnlHndl[panelIndex])
		{
			DiscardPanel(gsB3PanelPnlHndl[panelIndex]);
			gsB3PanelPnlHndl[panelIndex] = 0;
		}

		return;
	}

	int ctrlArrayHnd = 0;
	int controlId = 0;
	//int serialNumEntered = 0;
	//int stationControl[B3_MAX_STATION_COUNT] = {0};

	for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
	{
		int boxColor = 0;

		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_PANEL);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
		GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &boxColor);

		if (!gsSystem.panel[panelIndex].stationInstalled[stationIndex])
		{
			if (boxColor != VAL_GRAY)
			{
				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "");
				strcpy(gsSystem.panel[panelIndex].station[stationIndex].serialNum, "");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId, ATTR_FRAME_COLOR, VAL_GRAY);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0.0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], RUN_TIME_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "00:00:00");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, B3_STATION_STATE_OFF);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
			}
		}
		else
		{
			if (boxColor == VAL_GRAY)
			{
				int frameColor;

				GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_PANEL, ATTR_FRAME_COLOR, &frameColor);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BOX_STATION_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
				SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], controlId, ATTR_FRAME_COLOR, frameColor);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], BANNER_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, B3_STATION_STATE_OFF);
			}
		}
	}

	if (!stricmp(gsSystem.panel[panelIndex].activeProcedure, ""))
	{
		char procName[128];

		GetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, procName);

		if (stricmp(procName, "Select Procedure"))
		{
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, "Select Procedure");
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 1);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 0);

			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				strcpy(gsSystem.panel[panelIndex].station[stationIndex].serialNum, "");
			}

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
			SetCtrlArrayVal (ctrlArrayHnd, "");

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
			SetCtrlArrayAttribute(ctrlArrayHnd , controlId, ATTR_DIMMED, 1);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], RUN_TIME_CA);
			SetCtrlArrayAttribute(ctrlArrayHnd , controlId, ATTR_DIMMED, 1);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
			SetCtrlArrayAttribute(ctrlArrayHnd , controlId, ATTR_DIMMED, 1);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
			SetCtrlArrayAttribute(ctrlArrayHnd , controlId, ATTR_DIMMED, 1);
			SetCtrlArrayVal(ctrlArrayHnd, controlId, 0);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_DIMMED, 1);
			SetCtrlArrayVal (ctrlArrayHnd, B3_STATION_STATE_OFF);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ALERT_CA);
			SetCtrlArrayAttribute (ctrlArrayHnd, ATTR_VISIBLE, 0);
		}
	}
	else
	{
		SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, gsSystem.panel[panelIndex].procedure.name);
		SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 0);
		SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 1);

		for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
		{
			if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
			{

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);
			}
			else
			{
			}
		}
	}

	for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
	{
		int visible;

		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
		GetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, &visible);

		if (!stricmp(gsSystem.panel[panelIndex].station[stationIndex].serialNum, ""))
		{
			if (!visible)
			{
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "Enter Serial #");
			}
		}
		else
		{
			if (visible)
			{
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 1);

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);
				SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId,
						   gsSystem.panel[panelIndex].station[stationIndex].serialNum);
			}
		}
	}

	if (!gsSystem.panel[panelIndex].enabled)
	{
	}




	return;
}

void BuildRunTimeStr(int panelIndex, int stationIndex, char* timeStr)
{
	CVIAbsoluteTime currentTime;
	CVITimeInterval deltaTime;

	GetCurrentCVIAbsoluteTime (&currentTime);
	SubtractCVIAbsoluteTimes (currentTime, gsSystem.panel[panelIndex].station[stationIndex].summary.startTime, &deltaTime);

	double deltaTimeInSec = 0;

	CVITimeIntervalToSeconds (deltaTime, &deltaTimeInSec);

	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	hours = (int)floor(deltaTimeInSec / 3600.0);
	minutes = (int)floor((deltaTimeInSec - (3600.0 * hours))/60);
	seconds = (int)floor(deltaTimeInSec - (3600.0 * hours) - (60 * minutes));
	sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
	return;
}


int CVICALLBACK RemoveProcedure (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = -1;
			int panelId = 0;

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
				}
			}

			strcpy(gsSystem.panel[panelIndex].activeProcedure, "");
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE, "Select Procedure");
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_ALERT, ATTR_VISIBLE, 1);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], B3_PANEL_PROCEDURE_REMOVE, ATTR_VISIBLE, 0);
			SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_CTRL_MODE, VAL_INDICATOR);
			SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_SHOW_INCDEC_ARROWS, 0);
			SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_MIN_VALUE, B3_MIN_TEMP_IN_F);
			SetCtrlAttribute (gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, ATTR_MAX_VALUE, B3_MAX_TEMP_IN_F);
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, B3_MIN_TEMP_IN_F);

			int ctrlArrayHnd = 0;
			int controlId = 0;

			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{

				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, "");

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_ALERT_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], SERIAL_REMOVE_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

					if (strcmp(gsLogDir[panelIndex][stationIndex], ""))
					{
						char logPath[MAX_PATHNAME_LEN];
						int stat = 0;

						strcpy(logPath, gsLogPath[panelIndex][stationIndex]);
						strcat(logPath, ".sum");

						stat = FileExists(logPath, 0);

						if (stat == 0)
						{
							DeleteDir (gsLogDir[panelIndex][stationIndex]);
						}
					}

					strcpy(gsLogDir[panelIndex][stationIndex], "");
					strcpy(gsLogPath[panelIndex][stationIndex], "");

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);
				}
			}

			gsAppSetupChanged = 1;

			B3SystemLogWithVargs(gsAppLogPath, "Operator deleted active procedure for panel %d", panelId);
			break;
	}

	return 0;
}

int CVICALLBACK RemoveSerialNumber (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = -1;
			int panelId = 0;

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
				}
			}

			int ctrlArrayHnd = 0;
			int controlId = 0;
			int stationIndex = 0;
			int stationId = 0;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, SERIAL_REMOVE_CA);
			GetCtrlArrayIndex (ctrlArrayHnd, panel, control, &stationIndex);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(panel, controlId, ATTR_VISIBLE, 0);

			stationId = stationIndex+1;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, SERIAL_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(panel, controlId, ATTR_DIMMED, 0);
			SetCtrlVal(panel, controlId, "Enter Serial #");

			ctrlArrayHnd = GetCtrlArrayFromResourceID(panel, SERIAL_ALERT_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(panel, controlId, ATTR_VISIBLE, 1);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

			DeleteDir (gsLogDir[panelIndex][stationIndex]);
			strcpy(gsLogDir[panelIndex][stationIndex], "");
			strcpy(gsLogPath[panelIndex][stationIndex], "");

			B3SystemLogWithVargs(gsAppLogPath, "Operator deleted active serial number for panel %d - station %d", panelId, stationId);
			break;
	}

	return 0;
}

int32 CVICALLBACK DinChangeDetectionCallback(TaskHandle taskHandle, int32 signalID, void *callbackData)
{
	int32 error=0;
	uInt8 data[64]={0};
	int32 numReadPerChan;
	int32 bytesPerSamp;
	char errBuff[2048]={'\0'};
	int32 daqError = 0;

	int translatorInstalled = 0;
	int translatorPanelIndex = 0;
	int translatorStationIndex = 0;

	daqError = DAQmxReadDigitalLines (taskHandle, 1, 0.0, DAQmx_Val_GroupByChannel,
										 data, gsNumDinDAQmxChannels, &numReadPerChan,
										 &bytesPerSamp, NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxReadDigitalLines Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxReadDigitalLines Error (see app log for details) - exiting app");
		goto EXIT;
	}

	for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
		if (gsSystem.panelInstalled[panelIndex])
		{
			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					uInt8 dioState = data[gsSystem.control.translatorInPlaceIndex[panelIndex][stationIndex]];
					gsSystem.control.translatorInPlace[panelIndex][stationIndex] = dioState;

					if (gsSystem.panelInstalled[panelIndex])
					{
						if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
						{
							int ctrlArrayHnd = 0;
							int controlId = 0;

							ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], TRANSLATOR_INSTALLED_CA);
							controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
							SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, dioState);

							if (dioState == B3_DIN_LOGIC_ON)
							{
								translatorInstalled = 1;
								translatorPanelIndex = panelIndex;
								translatorStationIndex = stationIndex;
							}
						}
					}
				}
			}
		}
	}

	if (translatorInstalled)
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, translatorPanelIndex+1);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, translatorStationIndex+1);
		gsSystem.control.translatorPanelIndex = translatorPanelIndex;
		gsSystem.control.translatorStationIndex = translatorStationIndex;
	}
	else
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, 0);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, 0);
		gsSystem.control.translatorPanelIndex = -1;
		gsSystem.control.translatorStationIndex = -1;
	}

	return 0;

EXIT:
	if ( DAQmxFailed(error) )
	{
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	}

	if ( gsDinTaskHnd )
	{
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(gsDinTaskHnd);
		DAQmxClearTask(gsDinTaskHnd);
		gsDinTaskHnd = 0;
	}

	if( DAQmxFailed(error) )
		Beep();
		MessagePopup("DAQmx Error",errBuff);
	return 0;
}

int32 CVICALLBACK DaqMXTempDeviceReadCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32 error=0;
	char errBuff[2048]={'\0'};
	int32 numReadPerChan;
	double filterLevel;
	double avgeragedVoltage[B3_MAX_TEMPERATURE_COUNT_PER_DEVICE] = {0.0};
	uInt64 samplesPerChan = gsSystem.temp.sampleRateIPerChannelnHz;
	double setPointInF = 0.0;
	double setPointToleranceInF = 0.0;
	int32 daqError = 0;

	static double filteredVoltage[B3_MAX_TEMPERATURE_COUNT_PER_DEVICE] = {0.0};
	static int lastControlOperation[B3_MAX_PANEL_COUNT] = {B3_PANEL_VORTEX_OFF};
	//static int plotBaseCount[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {999};
	static int plotBaseCount[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 999};
	static unsigned int logBaseCount[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = 999};

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	daqError = DAQmxReadAnalogF64 (taskHandle, -1, 0.0,
									 DAQmx_Val_GroupByChannel, gsTemperatureData,
									 (uInt32)(samplesPerChan*gsNumTempDAQmxChannels), &numReadPerChan, NULL);

	if (daqError < 0)
	{
		int32 bufSize = 0;

		bufSize = DAQmxGetExtendedErrorInfo(NULL, 0);

		char daqErrorStr[bufSize];

		DAQmxGetExtendedErrorInfo(daqErrorStr, bufSize);
		LOG_ERROR_WITH_VARGS(daqError, "DAQmxReadAnalogF64 Error: <%s>", daqErrorStr);
		B3SystemErrorLog(gsAppLogPath, &gLastError);
		Beep();
		MessagePopup ("FATAL DAQmx ERROR", "DAQmxReadAnalogF64 Error (see app log for details) - exiting app");
		goto EXIT;
	}

	if ((uInt64)numReadPerChan != samplesPerChan)
	{
		return(0);
	}

	avgeragedVoltage[0] = 0.0;

	for (uInt32 i=0; i<gsNumTempDAQmxChannels; i++)	// start with ai1 since the thermocouple reference is using ai0
	{
		uInt64 offset = i*samplesPerChan;
		Mean (&gsTemperatureData[offset], samplesPerChan, &avgeragedVoltage[i]);
	}

	filterLevel = gsSystem.temp.filterLevel;

	for (uInt32 i=0; i<gsNumTempDAQmxChannels; i++)
	{
		if (gsSystem.temp.systemTempInF[i] == 0.0)
		{
			filteredVoltage[i] = avgeragedVoltage[i];
		}
		else
		{
			filteredVoltage[i] = ((avgeragedVoltage[i] * (100.0-filterLevel)) + (filteredVoltage[i] * filterLevel))/ 100.0;
		}

		gsSystem.temp.systemTempInF[i] = filteredVoltage[i];
	}

	for (int sbcDeviceIndex=0; sbcDeviceIndex<B3_SCB_68A_COUNT; sbcDeviceIndex++)
	{
		for (int thermocoupleIndexPerDevice=0; thermocoupleIndexPerDevice<B3_THERMOCOUPLES_PER_SCB_68A; thermocoupleIndexPerDevice++)
		{
			int thermocoupleIndex = (sbcDeviceIndex * B3_THERMOCOUPLES_PER_SCB_68A) + thermocoupleIndexPerDevice;
			gsSystem.temp.systemTempInF[thermocoupleIndex] += gsSystem.temp.tempCalOffset[sbcDeviceIndex];
		}
	}

	gsSystem.roomTempInF = gsSystem.temp.systemTempInF[gsSystem.temp.roomTempIndex];

#ifdef INSTALL_THERMAL_SIMULATOR
	GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ROOM_TEMP, &gsSystem.roomTempInF);
	//gsSystem.roomTempInF += gsSystem.temp.tempCalOffset[3];
#endif

	SetCtrlVal(gsSystemPnlHndl, MACHINE_ROOM_TEMP, gsSystem.roomTempInF);

	for (int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
	{
#ifdef INSTALL_THERMAL_SIMULATOR
		int panelId = panelIndex + 1;
#endif

		if (gsSystem.panelInstalled[panelIndex])
		{
			double boxTempInF = gsSystem.temp.systemTempInF[gsSystem.temp.boxTempIndex[panelIndex]];
#ifdef INSTALL_THERMAL_SIMULATOR

			switch(panelId)
			{
				case 1:
					GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_1, &boxTempInF);
					break;

				case 2:
					GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_2, &boxTempInF);
					break;

				case 3:
					GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_3, &boxTempInF);
					break;

				case 4:
					GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_4, &boxTempInF);
					break;

				case 5:
					GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_5, &boxTempInF);
					break;

			}
#endif

			gsSystem.panel[panelIndex].boxTempInF = boxTempInF;
			SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_BOX_TEMP, boxTempInF);

			if (gsSystem.panel[panelIndex].enabled)
			{
				if (gsSystem.panel[panelIndex].postTranslationHeating)
				{
					setPointInF = gsSystem.panel[panelIndex].procedure.postTranslationHeatingSetPointInF;
					setPointToleranceInF = gsSystem.panel[panelIndex].procedure.postTranslationHeatingSetPointToleranceInF;
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, gsSystem.panel[panelIndex].procedure.postTranslationHeatingSetPointInF);
				}
				else
				{
					setPointInF = gsSystem.panel[panelIndex].setPointInF;
					setPointToleranceInF = gsSystem.panel[panelIndex].procedure.setPointToleranceInF;
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_SET_TEMP, gsSystem.panel[panelIndex].setPointInF);
				}

				if (lastControlOperation[panelIndex] == B3_PANEL_COOLING)
				{
					if (boxTempInF < setPointInF)
					{
						lastControlOperation[panelIndex] = B3_PANEL_VORTEX_OFF;
						B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
					}
				}
				else if (lastControlOperation[panelIndex] == B3_PANEL_HEATING)
				{
					if (boxTempInF > setPointInF)
					{
						lastControlOperation[panelIndex] = B3_PANEL_VORTEX_OFF;
						B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
					}
				}
				else
				{
					if (boxTempInF < setPointInF - setPointToleranceInF)
					{
						lastControlOperation[panelIndex] = B3_PANEL_HEATING;
						B3SystemControl(B3_PANEL_HEATING, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 1);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
					}
					else if (boxTempInF > setPointInF + setPointToleranceInF)
					{
						lastControlOperation[panelIndex] = B3_PANEL_COOLING;
						B3SystemControl(B3_PANEL_COOLING, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 1);
					}
				}
/*
				if (boxTempInF < setPointInF - setPointToleranceInF)
				{
					if (lastControlOperation[panelIndex] != B3_PANEL_HEATING)
					{
						lastControlOperation[panelIndex] = B3_PANEL_HEATING;
						B3SystemControl(B3_PANEL_HEATING, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 1);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
					}
				}
				else if (boxTempInF > setPointInF + setPointToleranceInF)
				{
					if (lastControlOperation[panelIndex] != B3_PANEL_COOLING)
					{
						lastControlOperation[panelIndex] = B3_PANEL_COOLING;
						B3SystemControl(B3_PANEL_COOLING, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 1);
					}
				}
				else
				{
					if (lastControlOperation[panelIndex] != B3_PANEL_VORTEX_OFF)
					{
						lastControlOperation[panelIndex] = B3_PANEL_VORTEX_OFF;
						B3SystemControl(B3_PANEL_VORTEX_OFF, panelIndex, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, 0);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, 0);
					}
				}
//*/
			}
			else
			{
				lastControlOperation[panelIndex] = B3_PANEL_VORTEX_OFF;
			}

			int ctrlArrayHnd = 0;
			int controlId = 0;

			for (int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
#ifdef INSTALL_THERMAL_SIMULATOR
				int stationId = stationIndex + 1;
#endif

				if (gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					double anchorTempInF = gsSystem.temp.systemTempInF[gsSystem.temp.anchorTempIndex[panelIndex][stationIndex]];

#ifdef INSTALL_THERMAL_SIMULATOR
					switch(panelId)
					{
						case 1:
							switch(stationId)
							{
								case 1:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_1, &anchorTempInF);
									break;

								case 2:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_1, &anchorTempInF);
									break;

								case 3:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_1, &anchorTempInF);
									break;

								case 4:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_1, &anchorTempInF);
									break;
							}

							break;

						case 2:
							switch(stationId)
							{
								case 1:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_2, &anchorTempInF);
									break;

								case 2:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_2, &anchorTempInF);
									break;

								case 3:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_2, &anchorTempInF);
									break;

								case 4:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_2, &anchorTempInF);
									break;
							}

							break;

						case 3:
							switch(stationId)
							{
								case 1:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_3, &anchorTempInF);
									break;

								case 2:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_3, &anchorTempInF);
									break;

								case 3:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_3, &anchorTempInF);
									break;

								case 4:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_3, &anchorTempInF);
									break;
							}

							break;

						case 4:
							switch(stationId)
							{
								case 1:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_4, &anchorTempInF);
									break;

								case 2:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_4, &anchorTempInF);
									break;

								case 3:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_4, &anchorTempInF);
									break;

								case 4:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_4, &anchorTempInF);
									break;
							}

							break;

						case 5:
							switch(stationId)
							{
								case 1:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_5, &anchorTempInF);
									break;

								case 2:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_5, &anchorTempInF);
									break;

								case 3:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_5, &anchorTempInF);
									break;

								case 4:
									GetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_5, &anchorTempInF);
									break;
							}

							break;
					}
#endif

					gsSystem.panel[panelIndex].station[stationIndex].anchorTempInF = anchorTempInF;
					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], ANCHOR_TEMP_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, anchorTempInF);

					TempControlStateEngine(RUN_STATE_ENGINE, panelIndex, stationIndex);

					int stationControlState = 0;

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &stationControlState);

					if (stationControlState)
					{
						char timeStr[16];

						BuildRunTimeStr(panelIndex, stationIndex, timeStr);

						ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], RUN_TIME_CA);
						controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
						SetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, timeStr);

						if (gsB3GraphPnlHndl[panelIndex][stationIndex])
						{
							if (plotBaseCount[panelIndex][stationIndex] >= SECONDS_PER_PLOT_POINT)
							{
								double data[3];

								data[0] = setPointInF;
								data[1] = anchorTempInF;
								data[2] = boxTempInF;
								PlotStripChart (gsB3GraphPnlHndl[panelIndex][stationIndex], GRAPH_STRIPCHART, data, 3, 0, 0, VAL_DOUBLE);
								plotBaseCount[panelIndex][stationIndex] = 0;
							}

							plotBaseCount[panelIndex][stationIndex]++;
						}

//						TempControlStateEngine(RUN_STATE_ENGINE, panelIndex, stationIndex);

						if (logBaseCount[panelIndex][stationIndex] >= gsSystem.panel[panelIndex].procedure.temperatureLogPeriodInSecPerLog)
						{
							char logPath[MAX_PATHNAME_LEN];

							strcpy(logPath, gsLogPath[panelIndex][stationIndex]);
							strcat(logPath, ".slg");

							B3StationLog(logPath,
										 timeStr,
										 gsSystem.roomTempInF,
										 setPointInF,
										 boxTempInF,
										 anchorTempInF,
										 gsSystem.panel[panelIndex].station[stationIndex].state,
										 gsSystem.panel[panelIndex].station[stationIndex].subState);
							logBaseCount[panelIndex][stationIndex] = 0;
						}

						logBaseCount[panelIndex][stationIndex]++;
					}
				}
			}
		}
	}

#ifdef INSTALL_TRANSLATOR_SIMULATOR
	int translatorInstalled = 0;
	int translatorPanelIndex = -1;
	int translatorStationIndex = -1;
	int translatorPanelId = 0;
	int translatorStationId = 0;

	GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_TRANS_IP, &translatorInstalled);
	GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PANEL_ID, &translatorPanelId);
	GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, &translatorStationId);
	translatorPanelIndex = translatorPanelId -1;
	translatorStationIndex = translatorStationId - 1;

	if (translatorInstalled)
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, translatorPanelId);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, translatorStationId);
		gsSystem.control.translatorPanelIndex = translatorPanelIndex;
		gsSystem.control.translatorStationIndex = translatorStationIndex;
	}
	else
	{
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_PANEL_ID, 0);
		SetCtrlVal(gsTranslatePnlHndl, TRANSL_STATION_ID, 0);
		gsSystem.control.translatorPanelIndex = -1;
		gsSystem.control.translatorStationIndex = -1;
	}

	if (translatorPanelId > 0 && translatorStationId > 0)
	{
		int ctrlArrayHnd = 0;
		int controlId = 0;

		ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[translatorPanelIndex], TRANSLATOR_INSTALLED_CA);
		controlId = GetCtrlArrayItem (ctrlArrayHnd, translatorStationIndex);
		SetCtrlVal(gsB3PanelPnlHndl[translatorPanelIndex], controlId, translatorInstalled);
	}
#endif

EXIT:

	if(DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, (uInt32)sizeof(errBuff)-1);
		/*********************************************/
		/*/ DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		gsDAQmxTempTaskHnd = 0;

		if( gsTemperatureData )
		{
			free(gsTemperatureData);
			gsTemperatureData = NULL;
		}

		Beep();
		MessagePopup("DAQmx Error",errBuff);
	}

	return 0;
}

int CVICALLBACK ChangeSetPoint (int panel, int control, int event,
								void *callbackData, int eventData1, int eventData2)
{
	int panelIndex = 0;

	switch (event)
	{
		case EVENT_COMMIT:
			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					break;
				}
			}

			double setPointInF = 0.0;

			GetCtrlVal(panel, control, &setPointInF);
			gsSystem.panel[panelIndex].setPointInF = setPointInF;
			break;
	}
	return 0;
}

int CVICALLBACK ProcessStatusError (int panel, int control, int event,
									void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = 0;
			int panelId = 0;
			int stationIndex = 0;
			int stationId = 0;

			for (int i=0; i<B3_MAX_PANEL_COUNT; i++)
			{
				if (gsB3PanelPnlHndl[i] == panel)
				{
					panelIndex = i;
					panelId = panelIndex+1;
					break;
				}
			}

			switch(control)
			{
				case B3_PANEL_STATUS_ERROR_1:
					stationIndex = 0;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ERROR_2:
					stationIndex = 1;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ERROR_3:
					stationIndex = 2;
					stationId = stationIndex+1;
					break;

				case B3_PANEL_STATUS_ERROR_4:
					stationIndex = 3;
					stationId = stationIndex+1;
					break;

			}

			SetCtrlAttribute(panel, control, ATTR_VISIBLE, 0);
			B3SystemLogWithVargs(gsAppLogPath, "Operator cleared error for panel %d - station %d",
								panelId, stationId);

			switch(gsSystem.panel[panelIndex].station[stationIndex].state)
			{
				case B3_STATION_STABILIZATION_READY_DELAY_ERROR:
				case B3_STATION_TRANSLATION_READY_DELAY_ERROR:
				case B3_STATION_TRANSLATION_START_DELAY_ERROR:
				case B3_STATION_COOLDOWN_READY_DELAY_ERROR:
					SetCtrlVal(gsErrorBypassAuthorizationPnlHnd, EAC_PANEL, panelId);
					SetCtrlVal(gsErrorBypassAuthorizationPnlHnd, EAC_STATION, stationId);
					SetCtrlVal(gsErrorBypassAuthorizationPnlHnd, EAC_SERIAL, gsSystem.panel[panelIndex].station[stationIndex].serialNum);
					SetCtrlVal(gsErrorBypassAuthorizationPnlHnd, EAC_PIN, 0);

					int ctrlArrayHnd = 0;
					int controlId = 0;

					ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
					controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
					SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 1);

					DisplayPanel(gsErrorBypassAuthorizationPnlHnd);
					break;
			}

			break;
	}

	return 0;
}

void  DeferredCallToB3StationControl(void *data)
{
	DEFERRED_CALL_DATA *deferredCallDataPtr = (DEFERRED_CALL_DATA*)data;

	B3StationControl (deferredCallDataPtr->panel, deferredCallDataPtr->control, EVENT_COMMIT, "AUTO", 0, 0);
	return;
}

int CVICALLBACK CheckGalilCommConnection (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
#ifndef INSTALL_TRANSLATOR_SIMULATOR
			GALIL_CMD_Close();

			Sleep(100);

			int stat = GALIL_CMD_Open(gsSystem.translateInfo.galilMotionControllerDeviceId);

			if (stat < GALIL_CMD_OK)
			{
				LOG_ERROR(stat, "Unable to connect with the Galil controller");
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("COMM ERROR", "Unable to connect with the Galil controller");
				return(0);
			}
#else
			Sleep(100);

			int state = 0;

			GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, &state);

			if (state == GALIL_CMD_COMM_ERROR)
			{
				LOG_ERROR(state, "Unable to connect with the Galil controller");
				B3SystemErrorLog(gsAppLogPath, &gLastError);
				Beep();
				MessagePopup ("COMM ERROR", "Unable to connect with the Galil controller");
				return(0);
			}
#endif

			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);
			B3SystemLog(gsAppLogPath, "Galil controller operations restarted");
			SetAsyncTimerAttribute (gsGalilAsyncTimerId, ASYNC_ATTR_ENABLED, 1);
			HidePanel(gsGalilErrorPnlHnd);
			break;
	}

	return 0;
}

#ifdef INSTALL_THERMAL_SIMULATOR
int CVICALLBACK ThermalSimulatorAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2)
{
	if (event == EVENT_TIMER_TICK)
	{
		static int lastStatus[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = -999};
		static double startTimeInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)]  = 0.0};
		static int stationStatus[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)][0 ... (B3_MAX_STATION_COUNT-1)] = B3_STATION_STATE_OFF};;
		static double boxTempInF[B3_MAX_PANEL_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)] = 0.0};
		static int panelEnabled[B3_MAX_PANEL_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)] = 0};
		static double tempRateInFPerSec[B3_MAX_PANEL_COUNT] = {[0 ... (B3_MAX_PANEL_COUNT-1)] = 0.0};

		for (unsigned int panelIndex=0; panelIndex<B3_MAX_PANEL_COUNT; panelIndex++)
		{
			unsigned int panelId = panelIndex + 1;

			if (!gsSystem.panelInstalled[panelIndex])
			{
				continue;
				//return(0);
			}

//			if (!gsSystem.panel[panelIndex].enabled)
//			{
//				panelEnabled[panelIndex] = gsSystem.panel[panelIndex].enabled;
//				return(0);
//			}
//			else if (panelEnabled[panelIndex] != gsSystem.panel[panelIndex].enabled)
			if (panelEnabled[panelIndex] != gsSystem.panel[panelIndex].enabled)
			{
				panelEnabled[panelIndex] = gsSystem.panel[panelIndex].enabled;
				SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ROOM_TEMP, gsSystem.control.simThermalData.initRoomTempInF);
				SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_1, gsSystem.control.simThermalData.initBoxTempInF);
				boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;
			}

			unsigned int anchorHeatingCount = 0;

			for (unsigned int stationIndex=0; stationIndex<B3_MAX_STATION_COUNT; stationIndex++)
			{
				int stationId = stationIndex + 1;

				if (!gsSystem.panel[panelIndex].stationInstalled[stationIndex])
				{
					continue;
				}

				int ctrlArrayHnd = 0;
				int controlId = 0;

				ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_CA);
				controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
				GetCtrlVal(gsB3PanelPnlHndl[panelIndex], controlId, &stationStatus[panelIndex][stationIndex]);

				switch(stationStatus[panelIndex][stationIndex])
				{
					case B3_STATION_STATE_OFF:
						if (lastStatus[panelIndex][stationIndex] != stationStatus[panelIndex][stationIndex])
						{
							lastStatus[panelIndex][stationIndex] = stationStatus[panelIndex][stationIndex];

							switch(panelId)
							{
								case 1:
									SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_1, gsSystem.control.simThermalData.initBoxTempInF);
									boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;

									switch(stationId)
									{
										case 1:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_1, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 2:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_1, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 3:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_1, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 4:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_1, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;
									}

									break;

								case 2:
									SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_2, gsSystem.control.simThermalData.initBoxTempInF);
									boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;

									switch(stationId)
									{
										case 1:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_2, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 2:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_2, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 3:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_2, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 4:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_2, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;
									}

									break;

								case 3:
									SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_3, gsSystem.control.simThermalData.initBoxTempInF);
									boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;

									switch(stationId)
									{
										case 1:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_3, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 2:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_3, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 3:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_3, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 4:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_3, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;
									}

									break;

								case 4:
									SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_4, gsSystem.control.simThermalData.initBoxTempInF);
									boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;

									switch(stationId)
									{
										case 1:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_4, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 2:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_4, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 3:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_4, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 4:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_4, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;
									}

									break;
									;
								case 5:
									SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_5, gsSystem.control.simThermalData.initBoxTempInF);
									boxTempInF[panelIndex] = gsSystem.control.simThermalData.initBoxTempInF;

									switch(stationId)
									{
										case 1:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_5, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 2:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_5, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 3:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_5, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;

										case 4:
											SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_5, gsSystem.control.simThermalData.anchorPreResinTempInF[panelIndex][stationIndex]);
											break;
									}

									break;
							}
						}

						break;

					case B3_STATION_STATE_STABILIZING:
					case B3_STATION_STATE_STABILIZING_EACG:
						if (lastStatus[panelIndex][stationIndex] != stationStatus[panelIndex][stationIndex])
						{
							lastStatus[panelIndex][stationIndex] = stationStatus[panelIndex][stationIndex];
						}

						break;

					case B3_STATION_STABILIZATION_READY_DELAY_ERROR:
					case B3_STATION_TRANSLATION_READY_DELAY_ERROR:
					case B3_STATION_TRANSLATION_START_DELAY_ERROR:
					case B3_STATION_COOLDOWN_READY_DELAY_ERROR:
					case B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY:
					case B3_STATION_STATE_READY_FOR_TRANSLATION:
					case B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION:
					case B3_STATION_STATE_TRANSLATING:
					case B3_STATION_STATE_TRANSLATING_COMPLETE:
					case B3_STATION_STATE_WAITING_FOR_HEATING_READY:
					case B3_STATION_STATE_READY_FOR_HEATING:
					case B3_STATION_STATE_HEATING:
					case B3_STATION_STATE_COOL_DOWN:
					case B3_STATION_STATE_COMPLETE:
					case B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG:
					case B3_STATION_STATE_READY_FOR_TRANSLATION_EACG:
					case B3_STATION_STATE_COOL_DOWN_EACG:
						if (lastStatus[panelIndex][stationIndex] == B3_STATION_STATE_READY_FOR_RESIN)
						{
							startTimeInSec[panelIndex][stationIndex] = Timer();
						}

						lastStatus[panelIndex][stationIndex] = stationStatus[panelIndex][stationIndex];
						anchorHeatingCount++;

						double deltaTimeInSec = Timer() - startTimeInSec[panelIndex][stationIndex];
						double tempInF = 0;

						int foundIndex = 0;

						for (unsigned int dataIndex = 0; dataIndex<(gsSystem.control.simThermalData.anchorPostResinDataCount[panelIndex][stationIndex]-1); dataIndex++)
						{
							if (deltaTimeInSec >= gsSystem.control.simThermalData.anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex] &&
								deltaTimeInSec <= gsSystem.control.simThermalData.anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex+1])
							{
								foundIndex = 1;
								double lowerInSec = gsSystem.control.simThermalData.anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex];
								double upperInSec = gsSystem.control.simThermalData.anchorPostResinTimeInSec[panelIndex][stationIndex][dataIndex+1];
								double scale = (deltaTimeInSec - lowerInSec) / (upperInSec - lowerInSec);
								double lowerInF = gsSystem.control.simThermalData.anchorPostResinTempInF[panelIndex][stationIndex][dataIndex];
								double upperInF = gsSystem.control.simThermalData.anchorPostResinTempInF[panelIndex][stationIndex][dataIndex+1];
								tempInF = lowerInF + (scale * (upperInF - lowerInF));
								break;
							}
						}

						if (foundIndex == 0)
						{
							int dataCount = gsSystem.control.simThermalData.anchorPostResinDataCount[panelIndex][stationIndex];
							tempInF = gsSystem.control.simThermalData.anchorPostResinTimeInSec[panelIndex][stationIndex][dataCount-1];
						}

						if (tempInF > 90.0)
						{
							;
						}

						switch(panelId)
						{
							case 1:
								switch(stationId)
								{
									case 1:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_1, tempInF);
										break;

									case 2:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_1, tempInF);
										break;

									case 3:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_1, tempInF);
										break;

									case 4:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_1, tempInF);
										break;
								}

								break;

							case 2:
								switch(stationId)
								{
									case 1:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_2, tempInF);
										break;

									case 2:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_2, tempInF);
										break;

									case 3:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_2, tempInF);
										break;

									case 4:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_2, tempInF);
										break;
								}

								break;

							case 3:
								switch(stationId)
								{
									case 1:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_3, tempInF);
										break;

									case 2:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_3, tempInF);
										break;

									case 3:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_3, tempInF);
										break;

									case 4:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_3, tempInF);
										break;
								}

								break;

							case 4:
								switch(stationId)
								{
									case 1:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_4, tempInF);
										break;

									case 2:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_4, tempInF);
										break;

									case 3:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_4, tempInF);
										break;

									case 4:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_4, tempInF);
										break;
								}

								break;
								;
							case 5:
								switch(stationId)
								{
									case 1:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_1_5, tempInF);
										break;

									case 2:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_2_5, tempInF);
										break;

									case 3:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_3_5, tempInF);
										break;

									case 4:
										SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_ANCHOR_TEMP_4_5, tempInF);
										break;
								}

								break;
						}

						break;

					default:
						if (lastStatus[panelIndex][stationIndex] != stationStatus[panelIndex][stationIndex])
						{
							lastStatus[panelIndex][stationIndex] = stationStatus[panelIndex][stationIndex];
						}
				}
			}

			int coolingOn = 0;
			int heatingOn = 0;

			GetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_COOLING_LED, &coolingOn);
			GetCtrlVal(gsB3PanelPnlHndl[panelIndex], B3_PANEL_HEATING_LED, &heatingOn);

			double deltaTempInF = 0.0;

			if (coolingOn)
			{
				tempRateInFPerSec[panelIndex] -= SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.tempChangeRateInFPerSec2;

				if (tempRateInFPerSec[panelIndex] < gsSystem.control.simThermalData.vortexCoolingRateInFPerSec)
				{
					tempRateInFPerSec[panelIndex] = SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.vortexCoolingRateInFPerSec;
				}
			}
			else if (heatingOn)
			{
				tempRateInFPerSec[panelIndex] += SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.tempChangeRateInFPerSec2;

				if (tempRateInFPerSec[panelIndex] > gsSystem.control.simThermalData.vortexCoolingRateInFPerSec)
				{
					tempRateInFPerSec[panelIndex] = SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.vortexHeatingRateInFPerSec;
				}
			}
			else if (anchorHeatingCount > 0)
			{
				tempRateInFPerSec[panelIndex] += anchorHeatingCount * gsSystem.control.simThermalData.tempChangeRateInFPerSec2;

				if (tempRateInFPerSec[panelIndex] > anchorHeatingCount * gsSystem.control.simThermalData.anchorHeatingRateInFPerSec)
				{
					tempRateInFPerSec[panelIndex] = anchorHeatingCount * gsSystem.control.simThermalData.anchorHeatingRateInFPerSec;
				}
			}
			else
			{
				if (tempRateInFPerSec[panelIndex] < 0.0)
				{
					tempRateInFPerSec[panelIndex] += SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.tempChangeRateInFPerSec2;

					if (tempRateInFPerSec[panelIndex] > 0.0)
					{
						tempRateInFPerSec[panelIndex] = 0.0;
					}
				}
				else if (tempRateInFPerSec[panelIndex] > 0.0)
				{
					tempRateInFPerSec[panelIndex] -= SIM_ASYNC_TIMER_PERIOD * gsSystem.control.simThermalData.tempChangeRateInFPerSec2;

					if (tempRateInFPerSec[panelIndex] < 0.0)
					{
						tempRateInFPerSec[panelIndex] = 0.0;
					}
				}
			}


			deltaTempInF = SIM_ASYNC_TIMER_PERIOD * tempRateInFPerSec[panelIndex];
			boxTempInF[panelIndex] += deltaTempInF;

			switch(panelId)
			{
				case 1:
					SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_1, boxTempInF[panelIndex]);
					break;

				case 2:
					SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_2, boxTempInF[panelIndex]);
					break;

				case 3:
					SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_3, boxTempInF[panelIndex]);
					break;

				case 4:
					SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_4, boxTempInF[panelIndex]);
					break;

				case 5:
					SetCtrlVal(gsThermalSimulatorPanelHnd, TEMP_SIM_BOX_TEMP_5, boxTempInF[panelIndex]);
					break;

			}
		}
	}

	return(0);
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK TranslatorSimulatorAsyncCallback(int reserved, int timerId, int event, void *callbackData, int eventData1, int eventData2)
{
	if (event == EVENT_TIMER_TICK)
	{
		static int sLastStatus = GALIL_CMD_POWER_OFF;
		static double sStartPositionInInches = 0.0;
		static double sStartLoadInLBS = 0.0;
		static double startTimeInSec = 0.0;
		static int sTranslatePanelId = 0;
		static double sTranslateVelocityInIPS = 0.0;
		static int sTranslateMode = 0;
		static int sPanelIndex = 0;

		int status = 0;
		int stationId = 0;
		int stationIndex = 0;

		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, &status);
		GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, &stationId);

		stationIndex = stationId - 1;

		switch(status)
		{
			double positionInInches = 0.0;
			double loadInLBS = 0.0;

			case GALIL_CMD_TRANSLATE_READY:
				if (sLastStatus != status)
				{
					sLastStatus = status;
					//SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, gsSystem.control.simTranslatorData.translateStartPositionInInches);
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, gsSystem.control.simTranslatorData.translateStartLoadInLBS);

					GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PANEL_ID, &sTranslatePanelId);
					sPanelIndex = sTranslatePanelId - 1;

					switch(gsSystem.panel[sPanelIndex].procedure.translationMethod)
					{
						case B3_TRANSLATE_FIXED_LENGHT_METHOD:
							sTranslateVelocityInIPS = gsSystem.panel[sPanelIndex].procedure.fixedLengthTranslationMethodInfo.velocityInIPS;
							break;

						case B3_TRANSLATE_LOAD_LIMIT_METHOD:
							sTranslateVelocityInIPS = gsSystem.panel[sPanelIndex].procedure.loadLimitTranslateMethodInfo.velocityInIPS;
							break;
					}

					sTranslateMode = gsSystem.panel[sPanelIndex].procedure.translationMethod;
				}

				break;

			case GALIL_CMD_POWER_ON_HOMED:
				if (sLastStatus != status)
				{
					sLastStatus = status;
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, gsSystem.control.simTranslatorData.homePositionInInches);
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, 0.0);
				}

				break;

			case GALIL_CMD_AT_PARK:
				if (sLastStatus != status)
				{
					sLastStatus = status;
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, gsSystem.control.simTranslatorData.parkPositionInInches);
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, 0.0);
				}

				break;

			case GALIL_CMD_MOVING_TO_POSITION:
				if (sLastStatus != status)
				{
					sLastStatus = status;
				}

				int direction = 0;

				GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, &positionInInches);

				if (positionInInches < gsSimTargetPosition)
				{
					direction = 1;
					positionInInches += gsSystem.control.simTranslatorData.jogVelocityInIPS * SIM_ASYNC_TIMER_PERIOD;

					if (positionInInches >= gsSimTargetPosition)
					{
						positionInInches = gsSimTargetPosition;

						if (gsSimTargetPosition >= gsSystem.control.simTranslatorData.translateStartPositionInInches)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_READY);
						}
						else
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_POWER_ON);
						}
					}
				}
				else if (positionInInches > gsSimTargetPosition)
				{
					direction = -1;
					positionInInches -= gsSystem.control.simTranslatorData.jogVelocityInIPS * SIM_ASYNC_TIMER_PERIOD;

					if (positionInInches <= gsSimTargetPosition)
					{
						positionInInches = gsSimTargetPosition;

						if (gsSimTargetPosition <= gsSystem.control.simTranslatorData.parkPositionInInches)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_AT_PARK);
						}
						else
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_POWER_ON);
						}
					}
				}
				else
				{
					direction = 0;

					if (gsSimTargetPosition == gsSystem.control.simTranslatorData.translateStartPositionInInches)
					{
						SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_READY);
					}
					else if (gsSimTargetPosition <= gsSystem.control.simTranslatorData.parkPositionInInches)
					{
						SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_AT_PARK);
					}
					else
					{
						SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_POWER_ON);
					}
				}

				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, positionInInches);
				break;

			case GALIL_CMD_JOGGING:
				if (sLastStatus != status)
				{
					sLastStatus = status;
				}

				double jogPositionInInches = 0.0;

				GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, &jogPositionInInches);

				if (gsSimJog > 0)
				{
					jogPositionInInches += gsSystem.control.simTranslatorData.jogVelocityInIPS * SIM_ASYNC_TIMER_PERIOD;

					if (jogPositionInInches > gsSystem.control.simTranslatorData.maxPositionInInches)
					{
						jogPositionInInches = gsSystem.control.simTranslatorData.maxPositionInInches;
					}
				}
				else if (gsSimJog < 0)
				{
					jogPositionInInches -= gsSystem.control.simTranslatorData.jogVelocityInIPS * SIM_ASYNC_TIMER_PERIOD;

					if (jogPositionInInches < gsSystem.control.simTranslatorData.minPositionInInches)
					{
						jogPositionInInches = gsSystem.control.simTranslatorData.minPositionInInches;
					}
				}

				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, jogPositionInInches);
				break;

			case GALIL_CMD_TRANSLATING:
				if (sLastStatus != status)
				{
					sLastStatus = status;
					GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, &sStartPositionInInches);
					GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, &sStartLoadInLBS);
					startTimeInSec = Timer();
				}

				double deltaTimeInSec = Timer() - startTimeInSec;
				positionInInches = sStartPositionInInches + (deltaTimeInSec * sTranslateVelocityInIPS);
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_DISP, positionInInches);

				int foundIndex = 0;

				for (unsigned int dataIndex = 0; dataIndex<(gsSystem.control.simTranslatorData.dataCount-1); dataIndex++)
				{
					if (positionInInches >= gsSystem.control.simTranslatorData.translatePositionInInches[dataIndex] &&
						positionInInches < gsSystem.control.simTranslatorData.translatePositionInInches[dataIndex+1])
					{
						foundIndex = 1;
						double deltaPositionInInches = positionInInches - sStartPositionInInches;
						double num = deltaPositionInInches - gsSystem.control.simTranslatorData.translatePositionInInches[dataIndex];
						double dem = gsSystem.control.simTranslatorData.translatePositionInInches[dataIndex+1] -
									 gsSystem.control.simTranslatorData.translatePositionInInches[dataIndex];
						double scale = num/dem;
						double range = gsSystem.control.simTranslatorData.translateLoadInLBS[dataIndex+1] -
									   gsSystem.control.simTranslatorData.translateLoadInLBS[dataIndex];
						loadInLBS = gsSystem.control.simTranslatorData.translateLoadInLBS[dataIndex] +
									range * scale;
						break;
					}
				}

				if (foundIndex == 0)
				{
					loadInLBS = gsSystem.control.simTranslatorData.translateLoadInLBS[gsSystem.control.simTranslatorData.dataCount-1];
				}

				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, loadInLBS);

				switch(sTranslateMode)
				{
					double deltaPositionInInches = 0.0;

					case B3_TRANSLATE_FIXED_LENGHT_METHOD:
						deltaPositionInInches = positionInInches - sStartPositionInInches;

						if (deltaPositionInInches >= gsSystem.panel[sPanelIndex].procedure.fixedLengthTranslationMethodInfo.displacementInInches)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_COMPLETE);
						}
						else if (loadInLBS >= gsSystem.panel[sPanelIndex].procedure.fixedLengthTranslationMethodInfo.maxLoadLimitInPounts)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP);
						}

						break;

					case B3_TRANSLATE_LOAD_LIMIT_METHOD:
						deltaPositionInInches = positionInInches - sStartPositionInInches;

						if (loadInLBS >= gsSystem.panel[sPanelIndex].procedure.loadLimitTranslateMethodInfo.loadLimitInPounds)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_COMPLETE);
						}
						else if (deltaPositionInInches >= gsSystem.panel[sPanelIndex].procedure.loadLimitTranslateMethodInfo.maxDisplacementInInches)
						{
							SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP);
						}

						break;
				}

				break;

			case GALIL_CMD_TRANSLATE_COMPLETE:
			case GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP:
				if (sLastStatus != status)
				{
					sLastStatus = status;
					SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_SUPPORT, ATTR_DIMMED, 0);
				}

				GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, &loadInLBS);

				if (loadInLBS < gsSystem.control.simTranslatorData.safeMaxLoadInLBS)
				{
					SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 0);
				}

				break;

			case GALIL_CMD_MOTION_HALTED:
				if (sLastStatus != status)
				{
					sLastStatus = status;
				}

				break;

			default:

				break;
		}
	}

	return(0);
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimTranslatorAtStation (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int state = 0;

			GetCtrlVal(panel, control, &state);
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_TRANS_IP, state);

			if (state)
			{
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PENDANT, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STOP, ATTR_DIMMED, 0);
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_AT_PARK);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PANEL_ID, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, ATTR_DIMMED, 1);
			}
			else
			{
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PENDANT, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_UP, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_DOWN, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STOP, ATTR_DIMMED, 1);
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_POWER_OFF);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_PANEL_ID, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, ATTR_DIMMED, 0);
			}

			break;
	}
	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimTranslatorReadyButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int status = 0;
			int stationId = 0;
			int stationIndex = 0;

			GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, &status);
			GetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATION_ID, &stationId);

			stationIndex = stationId - 1;

			SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 1);

			if (status == GALIL_CMD_AT_PARK || status == GALIL_CMD_MOTION_HALTED || status == GALIL_CMD_POWER_ON)
			{

				if (gsSystem.control.simTranslatorData.autoPositionToTranslateReady)
				{
					gsSimTargetPosition = gsSystem.control.simTranslatorData.translateStartPositionInInches;
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_MOVING_TO_POSITION);
				}
				else
				{
					SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_TRANSLATE_READY);
				}
			}
			else if ((status == GALIL_CMD_TRANSLATE_COMPLETE) || (status == GALIL_CMD_TRANSLATE_COMPLETE_WITH_LIMIT_TRIP))
			{
				gsSimTargetPosition = gsSystem.control.simTranslatorData.parkPositionInInches;
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_MOVING_TO_POSITION);
			}

			break;
	}

	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimPendantSwitch (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int state = 0;

			GetCtrlVal(panel, control, &state);

			if (state)
			{
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STOP, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_UP, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_DOWN, ATTR_DIMMED, 0);
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_PENDANT_ACTIVE);
			}
			else
			{
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STOP, ATTR_DIMMED, 0);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_UP, ATTR_DIMMED, 1);
				SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_JOG_DOWN, ATTR_DIMMED, 1);
				SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_POWER_ON);
			}

			break;
	}
	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimJogUp (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_LEFT_CLICK:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_JOGGING);
			gsSimJog = 1;
			break;

		case EVENT_LEFT_CLICK_UP:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_PENDANT_ACTIVE);
			gsSimJog = 0;
			break;
	}
	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimJogDown (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_LEFT_CLICK:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_JOGGING);
			gsSimJog = -1;
			break;

		case EVENT_LEFT_CLICK_UP:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_PENDANT_ACTIVE);
			gsSimJog = 0;
			break;
	}
	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimTranslatorStopButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_STATUS, GALIL_CMD_MOTION_HALTED);
			SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_SUPPORT, ATTR_DIMMED, 1);
			break;
	}
	return 0;
}
#endif

#ifdef INSTALL_TRANSLATOR_SIMULATOR
int CVICALLBACK SimTranslatorSupportButton (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			SetCtrlVal(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_LOAD, 0.0);
			SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_SUPPORT, ATTR_DIMMED, 1);
			SetCtrlAttribute(gsTranslatorSimulatorPanelHnd, TRANSL_SIM_READY, ATTR_DIMMED, 0);
			break;
	}
	return 0;
}
#endif

int CVICALLBACK CancelAuthorization (int panel, int control, int event,
									 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelId = 0;
			int stationId = 0;

			GetCtrlVal(panel, EAC_PANEL, &panelId);
			GetCtrlVal(panel, EAC_STATION, &stationId);

			int panelIndex = panelId-1;
			int stationIndex = stationId-1;

			int ctrlArrayHnd = 0;
			int controlId = 0;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

			HidePanel(gsErrorBypassAuthorizationPnlHnd);
			break;
	}
	return 0;
}

int CVICALLBACK ProcessPin (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelId = 0;
			int stationId = 0;
			char serialNum[41];
			uint32_t operatorPinVal = 0;
			unsigned int tmpVal;

			GetCtrlVal(panel, EAC_PANEL, &panelId);
			GetCtrlVal(panel, EAC_STATION, &stationId);
			GetCtrlVal(panel, EAC_SERIAL, serialNum);
			GetCtrlVal(panel, EAC_PIN, &tmpVal);
			operatorPinVal = (uint32_t)tmpVal;

			int panelIndex = panelId-1;
			int stationIndex = stationId-1;
			RemoveSurroundingWhiteSpace(serialNum);
			StringUpperCase(serialNum);

			uint32_t hashVal = DS_UTIL_StringHash(serialNum, strlen(serialNum));
			uint32_t pinVal = hashVal % 10000;

			if (pinVal != operatorPinVal)
			{
				MessagePopup ("Invalid PIN",
              					"Invalid PIN entered\n\nReenter PIN then press OK");
				SetCtrlVal(panel, EAC_PIN, 0);
				B3SystemLogWithVargs(gsAppLogPath, "Invalid error authorization PIN (%d) entered for panel %d - station %d",
									pinVal, panelIndex+1, stationIndex+1);

				return(0);
			}

			switch(gsSystem.panel[panelIndex].station[stationIndex].state)
			{
				case B3_STATION_STABILIZATION_READY_DELAY_ERROR:
					gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_STABILIZING_EACG;
					break;

				case B3_STATION_TRANSLATION_READY_DELAY_ERROR:
					gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG;
					break;

				case B3_STATION_TRANSLATION_START_DELAY_ERROR:
					if (gsSystem.panel[panelIndex].procedure.translationMethod == B3_TRANSLATE_MANUAL_METHOD)
					{
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION_EACG;
					}
					else
					{
						gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_READY_FOR_TRANSLATION_EACG;
					}

					break;

				case B3_STATION_COOLDOWN_READY_DELAY_ERROR:
					gsSystem.panel[panelIndex].station[stationIndex].state = B3_STATION_STATE_COOL_DOWN_EACG;
					break;
			}

			int ctrlArrayHnd = 0;
			int controlId = 0;

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], CONTROL_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_DIMMED, 0);

			ctrlArrayHnd = GetCtrlArrayFromResourceID(gsB3PanelPnlHndl[panelIndex], STATUS_ERROR_CA);
			controlId = GetCtrlArrayItem (ctrlArrayHnd, stationIndex);
			SetCtrlAttribute(gsB3PanelPnlHndl[panelIndex], controlId, ATTR_VISIBLE, 0);

			B3SystemControl(B3_SYSTEM_LIGHTS, B3_SYSTEM_ERROR, 0);

			B3SystemLogWithVargs(gsAppLogPath, "Error authorization PIN (%d) entered for panel %d - station %d",
								pinVal, panelIndex+1, stationIndex+1);

			HidePanel(gsErrorBypassAuthorizationPnlHnd);
			break;
	}

	return 0;
}

int CVICALLBACK CancelChartScale(int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			HidePanel(gsChartScalePnlHnd);
			break;
	}

	return 0;
}

int CVICALLBACK SetNewScale(int panel, int control, int event,
							 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelId = 0;
			int stationId = 0;
			int panelIndex = 0;
			int stationIndex = 0;
			double minTempInF = 0;
			double maxTempInF = 0;

			GetCtrlVal(panel, CHRT_SETUP_PANEL, &panelId);
			GetCtrlVal(panel, CHRT_SETUP_STATION, &stationId);
			panelIndex = panelId - 1;
			stationIndex = stationId - 1;
			GetCtrlVal(panel, CHRT_SETUP_MIN_TEMP, &minTempInF);
			GetCtrlVal(panel, CHRT_SETUP_MAX_TEMP, &maxTempInF);

			minTempInF = (double)RoundRealToNearestInteger(minTempInF);
			maxTempInF = (double)RoundRealToNearestInteger(maxTempInF);

			if (minTempInF < maxTempInF)
			{
				SetAxisScalingMode (gsB3GraphPnlHndl[panelIndex][stationIndex],
									GRAPH_STRIPCHART, VAL_LEFT_YAXIS, VAL_MANUAL,
									minTempInF,
									maxTempInF);
				B3SystemLogWithVargs(gsAppLogPath, "Operator changed graph scaling to Min. Temp = %.0lfF and Max. Temp = %.0lfF for panel %d - station %d",
									minTempInF, maxTempInF, panelId, stationId);
			}
			else
			{
				MessagePopup ("Invalid Range", "Min. Temp must be less than the Max. Temp");
			}

			HidePanel(gsChartScalePnlHnd);
			break;
	}

	return 0;
}

int CVICALLBACK ResetScale (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int panelIndex = 0;
			int panelId = 0;

			GetCtrlVal(panel, CHRT_SETUP_PANEL, &panelId);
			panelIndex = panelId - 1;

			SetCtrlVal(panel, CHRT_SETUP_MIN_TEMP, gsSystem.panel[panelIndex].procedure.stripChartMinTempInF);
			SetCtrlVal(panel, CHRT_SETUP_MAX_TEMP, gsSystem.panel[panelIndex].procedure.stripChartMaxTempInF);
			break;
	}

	return 0;
}

#ifndef BORIS_3_DEFINITIONS

#define BORIS_3_DEFINITIONS

#include "userint.h"
#include "utility.h"
#include "nidaqmx.h"

#define B3_ON									1
#define B3_OFF									0

#define B3_TRUE									1
#define B3_FALSE								0

#define B3_OK									0
#define B3_SYSTEM_ERROR							-1
#define B3_INI_FILE_ERROR						-2
#define B3_ARGUMENT_ERROR						-3

#define B3_PANEL_STATE_ERROR					-1
#define B3_PANEL_STATE_IDLE						0
#define B3_PANEL_STATE_RUNNING					1

#define B3_STATION_STABILIZATION_READY_DELAY_ERROR			-1
#define B3_STATION_TRANSLATION_READY_DELAY_ERROR			-2
#define B3_STATION_TRANSLATION_START_DELAY_ERROR			-3
#define B3_STATION_COOLDOWN_READY_DELAY_ERROR				-4
#define B3_STATION_STATE_ERROR								-100
#define B3_STATION_STATE_OFF								0
#define B3_STATION_STATE_STABILIZING						1
#define B3_STATION_STATE_READY_FOR_RESIN					2
#define B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY		3
#define B3_STATION_STATE_READY_FOR_TRANSLATION				4
#define B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION		5
#define B3_STATION_STATE_TRANSLATING						6
#define B3_STATION_STATE_TRANSLATING_COMPLETE				7
#define B3_STATION_STATE_WAITING_FOR_HEATING_READY			8
#define B3_STATION_STATE_READY_FOR_HEATING					9
#define B3_STATION_STATE_HEATING							10
#define B3_STATION_STATE_COOL_DOWN							11
#define B3_STATION_STATE_COMPLETE							12

#define B3_STATION_STATE_STABILIZING_EACG					101
#define B3_STATION_STATE_WAITING_FOR_TRANSLATION_READY_EACG	103
#define B3_STATION_STATE_READY_FOR_TRANSLATION_EACG			104
#define B3_STATION_STATE_READY_FOR_MANUAL_TRANSLATION_EACG	105
#define B3_STATION_STATE_COOL_DOWN_EACG						111

#define B3_TRANS_DETECT_OFF						99
#define B3_TRANS_DETECT_IDLE					0
#define B3_PEAK_DETECTION						1
#define B3_TRANS_WAITING_FOR_COMPLETION			2
#define B3_TRANS_COMPLETE						3
#define B3_TRANS_ERROR							-1

#define B3_TRANSLATE_STATE_ERROR				-1
#define B3_TRANSLATE_STATE_IDLE					0
#define B3_TRANSLATE_STATE_READY				1
#define B3_TRANSLATE_STATE_RUNNING				2
#define B3_TRANSLATE_STATE_COMPLETE				3

#define B3_TRANSLATE_DETECTION_PEAK_METHOD			1
#define B3_TRANSLATE_DETECTION_FIXED_TIME_METHOD	2

#define B3_TRANSLATE_FIXED_LENGHT_METHOD		1
#define B3_TRANSLATE_LOAD_LIMIT_METHOD			2
#define B3_TRANSLATE_MANUAL_METHOD				3

#define B3_MAX_PANEL_COUNT						5
	
#define B3_MAX_STATION_COUNT					4

#define B3_MAX_TEMPERATURE_COUNT_PER_DEVICE		32

#define B3_MAX_DEVICE_NAME_LENGTH				256

#define B3_MINUTES_TO_SECONDS					60

#define B3_LM35_VOLTS_PER_C						0.010

#define B3_SYSTEM_LIGHTS						100
#define B3_SYSTEM_RUNNING						1
#define B3_SYSTEM_ALERT							2
#define B3_SYSTEM_ERROR							3

#define B3_PANEL_VORTEX_OFF						0
#define B3_PANEL_HEATING						1
#define B3_PANEL_COOLING						2

#define B3_SUB_STATE_NOT_USED					0

#define B3_DOUT_LOGIC_ON						0
#define B3_DOUT_LOGIC_OFF						1

#define B3_DIN_LOGIC_ON							1
#define B3_DIN_LOGIC_OFF						0

#define B3_SCB_68A_COUNT						4
#define B3_THERMOCOUPLES_PER_SCB_68A			7

#define B3_MIN_TEMP_IN_F						32.0
#define B3_MAX_TEMP_IN_F						210.0

#define B3_MAX_TRANSLATOR_VELOCITY_IN_IPM		60.0
#define B3_MAX_TRANSLATOR_DISTANCE_IN_INCHES	5.0
#define B3_MAX_TRANSLATOR_LOAD_IN_POUNDS		500.0

//#ifdef INSTALL_TRANSLATOR_SIMULATOR
#define B3_SIM_TRANSLATE_MAX_DATA_COUNT			16

typedef struct b3SimTranslateData
{
	double homePositionInInches;
	double parkPositionInInches;
	double translateStartPositionInInches;
	double translateStartLoadInLBS;

	double maxVelocityInIPS;
	double homeVelocityInIPS;
	double jogVelocityInIPS;
	double minPositionInInches;
	double maxPositionInInches;
	double maxLoadInLBS;
	double safeMaxLoadInLBS;
	int autoPositionToTranslateReady;
	
	unsigned int dataCount;
	double translatePositionInInches[B3_SIM_TRANSLATE_MAX_DATA_COUNT];
	double translateLoadInLBS[B3_SIM_TRANSLATE_MAX_DATA_COUNT];
} B3_SIM_TRANSLATE_DATA;
//#endif

//#ifdef INSTALL_TEMP_SIMULATOR
#define B3_SIM_THERMAL_MAX_DATA_COUNT			64

typedef struct b3SimThermalData
{
	double initRoomTempInF;
	double initBoxTempInF;
	double anchorHeatingRateInFPerSec;
	double vortexHeatingRateInFPerSec;
	double vortexCoolingRateInFPerSec;
	double tempChangeRateInFPerSec2;
	double anchorPreResinTempInF[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT];
	unsigned int anchorPostResinDataCount[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT];
	double anchorPostResinTimeInSec[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT][B3_SIM_THERMAL_MAX_DATA_COUNT];
	double anchorPostResinTempInF[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT][B3_SIM_THERMAL_MAX_DATA_COUNT];
} B3_SIM_THERMAL_DATA;
//#endif

typedef struct b3LastError
{
	int error;
	char errorMsg[512];
	char func[256];
	int line;
	CVIAbsoluteTime errTime;
} B3_LAST_ERROR;

typedef struct b3PeakTempTranslationDetectionMethodInfo
{
	double heatingDetectionThresholdInF;
	double percentTempDropForThresholdDetection;
	double percentTempDropForThresholdDetectionError;
	double deadZoneDelayInSeconds;
	double maxTimeForThresholdDetectionInSeconds;
} B3_PEAK_TEMP_TRANS_DETECTION_METHOD_INFO;

typedef struct b3FixedTimeTranslationDetectionMethodInfo
{
	double delayInSeconds;
	double maxTimeForTranslationReadyAckInSeconds;
} B3_FIXED_TIME_TRANS_DETECTION_METHOD_INFO;

typedef struct b3FixedLengthTranslateMethodInfo
{
	double velocityInIPS;
	double displacementInInches;
	double maxLoadLimitInPounts;
} B3_FIXED_LENGTH_TRANSLATE_METHOD_INFO;

typedef struct b3LoadLimitTranslateMethodInfo
{
	double velocityInIPS;
	double maxDisplacementInInches;
	double loadLimitInPounds;
} B3_LOAD_LIMIT_TRANSLATE_METHOD_INFO;

typedef struct b3ProceduerInfo
{
	char version[16];
	char name[41];
	char created[64];
	unsigned int checksum;
	double stripChartMinTempInF;
	double stripChartMaxTempInF;
	unsigned int stripChartSpanInMinutes;
	unsigned int temperatureLogPeriodInSecPerLog;
	double setPointInF;
	double setPointToleranceInF;
	int setPointAdjustable;
	double stabilizationReadyDelayInSec;
	double maxTimeToStabilizationInSeconds;
	int translationDetectionMethod;
	B3_PEAK_TEMP_TRANS_DETECTION_METHOD_INFO peakTemperatureTranslationDetectionMethodInfo;
	B3_FIXED_TIME_TRANS_DETECTION_METHOD_INFO fixedTimeTranslationDetectionMethodInfo;
	int translationMethod;
	B3_FIXED_LENGTH_TRANSLATE_METHOD_INFO fixedLengthTranslationMethodInfo;
	B3_LOAD_LIMIT_TRANSLATE_METHOD_INFO loadLimitTranslateMethodInfo;
	int postTranslationHeatingEnabled;
	double postTranslationHeatingSetPointInF;
	double postTranslationHeatingSetPointToleranceInF;
	double postTranslationHeatingSetPointTriggerDelayInSec;
	double postTranslationHeatingPeakTemperatureDropInPercent;
	double postTranslationHeatingTimeInSeconds;
	double coolDownReadyDelayInSec;
	double maxCoolDownTimeInSeconds;
} B3_PROCEDURE_INFO;

typedef struct b3StationSummaryInfo
{
	CVIAbsoluteTime startTime;
	CVIAbsoluteTime stabilizationCompleteTime;
	CVIAbsoluteTime opAckOfInjectionReadyTime;
	CVIAbsoluteTime peakTempTime;
	CVIAbsoluteTime translationReadyTime;
	CVIAbsoluteTime opAckOfTranslationReadyTime;
	CVIAbsoluteTime translationStartTime;
	CVIAbsoluteTime translationCompleteTime;
	CVIAbsoluteTime startOfPostBakeHeatingTime;
	CVIAbsoluteTime completionTime;
	CVIAbsoluteTime opAckOfCompletionTime;
	int completionState;
	double peakTempInF;
	double translationDetectTempInF;
	//double postBakeHeatingDetectTempInF;
} B3_STATION_SUMMARY_INFO;

typedef struct b3StationInfo
{
	int state;
	int subState;
	double anchorTempInF;
	char serialNum[41];
	B3_STATION_SUMMARY_INFO summary;
	char logPath[MAX_PATHNAME_LEN];
} B3_STATION_INFO;

typedef struct b3PanelInfo
{
	int enabled;
	double boxTempInF;
	double setPointInF;
	int vortex;
	int cooling;
	int postTranslationHeating;
	int stationInstalled[B3_MAX_STATION_COUNT];
	B3_STATION_INFO station[B3_MAX_STATION_COUNT];
	B3_PROCEDURE_INFO procedure;
	char activeProcedure[MAX_PATHNAME_LEN];
} B3_PANEL_INFO;

typedef struct b3TranslateInfo
{
	int connected;
	int state;
	int panel;
	int station;
	double loadInLBS;
	double displacementInInches;
	unsigned short int galilMotionControllerDeviceId;
	char logPath[MAX_PATHNAME_LEN];
	double maxLimitForTranslatorCompleteInPounds;
} B3_TRANSLATE_INFO;

typedef struct b3TempInfo
{
	char device[B3_MAX_DEVICE_NAME_LENGTH];
	double systemTempInF[B3_MAX_TEMPERATURE_COUNT_PER_DEVICE];
	uInt64 sampleRateIPerChannelnHz;
	double filterLevel;
	int boxTempIndex[B3_MAX_PANEL_COUNT];
	int anchorTempIndex[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT];
	int roomTempIndex;
	double tempCalOffset[B3_SCB_68A_COUNT];
}B3_TEMP_INFO;

typedef struct b3Control
{
	int systemOn;
	int systemAlert;
	int systemError;
	int panelVortex[B3_MAX_PANEL_COUNT];
	int panelHeating[B3_MAX_PANEL_COUNT];
	int panelCooling[B3_MAX_PANEL_COUNT];
	char systemOnDevice[B3_MAX_DEVICE_NAME_LENGTH];
	char systemAlertDevice[B3_MAX_DEVICE_NAME_LENGTH];
	char systemErrorDevice[B3_MAX_DEVICE_NAME_LENGTH];
	char panelVortexDevice[B3_MAX_PANEL_COUNT][B3_MAX_DEVICE_NAME_LENGTH];
	char panelHeatingDevice[B3_MAX_PANEL_COUNT][B3_MAX_DEVICE_NAME_LENGTH];
	char panelCoolingDevice[B3_MAX_PANEL_COUNT][B3_MAX_DEVICE_NAME_LENGTH];
	char digitalInDevice[B3_MAX_DEVICE_NAME_LENGTH];
	int translatorInPlaceIndex[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT];
	uInt8 translatorInPlace[B3_MAX_PANEL_COUNT][B3_MAX_STATION_COUNT];
	int translatorPanelIndex;
	int translatorStationIndex;
//#ifdef INSTALL_TRANSLATOR_SIMULATOR
	B3_SIM_TRANSLATE_DATA simTranslatorData;
//#endif
	B3_SIM_THERMAL_DATA simThermalData;
} B3_CONTROL;

typedef struct b3Logging
{
	char logDir[MAX_PATHNAME_LEN];
} B3_LOGGING;

typedef struct b3SystemInfo
{
	char version[16];
	char procedureDir[MAX_PATHNAME_LEN];
	int panelInstalled[B3_MAX_PANEL_COUNT];
	double roomTempInF;
	B3_PANEL_INFO panel[B3_MAX_PANEL_COUNT];
	B3_TRANSLATE_INFO translateInfo;
	B3_TEMP_INFO temp;
	B3_CONTROL control;
	B3_LOGGING logging;
	int graphsEnabled;
} B3_SYSTEM_INFO;

#ifdef __cplusplus
extern "C" {
#endif
	
#ifdef __cplusplus
}
#endif

#endif


#ifndef BORIS_3_UTILITIES
#define BORIS_3_UTILITIES

#include "BORIS 3 Definitions.h"
#include <stdint.h>

void B3_GetLastError(B3_LAST_ERROR *lastError);
int B3_LoadIni(char *path, B3_SYSTEM_INFO *system);
int B3_SaveIni(char *path, B3_SYSTEM_INFO *system);
int B3_LoadProcedure(char *path, B3_PROCEDURE_INFO *procedure);
int B3_SaveProcedure(char *path, B3_PROCEDURE_INFO *procedure);
int B3SystemLog(char *path, char *logMsg);
int B3SystemLogWithVargs(char *path, const char* const fmt, ...);
int B3SystemErrorLog(char *path, B3_LAST_ERROR *lastError);
int B3StationLog(char *path, char *elapsedTimeStr, double roomTempInF, double setPointInF, double boxTempInF, double anchorTempInF, int state, int subState);
int B3TranslationLog(char *path, double loadInPounds, double displacementInInches);
int B3StationSummaryLog(const char const *path, const B3_SYSTEM_INFO const *system, const int panelIndex, const int stationIndex);
uInt32 B3_CalcProcedureChecksum(char *path);

void _LogError(int error, char* func, int line, char* errorMsg);
void _LogErrorWithVargs(int error, char* func, int line, const char* const fmt, ...);

int B3_LoadSimTranslateData(char *path, B3_SIM_TRANSLATE_DATA *data);
int B3_LoadSimThermalData(char *path, B3_SIM_THERMAL_DATA *data);

#endif

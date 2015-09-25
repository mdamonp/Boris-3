/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  ABOUT                            1
#define  ABOUT_TEXTMSG                    2       /* control type: textMsg, callback function: (none) */
#define  ABOUT_OK                         3       /* control type: command, callback function: CloseAbout */
#define  ABOUT_DISCLAIMER                 4       /* control type: command, callback function: ShowDisclaimer */
#define  ABOUT_VERSION_HDR                5       /* control type: textMsg, callback function: (none) */
#define  ABOUT_VERSION_HDR_2              6       /* control type: textMsg, callback function: (none) */
#define  ABOUT_TEXTMSG_3                  7       /* control type: textMsg, callback function: (none) */
#define  ABOUT_PICTURE                    8       /* control type: picture, callback function: (none) */
#define  ABOUT_PICTURE_2                  9       /* control type: picture, callback function: (none) */
#define  ABOUT_VERSION                    10      /* control type: textMsg, callback function: (none) */

#define  B3_PANEL                         2
#define  B3_PANEL_ANCHOR_TEMP_4           2       /* control type: numeric, callback function: (none) */
#define  B3_PANEL_ANCHOR_TEMP_3           3       /* control type: numeric, callback function: (none) */
#define  B3_PANEL_ANCHOR_TEMP_2           4       /* control type: numeric, callback function: (none) */
#define  B3_PANEL_ANCHOR_TEMP_1           5       /* control type: numeric, callback function: (none) */
#define  B3_PANEL_BOX_STATION_4           6       /* control type: deco, callback function: (none) */
#define  B3_PANEL_SET_TEMP                7       /* control type: numeric, callback function: ChangeSetPoint */
#define  B3_PANEL_BOX_STATION_3           8       /* control type: deco, callback function: (none) */
#define  B3_PANEL_BOX_TEMP                9       /* control type: numeric, callback function: (none) */
#define  B3_PANEL_BOX_STATION_2           10      /* control type: deco, callback function: (none) */
#define  B3_PANEL_HEATING_LED             11      /* control type: LED, callback function: (none) */
#define  B3_PANEL_TRANS_INSTALLED_4       12      /* control type: LED, callback function: (none) */
#define  B3_PANEL_TRANS_INSTALLED_3       13      /* control type: LED, callback function: (none) */
#define  B3_PANEL_TRANS_INSTALLED_2       14      /* control type: LED, callback function: (none) */
#define  B3_PANEL_TRANS_INSTALLED_1       15      /* control type: LED, callback function: (none) */
#define  B3_PANEL_COOLING_LED             16      /* control type: LED, callback function: (none) */
#define  B3_PANEL_STATUS_4                17      /* control type: ring, callback function: (none) */
#define  B3_PANEL_STATUS_3                18      /* control type: ring, callback function: (none) */
#define  B3_PANEL_STATUS_2                19      /* control type: ring, callback function: (none) */
#define  B3_PANEL_STATUS_1                20      /* control type: ring, callback function: (none) */
#define  B3_PANEL_SERIAL_4                21      /* control type: string, callback function: (none) */
#define  B3_PANEL_SERIAL_3                22      /* control type: string, callback function: (none) */
#define  B3_PANEL_SERIAL_2                23      /* control type: string, callback function: (none) */
#define  B3_PANEL_SERIAL_1                24      /* control type: string, callback function: (none) */
#define  B3_PANEL_BOX_STATION_1           25      /* control type: deco, callback function: (none) */
#define  B3_PANEL_DECORATION_7            26      /* control type: deco, callback function: (none) */
#define  B3_PANEL_BANNER_1                27      /* control type: textMsg, callback function: (none) */
#define  B3_PANEL_BANNER_2                28      /* control type: textMsg, callback function: (none) */
#define  B3_PANEL_PROCEDURE               29      /* control type: string, callback function: (none) */
#define  B3_PANEL_BANNER_3                30      /* control type: textMsg, callback function: (none) */
#define  B3_PANEL_BANNER_4                31      /* control type: textMsg, callback function: (none) */
#define  B3_PANEL_DECORATION_6            32      /* control type: deco, callback function: (none) */
#define  B3_PANEL_BOX_PANEL               33      /* control type: deco, callback function: (none) */
#define  B3_PANEL_CONTROL_4               34      /* control type: binary, callback function: B3StationControl */
#define  B3_PANEL_STATUS_ALERT_4          35      /* control type: pictButton, callback function: ProcessStatusAlert */
#define  B3_PANEL_CONTROL_3               36      /* control type: binary, callback function: B3StationControl */
#define  B3_PANEL_STATUS_ALERT_3          37      /* control type: pictButton, callback function: ProcessStatusAlert */
#define  B3_PANEL_CONTROL_2               38      /* control type: binary, callback function: B3StationControl */
#define  B3_PANEL_STATUS_ALERT_2          39      /* control type: pictButton, callback function: ProcessStatusAlert */
#define  B3_PANEL_CONTROL_1               40      /* control type: binary, callback function: B3StationControl */
#define  B3_PANEL_SERIAL_REMOVE_4         41      /* control type: pictButton, callback function: RemoveSerialNumber */
#define  B3_PANEL_SERIAL_REMOVE_3         42      /* control type: pictButton, callback function: RemoveSerialNumber */
#define  B3_PANEL_SERIAL_REMOVE_2         43      /* control type: pictButton, callback function: RemoveSerialNumber */
#define  B3_PANEL_SERIAL_REMOVE_1         44      /* control type: pictButton, callback function: RemoveSerialNumber */
#define  B3_PANEL_PROCEDURE_REMOVE        45      /* control type: pictButton, callback function: RemoveProcedure */
#define  B3_PANEL_SERIAL_ALERT_4          46      /* control type: pictButton, callback function: EnterSerialNumber */
#define  B3_PANEL_SERIAL_ALERT_3          47      /* control type: pictButton, callback function: EnterSerialNumber */
#define  B3_PANEL_SERIAL_ALERT_2          48      /* control type: pictButton, callback function: EnterSerialNumber */
#define  B3_PANEL_SERIAL_ALERT_1          49      /* control type: pictButton, callback function: EnterSerialNumber */
#define  B3_PANEL_RUN_TIME_4              50      /* control type: string, callback function: (none) */
#define  B3_PANEL_STATUS_ERROR_3          51      /* control type: pictButton, callback function: ProcessStatusError */
#define  B3_PANEL_RUN_TIME_1              52      /* control type: string, callback function: (none) */
#define  B3_PANEL_STATUS_ALERT_1          53      /* control type: pictButton, callback function: ProcessStatusAlert */
#define  B3_PANEL_STATUS_ERROR_4          54      /* control type: pictButton, callback function: ProcessStatusError */
#define  B3_PANEL_RUN_TIME_2              55      /* control type: string, callback function: (none) */
#define  B3_PANEL_STATUS_ERROR_1          56      /* control type: pictButton, callback function: ProcessStatusError */
#define  B3_PANEL_PROCEDURE_ALERT         57      /* control type: pictButton, callback function: LoadProcedure */
#define  B3_PANEL_RUN_TIME_3              58      /* control type: string, callback function: (none) */
#define  B3_PANEL_STATUS_ERROR_2          59      /* control type: pictButton, callback function: ProcessStatusError */

#define  CHRT_SETUP                       3
#define  CHRT_SETUP_CANCEL                2       /* control type: command, callback function: CancelChartScale */
#define  CHRT_SETUP_PROCEDURE             3       /* control type: command, callback function: ResetScale */
#define  CHRT_SETUP_OK                    4       /* control type: command, callback function: SetNewScale */
#define  CHRT_SETUP_STATION               5       /* control type: numeric, callback function: (none) */
#define  CHRT_SETUP_PANEL                 6       /* control type: numeric, callback function: (none) */
#define  CHRT_SETUP_MAX_TEMP              7       /* control type: numeric, callback function: (none) */
#define  CHRT_SETUP_MIN_TEMP              8       /* control type: numeric, callback function: (none) */

#define  DISCLAIMER                       4
#define  DISCLAIMER_TEXTBOX               2       /* control type: textBox, callback function: (none) */
#define  DISCLAIMER_COMMANDBUTTON         3       /* control type: command, callback function: CloseDisclaimer */

#define  EAC                              5
#define  EAC_PIN                          2       /* control type: numeric, callback function: (none) */
#define  EAC_CANCEL                       3       /* control type: command, callback function: CancelAuthorization */
#define  EAC_OK                           4       /* control type: command, callback function: ProcessPin */
#define  EAC_STATION                      5       /* control type: numeric, callback function: (none) */
#define  EAC_PANEL                        6       /* control type: numeric, callback function: (none) */
#define  EAC_SERIAL                       7       /* control type: string, callback function: (none) */

#define  GALIL_ERR                        6
#define  GALIL_ERR_OK                     2       /* control type: command, callback function: CheckGalilCommConnection */
#define  GALIL_ERR_TEXTBOX                3       /* control type: textBox, callback function: (none) */

#define  GRAPH                            7
#define  GRAPH_STRIPCHART                 2       /* control type: strip, callback function: ResizeStripChart */

#define  MACHINE                          8
#define  MACHINE_ROOM_TEMP                2       /* control type: numeric, callback function: ChangeSetPoint */
#define  MACHINE_ERROR_LED                3       /* control type: LED, callback function: (none) */
#define  MACHINE_ALERT_LED                4       /* control type: LED, callback function: (none) */
#define  MACHINE_RUNNING_LED              5       /* control type: LED, callback function: (none) */

#define  MAIN                             9
#define  MAIN_EXIT_APP                    2       /* control type: command, callback function: ExitApp */

#define  PANEL_MGMT                       10
#define  PANEL_MGMT_PANEL_5               2       /* control type: radioButton, callback function: ManagePanels */
#define  PANEL_MGMT_PANEL_4               3       /* control type: radioButton, callback function: ManagePanels */
#define  PANEL_MGMT_PANEL_3               4       /* control type: radioButton, callback function: ManagePanels */
#define  PANEL_MGMT_CANCEL                5       /* control type: command, callback function: ClosePanelMgmtPanel */
#define  PANEL_MGMT_PANEL_2               6       /* control type: radioButton, callback function: ManagePanels */
#define  PANEL_MGMT_PANEL_1               7       /* control type: radioButton, callback function: ManagePanels */

#define  SERIAL_NUM                       11
#define  SERIAL_NUM_VAL                   2       /* control type: string, callback function: (none) */
#define  SERIAL_NUM_VERIFY                3       /* control type: string, callback function: (none) */
#define  SERIAL_NUM_OK                    4       /* control type: command, callback function: EnterSerialNumberOk */
#define  SERIAL_NUM_CANCEL                5       /* control type: command, callback function: EnterSerialNumberCancel */
#define  SERIAL_NUM_STATION_INDEX         6       /* control type: numeric, callback function: (none) */
#define  SERIAL_NUM_PANEL_INDEX           7       /* control type: numeric, callback function: (none) */

#define  TEMP_SIM                         12
#define  TEMP_SIM_PANEL_ID_5              2       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_PANEL_ID_4              3       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_PANEL_ID_3              4       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_PANEL_ID_2              5       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_PANEL_ID_1              6       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_4_5         7       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_3_5         8       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_2_5         9       /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_1_5         10      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_4_4         11      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_3_4         12      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_2_4         13      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_1_4         14      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_4_3         15      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_3_3         16      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_2_3         17      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_1_3         18      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_4_2         19      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_3_2         20      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_2_2         21      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_1_2         22      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_4_1         23      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_3_1         24      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_2_1         25      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ANCHOR_TEMP_1_1         26      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_ROOM_TEMP               27      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_BOX_TEMP_5              28      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_BOX_TEMP_4              29      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_BOX_TEMP_3              30      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_BOX_TEMP_2              31      /* control type: numeric, callback function: (none) */
#define  TEMP_SIM_BOX_TEMP_1              32      /* control type: numeric, callback function: (none) */

#define  TRANSL                           13
#define  TRANSL_DISP                      2       /* control type: numeric, callback function: (none) */
#define  TRANSL_STATION_ID                3       /* control type: numeric, callback function: (none) */
#define  TRANSL_PANEL_ID                  4       /* control type: numeric, callback function: (none) */
#define  TRANSL_LOAD                      5       /* control type: numeric, callback function: (none) */
#define  TRANSL_STATUS                    6       /* control type: ring, callback function: (none) */

#define  TRANSL_SIM                       14
#define  TRANSL_SIM_DECORATION_2          2       /* control type: deco, callback function: (none) */
#define  TRANSL_SIM_PANEL_ID              3       /* control type: numeric, callback function: (none) */
#define  TRANSL_SIM_STATION_ID            4       /* control type: numeric, callback function: (none) */
#define  TRANSL_SIM_DECORATION            5       /* control type: deco, callback function: (none) */
#define  TRANSL_SIM_DISP                  6       /* control type: numeric, callback function: (none) */
#define  TRANSL_SIM_LOAD                  7       /* control type: numeric, callback function: (none) */
#define  TRANSL_SIM_STATUS                8       /* control type: ring, callback function: (none) */
#define  TRANSL_SIM_SPLITTER_2            9       /* control type: splitter, callback function: (none) */
#define  TRANSL_SIM_PENDANT               10      /* control type: binary, callback function: SimPendantSwitch */
#define  TRANSL_SIM_JOG_DOWN              11      /* control type: command, callback function: SimJogDown */
#define  TRANSL_SIM_JOG_UP                12      /* control type: command, callback function: SimJogUp */
#define  TRANSL_SIM_STOP                  13      /* control type: command, callback function: SimTranslatorStopButton */
#define  TRANSL_SIM_SUPPORT               14      /* control type: command, callback function: SimTranslatorSupportButton */
#define  TRANSL_SIM_READY                 15      /* control type: command, callback function: SimTranslatorReadyButton */
#define  TRANSL_SIM_TRANS_IP              16      /* control type: LED, callback function: (none) */
#define  TRANSL_SIM_TRANS_AT_STATION      17      /* control type: binary, callback function: SimTranslatorAtStation */


     /* Control Arrays: */

#define  ANCHOR_TEMP_CA                   1
#define  BANNER_CA                        2
#define  BOX_STATION_CA                   3
#define  CONTROL_CA                       4
#define  PANEL_CA                         5
#define  RUN_TIME_CA                      6
#define  SERIAL_ALERT_CA                  7
#define  SERIAL_CA                        8
#define  SERIAL_REMOVE_CA                 9
#define  STATUS_ALERT_CA                  10
#define  STATUS_CA                        11
#define  STATUS_ERROR_CA                  12
#define  TRANSLATOR_INSTALLED_CA          13

     /* Menu Bars, Menus, and Menu Items: */

#define  MAIN_MENU                        1
#define  MAIN_MENU_SYSTEM                 2
#define  MAIN_MENU_SYSTEM_EXIT            3       /* callback function: MAIN_MENU_SystemExit */
#define  MAIN_MENU_PANEL                  4
#define  MAIN_MENU_PANEL_MANAGE           5       /* callback function: MAIN_MENU_PANEL_Manage */
#define  MAIN_MENU_HELP                   6
#define  MAIN_MENU_HELP_ABOUT             7       /* callback function: DisplayAbout */

#define  PANEL_MENU                       2
#define  PANEL_MENU_STATION               2
#define  PANEL_MENU_STATION_ADD           3       /* callback function: PANEL_MENU_STATION_Add */
#define  PANEL_MENU_STATION_REMOVE        4       /* callback function: PANEL_MENU_STATION_Remove */

#define  TRANS_MENU                       3
#define  TRANS_MENU_UTILITY               2
#define  TRANS_MENU_UTILITY_HOME          3
#define  TRANS_MENU_UTILITY_PARK          4


     /* Callback Prototypes: */

int  CVICALLBACK B3StationControl(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CancelAuthorization(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CancelChartScale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ChangeSetPoint(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CheckGalilCommConnection(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CloseAbout(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CloseDisclaimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ClosePanelMgmtPanel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK DisplayAbout(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK EnterSerialNumber(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK EnterSerialNumberCancel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK EnterSerialNumberOk(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ExitApp(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK LoadProcedure(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK MAIN_MENU_PANEL_Manage(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MAIN_MENU_SystemExit(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK ManagePanels(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK PANEL_MENU_STATION_Add(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK PANEL_MENU_STATION_Remove(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK ProcessPin(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ProcessStatusAlert(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ProcessStatusError(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RemoveProcedure(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RemoveSerialNumber(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ResetScale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ResizeStripChart(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetNewScale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ShowDisclaimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimJogDown(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimJogUp(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimPendantSwitch(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimTranslatorAtStation(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimTranslatorReadyButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimTranslatorStopButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SimTranslatorSupportButton(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif

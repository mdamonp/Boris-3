/*! \file Dmccom.h
*
* OS Upgrade for Dmccom.h.
* Derived from original Dmccom.h.
* Comments have been updated and unnecessary code has been commented out.
*
* All functions return an error code. DMCNOERROR (0) indicates function completed
* successfully. < 0 is a local error (see the error codes below). 
*
* Unsupported functions return appropriate errors.
*/

#ifndef DMCCOM_H_I_2FC43D18_4D95_434F_A3D5_6D32828701C4
#define DMCCOM_H_I_2FC43D18_4D95_434F_A3D5_6D32828701C4

#ifdef __cplusplus
	extern "C" {
#endif
/*!
* \cond
*/

// typedefs to supplant Windows.h
typedef int BOOL;
typedef unsigned long DWORD;
typedef void VOID;
typedef void *HANDLE;
// NEAR and FAR are cruft throwbacks to 16bit Win/Dos
// Kept to maintain syntax of old prototypes
#ifndef FAR
	#define FAR
#endif
#ifndef NEAR
	#define NEAR
#endif
typedef short int SHORT;
typedef short int NEAR* PSHORT;
typedef short int FAR* LPSHORT;
typedef unsigned short int USHORT;
typedef unsigned short int NEAR* PUSHORT;
typedef unsigned short int FAR* LPUSHORT;
#if defined(_WIN32) || defined(__WIN32__)
	typedef long int LONG;
#endif
typedef long int NEAR* PLONG;
typedef long int FAR* LPLONG;
typedef unsigned long int ULONG;
typedef unsigned long int NEAR* PULONG;
typedef unsigned long int FAR* LPULONG;
typedef char CHAR;
typedef char NEAR* PCHAR;
typedef char FAR* LPCHAR;
typedef unsigned char UCHAR;
typedef unsigned char NEAR* PUCHAR;
typedef unsigned char FAR* LPUCHAR;
typedef unsigned char BYTE;
typedef unsigned char NEAR* PBYTE;
typedef unsigned char FAR* LPBYTE;
typedef void NEAR* PVOID;
typedef void FAR* LPVOID;
typedef char FAR* PSZ;

#if defined(_WIN32) || defined(__WIN32__)
	#define GALILCALL __stdcall
#else
	#define GALILCALL FAR PASCAL __export
#endif

#if !defined(_WIN32) && !defined(__WIN32__)
	#ifndef TEXT
		#define TEXT
	#endif
#endif

// Galil types
typedef LONG HANDLEDMC;
typedef HANDLEDMC FAR* PHANDLEDMC;

//Deprecated types redefined for function stubs below.
typedef int GALILREGISTRY;
typedef GALILREGISTRY* PGALILREGISTRY;
typedef GALILREGISTRY* PGALILREGISTRY2;
typedef GALILREGISTRY* PGALILREGISTRY3;

/*
// Controller model constants
#define DMC400   TEXT("DMC-400")
#define DMC600   TEXT("DMC-600")
#define DMC700   TEXT("DMC-700")
#define DMC1000  TEXT("DMC-1000")
#define DMC1200  TEXT("DMC-1200")
#define DMC1300  TEXT("DMC-1300")
#define DMC1410  TEXT("DMC-1410")
#define DMC1411  TEXT("DMC-1411")
#define DMC1412  TEXT("DMC-1412")
#define DMC1413  TEXT("DMC-1413")
#define DMC1414  TEXT("DMC-1414")
#define DMC1415  TEXT("DMC-1415")
#define DMC1425  TEXT("DMC-1425")
#define DMC3425  TEXT("DMC-3425")
#define DMC1416  TEXT("DMC-1416")
#define DMC1417  TEXT("DMC-14x7")
#define DMC1500  TEXT("DMC-1500")
#define DMC1600  TEXT("DMC-1600")
#define DMC1700  TEXT("DMC-1700")
#define DMC1800  TEXT("DMC-1800")
#define DMC1802  TEXT("DMC-1802")
#define DMC1806  TEXT("DMC-18x6")
#define DMC2000  TEXT("DMC-2000")
#define DMC2100  TEXT("DMC-2100")
#define DMC2102	 TEXT("DMC-21x3/2")
#define DMC2200	 TEXT("DMC-2200")
#define CDS3300	 TEXT("CDS-33x0")
#define DMC4000	 TEXT("DMC-40x0")
#define DMC7007	 TEXT("IOC-7007")

#define MODEL_UNKNOWN       0
#define MODEL_600           600
#define MODEL_700           700
#define MODEL_1000          1000
#define MODEL_1200          1200
#define MODEL_1300          1300
#define MODEL_1410          1410
#define MODEL_1411          1411
#define MODEL_1412          1412
#define MODEL_1413          1413
#define MODEL_1414          1414
#define MODEL_1415          1415
#define MODEL_1416          1416
#define MODEL_1417          1417
#define MODEL_1500          1500
#define MODEL_1600          1600
#define MODEL_1700          1700
#define MODEL_1800          1800
#define MODEL_1802          1802
#define MODEL_1806          1806
#define MODEL_2000          2000
#define MODEL_2100          2100
#define MODEL_2102          2102
#define MODEL_2200          2200
#define MODEL_3300          3300
#define MODEL_7007			7007
#define MODEL_4000			4000
#define MODEL_USERDEFINED   9999


// General defines
#define COMMAND_SIZE        80
#define MAX_CONTROLLERS     64

// User defined message to notify application program of an interrupt
#define WM_DMCINTERRUPT    (WM_USER+128)	// Status byte from the controller is passed to user via the wParam argument
#define WM_MOTIONCOMPLETE  (WM_USER+129)
#define WM_UNSOLICITEDMSG  (WM_USER+130)
#define WM_NEWDATARECORD   (WM_USER+131)
#define WM_DMCTHREADCLOSING (WM_USER+132)
*/

// Errors
#define DMCNOERROR             0
#define DMCWARNING_MONITOR     1

#define DMCERROR_TIMEOUT              -1
#define DMCERROR_COMMAND              -2
#define DMCERROR_CONTROLLER           -3
#define DMCERROR_FILE                 -4
#define DMCERROR_DRIVER               -5
#define DMCERROR_HANDLE               -6
#define DMCERROR_HMODULE              -7
#define DMCERROR_MEMORY               -8
#define DMCERROR_BUFFERFULL           -9
#define DMCERROR_RESPONSEDATA         -10
#define DMCERROR_DMA                  -11 
#define DMCERROR_ARGUMENT             -12
#define DMCERROR_DATARECORD           -13
#define DMCERROR_DOWNLOAD             -14
#define DMCERROR_FIRMWARE             -15
#define DMCERROR_CONVERSION           -16
#define DMCERROR_RESOURCE             -17
#define DMCERROR_REGISTRY             -18
#define DMCERROR_BUSY                 -19
#define DMCERROR_DEVICE_DISCONNECTED  -20
#define DMCERROR_TIMEING_ERROR				-21
#define DMCERROR_WRITEBUFFER_TOO_LARGE		-22
#define DMCERROR_NO_MODIFY_PNP_CONTROLLER	-23
#define DMCERROR_FUNCTION_OBSOLETE			-24
#define DMCERROR_STREAMING_COMMAND_IN_PROGRESS	-25
#define DMCERROR_DEVICEDRIVER_VERSION_TOO_OLD	-26
#define DMCERROR_STREAMING_COMMAND_MUST_BE_SOLITARY	-27
#define DMCERROR_FIRMWARE_VERSION_TOO_OLD	  -28
#define DMCERROR_ETHERNET_NO_MORE_HANDLES	  -29
#define DMCERROR_NETWORK_UNREACHABLE		  -30

/*
// Constant values

// Constant values for data record access
#include "dmcdrc.h"

// Constant values for registry structures

enum DMCControllerTypes
{
   ControllerTypeISABus = 0,     // ISA or PC-104 bus controller
   ControllerTypeSerial = 1,     // RS-232 serial controller
   ControllerTypePCIBus = 2,     // PCI or Compact PCI bus controller
   ControllerTypeUSB = 3,        // Universal serial bus controller
   ControllerTypeEthernet = 4,   // Ethernet controller
   ControllerTypeVMEBus = 5      // VME bus controller
};

enum DMCDeviceDrivers
{
   DeviceDriverWinRT = 0,        // Use WinRT device driver
   DeviceDriverGalil = 1         // Use Galil device driver
};

enum DMCSerialHandshake
{
   SerialHandshakeHardware = 0,  // Hardware handshake (RTS/CTS)
   SerialHandshakeSoftware = 1,  // Software handshake (XOn/XOff) 
   SerialHandshakeBoth = 2       // RESERVED FOR GALIL USE ONLY
};

enum DMCBusIOStyle
{
   DMC600IOStyle = 0,            // For DMC-600, DMC-1400
   DMC1000IOStyle = 1,           // For DMC-1000, DMC-1200, DMC-1700, DMC-1802
   DMC1600IOStyle = 2            // For DMC-1600, DMC-1800
};

enum DMCInterruptStyle
{
   DMC1000InterruptStyle = 0,    // For DMC-1000
   DMC1400InterruptStyle = 1,    // For DMC-1400
   DMC1700InterruptStyle = 2,    // For DMC-1200, DMC-1700, DMC-1802
   DMC1600InterruptStyle = 3     // For DMC-1600, DMC-1800
};

enum DMCDataRecordAccess
{
   DataRecordAccessNone = 0,     // No data record access capability or data record access is off
   DataRecordAccessDMA = 1,      // Use DMA for data record access
   DataRecordAccessFIFO = 2,     // Use FIFO for data record access
   DataRecordAccessBoth = 3,     // RESERVED FOR GALIL USE ONLY
   DataRecordAccessQR = 4        // RESERVED FOR GALIL USE ONLY
};

enum DMCEthernetProtocol
{
   EthernetProtocolTCP = 0,
   EthernetProtocolUDP = 1
};
enum DMCVMEBusInterface
{
   VMEBusInterfaceBit3 = 0,
   VMEBusInterfaceVMIC = 1
};

// Ethernet flags
#define ETH_NO_MULTICAST					0x0001	// Do not open a multi-cast session
#define ETH_UNSOLICITEDMESSAGES				0x0002	// Open an unsolicited message session on second handle
#define ETH_UNSOLICITEDMESSAGES_SAMEHANDLE	0x0004	// Open an unsolicited message session on same handle

// Structures

// Structure used to add/change/delete registry information
// Old-style structure
typedef struct _GALILREGISTRY
{
#ifdef UNDER_CE
   TCHAR   szModel[32];          // Controller model string
#else
   CHAR    szModel[16];          // Controller model string
#endif
   USHORT  usDeviceNumber;       // Device number - for Galil use only
   USHORT  fDeviceDriver;        // Use Galil or WinRT device driver
   ULONG   ulTimeout;            // Time-out in milliseconds
   ULONG   ulDelay;              // Delay in microsceonds
   USHORT  fControllerType;      // Controller type (ISA bus, PCI bus, serial, etc.)
   USHORT  usCommPort;           // Serial communications port
   ULONG   ulCommSpeed;          // Serial Communications speed
   USHORT  fHandshake;           // Serial communications handshake
   USHORT  usAddress;            // Bus address
   USHORT  usInterrupt;          // Interrupt
   USHORT  fDataRecordAccess;    // Data record access type
   USHORT  usDMAChannel;         // DMA channel
   USHORT  usDataRecordSize;     // Data record size (for data record access)
   USHORT  usRefreshRate;        // Data record refresh rate in 2^usRefreshRate ms
   USHORT  usSerialNumber;       // Controller Serial Number
#ifdef UNDER_CE
   TCHAR   szPNPHardwareKey[128];// Hardware registry key for PNP controllers - for Galil use only
#else
   CHAR    szPNPHardwareKey[64]; // Hardware registry key for PNP controllers - for Galil use only
#endif
} GALILREGISTRY, FAR* PGALILREGISTRY;

// Hardware info - ISA and PCI bus communications
typedef struct _BUSINFO
{
   USHORT  usDeviceNumber;       // Device number - for Galil use only
   USHORT  fDeviceDriver;        // Use Galil or WinRT device driver
   USHORT  fIOStyle;             // Style of addressing status register on controller
   USHORT  usAddress;            // I/O port address
   USHORT  fInterruptStyle;      // Style of handling interrupts from controller  
   USHORT  usInterrupt;          // Interrupt
   USHORT  fDataRecordAccess;    // Data record access type
   USHORT  usDMAChannel;         // DMA channel
   USHORT  usDataRecordSize;     // Data record size (for data record access)
   USHORT  usRefreshRate;        // Data record refresh rate in 2^usRefreshRate ms
   USHORT  bPNP;                 // Plug and play? (TRUE | FALSE)
   USHORT  usAddress2;           // Alternate I/O port address for PCI controllers
   USHORT  usReserved1;          // Reserved for future use
   USHORT  nCommWaitMethod	;	 // Added 8/8/02 for specifying DMCCommand communication method.
   BOOL	   bFirmSupportsIntComm; // Added 8/8/02 for signaling if device firmware supports int comm.
   SHORT   nDRCacheDepth;		 // Added 10/28/02 for recording data record cache depth.
   ULONG   ulHardwareID;		 // Added to uniquely identify controllers by serial number and function.
#ifdef UNDER_CE
   TCHAR   szPNPHardwareKey[128];// Hardware registry key for PNP controllers - for Galil use only
#else
   CHAR    szPNPHardwareKey[55]; // Hardware registry key for PNP controllers - for Galil use only
#endif
} BUSINFO;

// Hardware info - serial communications
typedef struct _SERIALINFO
{
   USHORT  usCommPort;           // Communications port
   ULONG   ulCommSpeed;          // Communications speed
   USHORT  fHandshake;           // Communications handshake
   USHORT  usDeviceNumber;       // Device number - for Galil use only
   USHORT  usReserved1;          // Reserved for future use
} SERIALINFO;

// Hardware info - serial communications
typedef struct _WINSOCKINFO
{
   ULONG   ulPort;					// Host port number - for Galil use only
#ifdef UNDER_CE
   TCHAR   szIPAddress[64];      // Host name string
#else
   CHAR    szIPAddress[32];      // Host name string
#endif
   USHORT  fProtocol;            // UDP or TCP
   ULONG   fFlags;				 // Controls auto opening of multi-cast and unsolicited message sessions
   USHORT  fMsgProtocol;		 // Added 9/15/03. UDP or TCP. Relevant only when second handle is used for unsolicited messages.
   USHORT  fUseEthernetWait;     // Added 9/15/03. Allows Ethernet read calls to wait on a FD_READ event rather than spinning the processor.
   USHORT  fUseUnsolicitedDR;	 // Added 10/7/03. Commands driver to open dedicated data record handle.
   USHORT  nRefreshRate;         // Added 10/15/03. Sets the ethernet DR command frequency in millisec.
   USHORT  nCacheDepth;			 // Added 12/1/03.  Allows ethernet implementation of DMCmGetDataRecordConstPointerArray and DMCmGetDataRecordArray.
   USHORT  usReserved3;          // Reserved for future use
} WINSOCKINFO;

// Hardware info - ISA and PCI bus communications
typedef struct _VMEBUSINFO
{
   USHORT  usDeviceNumber;       // Device number - for Galil use only
   USHORT  fDeviceDriver;        // Use Galil or WinRT device driver
	USHORT  fInterface;           // Which VME to PC interface
	ULONG   ulMemoryAddress;      // Physical memory address of VME interface card
	ULONG   ulMemoryOffset;       // Offset in memory to Galil controller
   USHORT  usAddress;            // I/O port address for VME interface card
   USHORT  fIOStyle;             // Style of addressing status register on controller
   USHORT  fInterruptStyle;      // Style of handling interrupts from controller  
   USHORT  usInterrupt;          // Interrupt (IRQ) mapped to PC (jumper on VME interface card)
   USHORT  usInterruptLevel;     // Interrupt level on the VME bus (jumper on controller)
   USHORT  fDataRecordAccess;    // Data record access type
   USHORT  usDataRecordSize;     // Data record size (for data record access)
   USHORT  usRefreshRate;        // Data record refresh rate in 2^usRefreshRate ms
   USHORT  bPNP;                 // Plug and play? (TRUE | FALSE)
   ULONG   ulReserved1;          // Reserved for future use
   ULONG   ulReserved2;          // Reserved for future use
   ULONG   ulReserved3;          // Reserved for future use
   ULONG   ulReserved4;          // Reserved for future use
} VMEBUSINFO;

// Hardware info for registry
typedef union _HARDWAREINFO
{
   BUSINFO      businfo;         // ISA and PCI bus information
   SERIALINFO   serialinfo;      // Serial information
	WINSOCKINFO  winsockinfo;     // WinSock (e.g. Ethernet) information
	VMEBUSINFO   vmebusinfo;      // VME bus information
} HARDWAREINFO; 

// Structure used to add/change/delete registry information
// New-style structure
typedef struct _GALILREGISTRY2
{
	USHORT  usVersion;                   // Structure version
	CHAR    szModel[16];                 // Controller model string
	USHORT  usModelID;                   // Model ID
	USHORT  fControllerType;             // Controller type (ISA bus, PCI bus, serial, etc.)
	ULONG   ulTimeout;                   // Time-out in milliseconds
	ULONG   ulDelay;                     // Delay in microsceonds
	ULONG   ulSerialNumber;              // Controller serial number
	HARDWAREINFO hardwareinfo;           // Union defining the hardware characteristics of the controller
} GALILREGISTRY2, FAR* PGALILREGISTRY2;

// Added 8/4/04 to support friendly name.
typedef struct _GALILREGISTRY3
{
	USHORT  usVersion;                   // Structure version
	CHAR    szModel[16];                 // Controller model string
	USHORT  usModelID;                   // Model ID
	USHORT  fControllerType;             // Controller type (ISA bus, PCI bus, serial, etc.)
	ULONG   ulTimeout;                   // Time-out in milliseconds
	ULONG   ulDelay;                     // Delay in microsceonds
	ULONG   ulSerialNumber;              // Controller serial number
	HARDWAREINFO hardwareinfo;           // Union defining the hardware characteristics of the controller
	CHAR	szDescription[16];			 // 8/4/04.  Added so that the controller can be referred to by description.
} GALILREGISTRY3, FAR* PGALILREGISTRY3;

// Hardware info for user-defined bus controllers
// RESERVED FOR GALIL USE ONLY 
typedef struct _BUSDEF
{
   SHORT iIOType;
   SHORT iIOMin;
   SHORT iIOMax;
   SHORT iIOSize;
   SHORT iInterruptType;
   SHORT iInterrupts;
   SHORT iInterruptList[16];
   SHORT iDataRecordAccessType;
   SHORT iDMAChannels;
   SHORT iDMAChannelList[8];
   SHORT nCommWaitMethod;
   UCHAR bFirmSupportsIntComm;
} BUSDEF;

// Hardware info for user-defined serial controllers 
// RESERVED FOR GALIL USE ONLY 
typedef struct _SERIALDEF
{
   SHORT iCommPorts;
   SHORT iCommPortList[16];
   SHORT iCommSpeeds;
   LONG  iCommSpeedList[16];
   SHORT iHandshake;
} SERIALDEF;

// Hardware info for user-defined controllers 
// RESERVED FOR GALIL USE ONLY 
typedef union _HARDWAREDEF
{
   BUSDEF     busdef;
   SERIALDEF  serialdef;
} HARDWAREDEF;

// Structure used to describe user-defined controllers
// RESERVED FOR GALIL USE ONLY 
typedef struct _CONTROLLERDEF
{
#ifdef UNDER_CE
   TCHAR szModel[32];
#else
   CHAR  szModel[16];
#endif
   SHORT iControllerType;
   HARDWAREDEF hardwaredef;
} CONTROLLERDEF;
*/

/*!
* \endcond
*/

//! Open communications with the Galil controller.
/*! 
*  The handle to the Galil controller is returned in the argument phdmc.
*
* \param usController  A number between 1 and 16.
*                      Up to 16 Galil controllers may be addressed per process.
*                      Maps to "number" attribute of controller tag in registry.xml.
* \param hwnd          Not used in Windows OS Upgrade version. 
*                      Maintained to provide argument compatibility with the legacy version.
* \param phdmc         Buffer to receive the handle to the Galil controller to be
*                      used for all subsequent API calls. Users should declare a
*                      variable of type HANDLEDMC and pass the address of the
*                      variable to the function. Output only.
*/
LONG FAR GALILCALL DMCOpen(USHORT usController,int temp,
   PHANDLEDMC phdmc);

//! Open communications with the Galil controller. DMCOpen2 is a wrapper around DMCOpen.
/*!
*  The handle to the Galil controller is returned in the argument phdmc.
*
* \param usController  A number between 1 and 16.
*                      Up to 16 Galil controllers may be addressed per process.
*                      Maps to "number" attribute of controller tag in registry.xml.
* \param lThreadID     Not used in Windows OS Upgrade version.
*                      Maintained to provide argument compatibility with the legacy version.
* \param phdmc         Buffer to receive the handle to the Galil controller to be
*                      used for all subsequent API calls. Users should declare a
*                      variable of type HANDLEDMC and pass the address of the
*                      variable to the function. Output only.
*/
LONG FAR GALILCALL DMCOpen2(USHORT usController, LONG lThreadID,
   PHANDLEDMC phdmc)
{
	return DMCOpen(usController, 0, phdmc); 
}

/*!
* \cond
*/
LONG FAR GALILCALL DMCOpenDesc(PSZ pszDescription,int temp, PHANDLEDMC phdmc, PUSHORT pControllerNum)
{
	return DMCERROR_ARGUMENT; //error code returned from legacy library when description not found.
}


LONG FAR GALILCALL DMCOpenDesc2(PSZ pszDescription, LONG lThreadID, PHANDLEDMC phdmc, PUSHORT pControllerNum)
{
	return DMCERROR_ARGUMENT;
}


LONG FAR GALILCALL DMCGetHandle(USHORT usController, PHANDLEDMC phdmc)
{
	return DMCERROR_HANDLE; 
}

HANDLE GALILCALL DMCInterruptEventHandle( USHORT temp)
{
	return 0;
}

LONG FAR GALILCALL DMCChangeInterruptNotification(HANDLEDMC hdmc, LONG lHandle)
{
	return DMCERROR_HANDLE; 
}
/*!
* \endcond
*/


//! Close communications with the Galil controller.
/*!
*  User must call close when done to disconnect from the controller. 
*  Failure to do so may result in the failure of future DMCOpen() calls until the controller/PC is reset.
*
*  \param hdmc Handle to the Galil controller.  
*/
LONG FAR GALILCALL DMCClose(HANDLEDMC hdmc);

//! Send a DMC command in ascii format to the Galil controller.
/*!
*  Performs a command-and-response transaction with the hardware.
*
*  \param hdmc        Handle to the Galil controller.
*
*  \param pszCommand  The command to send to the Galil controller.
*
*  \param pchResponse Buffer to receive the response data. If the buffer is too
*                     small to recieve all the response data from the controller,
*                     the error code DMCERROR_BUFFERFULL will be returned. The
*                     user may get additional response data by calling the
*                     function DMCGetAdditionalResponse. The length of the
*                     additonal response data may ascertained by calling the
*                     function DMCGetAdditionalResponseLen. If the response
*                     data from the controller is too large for the internal
*                     additional response buffer, the error code
*                     DMCERROR_RESPONSEDATA will be returned. Output only.
*
*  \param cbResponse  Length of the buffer.
*/
LONG FAR GALILCALL DMCCommand(HANDLEDMC hdmc, PSZ pszCommand,
   LPCHAR pchResponse, ULONG cbResponse);

/*!
* \cond
*/
LONG FAR GALILCALL DMCFastCommand(HANDLEDMC hdmc, PSZ pszCommand)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCBinaryCommand(HANDLEDMC hdmc, LPBYTE pbCommand, ULONG ulCommandLength,
	LPCHAR pchResponse, ULONG cbResponse)
{
	return DMCERROR_HANDLE; 
}
/*!
* \endcond
*/

//! Query the Galil controller for unsolicited responses. 
/*! 
*  Unsolicited messages output from programs running in the background on the Galil controller.
*  The data placed in the user buffer (pchResponse) is NULL terminated.
*
*  \param hdmc              Handle to the Galil controller.
*  \param pchResponse       Buffer to receive the response data.
*  \param cbResponse        Length of the buffer.
*/
LONG FAR GALILCALL DMCGetUnsolicitedResponse(HANDLEDMC hdmc,
	LPCHAR pchResponse, ULONG cbResponse);

/*!
* \cond
*/
LONG FAR GALILCALL DMCWriteData(HANDLEDMC hdmc,
	LPCHAR pchBuffer, ULONG cbBuffer, LPULONG pulBytesWritten)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCReadData(HANDLEDMC hdmc,
	LPCHAR pchBuffer, ULONG cbBuffer, LPULONG pulBytesRead)
{
	return DMCERROR_HANDLE; 
}

/*!
* \endcond
*/

//! Query the Galil controller for the length of additional response data. 
/*! 
* There will be more response data available if the DMCCommand function returned
* DMCERROR_BUFFERFULL.
*
* \param hdmc              Handle to the Galil controller.
* \param pulResponseLen    Buffer to receive the additional response data length. Output only.
*/
LONG FAR GALILCALL DMCGetAdditionalResponseLen(HANDLEDMC hdmc,
	LPULONG pulResponseLen);

//! Query the Galil controller for more response data.
/*! 
*  There will be more response data available if the DMCCommand function returned
*  DMCERROR_BUFFERFULL. Once this function is called, the internal
*  additonal response buffer is cleared.
*
*  \param hdmc              Handle to the Galil controller.
*  \param pchResponse       Buffer to receive the response data. Output only.
*  \param cbResponse        Length of the buffer.
*/
LONG FAR GALILCALL DMCGetAdditionalResponse(HANDLEDMC hdmc,
	LPCHAR pchResponse, ULONG cbResponse);

/*!
* \cond
*/
LONG FAR GALILCALL DMCError(HANDLEDMC hdmc, LONG lError, LPCHAR pchMessage,
	ULONG cbMessage)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCClear(HANDLEDMC hdmc)
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCReset(HANDLEDMC hdmc)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCMasterReset(HANDLEDMC hdmc)
{
	return DMCERROR_HANDLE; 
}

/*!
* \endcond
*/


//! Get the version of the Galil controller.
/*!
*  \note
*  The output of DMCVersion() in the OSU version is NOT identical to the legacy version.
*  The following are examples of the output.
*        
*      Galil Motion Controller DMC4020 Rev 1.2b (OSU) 192.168.0.43
*      Galil Motion Controller DMC1846 Rev 1.0c (OSU) GALILPCI1
*      Galil Motion Controller DMC4020 Rev 1.2b (OSU) COM1
*  
*  \param hdmc              Handle to the Galil controller.
*  \param pchVersion        Buffer to receive the version information. Output only.
*  \param cbVersion         Length of the buffer.
*/
LONG FAR GALILCALL DMCVersion(HANDLEDMC hdmc, LPCHAR pchVersion,
	ULONG cbVersion);

//! Download a file to the Galil controller.
/*!
* \param hdmc              Handle to the Galil controller.
* \param pszFileName       File name to download to the Galil controller.
* \param pszLabel          Program label download destination. This argument is ignored if NULL.
*/
LONG FAR GALILCALL DMCDownloadFile(HANDLEDMC hdmc, PSZ pszFileName,
   PSZ pszLabel);

//! Download from a buffer to the Galil controller.
/*!
* \param hdmc              Handle to the Galil controller.
* \param pszBuffer         Buffer of DMC commands to download to the Galil controller.
* \param pszLabel          Program label download destination. This argument is ignored if NULL.
*/
LONG FAR GALILCALL DMCDownloadFromBuffer(HANDLEDMC hdmc, PSZ pszBuffer,
   PSZ pszLabel);

//! Upload a file from the Galil controller.
/*!
* \param hdmc              Handle to the Galil controller.
* \param pszFileName       File name to upload from the Galil controller.
*/
LONG FAR GALILCALL DMCUploadFile(HANDLEDMC hdmc, PSZ pszFileName);

//! Upload to a buffer from the Galil controller.
/*!
* \param hdmc              Handle to the Galil controller.
* \param pchBuffer         Buffer of DMC commands to upload from the Galil controller.
*                          Output only.
* \param cbBuffer          Length of the buffer.
*/
LONG FAR GALILCALL DMCUploadToBuffer(HANDLEDMC hdmc, LPCHAR pchBuffer,
	ULONG cbBuffer);

/*!
* \cond
*/

LONG FAR GALILCALL DMCSendFile(HANDLEDMC hdmc, PSZ pszFileName)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCSendBinaryFile(HANDLEDMC hdmc, PSZ pszFileName)
{
	return DMCERROR_HANDLE; 
}

/*!
* \endcond
*/

//! Download an array to the Galil controller.
/*! 
* The array must exist on the controller. Array data can be
* delimited by a comma or CR (0x0D) or CR/LF (0x0D0A).
* \note The firmware on the controller must be recent enough to support the QD command.
*
* \param hdmc              Handle to the Galil controller.
* \param pszArrayName      Array name to download to the Galil controller.
* \param usFirstElement    First array element.
* \param usLastElement     Last array element.
* \param pchData           Buffer to write the array data from. Data does not need to be
*                          NULL terminated.
* \param cbData            Length of the array data in the buffer.
* \param cbBytesWritten    Number of bytes written.
*/
LONG FAR GALILCALL DMCArrayDownload(HANDLEDMC hdmc, PSZ pszArrayName,
   USHORT usFirstElement, USHORT usLastElement, LPCHAR pchData, ULONG cbData,
	LPULONG cbBytesWritten);

//! Upload an array from the Galil controller.
/*!
* The array must exist on the controller. Array data will be
* delimited by a comma or CR (0x0D) depending of the value of fComma.
* \note The firmware on the controller must be recent enough to support the QU command.
*
* \param hdmc              Handle to the Galil controller.
* \param pszArrayName      Array name to upload from the Galil controller.
* \param usFirstElement    First array element.
* \param usLastElement     Last array element.
* \param pchData           Buffer to read the array data into. Array data will not be
*                          NULL terminated.
* \param cbData            Length of the buffer.
* \param pulBytesRead      Number of bytes read.
* \param fComma            0 = delimit by "\r", 1 = delimit by ",".
*/
LONG FAR GALILCALL DMCArrayUpload(HANDLEDMC hdmc, PSZ pszArrayName,
   USHORT usFirstElement, USHORT usLastElement, LPCHAR pchData, ULONG cbData,
	LPULONG pulBytesRead, SHORT fComma);

/*!
* \cond
*/

LONG FAR GALILCALL DMCCommand_AsciiToBinary(HANDLEDMC hdmc, PSZ pszAsciiCommand,
   ULONG ulAsciiCommandLength, LPBYTE pbBinaryResult,
	ULONG cbBinaryResult, LPULONG pulBinaryResultLength)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCCommand_BinaryToAscii(HANDLEDMC hdmc, LPBYTE pbBinaryCommand,
   ULONG ulBinaryCommandLength, PSZ pszAsciiResult,
	ULONG cbAsciiResult, LPULONG pulAsciiResultLength)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCFile_AsciiToBinary(HANDLEDMC hdmc, PSZ pszInputFileName,
   PSZ pszOutputFileName)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCFile_BinaryToAscii(HANDLEDMC hdmc, PSZ pszInputFileName,
   PSZ pszOutputFileName)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCReadSpecialConversionFile(HANDLEDMC hdmc, PSZ pszFileName)
{
	return DMCERROR_HANDLE; 
}

/*!
* \endcond
*/

//! Refresh the data record used for fast polling.
/*!
* \param hdmc              Handle to the Galil controller.
* \param ulLength          Deprecated, set to zero.
*/
LONG FAR GALILCALL DMCRefreshDataRecord(HANDLEDMC hdmc, ULONG ulLength);

/*!
* \cond
*/

LONG FAR GALILCALL DMCGetDataRecord(HANDLEDMC hdmc, USHORT usGeneralOffset,
	USHORT usAxisInfoOffset, LPUSHORT pusDataType, LPLONG plData)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCGetDataRecordByItemId(HANDLEDMC hdmc, USHORT usItemId,
	USHORT usAxisId, LPUSHORT pusDataType, LPLONG plData)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCGetDataRecordSize(HANDLEDMC hdmc, LPUSHORT pusRecordSize)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCCopyDataRecord(HANDLEDMC hdmc, PVOID pDataRecord)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCGetDataRecordRevision(HANDLEDMC hdmc, LPUSHORT pusRevision)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCDiagnosticsOn(HANDLEDMC hdmc, PSZ pszFileName,
	SHORT fAppend)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCDiagnosticsOff(HANDLEDMC hdmc)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCGetTimeout(HANDLEDMC hdmc, LONG FAR* pTimeout)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCSetTimeout(HANDLEDMC hdmc, LONG lTimeout)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCGetDelay(HANDLEDMC hdmc, LONG FAR* pDelay)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCSetDelay(HANDLEDMC hdmc, LONG lDelay)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCAddGalilRegistry(PGALILREGISTRY pgalilregistry,
	LPUSHORT pusController)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCAddGalilRegistry2(PGALILREGISTRY2 pgalilregistry2,
	LPUSHORT pusController)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCAddGalilRegistry3(PGALILREGISTRY3 pgalilregistry3,
	LPUSHORT pusController)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCModifyGalilRegistry(USHORT usController,
	PGALILREGISTRY pgalilregistry)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCModifyGalilRegistry2(USHORT usController,
	PGALILREGISTRY2 pgalilregistry2)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCModifyGalilRegistry3(USHORT usController,
	PGALILREGISTRY3 pgalilregistry3)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCGetGalilRegistryInfo(USHORT usController,
	PGALILREGISTRY pgalilregistry)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCGetGalilRegistryInfo2(USHORT usController,
	PGALILREGISTRY2 pgalilregistry2)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCGetGalilRegistryInfo3(USHORT usController,
	PGALILREGISTRY3 pgalilregistry3)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCEnumGalilRegistry(USHORT FAR* pusCount,
	PGALILREGISTRY pgalilregistry)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCEnumGalilRegistry2(USHORT FAR* pusCount,
	PGALILREGISTRY2 pgalilregistry2)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCEnumGalilRegistry3(USHORT FAR* pusCount,
	PGALILREGISTRY3 pgalilregistry3)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCDeleteGalilRegistry(SHORT sController)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCGetControllerDesc(USHORT usController, PSZ pszControllerDesc, ULONG cbControllerDesc)
{
	return DMCERROR_RESOURCE;
}

LONG FAR GALILCALL DMCRegisterPnpControllers(USHORT* pusCount)
{
	*pusCount = 0;
	return DMCNOERROR;
}

SHORT FAR GALILCALL DMCSelectController(/*HWND*/  int * hwnd)
{
	return -1; //No controller selected. 
	//1 would likely be the desired answer, but not safe for multi-controller apps.
}

VOID FAR GALILCALL DMCEditRegistry(/*HWND*/  int * hwnd)
{
	return;
}

LONG FAR GALILCALL DMCAssignIPAddress(/*HWND*/  int * hWnd, PGALILREGISTRY2 pgalilregistry2)
{ 
	return DMCERROR_ARGUMENT;
}

LONG FAR GALILCALL DMCWaitForMotionComplete(HANDLEDMC hdmc, PSZ pszAxes, SHORT fDispatchMsgs)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCDownloadFirmwareFile(HANDLEDMC hdmc, PSZ pszFileName, SHORT fDisplayDialog)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCCompressFile(PSZ pszInputFileName, PSZ pszOutputFileName, USHORT usLineWidth,
	PUSHORT pusLineCount)
{
	return DMCERROR_FILE;
}

//Start Functions in the legacy version of dmc32.dll, but not defined in legacy dmccom.h
/*
LONG FAR GALILCALL DMCOpen3(PGALILREGISTRY2 pgalilregistry2, LONG lHandle, USHORT fhwnd, PHANDLEDMC phdmc)
{ 	
	return DMCERROR_HANDLE; 
}
LONG FAR GALILCALL GetAxisNumAndModelIndex(long hDMC, unsigned short *pNumAxis, long *pModelIndex)
{
	*pNumAxis = 8;
	*pModelIndex = MODEL_UNKNOWN;
	return DMCNOERROR;
}
void FAR GALILCALL GetModelStringFromModelIndex(long *pModelIndex, char *pModelStringBuffer)
{
	*pModelStringBuffer = 0; //null string 
}
LONG FAR GALILCALL GetModelIndexFromModelString(char *pModelStringBuffer)
{
	return MODEL_UNKNOWN;
}
*/
//End Functions in the legacy version of dmc32.dll, but not defined in legacy dmccom.h


#if defined(_WIN32) || defined(__WIN32__)
// Win32 device driver functions

LONG FAR GALILCALL DMCReadRegister(HANDLEDMC hdmc, USHORT usOffset, PBYTE pbStatus)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCWriteRegister(HANDLEDMC hdmc, USHORT usOffset, BYTE bStatus)
{
	return DMCERROR_HANDLE; 
}

LONG FAR GALILCALL DMCStartDeviceDriver(USHORT usController)
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCStopDeviceDriver(USHORT usController)
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCStopDeviceDriverEx( DWORD dwType )
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCStartDeviceDriverEx( DWORD dwType )
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCQueryDeviceDriver( DWORD dwType )
{
	return DMCNOERROR;
}

LONG FAR GALILCALL DMCSendCW2OnClose( HANDLEDMC hdmc, BOOL *pbValue )
{
	return DMCERROR_HANDLE;
}

LONG FAR GALILCALL DMCGetDataRecordItemOffsetById( HANDLEDMC hdmc, USHORT usItemId,
   USHORT usAxisId, LPUSHORT pusOffset, LPUSHORT pusDataType )
{
	return DMCERROR_HANDLE;
}

/*!
* \endcond
*/

//! Get a const pointer to the most recent data record.
/*!
* using this method to access the information
* in the data record eliminates the copying necessary with DMCCopyDataRecord.  
*
* \param hdmc              Handle to the Galil controller.
* \param pchDataRecord     Const pointer to the data record.
*/
LONG FAR GALILCALL DMCGetDataRecordConstPointer(HANDLEDMC hdmc, const char **pchDataRecord);

/*!
* \cond
*/

LONG FAR GALILCALL DMCGetDataRecordConstPointerArray(HANDLEDMC hdmc, const char **pchDataRecord, LPUSHORT pusNumDataRecords)
{
	return DMCERROR_HANDLE;
}


#ifdef __cplusplus

struct CDMCFullDataRecord;	// Forward declaration.
LONG FAR GALILCALL DMCGetDataRecordArray(HANDLEDMC hdmc, CDMCFullDataRecord **pDataRecordArray, LPUSHORT pusNumDataRecords)
{
	return DMCERROR_HANDLE;
}

struct CDMCFullDataRecord2;	// Forward declaration.
LONG FAR GALILCALL DMCGetDataRecordArray2(HANDLEDMC hdmc, CDMCFullDataRecord2 **pDataRecordArray, LPUSHORT pusNumDataRecords)
{
	return DMCERROR_HANDLE;
}

/*!
* \endcond
*/

#endif

#endif

#ifdef __cplusplus
	}
#endif

#endif // DMCCOM_H_I_2FC43D18_4D95_434F_A3D5_6D32828701C4

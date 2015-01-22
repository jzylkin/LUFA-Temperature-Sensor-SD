/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the TemperatureDataLogger project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "TempDataLogger.h"
#include "IO_Macros.h"
#include "Config_IO.h"

/** LUFA Mass Storage Class driver interface configuration and state information. This structure is
 *  passed to all Mass Storage Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MS_Device_t Disk_MS_Interface =
	{
		.Config =
			{
				.InterfaceNumber           = INTERFACE_ID_MassStorage,
				.DataINEndpoint            =
					{
						.Address           = MASS_STORAGE_IN_EPADDR,
						.Size              = MASS_STORAGE_IO_EPSIZE,
						.Banks             = 1,
					},
				.DataOUTEndpoint           =
					{
						.Address           = MASS_STORAGE_OUT_EPADDR,
						.Size              = MASS_STORAGE_IO_EPSIZE,
						.Banks             = 1,
					},
				.TotalLUNs                 = 1,
			},
	};

/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevHIDReportBuffer[GENERIC_REPORT_SIZE];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Generic_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = INTERFACE_ID_HID,
				.ReportINEndpoint             =
					{
						.Address              = GENERIC_IN_EPADDR,
						.Size                 = GENERIC_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
			},
	};

/** Non-volatile Logging Interval value in EEPROM, stored as a number of 500ms ticks */
static uint8_t EEMEM LoggingInterval500MS_EEPROM = DEFAULT_LOG_INTERVAL;

/** SRAM Logging Interval value fetched from EEPROM, stored as a number of 500ms ticks */
static uint8_t LoggingInterval500MS_SRAM;

/** Total number of 500ms logging ticks elapsed since the last log value was recorded */
static uint16_t CurrentLoggingTicks;

/** FAT Fs structure to hold the internal state of the FAT driver for the Dataflash contents. */
static FATFS DiskFATState;

/** FAT Fs structure to hold a FAT file handle for the log data write destination. */
static FIL TempLogFile;



static bool FileIsOpen;



/** ISR to handle the 500ms ticks for sampling and data logging */
ISR(TIMER1_COMPA_vect, ISR_BLOCK)
{
	/* Only log when not connected to a USB host */
	if (USB_DeviceState != DEVICE_STATE_Configured)
	{
		char     LineBuffer[100];
		uint16_t BytesWritten;

		BytesWritten = sprintf(LineBuffer, "TEST1 TEST2 TEST3/r/n");//write the letter a to the write buffer.  BytesWritten is the number of bytes, conveniently returned by sprintf
		
		f_write(&TempLogFile, LineBuffer, BytesWritten, &BytesWritten);
		f_sync(&TempLogFile);
	}
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	/* Mount and open the log file on the Dataflash FAT partition */
	OpenLogFile();
	
	GlobalInterruptEnable();

	for (;;)
	{
		MS_Device_USBTask(&Disk_MS_Interface);
		HID_Device_USBTask(&Generic_HID_Interface);
		USB_USBTask();
		if(FileIsOpen && USB_DeviceState == DEVICE_STATE_Configured){
			CloseLogFile();
		}
		else if(!FileIsOpen){
			OpenLogFile();
		}
	}
}

/** Opens the log file on the Dataflash's FAT formatted partition according to the current date */
void OpenLogFile(void)
{
	char LogFileName[12]; // max length of file name is 12 characters, I guess
	static uint16_t FileNumber=0; // defaults to 0
	FileNumber ++; //Increment the Filenumber
	/* Get the current date for the filename as "DDMMYY.csv" */
	sprintf(LogFileName, "F%05d.csv", FileNumber);
	FRESULT diskstatus;


	
	diskstatus = f_mount(&DiskFATState,"",1);
	for(int i=0; i<=diskstatus;i++){
		set_low(LED1);
		Delay_MS(500);
		set_high(LED1);
		Delay_MS(500);
	}
	while(1){;}
	
		/* Mount the storage device, open the file */
		if(diskstatus == FR_NO_FILESYSTEM){//if there is no file system,
			set_low(LED1);
			diskstatus = f_mkfs("", 0,0); //make a new fat file system in default partition with automatically determined sector size.
		}
		set_high(LED1);
	if (diskstatus == FR_OK){
		f_open(&TempLogFile, LogFileName, FA_OPEN_ALWAYS | FA_WRITE);
		f_lseek(&TempLogFile, TempLogFile.fsize);
		FileIsOpen = true;

	}
}

/** Closes the open data log file on the Dataflash's FAT formatted partition */
void CloseLogFile(void)
{
	/* Sync any data waiting to be written, unmount the storage device */
	f_sync(&TempLogFile);
	f_close(&TempLogFile);
	FileIsOpen = false;
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#endif

	/* Hardware Initialization */
	Config_IO();
	SDCardManager_Init();
	USB_Init();

	/* Check if the Dataflash/SD is working, abort if not */ // this function does not currently do anything -- just returns true
	if (!(SDCardManager_CheckDataflashOperation()))
	{
		for(;;);
	}

	/* Clear Dataflash sector protections, if enabled */
//	SDCardManager_ResetDataflashProtections(); //THIS FUNCTION NO LONGER EXISTS
}

uint32_t get_num_of_sectors(){
	return 	DiskFATState.fsize;
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{

}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{

}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);
	ConfigSuccess &= MS_Device_ConfigureEndpoints(&Disk_MS_Interface);

}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MS_Device_ProcessControlRequest(&Disk_MS_Interface);
	HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

/** Mass Storage class driver callback function the reception of SCSI commands from the host, which must be processed.
 *
 *  \param[in] MSInterfaceInfo  Pointer to the Mass Storage class interface configuration structure being referenced
 */
bool CALLBACK_MS_Device_SCSICommandReceived(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo)
{
	bool CommandSuccess;
	
	CommandSuccess = SCSI_DecodeSCSICommand(MSInterfaceInfo);

	return CommandSuccess;
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	Device_Report_t* ReportParams = (Device_Report_t*)ReportData;

//	RTC_GetTimeDate(&ReportParams->TimeDate);

	ReportParams->LogInterval500MS = LoggingInterval500MS_SRAM;

	*ReportSize = sizeof(Device_Report_t);
	return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	Device_Report_t* ReportParams = (Device_Report_t*)ReportData;


	/* If the logging interval has changed from its current value, write it to EEPROM */
	if (LoggingInterval500MS_SRAM != ReportParams->LogInterval500MS)
	{
		LoggingInterval500MS_SRAM = ReportParams->LogInterval500MS;
		eeprom_update_byte(&LoggingInterval500MS_EEPROM, LoggingInterval500MS_SRAM);
	}
}


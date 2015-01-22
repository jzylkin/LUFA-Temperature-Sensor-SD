/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
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
 *  Functions to manage the physical dataflash media, including reading and writing of
 *  blocks of data. These functions are called by the SCSI layer when data must be stored
 *  or retrieved to/from the physical storage media. If a different media is used (such
 *  as a SD card or EEPROM), functions similar to these will need to be generated.
 */

#define  INCLUDE_FROM_SDCARDMANAGER_C
#include "SDCardManager.h"
#include "diskio.h"
#include "IO_Macros.h"

static uint32_t CachedTotalBlocks = 0;
static uint8_t Buffer[512];

void SDCardManager_Init(void)
{
	set_low(SD_POWER);
	Delay_MS(1000);
//	while(!disk_initialize(0))
//		printf_P(PSTR("MMC/SD initialization failed\r\n"));

	disk_initialize(0);
}

uint32_t SDCardManager_GetNbBlocks(void)
{
	
	if (CachedTotalBlocks != 0)
		return CachedTotalBlocks;
		
	CachedTotalBlocks = get_num_of_sectors(); //a sector is the same thing as a block in SD world
	
	if(CachedTotalBlocks == 0)
	{
	//	printf_P(PSTR("Error reading SD card info\r\n"));
		return 0;
	}

	
	//printf_P(PSTR("SD blocks: %li\r\n"), TotalBlocks);
	
	return CachedTotalBlocks;
}

/** Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board dataflash IC(s), from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and writes
 *  them to the dataflash in Dataflash page sized blocks.
 *
 *  \param[in] BlockAddress  Data block starting address for the write sequence
 *  \param[in] TotalBlocks   Number of blocks of data to write
 */
uintptr_t SDCardManager_WriteBlockHandler(uint8_t* buffer, uint8_t offset)
{
	/* Check if the endpoint is currently empty */
	if (!(Endpoint_IsReadWriteAllowed()))
	{
		/* Clear the current endpoint bank */
		Endpoint_ClearOUT();
		
		/* Wait until the host has sent another packet */
		if (Endpoint_WaitUntilReady())
		  return 0;
	}
	
	/* Write one 16-byte chunk of data to the dataflash */
	buffer[0+offset] = Endpoint_Read_8();
	buffer[1+offset] = Endpoint_Read_8();
	buffer[2+offset] = Endpoint_Read_8();
	buffer[3+offset] = Endpoint_Read_8();
	buffer[4+offset] = Endpoint_Read_8();
	buffer[5+offset] = Endpoint_Read_8();
	buffer[6+offset] = Endpoint_Read_8();
	buffer[7+offset] = Endpoint_Read_8();
	buffer[8+offset] = Endpoint_Read_8();
	buffer[9+offset] = Endpoint_Read_8();
	buffer[10+offset] = Endpoint_Read_8();
	buffer[11+offset] = Endpoint_Read_8();
	buffer[12+offset] = Endpoint_Read_8();
	buffer[13+offset] = Endpoint_Read_8();
	buffer[14+offset] = Endpoint_Read_8();
	buffer[15+offset] = Endpoint_Read_8();
	
	return 16;
}

void SDCardManager_WriteBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo, uint32_t BlockAddress, uint16_t TotalBlocks)
{
	bool     UsingSecondBuffer   = false;
	uint8_t  BytesWritten = 0;

//	printf_P(PSTR("W %li %i\r\n"), BlockAddress, TotalBlocks);

	/* Wait until endpoint is ready before continuing */
	if (Endpoint_WaitUntilReady())
	  return;
	
	while (TotalBlocks)
	{
	//	sd_raw_write_interval(BlockAddress *  VIRTUAL_MEMORY_BLOCK_SIZE, Buffer, VIRTUAL_MEMORY_BLOCK_SIZE, &SDCardManager_WriteBlockHandler, NULL);
	disk_write (0, Buffer, BlockAddress, 1);//write to disk 0, from Buffer array, into BlockAddress, Write only 1 sector (block);
	SDCardManager_WriteBlockHandler(Buffer, BytesWritten);
	
		/* Decrement the blocks remaining counter and reset the sub block counter */
		BlockAddress++;
		TotalBlocks--;
		
		if (MSInterfaceInfo->State.IsMassStoreReset)
			return;
			
			
		
	}

	/* If the endpoint is empty, clear it ready for the next packet from the host */
	if (!(Endpoint_IsReadWriteAllowed()))
	  Endpoint_ClearOUT();
}

/** Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board dataflash IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  \param[in] BlockAddress  Data block starting address for the read sequence
 *  \param[in] TotalBlocks   Number of blocks of data to read
 */

uint8_t SDCardManager_ReadBlockHandler(uint8_t* buffer, uint8_t offset)
{
	

	/* Check if the endpoint is currently full */
	if (!(Endpoint_IsReadWriteAllowed()))
	{
		/* Clear the endpoint bank to send its contents to the host */
		Endpoint_ClearIN();
		
		/* Wait until the endpoint is ready for more data */
		if (Endpoint_WaitUntilReady())
		  return 0;
	}
		
	Endpoint_Write_8(buffer[0+offset]);
	Endpoint_Write_8(buffer[1+offset]);
	Endpoint_Write_8(buffer[2+offset]);
	Endpoint_Write_8(buffer[3+offset]);
	Endpoint_Write_8(buffer[4+offset]);
	Endpoint_Write_8(buffer[5+offset]);
	Endpoint_Write_8(buffer[6+offset]);
	Endpoint_Write_8(buffer[7+offset]);
	Endpoint_Write_8(buffer[8+offset]);
	Endpoint_Write_8(buffer[9+offset]);
	Endpoint_Write_8(buffer[10+offset]);
	Endpoint_Write_8(buffer[11+offset]);
	Endpoint_Write_8(buffer[12+offset]);
	Endpoint_Write_8(buffer[13+offset]);
	Endpoint_Write_8(buffer[14+offset]);
	Endpoint_Write_8(buffer[15+offset]);
	
	/* Check if the current command is being aborted by the host */
//	if (IsMassStoreReset)
//	  return 0;
	
	return 1;
}

void SDCardManager_ReadBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo, uint32_t BlockAddress, uint16_t TotalBlocks)
{
	uint16_t CurrPage          = BlockAddress;
	uint16_t CurrPageByte      = 0;
	
	uint8_t BytesRead			= 0;

//	printf_P(PSTR("R %li %i\r\n"), BlockAddress, TotalBlocks);
	
	/* Wait until endpoint is ready before continuing */
	if (Endpoint_WaitUntilReady())
	  return;
	
	while (TotalBlocks)
	{
		/* Read a data block from the SD card */
		//sd_raw_read_interval(BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE, Buffer, 16, 512, &SDCardManager_ReadBlockHandler, NULL);
		
		disk_read (0, Buffer, BlockAddress, 1);//  read disk 0,  into buffer,  starting at block address,  read only 1 sector (block=sector)
		while(BytesRead<512){  //send the results to the usb endpoint buffer, 16 bytes at a time.
			BytesRead = SDCardManager_ReadBlockHandler(Buffer, BytesRead); // Bytes Read increases 16 every time handler is called, if all goes well.
		}
		
		/* Decrement the blocks remaining counter */
		BlockAddress++;
		TotalBlocks--;
		
		if (MSInterfaceInfo->State.IsMassStoreReset)
				return;
		
	}
	
	/* If the endpoint is full, send its contents to the host */
	if (!(Endpoint_IsReadWriteAllowed()))
	  Endpoint_ClearIN();
}

/** Performs a simple test on the attached Dataflash IC(s) to ensure that they are working.
 *
 *  \return Boolean true if all media chips are working, false otherwise
 */
bool SDCardManager_CheckDataflashOperation(void)
{	
	return true;
}

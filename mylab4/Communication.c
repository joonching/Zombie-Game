/***************************************************************************//**
 *   @file   Communication.c
 *   @brief  Implementation of Communication Driver for PIC32MX320F128H
             Processor.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "Communication.h"    /*!< Communication definitions */
#include <peripheral/spi.h>
#include <peripheral/ports.h>
#include <plib.h>
#include "PmodOLED.h"


/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst  - Transfer format (0 or 1).
 *                    Example: 0x0 - MSB first.
 *                             0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol  - SPI clock polarity (0 or 1).
 *                    Example: 0x0 - Idle state for clock is a low level; active
 *                                   state is a high level;
 *                             0x1 - Idle state for clock is a high level; active
 *                                   state is a low level.
 * @param clockEdg  - SPI clock edge (0 or 1).
 *                    Example: 0x0 - Serial output data changes on transition
 *                                   from idle clock state to active clock state;
 *                             0x1 - Serial output data changes on transition
 *                                   from active clock state to idle clock state.
 *
 * @return status   - Result of the initialization procedure.
 *                    Example:  0 - if initialization was successful;
 *                             -1 - if initialization was unsuccessful.
*******************************************************************************/
int SPI_CHNL;

//pin 1 on pmod acl is the slave select
//on the board pin1 for JF and JE is the slave select pin
//JE:register D for bit 14
//JF:register F for bit 12
#define SS_BIT_D = BIT_14
#define SS_BIT_F = BIT_12
//what does 1A and 3A mean on the SS1A
int SpiMasterInit(int channel)
{
    SPI_CHNL = 0;
    if(channel == 3) //channel 3 is for JE
    {
        mPORTDSetPinsDigitalOut(BIT_14);
        mPORTDSetBits(BIT_14); //so that the spi doesnt start read or write
        
         SpiChnOpen(3,
            SPI_OPEN_MSTEN
            | SPI_OPEN_CKP_HIGH
            | SPI_OPEN_MODE8
            | SPI_OPEN_ENHBUF
            | SPI_OPEN_ON,
            4); //the clock period

         SPI_CHNL = 3;

         return 0;
    }

     if(channel == 4) //channel 3 is for JF
    {
        mPORTFSetPinsDigitalOut(BIT_12);
        mPORTFSetBits(BIT_12);

         SpiChnOpen(4,
            SPI_OPEN_MSTEN
            | SPI_OPEN_CKP_HIGH
            | SPI_OPEN_MODE8
            | SPI_OPEN_ENHBUF
            | SPI_OPEN_ON,
            4);

         SPI_CHNL = 4;

         return 0;
    }

    return -1;
}

//bytes is just an array
//numWriteBytes is the index position for writing for the xyz calibration...set the offset for the acl
//numReadBytes is for is to read the data from from the ACL of xyz
//purpose is to read and write

//what is the purpose of the spimasterio? to just create an array of accessible data(?)
//what do we we write? offsets(?)
//do we read and write all the time or do we choose by MSB?
//Do we write the address values?
//how do we modify adxl345?

int SpiMasterIO(unsigned char bytes[], int numWriteBytes, int numReadBytes)
{
    if(SPI_CHNL == 3)
    {
        mPORTDClearBits(BIT_14);
        int i;
        for(i = 0; i < numWriteBytes; i++)
        {
            SpiChnPutC(3, bytes[i]);
            (void)SpiChnGetC(3);
        }
        for(i = 0;i<numReadBytes;i++)
        {
            SpiChnPutC(3, 0);
            bytes[numWriteBytes+i] = SpiChnGetC(3);
        }

        mPORTDSetBits(BIT_14);
        return 0;
    }

    if(SPI_CHNL == 4)
    {
         mPORTFClearBits(BIT_12);
        int i;
        for(i = 0; i < numWriteBytes; i++)
        {
            SpiChnPutC(4, bytes[i]);
            (void)SpiChnGetC(4);
        }
        for(i = 0;i<numReadBytes;i++)
        {
            SpiChnPutC(4, 0);
            bytes[numWriteBytes+i] = SpiChnGetC(4);
        }

        mPORTFSetBits(BIT_12);
        return 0;
    }


    return -1;
}
/**
 * @file spi_master.cpp
 *
 * @author FTDI
 * @date 2014-07-01
 *
 * Copyright © 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Rivision History:
 * 1.0 - initial version
 */

//------------------------------------------------------------------------------
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"


std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

//------------------------------------------------------------------------------
inline std::string DeviceFlagToString(DWORD flags)
{
    std::string msg;
    msg += (flags & 0x1)? "DEVICE_OPEN" : "DEVICE_CLOSED";
    msg += ", ";
    msg += (flags & 0x2)? "High-speed USB" : "Full-speed USB";
    return msg;
}

void ListFtUsbDevices()
{
    FT_STATUS ftStatus = 0;

    DWORD numOfDevices = 0;
    ftStatus = FT_CreateDeviceInfoList(&numOfDevices);

    for(DWORD iDev=0; iDev<numOfDevices; ++iDev)
    {
        FT_DEVICE_LIST_INFO_NODE devInfo;
        memset(&devInfo, 0, sizeof(devInfo));

        ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
                                        devInfo.SerialNumber,
                                        devInfo.Description,
                                        &devInfo.ftHandle);

        if (FT_OK == ftStatus)
        {
            printf("Dev %d:\n", iDev);
            printf("  Flags= 0x%x, (%s)\n", devInfo.Flags, DeviceFlagToString(devInfo.Flags).c_str());
            printf("  Type= 0x%x\n",        devInfo.Type);
            printf("  ID= 0x%x\n",          devInfo.ID);
            printf("  LocId= 0x%x\n",       devInfo.LocId);
            printf("  SerialNumber= %s\n",  devInfo.SerialNumber);
            printf("  Description= %s\n",   devInfo.Description);
            printf("  ftHandle= 0x%x\n",    devInfo.ftHandle);

            const std::string desc = devInfo.Description;
            if(desc == "FT4222" || desc == "FT4222 A")
            {
                g_FT4222DevList.push_back(devInfo);
            }
        }
    }
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(int argc, char const *argv[])
{
    ListFtUsbDevices();

    if(g_FT4222DevList.empty()) {
        printf("No FT4222 device is found!\n");
        return 0;
    }

    FT_HANDLE ftHandle = NULL;

    FT_STATUS ftStatus;
    ftStatus = FT_OpenEx((PVOID)g_FT4222DevList[0].LocId, FT_OPEN_BY_LOCATION, &ftHandle);
    if (FT_OK != ftStatus)
    {
        printf("Open a FT4222 device failed!\n");
        return 0;
    }


    ftStatus = FT4222_SPIMaster_Init(ftHandle, SPI_IO_SINGLE, CLK_DIV_4, CLK_IDLE_LOW, CLK_LEADING, 0x01);
    if (FT_OK != ftStatus)
    {
        printf("Init FT4222 as SPI master device failed!\n");
        return 0;
    }

    // TODO:
    //    Start to work as SPI master, and read/write data to a SPI slave
    //    FT4222_SPIMaster_SingleWrite
    //    FT4222_SPIMaster_SingleRead
    //    FT4222_SPIMaster_SingleReadWrite

    unsigned int wordRx;
    uint8_t byteRx;
    unsigned int wordTx;
    uint8_t byteTx;
    uint16_t transferred = 0;

    // Check for loop-back test of all values
    if ((argc == 2 && strcmp(argv[1], "-t") == 0) ||
        (argc == 2 && strcmp(argv[1], "-test") == 0))
    {
        // This code outputs all possible byte values.  
        // If you have MISO and MOSI connected, it will also read in the values that were written.

        for (uint16_t txValue = 0U; txValue <= UINT8_MAX; txValue++)
        {
            byteTx = (uint8_t)txValue;
            FT4222_STATUS status = FT4222_SPIMaster_SingleReadWrite(ftHandle, &byteRx, &byteTx, 1, &transferred, true);
            printf("Status = %d, byteRx = 0x%02X, byteTx = 0x%02X, transferred = %d\n\r", status, byteRx, byteTx, transferred);
        }
    }
    else if ((argc == 2 && 
             (strlen(argv[1]) == 6) && 
             (sscanf(argv[1], "0x%04X", &wordTx) == 1)))
    {
        uint8_t tx[2] = { (uint8_t)(wordTx >> 8), (uint8_t)(wordTx & 0xFF) };
        uint8_t rx[2];
        FT4222_STATUS status = FT4222_SPIMaster_SingleReadWrite(ftHandle, rx, tx, 2, &transferred, true);
        wordRx = (rx[0] << 8) | rx[1];
        printf("Status = %d, wordRx = 0x%04X, wordTx = 0x%04X, transferred = %d\n\r", status, wordRx, wordTx, transferred);
    }
    else if ((argc == 2 && 
             (strlen(argv[1]) == 4) && 
             (sscanf(argv[1], "0x%02X", &wordTx) == 1)))
    {
        byteTx = (uint8_t)wordTx;
        FT4222_STATUS status = FT4222_SPIMaster_SingleReadWrite(ftHandle, &byteRx, &byteTx, 1, &transferred, true);
        printf("Status = %d, byteRx = 0x%02X, byteTx = 0x%02X, transferred = %d\n\r", status, byteRx, byteTx, transferred);
    }
    else
    {
        printf("Usage:\n\r");
        printf("    spi_master [-t|-test]    Send/receive all possible 8-bit values\n\r");
        printf("    spi_master 0xNNNN        Send 16-bit value, MSB then LSB (must be of form 0xNNNN)\n\r");
        printf("    spi_master 0xNN          Send single 8-bit value (must be of form 0xNN)\n\r");
    }

    FT4222_UnInitialize(ftHandle);
    FT_Close(ftHandle);

    getc(stdin);

    return 0;
}


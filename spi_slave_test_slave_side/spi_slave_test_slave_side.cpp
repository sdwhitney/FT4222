/*
 * @file spi_slave_test_slave_side.cpp
 *
 * @author FTDI
 * @date 2018-03-27
 *
 * Copyright 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Revision History:
 * 1.0 - initial version
 * 1.1 - spi slave with protocol and ack function
  */


//------------------------------------------------------------------------------
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <conio.h>
#include <signal.h>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"

#define USER_WRITE_REQ      0x4a
#define USER_READ_REQ       0x4b

std::vector< FT_DEVICE_LIST_INFO_NODE > g_FTAllDevList;
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
            g_FTAllDevList.push_back(devInfo);

            if(desc == "FT4222" || desc == "FT4222 A")
            {
                g_FT4222DevList.push_back(devInfo);
            }
        }
    }
}


namespace
{

class TestPatternGenerator
{
public:
    TestPatternGenerator(uint16 size)
    {
        data.resize(size);

        for (uint16 i = 0; i < data.size(); i++)
        {
            data[i] = (uint8)i;
        }
    }

public:
    std::vector< unsigned char > data;
};
}

static bool keepRunning = true;

void intHandler(int dummy=0) {
    keepRunning = false;
}


//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(int argc, char const *argv[])
{
    signal(SIGINT, intHandler);


    ListFtUsbDevices();
     if(g_FT4222DevList.empty()) {
        printf("No FT4222 device is found!\n");
        return 0;
    }
    int num=0;
    for(int idx=0;idx<10;idx++)
    {
     printf("select dev num(0~%d) as spi slave\n",g_FTAllDevList.size()-1);
     num = getch();
     num = num - '0';
     if(num >=0 && num<g_FTAllDevList.size())
     {
         break;
     }
     else
     {
         printf("input error , please input again\n");
     }

    }

    FT_HANDLE ftHandle = NULL;
    FT_STATUS ftStatus;
    FT_STATUS ft4222_status;
    ftStatus = FT_Open(num, &ftHandle);
    if (FT_OK != ftStatus)
    {
     printf("Open a FT4222 device failed!\n");
     return 0;
    }

    ftStatus = FT4222_SetClock(ftHandle, SYS_CLK_80);
    if (FT_OK != ftStatus)
    {
        printf("FT4222_SetClock failed!\n");
        return 0;
    }

    //Set default Read and Write timeout 1 sec
    ftStatus = FT_SetTimeouts(ftHandle, 1000, 1000 );
    if (FT_OK != ftStatus)
    {
        printf("FT_SetTimeouts failed!\n");
        return 0;
    }

    // set latency to 1
    ftStatus = FT_SetLatencyTimer(ftHandle, 1);
    if (FT_OK != ftStatus)
    {
        printf("FT_SetLatencyTimerfailed!\n");
        return 0;
    }

    //
    ftStatus = FT_SetUSBParameters(ftHandle, 4*1024, 0);
    if (FT_OK != ftStatus)
    {
        printf("FT_SetUSBParameters failed!\n");
        return 0;
    }


    ft4222_status = FT4222_SPISlave_Init(ftHandle);
    if (FT4222_OK != ft4222_status)
    {
        printf("Init FT4222 as SPI master device failed!\n");
        return 0;
    }

    ft4222_status = FT4222_SPI_SetDrivingStrength(ftHandle,DS_4MA, DS_4MA, DS_4MA);
    if (FT4222_OK != ft4222_status)
    {
        printf("FT4222_SPI_SetDrivingStrength failed!\n");
        return 0;
    }

    HANDLE hRxEvent;

    hRxEvent = CreateEvent(
                     NULL,
                     false, // auto-reset event
                     false, // non-signalled state
                     NULL );


    ft4222_status = FT4222_SetEventNotification(ftHandle, FT4222_EVENT_RXCHAR, hRxEvent);
    if (FT_OK != ft4222_status)
    {
        printf("set event notification failed! ft4222_status = %d\n",ft4222_status);
        return 0;
    }
    uint16 rxSize;
    uint16 sizeTransferred;

    printf("start waiting master request..............\n");
    while(keepRunning)
    {
        WaitForSingleObject(hRxEvent, 1000);
        ft4222_status = FT4222_SPISlave_GetRxStatus(ftHandle, &rxSize);
        if(ft4222_status == FT4222_OK)
        {
            if(rxSize>0)
            {
                std::vector<unsigned char> tmpBuf;
                tmpBuf.resize(rxSize);
                ft4222_status = FT4222_SPISlave_Read(ftHandle, &tmpBuf[0], rxSize, &sizeTransferred);
                if((ft4222_status == FT4222_OK) && (rxSize == sizeTransferred))
                {
                    // write request
                    if(tmpBuf[0] == USER_WRITE_REQ)
                    {
                    }
                    // read request USER_READ_REQ
                    else
                    {
                        uint16 sizeToRead;

                        sizeToRead = tmpBuf[1] * 256 + tmpBuf[2];
                        TestPatternGenerator testPattern(sizeToRead);
                        // send back data to master
                        FT4222_SPISlave_Write(ftHandle, &testPattern.data[0], sizeToRead, &sizeTransferred);
                    }
                }
                else
                {
                    printf("FT4222_SPISlave_Read error ft4222_status=%d\n",ft4222_status);
                }
            }
        }
    }
    CloseHandle(hRxEvent);


    FT4222_UnInitialize(ftHandle);
    FT_Close(ftHandle);
    return 0;
}

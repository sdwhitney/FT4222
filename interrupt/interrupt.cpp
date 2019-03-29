/**
 * @file interrupt.cpp
 *
 * @author FTDI
 * @date 2015-12-03
 *
 * Copyright © 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Revision History:
 * 1.0 - initial version
 */

//------------------------------------------------------------------------------
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include  <signal.h>

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

        ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type,
                                        &devInfo.ID, &devInfo.LocId,
                                        devInfo.SerialNumber,
                                        devInfo.Description,
                                        &devInfo.ftHandle);

        if (FT_OK == ftStatus)
        {
            const std::string desc = devInfo.Description;
            // Edit this string to match your own device.
            // Note: GPIO is interface 'B' (in mode 0) and 'D' (in mode 1).
            if(desc == "FT4222 B")
            {
                g_FT4222DevList.push_back(devInfo);
            }
        }
    }
}


std::string GPIO_Trigger_Enum_to_String(GPIO_Trigger trigger)
{
    switch(trigger)
    {
        case GPIO_TRIGGER_RISING:
            return "GPIO_TRIGGER_RISING";
        case GPIO_TRIGGER_FALLING:
            return "GPIO_TRIGGER_FALLING";
        case GPIO_TRIGGER_LEVEL_HIGH:
            return "GPIO_TRIGGER_LEVEL_HIGH";
        case GPIO_TRIGGER_LEVEL_LOW:
            return "GPIO_TRIGGER_LEVEL_LOW";
        default:

            return "";
    }
    return "";
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

    const FT_DEVICE_LIST_INFO_NODE& devInfo = g_FT4222DevList[0];

    printf("Open Device\n");
    printf("  Flags= 0x%x, (%s)\n", devInfo.Flags, DeviceFlagToString(devInfo.Flags).c_str());
    printf("  Type= 0x%x\n",        devInfo.Type);
    printf("  ID= 0x%x\n",          devInfo.ID);
    printf("  LocId= 0x%x\n",       devInfo.LocId);
    printf("  SerialNumber= %s\n",  devInfo.SerialNumber);
    printf("  Description= %s\n",   devInfo.Description);
    printf("  ftHandle= 0x%x\n",    devInfo.ftHandle);


    FT_HANDLE ftHandle = NULL;

    FT_STATUS ftStatus;
    ftStatus = FT_OpenEx((PVOID)(uintptr_t)g_FT4222DevList[0].LocId, FT_OPEN_BY_LOCATION, &ftHandle);
    if (FT_OK != ftStatus)
    {
        printf("Open a FT4222 device failed!\n");
        return 0;
    }

    // FT4222 only have one interrupt GPIO3
    // master initialize
    GPIO_Dir gpioDir[4];

    gpioDir[0] = GPIO_OUTPUT;
    gpioDir[1] = GPIO_OUTPUT;
    gpioDir[2] = GPIO_OUTPUT;
    gpioDir[3] = GPIO_INPUT;

    // we must initial gpio before FT4222_SetInterruptTrigger, because interrupt data is transmitted by gpio interface.
    FT4222_GPIO_Init(ftHandle, gpioDir);    

    // enable interrupt
    FT4222_SetWakeUpInterrupt(ftHandle, true);
    // setup interrrupt trigger level
    FT4222_SetInterruptTrigger(ftHandle, GPIO_TRIGGER_RISING);

    // There are two ways to get interrupt status
    //  1. FT4222_GPIO_ReadTriggerQueue
    //  2. FT4222_GPIO_Read
    // These two ways are all read-clear function when we get interrupt data


    //method 1:  get interrupt by FT4222_GPIO_ReadTriggerQueue
    printf("Test interrupt by FT4222_GPIO_GetTriggerStatus!\n");
    printf("press ctrl+c if you want to exit!\n");


    while (keepRunning)
    {        
        uint16 queueSize;
        if(FT4222_GPIO_GetTriggerStatus(ftHandle, GPIO_PORT3, &queueSize) == FT4222_OK)
        {
            if(queueSize>0)
            {
                uint16 sizeofRead;        
                std::vector<GPIO_Trigger> tmpBuf;
                
                tmpBuf.resize(queueSize);
                if(FT4222_GPIO_ReadTriggerQueue(ftHandle, GPIO_PORT3, &tmpBuf[0], queueSize, &sizeofRead) == FT4222_OK)
                {
                    for(int idx=0; idx<sizeofRead; idx++)
                    {
                        printf("got interrupt =%s\n", GPIO_Trigger_Enum_to_String(tmpBuf[idx]).c_str());
                    }
                }
            }
        }   
    }
    printf("exit FT4222_GPIO_ReadTriggerQueue test!\n");

    // method 2:  get interrupt by FT4222_GPIO_Read
    printf("=========================================\n");
    printf("Test interrupt by FT4222_GPIO_Read!\n");
    printf("press ctrl+c if you want to exit!\n");
    keepRunning = true;
    while (keepRunning)
    {
        BOOL value;
        
        if(FT4222_GPIO_Read(ftHandle, (GPIO_Port)GPIO_PORT3, &value) == FT4222_OK)
        {
            if(value == 1)
            {
                printf("got interrupt\n");
            }
        }
        
        Sleep(1);

    }

    printf("exit FT4222_GPIO_Read test!\n");

    printf("UnInitialize FT4222\n");
    FT4222_UnInitialize(ftHandle);

    printf("Close FT device\n");
    FT_Close(ftHandle);
    return 0;
}

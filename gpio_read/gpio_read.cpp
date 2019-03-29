/**
 * @file gpio_read.cpp
 *
 * @author FTDI
 * @date 2014-07-01
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

    HANDLE hRxEvent;

    hRxEvent = CreateEvent(
                     NULL,
                     false, // auto-reset event
                     false, // non-signalled state
                     NULL );


    ftStatus = FT_SetEventNotification(ftHandle, FT_EVENT_RXCHAR, hRxEvent);
    if (FT_OK != ftStatus)
    {
        printf("FT_SetEventNotification failed!\n");
        return 0;
    }

    

    // disable Suspenout
    FT4222_SetSuspendOut(ftHandle, false);
    // disable interrupt
    FT4222_SetWakeUpInterrupt(ftHandle, false);


    GPIO_Dir gpioDir[4];

    gpioDir[0] = GPIO_INPUT;
    gpioDir[1] = GPIO_INPUT;
    gpioDir[2] = GPIO_INPUT;
    gpioDir[3] = GPIO_INPUT;

    // master initialize

    FT4222_GPIO_Init(ftHandle, gpioDir);

    FT4222_GPIO_SetInputTrigger(ftHandle, GPIO_PORT0, (GPIO_Trigger)(GPIO_TRIGGER_LEVEL_HIGH | GPIO_TRIGGER_LEVEL_LOW | GPIO_TRIGGER_RISING | GPIO_TRIGGER_FALLING));
    FT4222_GPIO_SetInputTrigger(ftHandle, GPIO_PORT1, (GPIO_Trigger)(GPIO_TRIGGER_LEVEL_HIGH | GPIO_TRIGGER_LEVEL_LOW | GPIO_TRIGGER_RISING | GPIO_TRIGGER_FALLING));
    FT4222_GPIO_SetInputTrigger(ftHandle, GPIO_PORT2, (GPIO_Trigger)(GPIO_TRIGGER_LEVEL_HIGH | GPIO_TRIGGER_LEVEL_LOW | GPIO_TRIGGER_RISING | GPIO_TRIGGER_FALLING));
    FT4222_GPIO_SetInputTrigger(ftHandle, GPIO_PORT3, (GPIO_Trigger)(GPIO_TRIGGER_LEVEL_HIGH | GPIO_TRIGGER_LEVEL_LOW | GPIO_TRIGGER_RISING | GPIO_TRIGGER_FALLING));

    
    std::vector<GPIO_Trigger> tmpBuf;
    while(keepRunning)
    {
        WaitForSingleObject(hRxEvent, 1000);

        for(int portIdx=0; portIdx<4; portIdx++)
        {
            uint16 queueSize;
            if(FT4222_GPIO_GetTriggerStatus(ftHandle, (GPIO_Port)portIdx, &queueSize) == FT4222_OK)
            {
                if(queueSize>0)
                {
                    uint16 sizeofRead;

                    tmpBuf.resize(queueSize);
                    if(FT4222_GPIO_ReadTriggerQueue(ftHandle, (GPIO_Port)portIdx, &tmpBuf[0], queueSize, &sizeofRead) == FT4222_OK)
                    {
                        for(int idx=0; idx<sizeofRead; idx++)
                        {
                            printf("port[%d] trigger =%s\n",portIdx, GPIO_Trigger_Enum_to_String(tmpBuf[idx]).c_str());
                        }
                    }
                    BOOL value;
                    FT4222_GPIO_Read(ftHandle, (GPIO_Port)portIdx, &value);
                    printf("port =%d value=%d\n",portIdx, value);
                }
            }
        }
    }

    CloseHandle(hRxEvent);



    printf("UnInitialize FT4222\n");
    FT4222_UnInitialize(ftHandle);

    printf("Close FT device\n");
    FT_Close(ftHandle);
    return 0;
}

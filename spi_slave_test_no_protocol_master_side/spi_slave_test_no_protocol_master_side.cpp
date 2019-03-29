/**
 * @file spi_slave_test_no_protocol_master_side.cpp
 *
 * @author FTDI
 * @date 2016-09-9
 *
 * Copyright © 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Rivision History:
 * 1.0 - initial version
 */

//------------------------------------------------------------------------------
// This sample must be used with another application spi_slave_test_no_protocol_slave_side.cpp
// It use two FT4222H , one act as spi master , the ohter act as spi slave

// design ourself protocol to spi slave transmision
// this provides a simple protocol here, you can do yours protocol.

// FOR spi slave receiving protocl
// byte0 : sync word  (0x5A)
// byte1 : write/read  (write:0 , read : 1)
// byte2 : size
// byte3~? : data          // read command does not have this part.


// FOR spi slave sending protocl
// byte0 : sync word  (0x5A)
// byte1 : size
// byte2~? : data



#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <conio.h>
#include <time.h>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"


std::vector< FT_DEVICE_LIST_INFO_NODE > g_FTAllDevList;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

#define SYNC_WORD       0x5A
#define CMD_WRITE       0x00
#define CMD_READ        0x01

enum UserCmd_t
{
    USER_CMD_WRITE = 0,
    USER_CMD_READ,
};

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


void WriteRndData(FT_HANDLE ftHandle)
{
    uint16 sizeTransferred = 0;
    FT_STATUS ft4222_status;
    std::vector<unsigned char> sendBuf;
    uint8 size = (rand()%255 +1); // a random size , 1~255 bytes to send
    sendBuf.resize(size+3);
    sendBuf[0] = SYNC_WORD;
    sendBuf[1] = CMD_WRITE;
    sendBuf[2] = size;

    TestPatternGenerator testPattern(size);

    // all data are sequential , we can check data at spi slave
    memcpy(&sendBuf[3], &testPattern.data[0], size);

    ft4222_status = FT4222_SPIMaster_SingleWrite(ftHandle,&sendBuf[0], sendBuf.size(), &sizeTransferred, true);

    printf("SPI Master is sending %d size data to slave\n",size);

    printf("=============================================\n",size);

}

void ReadRndData(FT_HANDLE ftHandle)
{
    // read data from slave
    //1. send a read request 
    FT_STATUS ft4222_status;
    uint16 sizeTransferred = 0;
    std::vector<unsigned char> sendBuf;
    std::vector<unsigned char> recvBuf;    
    uint8 size = (rand()%255 +1); // a random size , 1~255 bytes to send    
    uint8 recv_size =0;
    TestPatternGenerator testPattern(size);


    sendBuf.resize(3);
    sendBuf[0] = SYNC_WORD;
    sendBuf[1] = CMD_READ;
    sendBuf[2] = size;
           
    ft4222_status = FT4222_SPIMaster_SingleWrite(ftHandle,&sendBuf[0], sendBuf.size(), &sizeTransferred, true);

    printf("SPI Master send read %d size request to slave\n",size );
    

    // 2. read data from slave
     while(1)
     {
         recvBuf.resize(1);
         // got sync word
         ft4222_status = FT4222_SPIMaster_SingleRead(ftHandle,&recvBuf[0], 1, &sizeTransferred, true);
         if(recvBuf[0] == SYNC_WORD)
         {
             // got size
             ft4222_status = FT4222_SPIMaster_SingleRead(ftHandle,&recvBuf[0], 1, &sizeTransferred, true);
             recv_size = recvBuf[0];
             recvBuf.resize(recv_size);
             // got data
             ft4222_status = FT4222_SPIMaster_SingleRead(ftHandle,&recvBuf[0], recv_size, &sizeTransferred, true);

             if(memcmp(&recvBuf[0], &testPattern.data[0], size) == 0)
             {
                 printf("[OK]    [SPI master <== SPI slave]Read data from Slave got %d size and all data are equivalent\n", recv_size);
             }
             else
             {
                 printf("[ERROR] [SPI master <== SPI slave]Data are not equivalent\n");
             }

             break;
         }   
     }

    printf("=============================================\n",size);

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
    int num=0;
    for(int idx=0;idx<10;idx++)
    {
     printf("select dev num(0~%d) as spi master\n",g_FTAllDevList.size()-1);
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

    printf("\n\n");
    printf("Init FT4222 as SPI master\n");


    ftStatus = FT4222_SetClock(ftHandle, SYS_CLK_80);
    if (FT_OK != ftStatus)
    {
        printf("Set FT4222 clock to 80MHz failed!\n");
        return 0;
    }

    
    ft4222_status = FT4222_SPIMaster_Init(ftHandle, SPI_IO_SINGLE, CLK_DIV_4, CLK_IDLE_LOW, CLK_LEADING, 0x01);
    if (FT4222_OK != ft4222_status)
    {
        printf("Init FT4222 as SPI master device failed!\n");
        return 0;
    }
    
    ftStatus = FT4222_SPI_SetDrivingStrength(ftHandle,DS_4MA, DS_4MA, DS_4MA);
    if (FT_OK != ftStatus)
    {
        printf("Set SPI Slave driving strength failed!\n");
        return 0;
    }

    ftStatus = FT_SetUSBParameters(ftHandle, 4*1024, 0);
    if (FT_OK != ftStatus)
    {
        printf("FT_SetUSBParameters failed!\n");
        return 0;
    }
    
    srand(time(NULL));
    
    UserCmd_t userCmd;

    for(int idx=0;idx<10;idx++)
    {
        UserCmd_t userCmd;
        // 0 for write , 1 for read        
        userCmd = (UserCmd_t)(rand()%2);
 
        if(userCmd == USER_CMD_WRITE)
            WriteRndData(ftHandle);
        else
            ReadRndData(ftHandle);
    }

    printf("UnInitialize FT4222\n");
    FT4222_UnInitialize(ftHandle);

    printf("Close FT device\n");
    FT_Close(ftHandle);
    return 0;
}

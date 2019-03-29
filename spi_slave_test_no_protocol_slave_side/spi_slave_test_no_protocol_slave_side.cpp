/**
 * @file spi_slave_test_no_protocol_slave_side.cpp
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
// This sample must be used with another application spi_slave_test_no_protocol_master_side.cpp
// It use two FT4222H , one act as spi master , the ohter act as spi slave

// design ourself protocol to spi slave transmision
// this provides a simple protocol here, you can do yours protocol.

// FOR spi slave receiving protocl
// byte1 : sync word  (0x5A)
// byte2 : write/read  (write:0 , read : 1)
// byte3 : size
// byte 4~? : data          // read command does not have this part.


// FOR spi slave sending protocl
// byte1 : sync word  (0x5A)
// byte2 : size
// byte3~? : data



#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <conio.h>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"

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

std::vector< FT_DEVICE_LIST_INFO_NODE > g_FTAllDevList;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

#define SYNC_WORD       0x5A
#define CMD_WRITE       0x00
#define CMD_READ        0x01


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

void parse_packet(FT_HANDLE ftHandle, std::vector<unsigned char> & dataBuf)
{
    // skip all non "sync word " at the first byte.
    while(dataBuf.size()>0)
    {
        if(dataBuf[0] == SYNC_WORD)
        {
            break;
        }
        dataBuf.erase(dataBuf.begin()); 
    }

    // data is not enough to parse, wait for another input
    if(dataBuf.size() < 3)
    {
        return;    
    }

    switch(dataBuf[1])
    {
        case CMD_READ:
        {
            uint16 sizeTransferred;
            uint8 size = dataBuf[2];
            std::vector<unsigned char> sendData;
            sendData.resize(size+2); // including sync word and size
            sendData[0] = SYNC_WORD;
            sendData[1] = size;
            
            TestPatternGenerator testPattern(size);
            
            // all data are sequential , we can check data at spi slave
            memcpy(&sendData[2], &testPattern.data[0], size);

            printf("get a read request , read size = %d\n", size);
            
            FT4222_SPISlave_Write(ftHandle, &sendData[0], sendData.size(), &sizeTransferred);

            // drop the used data
            dataBuf.erase(dataBuf.begin(),dataBuf.begin()+3);
            printf("=============================================\n",size);
        }
            break;
        case CMD_WRITE:
        {
            // check data length
            uint16 size = dataBuf[2];
            // wait all data write from spi master
            if(dataBuf.size() < (size+3))
            {
              //  printf("we do not get enough data\n");
                return;
            }
            TestPatternGenerator testPattern(size);

            printf("get a write request , write size = %d\n", size);

            // check if all data are correct
            if(memcmp(&dataBuf[3], &testPattern.data[0], size) == 0)
            {
                printf("[OK]    [SPI master ==> SPI slave]Read data from Master and all data are equivalent\n");
            }
            else
            {
                printf("[ERROR] [SPI master ==> SPI slave]Data are not equivalent\n");
            }

            dataBuf.erase(dataBuf.begin(),dataBuf.begin()+3+size);
            printf("=============================================\n",size);            
        }
            break;
        default:
            printf("data error\n");
            return;
            

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

    printf("\n\n");
    printf("Init FT4222 as SPI slave\n");


    ftStatus = FT4222_SetClock(ftHandle, SYS_CLK_80);
    if (FT_OK != ftStatus)
    {
        printf("Set FT4222 clock to 80MHz failed!\n");
        return 0;
    }

    
    ftStatus = FT4222_SPISlave_InitEx(ftHandle, SPI_SLAVE_NO_PROTOCOL);
    if (FT_OK != ftStatus)
    {
        printf("Init FT4222 as SPI slave device failed!\n");
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


    //    Start to work as SPI slave, and read/write data
    uint16 sizeTransferred = 0;
    uint16 rxSize;
    std::vector<unsigned char> recvBuf;
    std::vector<unsigned char> dataBuf; // data need to be parsed

    while(1)
    {
        if(FT4222_SPISlave_GetRxStatus(ftHandle, &rxSize) == FT_OK)
        {
            if(rxSize>0)
            {    
                recvBuf.resize(rxSize);
                if(FT4222_SPISlave_Read(ftHandle,&recvBuf[0], rxSize, &sizeTransferred)== FT_OK)
                {
                    if(sizeTransferred!= rxSize)
                    {
                        printf("read data size is not equal\n");
                    }
                    else
                    {
                        // append data to dataBuf
                        dataBuf.insert(dataBuf.end(), recvBuf.begin(), recvBuf.end());
                    }
                }
                else
                {
                    printf("FT4222_SPISlave_Read error\n");
                }
            }
            if(dataBuf.size()>0)
                parse_packet(ftHandle, dataBuf);
        }    
    }
    
    printf("UnInitialize FT4222\n");
    FT4222_UnInitialize(ftHandle);

    printf("Close FT device\n");
    FT_Close(ftHandle);
    return 0;
}

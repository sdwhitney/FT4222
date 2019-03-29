/**
 * @file spi_flash_quad_test.cpp
 *
 * @author FTDI
 * @date 2014-07-01
 *
 * Copyright © 2011 Future Technology Devices International Limited
 * Company Confidential
 *
 * Access MX25L6435E flash example
 * Revision History:
 * 1.0 - initial version
 */

//------------------------------------------------------------------------------
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <algorithm>

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "ftd2xx.h"
#include "LibFT4222.h"


std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

// mxic flash command
#define CmdReadStatusRegister   0x05
#define CmdWriteEnable          0x06
#define CmdBlockErase           0xD8
#define CmdQuadIOPageProgram    0x38
#define CmdFastQuadRead         0xEB

#define SPI_FLASH_MAX_WRITE_SIZE 256

//------------------------------------------------------------------------------

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

inline bool WaitForFlashReady(FT_HANDLE ftHandle)
{
    const int WAIT_FALSH_READY_TIMES = 500;

    for(int i=0; i<WAIT_FALSH_READY_TIMES; ++i)
    {
        std::vector<unsigned char> tmp;
        tmp.push_back(CmdReadStatusRegister);
        tmp.push_back(0xFF);

        std::vector<unsigned char> recvData;
        recvData.resize(2);

        uint16 sizeTransferred;
        FT4222_STATUS ftStatus;

        ftStatus = FT4222_SPIMaster_SingleReadWrite(ftHandle, &recvData[0], &tmp[0], tmp.size(), &sizeTransferred, true);
        if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=tmp.size()))
        {
            return false;
        }

        if ((recvData[1] & 0x01) == 0x00) // not in write operation
        {
            return true;
        }

        Sleep(1);
    }

    return false;
}

inline bool WriteEnableCmd(FT_HANDLE ftHandle)
{
    uint8 outBuffer = CmdWriteEnable;
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &outBuffer, 1, &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=1))
    {
        return false;
    }

    return true;
}


inline bool BlockEraseCmd(FT_HANDLE ftHandle, uint32 _addr)
{
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;
    std::vector<unsigned char> tmp;

    tmp.push_back(CmdBlockErase);
    tmp.push_back((unsigned char)((_addr & 0xFF0000) >> 16));
    tmp.push_back((unsigned char)((_addr & 0x00FF00) >> 8));
    tmp.push_back((unsigned char)( _addr & 0x0000FF));

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    return true;
}


inline bool EarseFlash(FT_HANDLE ftHandle, uint32 startAddr, uint32 endAddr)
{
    if(!WaitForFlashReady(ftHandle))
        return false;
    while (startAddr < endAddr)
    {
        if(!WriteEnableCmd(ftHandle))
            return false;

        if(!BlockEraseCmd(ftHandle, startAddr))
            return false;

        startAddr += 0x10000;

        if(!WaitForFlashReady(ftHandle))
            return false;
        else
            continue;
    }
    return true;
}

inline bool QuadIOPageProgramCmd(FT_HANDLE ftHandle, uint32 _addr, unsigned char * pData , uint16 size)
{
    uint32 sizeOfRead;
    FT4222_STATUS ftStatus;
    std::vector<unsigned char> tmp;

    tmp.push_back(CmdQuadIOPageProgram);
    tmp.push_back((unsigned char)((_addr & 0xFF0000) >> 16));
    tmp.push_back((unsigned char)((_addr & 0x00FF00) >> 8));
    tmp.push_back((unsigned char)( _addr & 0x0000FF));

    tmp.insert(tmp.end(), pData, (pData + size));

    ftStatus = FT4222_SPIMaster_MultiReadWrite(ftHandle, NULL, &tmp[0], 1, 3+size, 0, &sizeOfRead);
    if(ftStatus!=FT4222_OK)
    {
        printf("ftStatus =%d\n",ftStatus);
        return false;
    }

    return true;
}

inline bool FastQuadReadCmd(FT_HANDLE ftHandle, uint32 _addr, unsigned char * pData , uint16 size)
{
    uint32 sizeOfRead;
    FT4222_STATUS ftStatus;
    std::vector<unsigned char> tmp;

    tmp.push_back(CmdFastQuadRead);
    tmp.push_back((unsigned char)((_addr & 0xFF0000) >> 16));
    tmp.push_back((unsigned char)((_addr & 0x00FF00) >> 8));
    tmp.push_back((unsigned char)( _addr & 0x0000FF));
    tmp.push_back(0xFF);
    tmp.push_back(0xFF);
    tmp.push_back(0xFF);

    ftStatus = FT4222_SPIMaster_MultiReadWrite(ftHandle, pData, &tmp[0], 1, 6, size, &sizeOfRead);
    if((ftStatus!=FT4222_OK) ||  (sizeOfRead != size))
    {
        return false;
    }

    return true;
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
    ftStatus = FT_OpenEx((PVOID)g_FT4222DevList[0].SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
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

    ftStatus = FT4222_SPI_SetDrivingStrength(ftHandle,DS_8MA, DS_8MA, DS_8MA);
    if (FT_OK != ftStatus)
    {
        printf("set spi driving strength failed!\n");
        return 0;
    }

    int testSize = 1024;

    // earse flsah
    TestPatternGenerator testPattern(testSize);
    uint32 startAddr = 0x000000;
    std::vector<unsigned char> sendData = testPattern.data;
    std::vector<unsigned char> recvData;
    recvData.resize(testSize);

    ftStatus = FT4222_SPIMaster_SetLines(ftHandle,SPI_IO_SINGLE);
    if (FT_OK != ftStatus)
    {
        printf("set spi single line failed!\n");
        return 0;
    }

    if(!EarseFlash(ftHandle, startAddr, startAddr + testSize))
    {
        printf("earse flash failed!\n");
        return 0;
    }
    else
    {
        printf("earse flash OK!\n");
    }

    uint16 notSentByte = testSize;
    uint16 sentByte=0;

    while (notSentByte > 0)
    {
        uint16 data_size = std::min<size_t>(SPI_FLASH_MAX_WRITE_SIZE, notSentByte);

        ftStatus = FT4222_SPIMaster_SetLines(ftHandle,SPI_IO_SINGLE);
        if (FT_OK != ftStatus)
        {
            printf("set spi single line failed!\n");
            return 0;
        }

        if(!WaitForFlashReady(ftHandle))
        {
            printf("wait flash ready failed!\n");
            return 0;
        }

        if(!WriteEnableCmd(ftHandle))
        {
            printf("write enable failed!\n");
            return 0;
        }

        ftStatus = FT4222_SPIMaster_SetLines(ftHandle,SPI_IO_QUAD);
        if (FT_OK != ftStatus)
        {
            printf("set spi quad line failed!\n");
            return 0;
        }

        if(!QuadIOPageProgramCmd(ftHandle, startAddr+sentByte, &sendData[sentByte], data_size))
        {
            printf("quad write failed!\n");
            return 0;
        }
        else
        {
            printf("writing flash start address =[%x] %d bytes\n", startAddr+sentByte, data_size);
        }

        notSentByte -= data_size;
        sentByte    += data_size;

        Sleep(10);
    }

	ftStatus = FT4222_SPIMaster_SetLines(ftHandle,SPI_IO_SINGLE);
    if (FT_OK != ftStatus)
    {
        printf("set spi single line failed!\n");
        return 0;
    }

    if(!WaitForFlashReady(ftHandle))
    {
        printf("wait flash ready failed!\n");
        return 0;
    }

    ftStatus = FT4222_SPIMaster_SetLines(ftHandle,SPI_IO_QUAD);
    if (FT_OK != ftStatus)
    {
        printf("set spi quad line failed!\n");
        return 0;
    }

    if(!FastQuadReadCmd(ftHandle, startAddr,&recvData[0], recvData.size()))
    {
        printf("quad read failed!\n");
        return 0;
    }
    else
    {
        printf("reading flash start address =[%x] %d bytes\n", startAddr, recvData.size());
    }

    if( 0 != memcmp(&testPattern.data[0], &recvData[0], testSize))
    {
        printf("compare data error!\n");
        return 0;
    }
    else
    {
        printf("data is equal\n");
    }

    FT4222_UnInitialize(ftHandle);
    FT_Close(ftHandle);
    return 0;
}

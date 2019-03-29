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

// toshiba flash command
#define CmdReadID                       0x9F
#define CmdReset                        0xFF
#define CmdGetFeature                   0x0F
#define CmdSetFeature                   0x1F
#define CmdBlockErase                   0xD8
#define CmdWriteEnable                  0x06
#define CmdProgramLoad                  0x02
#define CmdProgramExec                  0x10
#define CmdReadCellArray                0x13
#define CmdReadBuffer1Bit               0x0B
#define CmdReadBuffer2Bit               0x3B
#define CmdReadBuffer4Bit               0x6B
#define CmdStatus_A0                    0xA0
#define CmdStatus_B0                    0xB0
#define CmdStatus_C0                    0xC0



// C0 status bit
#define OIP_IS_IN_PROGRESS              0x01
#define WEL_ENABLE                      0x02
#define ERS_FAIL                        0x04
#define PROGRAM_FAIL                    0x08
#define ECC_BIT_01                      0x10
#define ECC_BIT_10                      0x20
#define ECC_BIT_11                      0x30

#define SPI_FLASH_MAX_WRITE_SIZE 256
#define SPI_PAGE_SIZE           4224

#define READ_FLASH_WITH_QUAD_MODE   1

//------------------------------------------------------------------------------


void press_enter_to_next_test()
{
    printf("press enter to test next test\n");

    // wait enter
    while(1)
    {
        char input;
        
        input=getchar();
        if ('\n' != input)
        {
            printf("press enter to test next test\n");
            continue;
        }
        fflush(stdin);
        break;
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



inline bool ReadID(FT_HANDLE ftHandle)
{
    uint16 sizeTransferred;

    FT4222_STATUS ftStatus;
    std::vector<unsigned char> tmp;
    std::vector<unsigned char> readData;
    
    tmp.push_back(CmdReadID);
    tmp.push_back(0xFF);
   
    // set get feature and get C0 status
    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, false);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=tmp.size()))
    {
        return false;
    }

    readData.resize(2);

    ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, &readData[0], readData.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=readData.size()))
    {
        return false;
    }
    if((readData[0] == 0x98) && (readData[1] == 0xCD))
        return true;
    
    return false;
}



bool getA0Status(FT_HANDLE ftHandle, uint8 &A0_Status )
{
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;

    std::vector<unsigned char> tmp;
    tmp.push_back(CmdGetFeature);
    tmp.push_back(CmdStatus_A0);
    // set get feature and get C0 status
    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, false);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false ;
    }

    // to terminate this transaction
    ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, &A0_Status, 1, &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != 1))
    {
        return false;
    }

    return true;
}


bool getB0Status(FT_HANDLE ftHandle, uint8 &B0_Status )
{
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;

    std::vector<unsigned char> tmp;
    tmp.push_back(CmdGetFeature);
    tmp.push_back(CmdStatus_B0);
    // set get feature and get C0 status
    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, false);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false ;
    }


    // to terminate this transaction
    ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, &B0_Status, 1, &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != 1))
    {
        return false;
    }
    printf("B0 status =%x\n",B0_Status);

    return true;
}





bool getC0Status(FT_HANDLE ftHandle, uint8 &C0_Status )
{
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;

    std::vector<unsigned char> tmp;
    tmp.push_back(CmdGetFeature);
    tmp.push_back(CmdStatus_C0);
    // set get feature and get C0 status
    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, false);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false ;
    }

    while(1)
    {
        uint8 readStatus;

        ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, &readStatus, 1, &sizeTransferred, false);
        if((ftStatus!=FT4222_OK) ||  (sizeTransferred != 1))
        {
            return false ;
        }
        // progress done
        if ((readStatus & OIP_IS_IN_PROGRESS) != OIP_IS_IN_PROGRESS) 
        {
            C0_Status = readStatus;

            // to terminate this transaction
            ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, &readStatus, 1, &sizeTransferred, true);
            if((ftStatus!=FT4222_OK) ||  (sizeTransferred != 1))
            {
                return false;
            }

            break;
        }
        
        Sleep(1);
    }

    return true;

}


inline bool Reset(FT_HANDLE ftHandle)
{
    uint8 c0_status;
    uint8 outBuffer = CmdReset;
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &outBuffer, 1, &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=1))
    {
        return false;
    }

    if(!getC0Status(ftHandle, c0_status))
    {
        printf("get c0 status failed\n");
        return false;
    }


    return true;
}

inline bool UnlockAllBlock(FT_HANDLE ftHandle)
{
    FT4222_STATUS ftStatus;
    uint16 sizeTransferred;
    uint8 A0_status;

    std::vector<unsigned char> tmp;
    tmp.push_back(CmdSetFeature);
    tmp.push_back(CmdStatus_A0);
    // reset all block
    tmp.push_back(0);

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size() , &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred!=tmp.size()))
    {
        return false;
    }

    if(!getA0Status(ftHandle, A0_status))
    {
        printf("get A0 status failed\n");
        return false;
    }


    return true;

}





inline bool EraseDone(FT_HANDLE ftHandle)
{
    uint8 c0_status;
    if(!getC0Status(ftHandle, c0_status))
    {
        printf("get c0 status failed\n");
        return false;
    }

    if ((c0_status & ERS_FAIL) == ERS_FAIL) 
        return false;
    else    
        return true;

    return true;        
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


inline bool BlockEraseCmd(FT_HANDLE ftHandle, uint32 pageIdx)
{
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;
    std::vector<unsigned char> tmp;

    tmp.push_back(CmdBlockErase);
    tmp.push_back((unsigned char)((pageIdx & 0xFF0000) >> 16));
    tmp.push_back((unsigned char)((pageIdx & 0x00FF00) >> 8));
    tmp.push_back((unsigned char)( pageIdx & 0x0000FF));

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    return true;
}


inline bool EarseFlash(FT_HANDLE ftHandle, uint32 startAddr, uint32 endAddr)
{
    while (startAddr < endAddr)
    {
        if(!WriteEnableCmd(ftHandle))
            return false;
        
        if(!BlockEraseCmd(ftHandle, startAddr/SPI_PAGE_SIZE))
            return false;

        startAddr += SPI_PAGE_SIZE;

        if(!EraseDone(ftHandle))
            return false;
        else
            continue;
    }

    return true;
}

inline bool WritePage(FT_HANDLE ftHandle, uint32 startAddr, unsigned char * pData , uint16 size)
{
    uint32 columnIdx = startAddr % SPI_PAGE_SIZE ;
    uint32 pageIdx = startAddr / SPI_PAGE_SIZE;

    if(columnIdx+size > SPI_PAGE_SIZE)
        return false;

    // step 1 : write enable
    if(!WriteEnableCmd(ftHandle))
        return false;

    // step 2 : program load
    uint16 sizeTransferred;
    FT4222_STATUS ftStatus;
    

    std::vector<unsigned char> tmp;
    tmp.resize(3+size);

    tmp[0] = CmdProgramLoad;
    tmp[1] = (unsigned char)((columnIdx & 0x00FF00) >> 8);
    tmp[2] = (unsigned char)( columnIdx & 0x0000FF);

    memcpy(&tmp[3],pData, size);

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    // step 3 : program exec
    tmp.resize(4);

    tmp[0] = CmdProgramExec;
    tmp[1] = (unsigned char)((pageIdx & 0xFF0000) >> 16);
    tmp[2] = (unsigned char)((pageIdx & 0x00FF00) >> 8);
    tmp[3] = (unsigned char)(pageIdx & 0x0000FF);

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    // step 4 :  get c0 status
    uint8 c0_status;
    getC0Status(ftHandle, c0_status);

    if ((c0_status & PROGRAM_FAIL) == PROGRAM_FAIL) 
        return false;
    else    
        return true;

    return true;  
}



inline bool ReadPage(FT_HANDLE ftHandle, uint32 startAddr, unsigned char * pData , uint16 size)
{
    uint32 columnIdx = startAddr % SPI_PAGE_SIZE ;
    uint32 pageIdx = startAddr / SPI_PAGE_SIZE;
    uint16 sizeTransferred;
    uint32 sizeOfRead;
    FT4222_STATUS ftStatus;
   
    std::vector<unsigned char> tmp;

    if(columnIdx+size > SPI_PAGE_SIZE)
        return false;


    // step 1 : read cell
    tmp.resize(4);

    tmp[0] = CmdReadCellArray;
    tmp[1] = (unsigned char)((pageIdx & 0xFF0000) >> 16);
    tmp[2] = (unsigned char)((pageIdx & 0x00FF00) >> 8);
    tmp[3] = (unsigned char)(pageIdx & 0x0000FF);

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    // step 2 :  get c0 status
    uint8 c0_status;
    getC0Status(ftHandle, c0_status);


#if READ_FLASH_WITH_QUAD_MODE // 4 bit mode
    FT4222_SPIMaster_SetLines(ftHandle, SPI_IO_QUAD);

    // step 3 : read buffer with 4 bit method
    tmp.resize(4);

    tmp[0] = CmdReadBuffer4Bit;
    tmp[1] = (unsigned char)((columnIdx & 0x00FF00) >> 8);
    tmp[2] = (unsigned char)( columnIdx & 0x0000FF);
    tmp[3] = 0xFF;

    ftStatus = FT4222_SPIMaster_MultiReadWrite(ftHandle, pData, &tmp[0], tmp.size(), 0, size, &sizeOfRead);
    if((ftStatus!=FT4222_OK) ||  (sizeOfRead != size))
    {
        FT4222_SPIMaster_SetLines(ftHandle, SPI_IO_SINGLE);

        return false;
    }

    FT4222_SPIMaster_SetLines(ftHandle, SPI_IO_SINGLE);
#else // 1 bit mode

    // step 3 : read buffer with 1 bit method
    tmp.resize(4);

    tmp[0] = CmdReadBuffer1Bit;
    tmp[1] = (unsigned char)((columnIdx & 0x00FF00) >> 8);
    tmp[2] = (unsigned char)( columnIdx & 0x0000FF);
    tmp[3] = 0xFF;

    ftStatus = FT4222_SPIMaster_SingleWrite(ftHandle, &tmp[0], tmp.size(), &sizeTransferred, false);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != tmp.size()))
    {
        return false;
    }

    ftStatus = FT4222_SPIMaster_SingleRead(ftHandle, pData, size, &sizeTransferred, true);
    if((ftStatus!=FT4222_OK) ||  (sizeTransferred != size))
    {
        return false;
    }
    

#endif

    return true;  
}


inline bool WriteFlash(FT_HANDLE ftHandle, uint32 startAddr, unsigned char * pData , uint16 size)
{
    uint16 notSentByte = size;
    uint16 sentByte=0;
    
    while (notSentByte > 0)
    {
        uint16 data_size = std::min<size_t>(SPI_PAGE_SIZE, notSentByte);

        // it will happen on the first page that you want to write
        if(startAddr % SPI_PAGE_SIZE != 0)
        {
            uint16 columnIdx = startAddr % SPI_PAGE_SIZE;

            if(columnIdx + data_size > SPI_PAGE_SIZE)
                data_size = SPI_PAGE_SIZE - columnIdx;
        }
        
        if(!WritePage(ftHandle, startAddr+sentByte, &pData[sentByte] , data_size))
        {
            printf("WritePage failed\n");
            return false;
        }
        notSentByte -= data_size;
        sentByte    += data_size;

    }

    return true;

}


inline bool ReadFlash(FT_HANDLE ftHandle, uint32 startAddr, unsigned char * pData , uint16 size)
{
    uint16 notSentByte = size;
    uint16 sentByte=0;
    
    while (notSentByte > 0)
    {
        uint16 data_size = std::min<size_t>(SPI_PAGE_SIZE, notSentByte);

        // it will happen on the first page that you want to write
        if(startAddr % SPI_PAGE_SIZE != 0)
        {
            uint16 columnIdx = startAddr % SPI_PAGE_SIZE;

            if(columnIdx + data_size > SPI_PAGE_SIZE)
                data_size = SPI_PAGE_SIZE - columnIdx;
        }

        if(!ReadPage(ftHandle, startAddr+sentByte, &pData[sentByte] , data_size))
        {
            printf("readPage failed\n");
            return false;
        }
        notSentByte -= data_size;
        sentByte    += data_size;

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

    
    FT4222_SetClock(ftHandle,SYS_CLK_48);

    ftStatus = FT4222_SPIMaster_Init(ftHandle, SPI_IO_SINGLE, CLK_DIV_4, CLK_IDLE_LOW, CLK_LEADING, 0x01);
    if (FT_OK != ftStatus)
    {
        printf("Init FT4222 as SPI master device failed!\n");
        return 0;
    }

    ftStatus = FT4222_SPI_SetDrivingStrength(ftHandle,DS_4MA, DS_4MA, DS_4MA);
    if (FT_OK != ftStatus)
    {
        printf("set spi driving strength failed!\n");
        return 0;
    }

    int testSize = 10240;

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

    if(!ReadID(ftHandle))
    {
        printf("read ID failed!\n");
        return 0;
    }
    else
    {
        printf("read ID OK!\n");
    }

    if(!Reset(ftHandle))
    {
        printf("Reset failed!\n");
        return 0;
    }
    else
    {
        printf("Reset OK!\n");
    }

    if(!UnlockAllBlock(ftHandle))
    {
        printf("UnlockAllBlock failed!\n");
        return 0;
    }
    else
    {
        printf("UnlockAllBlock OK!\n");
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

    if(!WriteFlash(ftHandle, startAddr, &sendData[0], testSize))
    {
        printf("write flash failed!\n");
        return 0;
    }
    else
    {
        printf("write flash OK!\n");
    }


    if(!ReadFlash(ftHandle, startAddr, &recvData[0], testSize))
    {
        printf("read flash failed!\n");
        return 0;
    }
    else
    {
        printf("read flash OK!\n");
    }
    if( 0 != memcmp(&sendData[0], &recvData[0], testSize))
    {
        printf("compare data error!\n");

        for(int idx=0;idx<testSize; idx++)
        {
            if(sendData[idx] != recvData[idx])
            {
                printf("[%x] sendData = %x , recvData = %x\n",idx, sendData[idx] , recvData[idx]);
            }
        }
        
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

using FTD2XX_NET;

namespace CSharp_FT4222_SPI_Master
{
    class Program
    {
        //**************************************************************************
        //
        // FUNCTION IMPORTS FROM FTD2XX DLL
        //
        //**************************************************************************

        [DllImport("ftd2xx.dll")]
        static extern FTDI.FT_STATUS FT_CreateDeviceInfoList(ref UInt32 numdevs);

        [DllImport("ftd2xx.dll")]
        static extern FTDI.FT_STATUS FT_GetDeviceInfoDetail(UInt32 index, ref UInt32 flags, ref FTDI.FT_DEVICE chiptype, ref UInt32 id, ref UInt32 locid, byte[] serialnumber, byte[] description, ref IntPtr ftHandle);

        //[DllImportAttribute("ftd2xx.dll", CallingConvention = CallingConvention.Cdecl)]
        [DllImport("ftd2xx.dll")]
        static extern FTDI.FT_STATUS FT_OpenEx(uint pvArg1, int dwFlags, ref IntPtr ftHandle);

        //[DllImportAttribute("ftd2xx.dll", CallingConvention = CallingConvention.Cdecl)]
        [DllImport("ftd2xx.dll")]
        static extern FTDI.FT_STATUS FT_Close(IntPtr ftHandle);


        const byte FT_OPEN_BY_SERIAL_NUMBER = 1;
        const byte FT_OPEN_BY_DESCRIPTION = 2;
        const byte FT_OPEN_BY_LOCATION = 4;

        //**************************************************************************
        //
        // FUNCTION IMPORTS FROM LIBFT4222 DLL
        //
        //**************************************************************************

        [DllImport("LibFT4222.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern FT4222_STATUS FT4222_SetClock(IntPtr ftHandle, FT4222_ClockRate clk);

        [DllImport("LibFT4222.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern FT4222_STATUS FT4222_GetClock(IntPtr ftHandle, ref FT4222_ClockRate clk);

        [DllImport("LibFT4222.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern FT4222_STATUS FT4222_SPIMaster_Init(IntPtr ftHandle, FT4222_SPIMode ioLine, FT4222_SPIClock clock, FT4222_SPICPOL cpol, FT4222_SPICPHA cpha, Byte ssoMap);

        [DllImport("LibFT4222.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern FT4222_STATUS FT4222_SPI_SetDrivingStrength(IntPtr ftHandle, SPI_DrivingStrength clkStrength, SPI_DrivingStrength ioStrength, SPI_DrivingStrength ssoStregth);

        [DllImport("LibFT4222.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern FT4222_STATUS FT4222_SPIMaster_SingleReadWrite(IntPtr ftHandle, ref byte readBuffer, ref byte writeBuffer, ushort bufferSize, ref ushort sizeTransferred, bool isEndTransaction);

        // FT4222 Device status
        public enum FT4222_STATUS
        {
            FT4222_OK,
            FT4222_INVALID_HANDLE,
            FT4222_DEVICE_NOT_FOUND,
            FT4222_DEVICE_NOT_OPENED,
            FT4222_IO_ERROR,
            FT4222_INSUFFICIENT_RESOURCES,
            FT4222_INVALID_PARAMETER,
            FT4222_INVALID_BAUD_RATE,
            FT4222_DEVICE_NOT_OPENED_FOR_ERASE,
            FT4222_DEVICE_NOT_OPENED_FOR_WRITE,
            FT4222_FAILED_TO_WRITE_DEVICE,
            FT4222_EEPROM_READ_FAILED,
            FT4222_EEPROM_WRITE_FAILED,
            FT4222_EEPROM_ERASE_FAILED,
            FT4222_EEPROM_NOT_PRESENT,
            FT4222_EEPROM_NOT_PROGRAMMED,
            FT4222_INVALID_ARGS,
            FT4222_NOT_SUPPORTED,
            FT4222_OTHER_ERROR,
            FT4222_DEVICE_LIST_NOT_READY,

            FT4222_DEVICE_NOT_SUPPORTED = 1000,        // FT_STATUS extending message
            FT4222_CLK_NOT_SUPPORTED,     // spi master do not support 80MHz/CLK_2
            FT4222_VENDER_CMD_NOT_SUPPORTED,
            FT4222_IS_NOT_SPI_MODE,
            FT4222_IS_NOT_I2C_MODE,
            FT4222_IS_NOT_SPI_SINGLE_MODE,
            FT4222_IS_NOT_SPI_MULTI_MODE,
            FT4222_WRONG_I2C_ADDR,
            FT4222_INVAILD_FUNCTION,
            FT4222_INVALID_POINTER,
            FT4222_EXCEEDED_MAX_TRANSFER_SIZE,
            FT4222_FAILED_TO_READ_DEVICE,
            FT4222_I2C_NOT_SUPPORTED_IN_THIS_MODE,
            FT4222_GPIO_NOT_SUPPORTED_IN_THIS_MODE,
            FT4222_GPIO_EXCEEDED_MAX_PORTNUM,
            FT4222_GPIO_WRITE_NOT_SUPPORTED,
            FT4222_GPIO_PULLUP_INVALID_IN_INPUTMODE,
            FT4222_GPIO_PULLDOWN_INVALID_IN_INPUTMODE,
            FT4222_GPIO_OPENDRAIN_INVALID_IN_OUTPUTMODE,
            FT4222_INTERRUPT_NOT_SUPPORTED,
            FT4222_GPIO_INPUT_NOT_SUPPORTED,
            FT4222_EVENT_NOT_SUPPORTED,
        };

        public enum FT4222_ClockRate
        {
            SYS_CLK_60 = 0,
            SYS_CLK_24,
            SYS_CLK_48,
            SYS_CLK_80,

        };

        public enum FT4222_SPIMode
        {
            SPI_IO_NONE = 0,
            SPI_IO_SINGLE = 1,
            SPI_IO_DUAL = 2,
            SPI_IO_QUAD = 4,

        };

        public enum FT4222_SPIClock
        {
            CLK_NONE = 0,
            CLK_DIV_2,      // 1/2   System Clock
            CLK_DIV_4,      // 1/4   System Clock
            CLK_DIV_8,      // 1/8   System Clock
            CLK_DIV_16,     // 1/16  System Clock
            CLK_DIV_32,     // 1/32  System Clock
            CLK_DIV_64,     // 1/64  System Clock
            CLK_DIV_128,    // 1/128 System Clock
            CLK_DIV_256,    // 1/256 System Clock
            CLK_DIV_512,    // 1/512 System Clock

        };

        public enum FT4222_SPICPOL
        {
            CLK_IDLE_LOW = 0,
            CLK_IDLE_HIGH = 1,
        };

        public enum FT4222_SPICPHA
        {
            CLK_LEADING = 0,
            CLK_TRAILING = 1,
        };

        public enum SPI_DrivingStrength
        {
            DS_4MA = 0,
            DS_8MA,
            DS_12MA,
            DS_16MA,
        };

        static FTDI.FT_DEVICE_INFO_NODE deviceA = new FTDI.FT_DEVICE_INFO_NODE();

        static void Main(string[] args)
        {
            // variable
            IntPtr ftHandle = new IntPtr();
            FTDI.FT_STATUS ftStatus = 0;
            FT4222_STATUS ft42Status = 0;

            // Check device
            UInt32 numOfDevices = 0;
            ftStatus = FT_CreateDeviceInfoList(ref numOfDevices);

            if (numOfDevices > 0)
            {
                Console.WriteLine("Devices: {0}", numOfDevices);

                // Check for valid devices to add to array
                for (uint n = 0; n < numOfDevices; n++)
                {
                    FTDI.FT_DEVICE_INFO_NODE devInfo = new FTDI.FT_DEVICE_INFO_NODE();

                    byte[] sernum = new byte[16];
                    byte[] desc = new byte[64];

                    ftStatus = FT_GetDeviceInfoDetail(n, ref devInfo.Flags, ref devInfo.Type, ref devInfo.ID, ref devInfo.LocId,
                                                sernum, desc, ref devInfo.ftHandle);

                    devInfo.SerialNumber = Encoding.ASCII.GetString(sernum, 0, 16);
                    devInfo.Description = Encoding.ASCII.GetString(desc, 0, 64);
                    devInfo.SerialNumber = devInfo.SerialNumber.Substring(0, devInfo.SerialNumber.IndexOf("\0"));
                    devInfo.Description = devInfo.Description.Substring(0, devInfo.Description.IndexOf("\0"));

                    if (FTDI.FT_STATUS.FT_OK == ftStatus)
                    {
                        Console.WriteLine("Device {0}", n);
                        Console.WriteLine("  Flags= 0x{0:X8}, ({1})", devInfo.Flags, devInfo.Flags.ToString());
                        if (devInfo.Type <= FTDI.FT_DEVICE.FT_DEVICE_BM || devInfo.Type <= FTDI.FT_DEVICE.FT_DEVICE_232H)
                            Console.WriteLine("  Type= {0}", devInfo.Type.ToString());
                        else
                            Console.WriteLine("  Type= {0}", devInfo.Type);
                        Console.WriteLine("  ID= 0x{0:X8}", devInfo.ID);
                        Console.WriteLine("  LocId= 0x{0:X8}", devInfo.LocId);
                        Console.WriteLine("  SerialNumber= {0}", devInfo.SerialNumber);
                        Console.WriteLine("  Description= {0}", devInfo.Description);
                        Console.WriteLine("  ftHandle= 0x{0:X8}", devInfo.ftHandle);

                        if (devInfo.Description.Equals("FT4222 A"))
                        {
                            deviceA = devInfo;
                        }
                    }
                }
            }
            else
            {
                Console.WriteLine("No FTDI device");
                Console.WriteLine("NG! Press Enter to continue.");
                Console.ReadLine();
                return;
            }


            // Open device "FT4222 A"
            ftStatus = FT_OpenEx(deviceA.LocId, FT_OPEN_BY_LOCATION, ref ftHandle);

            if (ftStatus != FTDI.FT_STATUS.FT_OK)
            {
                Console.WriteLine("Open NG: {0}", ftStatus);
                Console.WriteLine("NG! Press Enter to continue.");
                Console.ReadLine();
                return;
            }


            // Set FT4222 clock
            FT4222_ClockRate ft4222_Clock = FT4222_ClockRate.SYS_CLK_24;

            ft42Status = FT4222_SetClock(ftHandle, FT4222_ClockRate.SYS_CLK_24);
            if (ft42Status != FT4222_STATUS.FT4222_OK)
            {
                Console.WriteLine("SetClock NG: {0}. Press Enter to continue.", ft42Status);
                Console.ReadLine();
                return;
            }
            else
            {
                Console.WriteLine("SetClock OK");

                ft42Status = FT4222_GetClock(ftHandle, ref ft4222_Clock);
                if (ft42Status != FT4222_STATUS.FT4222_OK)
                {
                    Console.WriteLine("GetClock NG: {0}. Press Enter to continue.", ft42Status);
                    Console.ReadLine();
                    return;
                }
                else
                {
                    Console.WriteLine("GetClock:" + ft4222_Clock);
                }
            }



            // Init FT4222 SPI Master
            ft42Status = FT4222_SPIMaster_Init(ftHandle, FT4222_SPIMode.SPI_IO_SINGLE, FT4222_SPIClock.CLK_DIV_16, FT4222_SPICPOL.CLK_IDLE_LOW, FT4222_SPICPHA.CLK_LEADING, 0x01);
            if (ft42Status != FT4222_STATUS.FT4222_OK)
            {
                Console.WriteLine("Open NG: {0}", ft42Status);
                Console.WriteLine("NG! Press Enter to continue.");
                Console.ReadLine();
                return;
            }
            else
            {
                Console.WriteLine("Init FT4222 SPI Master OK");
            }

            ft42Status = FT4222_SPI_SetDrivingStrength(ftHandle, SPI_DrivingStrength.DS_12MA, SPI_DrivingStrength.DS_12MA, SPI_DrivingStrength.DS_16MA);
            if (ft42Status != FT4222_STATUS.FT4222_OK)
            {
                Console.WriteLine("SetDrivingStrength NG: {0}", ft42Status);
                Console.WriteLine("NG! Press Enter to continue.");
                Console.ReadLine();
                return;
            }


            // Read/Write data
            Console.WriteLine("Prepare to read/write data. Press Enter to continue.");
            Console.ReadLine();

            ushort sizeTransferred = 0;
            byte[] readBuf = new byte[] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] writeBuf = new byte[] { 0x46, 0x54, 0x34, 0x32, 0x32, 0x32 };

            ft42Status = FT4222_SPIMaster_SingleReadWrite(ftHandle, ref readBuf[0], ref writeBuf[0], (ushort)writeBuf.Length, ref sizeTransferred, true);

            if (ft42Status != FT4222_STATUS.FT4222_OK)
            {
                Console.WriteLine("Write NG: {0}", ft42Status);
            }
            else
            {
                // Show read/write data
                string strR = System.Text.Encoding.Default.GetString(readBuf);
                Console.WriteLine("R:["+ strR + "]");

                string strW = System.Text.Encoding.Default.GetString(writeBuf);
                Console.WriteLine("W:[" + strW + "]");

                Console.WriteLine("Read/Write OK, sizeTransferred: {0}", sizeTransferred);
            }

            Console.WriteLine("Done. Press Enter to continue.");
            Console.ReadLine();

            //End
            return;
        }
    }
}

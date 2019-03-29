# FT4222
FT4222 USB to SPI bridge tester

csharp_spi_master_GUI allows testing of a UMFT4222EV-D evaluation module.

Three radio buttons are provided.  The first writes/reads back all 1-byte patterns, 0x00 through 0xFF.
The second writes/reads back a single byte, which is entered into a text box.
The third writes/reads back a single word as two individual bytes, with the value entered into a text box.  
The values are written MSB first.

All values entered must be the correct length and must be valid hexadecimal characters.

Adapted from example csharp_spi_master from FTDI.

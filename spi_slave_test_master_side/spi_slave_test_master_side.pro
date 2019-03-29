TEMPLATE = app
CONFIG  -= qt
CONFIG  += console


TARGET = spi_slave_test_master_side

#####################
debug:OBJECTS_DIR   = ./tmp/debug_obj
release:OBJECTS_DIR = ./tmp/release_obj
MOC_DIR     = ./tmp
UI_DIR      = ./tmp


#####################
# ftd2xx

INCLUDEPATH  += ../../imports/ftd2xx
QMAKE_LIBDIR += ../../imports/ftd2xx/i386
LIBS += -lftd2xx


#####################
# LibFT4222

INCLUDEPATH  += ../../imports/LibFT4222/inc
QMAKE_LIBDIR += ../../imports/LibFT4222/lib
debug:LIBS += -lLibFT4222


#####################

SOURCES += spi_slave_test_master_side.cpp

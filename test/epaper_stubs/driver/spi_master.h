#pragma once
#include "host.h"

#define SPI_DEVICE_HALFDUPLEX (1U << 4)
#define SPI_DEVICE_3WIRE (1U << 2)
#define SPI_DEVICE_NO_DUMMY (1U << 6)
typedef struct {
    int clock_speed_hz;
    int mode;
    int spics_io_num;
    int queue_size;
    unsigned flags;
} spi_device_interface_config_t;

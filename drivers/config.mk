include $(ROOT)/drivers/sdhci/config.mk
include $(ROOT)/drivers/i2c/config.mk
C_SRC += \
   $(ROOT)/drivers/spi_nor/bm_spi_nor.c \
   $(ROOT)/drivers/spi_nand/bm_spi_nand.c

DEFS +=

CXXFLAGS +=

LDFLAGS +=

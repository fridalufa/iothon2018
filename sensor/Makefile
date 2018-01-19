APPLICATION = IoThon

# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x

BOARD_WHITELIST := fox iotlab-m3 msba2 mulle native pba-d-01-kw2x samr21-xpro nucleo-f401


# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

CFLAGS += -DDEVELHELP
CFLAGS += -DUSE_LINKLAYER
CFLAGS += -DCCNL_UAPI_H_
CFLAGS += -DUSE_SUITE_NDNTLV
CFLAGS += -DNEEDS_PREFIX_MATCHING
CFLAGS += -DNEEDS_PACKET_CRAFTING

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += timex
USEMODULE += xtimer
USEMODULE += random
USEMODULE += prng_minstd
#USEMODULE += netstats_l2
USEMODULE += gnrc_pktdump
USEMODULE += saul_default
USEMODULE += dht
USEMODULE += encx24j600

# set board specific peripheral configurations

ifneq (,$(filter nucleo-f401,$(BOARD)))

# these settings are probably valid for PoEll-i on most nucelo boards, but
# tested only on nucleo-f334

  ENC_SPI ?= SPI_DEV\(0\)
  ENC_CS  ?= GPIO_PIN\(PORT_C,10\)
  ENC_INT ?= GPIO_PIN\(PORT_D,2\)

endif

CFLAGS += -DENCX24J600_SPI=$(ENC_SPI)
CFLAGS += -DENCX24J600_CS=$(ENC_CS)
CFLAGS += -DENCX24J600_INT=$(ENC_INT)

FEATURES_REQUIRED += periph_gpio
FEATURES_REQUIRED += periph_spi

USEPKG += tlsf
USEPKG += ccn-lite
USEMODULE += ccn-lite-utils

include $(RIOTBASE)/Makefile.include

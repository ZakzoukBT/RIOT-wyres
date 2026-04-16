# name of your application
APPLICATION = test-lorachat

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= ${HOME}/github/RIOT-OS/RIOT

# Path to external boards
EXTERNAL_BOARD_DIRS ?= ../../boards
# If no BOARD is found in the environment, use this default:
BOARD ?= wyres-base

USEMODULE += od
USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_cmds_default

DRIVER ?= sx1272
USEMODULE += $(DRIVER)

# Keep debug sections small on constrained devices
CFLAGS_DBG =
CFLAGS += -DISR_STACKSIZE=1024U

include $(RIOTBASE)/Makefile.include

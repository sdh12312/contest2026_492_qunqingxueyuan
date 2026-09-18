# SPDX-License-Identifier: Apache-2.0

include $(APPDIR)/Make.defs

PROGNAME = $(CONFIG_FLYREFLEX_PROGNAME)
PRIORITY = $(CONFIG_FLYREFLEX_PRIORITY)
STACKSIZE = $(CONFIG_FLYREFLEX_STACKSIZE)
MODULE = $(CONFIG_FLYREFLEX)

MAINSRC = src/flyreflex_main.c
CSRCS = src/flyreflex_runtime.c src/looming_input.c src/reflex_engine.c
CSRCS += src/safety_arbiter.c src/flyreflex_ui.c
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/packages/demos/flyreflex/include

include $(APPDIR)/Application.mk

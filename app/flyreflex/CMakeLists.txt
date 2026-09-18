# SPDX-License-Identifier: Apache-2.0

if(CONFIG_FLYREFLEX)
  nuttx_add_application(
    NAME
    ${CONFIG_FLYREFLEX_PROGNAME}
    PRIORITY
    ${CONFIG_FLYREFLEX_PRIORITY}
    STACKSIZE
    ${CONFIG_FLYREFLEX_STACKSIZE}
    MODULE
    ${CONFIG_FLYREFLEX}
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    SRCS
    src/flyreflex_main.c
    src/flyreflex_runtime.c
    src/looming_input.c
    src/reflex_engine.c
    src/safety_arbiter.c
    src/flyreflex_ui.c)
endif()

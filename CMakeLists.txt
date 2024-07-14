cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pacman C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(pacman
	rp2040_ntsc_pacman.c
	rp2040_pwm_ntsc_textgraph.c
	text_graph_library.c
	fontdata.c
)

# Pull in basic dependencies
target_link_libraries(pacman pico_stdlib hardware_pwm hardware_dma)

# create map/bin/hex file etc.
pico_add_extra_outputs(pacman)

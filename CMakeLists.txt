cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(smartgate-mqtt C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME} 
    smartgate-mqtt.c 
    lib/hcSR04.c
    lib/ledRGB.c
    lib/buzzer.c
    lib/ssd1306.c
    lib/led_5x5.c)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/ws2812.pio)

pico_set_program_name(${PROJECT_NAME} "smartgate-mqtt")
pico_set_program_version(${PROJECT_NAME} "0.1")

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME}  1)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_gpio
        hardware_pwm
        hardware_i2c
        hardware_adc
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
        pico_mbedtls
        pico_lwip_mbedtls
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)


# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
        
        )


pico_add_extra_outputs(${PROJECT_NAME})
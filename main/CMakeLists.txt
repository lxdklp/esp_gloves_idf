file(GLOB LIB_SOURCES "lib/*/*.cpp")

set(INCLUDE_PATHS "lib")
foreach(subdir ${SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${subdir}")
        list(APPEND INCLUDE_PATHS "lib/${subdir}")
    endif()
endforeach()

idf_component_register(SRCS "main.cpp"
                        ${LIB_SOURCES}
                        INCLUDE_DIRS ${INCLUDE_PATHS}
                        PRIV_REQUIRES
                        spi_flash
                        esp_wifi
                        esp_netif
                        nvs_flash
                        esp_event
                        esp_http_server
                        driver
                        esp_driver_gpio
                        esp_driver_i2c
                        )

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++26)

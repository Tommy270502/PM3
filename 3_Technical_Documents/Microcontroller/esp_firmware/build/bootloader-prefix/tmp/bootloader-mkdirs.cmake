# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/thomas/Documents/git/esp-idf/components/bootloader/subproject"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/tmp"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/src/bootloader-stamp"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/src"
  "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/thomas/Documents/git/PM3/3_Technical_Documents/Microcontroller/esp_firmware/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

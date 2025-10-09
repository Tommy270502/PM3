################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/DMX.c \
../Core/Src/audio_codec.c \
../Core/Src/calc.c \
../Core/Src/display.c \
../Core/Src/main.c \
../Core/Src/menu.c \
../Core/Src/pushbutton.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/system_stm32f4xx.c 

OBJS += \
./Core/Src/DMX.o \
./Core/Src/audio_codec.o \
./Core/Src/calc.o \
./Core/Src/display.o \
./Core/Src/main.o \
./Core/Src/menu.o \
./Core/Src/pushbutton.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/Src/DMX.d \
./Core/Src/audio_codec.d \
./Core/Src/calc.d \
./Core/Src/display.d \
./Core/Src/main.d \
./Core/Src/menu.d \
./Core/Src/pushbutton.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32F429xx -DARM_MATH_CM4 -c -I../Core/Inc -I../Drivers/CMSIS/Include -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I"../Drivers/BSP/Components/Common" -I"../Drivers/BSP/Components/stmpe811" -I"../Drivers/BSP/Components/ili9341" -I"../Drivers/BSP/STM32F429I-Discovery" -I"../Utilities/Fonts" -I../Drivers/CMSIS/DSP/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/DMX.cyclo ./Core/Src/DMX.d ./Core/Src/DMX.o ./Core/Src/DMX.su ./Core/Src/audio_codec.cyclo ./Core/Src/audio_codec.d ./Core/Src/audio_codec.o ./Core/Src/audio_codec.su ./Core/Src/calc.cyclo ./Core/Src/calc.d ./Core/Src/calc.o ./Core/Src/calc.su ./Core/Src/display.cyclo ./Core/Src/display.d ./Core/Src/display.o ./Core/Src/display.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/menu.cyclo ./Core/Src/menu.d ./Core/Src/menu.o ./Core/Src/menu.su ./Core/Src/pushbutton.cyclo ./Core/Src/pushbutton.d ./Core/Src/pushbutton.o ./Core/Src/pushbutton.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su

.PHONY: clean-Core-2f-Src


################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../portable/heap_2.c \
../portable/port.c 

OBJS += \
./portable/heap_2.o \
./portable/port.o 

C_DEPS += \
./portable/heap_2.d \
./portable/port.d 


# Each subdirectory must supply rules for building sources it contributes
portable/%.o: ../portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\include" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\CMSISv1p30_LPC17xx\inc" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\portable" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\demo_code" -O1 -g3 -Wall -c -fmessage-length=0 -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



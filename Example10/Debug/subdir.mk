################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cr_startup_lpc17.c \
../main.c 

OBJS += \
./cr_startup_lpc17.o \
./main.o 

C_DEPS += \
./cr_startup_lpc17.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__USE_CMSIS=CMSISv1p30_LPC17xx -D__CODE_RED -D__REDLIB__ -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\CMSISv1p30_LPC17xx" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\demo_code" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\CMSISv1p30_LPC17xx\inc" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\include" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\FreeRTOS_Library\portable" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\Lib_MCU\inc" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\Lib_EaBaseBoard\inc" -I"C:\Users\tbsou\Documents\LPCXpresso_6.1.4_194\workspace\Lib_CMSISv1p30_LPC17xx\inc" -O0 -g3 -fsigned-char -c -fmessage-length=0 -fno-builtin -ffunction-sections -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



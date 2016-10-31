# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../src/MOPS.c \
../../src/MOPS_RTnet_Con.c \
../../src/MQTT.c 

OBJS += \
./src/MOPS.o \
./src/MOPS_RTnet_Con.o \
./src/MQTT.o 

C_DEPS += \
./src/MOPS.d \
./src/MOPS_RTnet_Con.d \
./src/MQTT.d 


# Each subdirectory must supply rules for building sources it contributes
src/MOPS.o: ../../src/MOPS.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../inc" -O2 -g -Wall -c -fmessage-length=0 -I/usr/local/rtnet/include -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__XENO__ -I/usr/xenomai/include/posix -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/MOPS_RTnet_Con.o: ../../src/MOPS_RTnet_Con.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../inc" -O2 -g -Wall -c -fmessage-length=0 -I/usr/local/rtnet/include -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__XENO__ -I/usr/xenomai/include/posix -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/MQTT.o: ../../src/MQTT.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../inc" -O2 -g -Wall -c -fmessage-length=0 -I/usr/local/rtnet/include -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__XENO__ -I/usr/xenomai/include/posix -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



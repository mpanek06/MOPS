# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../proces_pub.c

OBJS += \
./proces_pub.o

C_DEPS += \
./proces_pub.d


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../inc" -O2 -g -Wall -c -fmessage-length=0 -I/usr/local/rtnet/include -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__XENO__ -I/usr/xenomai/include/posix -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../App/Source/WormPlugin/PluginManager.cpp \
../App/Source/WormPlugin/WormHookImpl.cpp 

OBJS += \
./App/Source/WormPlugin/PluginManager.o \
./App/Source/WormPlugin/WormHookImpl.o 

CPP_DEPS += \
./App/Source/WormPlugin/PluginManager.d \
./App/Source/WormPlugin/WormHookImpl.d 


# Each subdirectory must supply rules for building sources it contributes
App/Source/WormPlugin/%.o: ../App/Source/WormPlugin/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	mips-openwrt-linux-g++ -std=c++1y -D_RELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



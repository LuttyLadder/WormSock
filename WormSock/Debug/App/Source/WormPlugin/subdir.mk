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
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -D__cplusplus=201103L -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



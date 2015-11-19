################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../App/Source/WormTool/Tool.cpp \
../App/Source/WormTool/WormConfig.cpp \
../App/Source/WormTool/WormToolImpl.cpp 

OBJS += \
./App/Source/WormTool/Tool.o \
./App/Source/WormTool/WormConfig.o \
./App/Source/WormTool/WormToolImpl.o 

CPP_DEPS += \
./App/Source/WormTool/Tool.d \
./App/Source/WormTool/WormConfig.d \
./App/Source/WormTool/WormToolImpl.d 


# Each subdirectory must supply rules for building sources it contributes
App/Source/WormTool/%.o: ../App/Source/WormTool/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -D_X86_RELEASE -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



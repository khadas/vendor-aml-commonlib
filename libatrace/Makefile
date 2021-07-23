CPP_LIBS := -lz

CPP_SRCS += \
atrace_cmd.cpp 

CPP_OBJS += \
atrace_cmd.o 

CPP_DEPS += \
atrace_cmd.d 

C_LIBS := -lpthread

C_SRCS += \
atrace.c 

C_OBJS += \
atrace.o 

C_DEPS += \
atrace.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: %.cpp
	$(CXX) -std=c++0x -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"

%.o: %.c
	$(CC)  -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"

# All Target
all: atrace libatrace.so

# Tool invocations
atrace: $(CPP_OBJS)
	$(CXX)  -o "atrace" $(CPP_OBJS) $(CPP_LIBS)

libatrace.so: $(C_OBJS)
	$(CC)  -shared -o "libatrace.so" $(C_OBJS) $(C_LIBS)


# Other Targets
clean:
	-$(RM) $(C_OBJS)$(CPP_OBJS)$(CPP_DEPS)$(C_DEPS) atrace libatrace.so

.PHONY: all clean dependents 

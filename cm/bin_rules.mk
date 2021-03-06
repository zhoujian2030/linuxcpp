# platform dependency
ifeq ($(PLATFORM), arm)
LIBSA_LOG4CPLUS = /usr/local/log4cplusarm/lib/liblog4cplus.a
else
LIBSA_LOG4CPLUS = /usr/local/log4cplusx86/lib/liblog4cplus.a
endif

all: $(TARGET)

# Targets
$(TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo $(TARGET): $(OBJS) $(LIBS) > $(DEPDIR)/$(TARGET).d
	

clean:
	rm -rf $(TARGET) $(LIBDIR) $(OBJDIR) $(SRCDIR)/*.o $(DEPDIR)
	
install:
	# nothing to do

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
-include $(DEPDIR)/$(TARGET).d
endif

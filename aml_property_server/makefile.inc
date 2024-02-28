SUFFIXES += .d
DEPFILES := $(OBJS:.o=.d)

NODEPS := clean install

$(OBJDIR)/$(BASEDIR)/%.o: %.c $(OBJDIR)/$(BASEDIR)/%.d
	@echo " [CC] $<"
	@$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/$(BASEDIR)/%.d: %.c
	@echo " [DEP] $<"
	@mkdir -p $(OBJDIR)/$(BASEDIR)
	@$(CC) $(CFLAGS) -M -MT "$(patsubst %.c,$(OBJDIR)/$(BASEDIR)/%.o,$<)" $< -MF $@

$(OBJDIR)/$(BASEDIR)/%.o: %.cpp $(OBJDIR)/$(BASEDIR)/%.d
	@echo " [CXX] $<"
	@$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/$(BASEDIR)/%.d: %.cpp
	@echo " [DEP] $<"
	@mkdir -p $(OBJDIR)/$(BASEDIR)
	@$(CXX) $(CPPFLAGS) $(CFLAGS) -M -MT "$(patsubst %.cpp,$(OBJDIR)/$(BASEDIR)/%.o,$<)" $< -MF $@

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif


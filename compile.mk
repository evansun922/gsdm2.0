#  Copyright (c) 2010,
#  sunlei
#


#toolchain paths
CCOMPILER=$(CC)
CXXCOMPILER=$(CXX)
CP=cp

#output settings
STATIC_LIB_SUFIX=.a
DYNAMIC_LIB_SUFIX=.so

#output settings
OUTPUT_BASE = ./output


FEATURES_DEFINES = \



DEFINES = $(PLATFORM_DEFINES) $(FEATURES_DEFINES)

#gsdm
GSDM_INCLUDE=-I$(PROJECT_BASE_PATH)/sources/gsdm/include -I$(PROJECT_BASE_PATH)
GSDM_SRCS = $(shell find $(PROJECT_BASE_PATH)/sources/gsdm/src -type f -name "*.cpp")
GSDM_OBJS = $(GSDM_SRCS:.cpp=.gsdm.o)
ST_OBJS = $(PROJECT_BASE_PATH)/3rdparty/st/obj/*.o


#targets
all:  create_output_dirs st gsdm

st:
	@cd $(PROJECT_BASE_PATH)/3rdparty/st/;make $(ST_MODE);cd -

create_output_dirs:
	@echo $(CONFIGURE)
	@echo
	@echo
	@mkdir -p $(OUTPUT_BASE)
	@mkdir -p $(OUTPUT_BASE)/gsdm
	@mkdir -p $(OUTPUT_BASE)/gsdm/buffering
	@mkdir -p $(OUTPUT_BASE)/gsdm/logging
	@mkdir -p $(OUTPUT_BASE)/gsdm/eventdrive
	@mkdir -p $(OUTPUT_BASE)/gsdm/pooldrive
	@mkdir -p $(OUTPUT_BASE)/gsdm/protocols
	@mkdir -p $(OUTPUT_BASE)/gsdm/platform
	@mkdir -p $(OUTPUT_BASE)/gsdm/utils
	@mkdir -p $(OUTPUT_BASE)/gsdm/3rdparty/
	@mkdir -p $(OUTPUT_BASE)/gsdm/3rdparty/include


gsdm:  $(GSDM_OBJS)
	$(AR) rs $(OUTPUT_BASE)/libgsdm$(STATIC_LIB_SUFIX) $(ST_OBJS) $(GSDM_OBJS) $(SSL_BASE)
	$(CXXCOMPILER) -fPIC -shared -Wl,-soname,libgsdm$(DYNAMIC_LIB_SUFIX) \
									-o $(OUTPUT_BASE)/libgsdm$(DYNAMIC_LIB_SUFIX) $(GSDM_OBJS)
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/gsdm.h $(OUTPUT_BASE)/gsdm/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/buffering/iobuffer.h $(OUTPUT_BASE)/gsdm/buffering/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/logging/logger.h $(OUTPUT_BASE)/gsdm/logging/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/eventdrive/basecmd.h $(OUTPUT_BASE)/gsdm/eventdrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/eventdrive/baseprocess.h $(OUTPUT_BASE)/gsdm/eventdrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/eventdrive/baseworkerex.h $(OUTPUT_BASE)/gsdm/eventdrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/eventdrive/manager.h $(OUTPUT_BASE)/gsdm/eventdrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/eventdrive/httpclient.h $(OUTPUT_BASE)/gsdm/eventdrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/pooldrive/manager.h $(OUTPUT_BASE)/gsdm/pooldrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/pooldrive/poolworkerex.h $(OUTPUT_BASE)/gsdm/pooldrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/pooldrive/poolprocess.h $(OUTPUT_BASE)/gsdm/pooldrive/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/protocols/wsprotocol.h $(OUTPUT_BASE)/gsdm/protocols/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/protocols/httpprotocol.h $(OUTPUT_BASE)/gsdm/protocols/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/protocols/urlcode.h $(OUTPUT_BASE)/gsdm/protocols/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/platform/linuxplatform.h $(OUTPUT_BASE)/gsdm/platform/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/container.h $(OUTPUT_BASE)/gsdm/utils/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/crypto.h $(OUTPUT_BASE)/gsdm/utils/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/file.h $(OUTPUT_BASE)/gsdm/utils/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/gsdmconfig.h $(OUTPUT_BASE)/gsdm/utils/
	$(CP) -p $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/timersmanager.h $(OUTPUT_BASE)/gsdm/utils/
	$(CP) -p $(PROJECT_BASE_PATH)/3rdparty/include/st.h $(OUTPUT_BASE)/gsdm/3rdparty/include/
	@sh cpopenssl.sh


%.gsdm.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(GSDM_INCLUDE) $(OPENSSL_INCLUDE) -c $< -o $@


clean:
	@rm -rfv $(OUTPUT_BASE)
	@sh cleanupobjs.sh
	@cd $(PROJECT_BASE_PATH)/3rdparty/st/;make clean;cd -

install:
	@rm -frv $(PREFIX)/include/gsdm
	$(CP) -rfv $(OUTPUT_BASE)/gsdm $(PREFIX)/include/
	$(CP) -fv $(OUTPUT_BASE)/libgsdm$(STATIC_LIB_SUFIX) $(PREFIX)/lib/

uninstall:
	@rm -frv $(PREFIX)/include/gsdm
	@rm -fv $(PREFIX)/lib/libgsdm$(STATIC_LIB_SUFIX)

unconfig:
	@rm -fv config.mk
	@rm -fv Makefile
	@rm -frv .crypto
	@rm -frv .include
	@rm -frv .ssl
	@rm -fv $(PROJECT_BASE_PATH)/sources/gsdm/include/utils/crypto.h
	@rm -fv cleanupobjs.sh
	@rm -fv cpopenssl.sh

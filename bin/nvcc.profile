
TOP              = $(_HERE_)/..

PATH            += $(TOP)/extools/bin;$(TOP)/open64/bin;$(TOP)/bin;$(TOP)/lib;

INCLUDES        +=  "-I$(TOP)/include" "-I$(TOP)/include/cudart" $(_SPACE_)

LIBRARIES        =+ $(_SPACE_) "/LIBPATH:$(TOP)/lib" cudart.lib

CUDAFE_FLAGS    +=
OPENCC_FLAGS    +=
PTXAS_FLAGS     +=

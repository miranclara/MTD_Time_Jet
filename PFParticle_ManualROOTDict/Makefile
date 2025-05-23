CXX           = g++
ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLDFLAGS   = $(shell root-config --libs)
CXXFLAGS      = -Wall -fPIC $(ROOTCFLAGS) -Iinclude
LDFLAGS       = -shared $(ROOTLDFLAGS)

DICT_SRC      = src/PFParticleDict.cxx
DICT_HDRS     = PFParticle.h
DICT_LINKDEF  = LinkDef.h

all: libPFParticle.so

libPFParticle.so: src/PFParticle.cc $(DICT_SRC)
	$(CXX) $(CXXFLAGS) -c src/PFParticle.cc -o src/PFParticle.o
	$(CXX) $(CXXFLAGS) -c $(DICT_SRC) -o src/PFParticleDict.o
	$(CXX) $(LDFLAGS) -o $@ src/PFParticle.o src/PFParticleDict.o

$(DICT_SRC): include/$(DICT_HDRS) include/$(DICT_LINKDEF)
	@echo "Generating ROOT dictionary with rootcling..."
	rootcling -f $(DICT_SRC) \
	  -s libPFParticle.so \
	  -rml libPFParticle.so \
	  -rmf PFParticleDict_rdict.pcm \
	  -c -Iinclude include/$(DICT_HDRS) include/$(DICT_LINKDEF)
	@echo "Patching include path in generated dictionary..."
	sed -i 's#include "include/PFParticle.h"#include "PFParticle.h"#' $(DICT_SRC)
	
clean:
	@echo "Cleaning build artifacts..."
	rm -f src/*.o src/PFParticleDict.* libPFParticle.so *.pcm


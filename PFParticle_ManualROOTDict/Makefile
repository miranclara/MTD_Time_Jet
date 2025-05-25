CXX           = g++
ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLDFLAGS   = $(shell root-config --libs)
CXXFLAGS      = -Wall -fPIC $(ROOTCFLAGS) -Iinclude
LDFLAGS       = -shared $(ROOTLDFLAGS)

DICT_SRC      = src/PFParticleDict.cxx
DICT_HDRS     = PFParticle.h
DICT_LINKDEF  = LinkDef.h
DICT_PCM      = libPFParticle_rdict.pcm  # 👈 match ROOT expectation

all: libPFParticle.so $(DICT_PCM)

libPFParticle.so: src/PFParticle.o src/PFParticleDict.o
	$(CXX) $(LDFLAGS) -o $@ $^

src/PFParticle.o: src/PFParticle.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/PFParticleDict.o: $(DICT_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DICT_SRC) $(DICT_PCM): include/$(DICT_HDRS) include/$(DICT_LINKDEF)
	@echo "📦 Generating ROOT dictionary and PCM..."
	rootcling -f $(DICT_SRC) \
	  -s libPFParticle.so \
	  -rmf $(DICT_PCM) \
	  -rml libPFParticle.so \
	  -c -Iinclude include/$(DICT_HDRS) include/$(DICT_LINKDEF)

clean:
	rm -f src/*.o src/PFParticleDict.* libPFParticle.so *.pcm


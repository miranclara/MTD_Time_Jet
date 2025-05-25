#!/bin/bash
set -e

echo "🔧 Cleaning old installed libraries from CMSSW lib/..."
rm -f "$CMSSW_BASE/lib/$SCRAM_ARCH/libPFParticle.so"
rm -f "$CMSSW_BASE/lib/$SCRAM_ARCH/libPFParticle_rdict.pcm"

echo "🔧 Cleaning local build artifacts..."
cd "$CMSSW_BASE/src/PFParticle_ManualROOTDict"
rm -f src/*.o src/PFParticleDict.* libPFParticle.so

echo "⚙️  Rebuilding dictionary and shared lib..."
g++ -Wall -fPIC -pthread -std=c++20 -m64 \
  -Iinclude $(root-config --cflags) \
  -c src/PFParticle.cc -o src/PFParticle.o

echo "📦 Generating ROOT dictionary and PCM..."
rootcling -f src/PFParticleDict.cxx \
  -s libPFParticle.so \
  -rmf libPFParticle_rdict.pcm \
  -rml libPFParticle.so \
  -c -Iinclude include/PFParticle.h include/LinkDef.h

echo "🩹 Patching include path (if needed)..."
sed -i 's#include "include/PFParticle.h"#include "PFParticle.h"#' src/PFParticleDict.cxx

g++ -Wall -fPIC -pthread -std=c++20 -m64 \
  -Iinclude $(root-config --cflags) \
  -c src/PFParticleDict.cxx -o src/PFParticleDict.o

g++ -shared $(root-config --libs) \
  -Wl,-rpath,$(root-config --libdir) \
  -o libPFParticle.so src/PFParticle.o src/PFParticleDict.o

echo "📦 Installing library and PCM into CMSSW lib path..."
mkdir -p "$CMSSW_BASE/lib/$SCRAM_ARCH"
cp libPFParticle.so "$CMSSW_BASE/lib/$SCRAM_ARCH/"

# Check PCM exists before copying
if [[ -f libPFParticle_rdict.pcm ]]; then
  cp libPFParticle_rdict.pcm "$CMSSW_BASE/lib/$SCRAM_ARCH/"
  echo "✅ Dictionary PCM installed."
else
  echo "⚠️  Warning: libPFParticle_rdict.pcm not found! Dictionary may be incomplete."
fi

echo "✅ Done: PFParticle library and dictionary ready."
cd "$CMSSW_BASE/src"


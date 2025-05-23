#!/bin/bash
set -e

echo "🔧 Cleaning old installed libraries from CMSSW lib/..."
rm -f "$CMSSW_BASE/lib/$SCRAM_ARCH/libPFParticle.so"
rm -f "$CMSSW_BASE/lib/$SCRAM_ARCH/libPFParticle_rdict.pcm"

echo "🔧 Cleaning local build artifacts..."
cd "$CMSSW_BASE/src/PFParticle_ManualROOTDict"
rm -f src/*.o src/PFParticleDict.* libPFParticle.so libPFParticle_rdict.pcm

echo "⚙️  Regenerating ROOT dictionary via rootcling..."
rootcling -f src/PFParticleDict.cxx \
  -s libPFParticle.so \
  -rml libPFParticle.so \
  -rmf libPFParticle_rdict.pcm \
  -c -Iinclude include/PFParticle.h include/LinkDef.h

echo "🩹 Patching include path (if needed)..."
sed -i 's#include "include/PFParticle.h"#include "PFParticle.h"#' src/PFParticleDict.cxx || true

echo "🧱 Building shared library..."
g++ -Wall -fPIC -pthread -std=c++20 -m64 \
  $(root-config --cflags) -Iinclude \
  -c src/PFParticle.cc -o src/PFParticle.o

g++ -Wall -fPIC -pthread -std=c++20 -m64 \
  $(root-config --cflags) -Iinclude \
  -c src/PFParticleDict.cxx -o src/PFParticleDict.o

g++ -shared $(root-config --libs) -Wl,-rpath,$(root-config --libdir) \
  -o libPFParticle.so src/PFParticle.o src/PFParticleDict.o

echo "📦 Installing library and PCM into CMSSW lib path..."
mkdir -p "$CMSSW_BASE/lib/$SCRAM_ARCH"
cp libPFParticle.so "$CMSSW_BASE/lib/$SCRAM_ARCH/"

if [[ -f libPFParticle_rdict.pcm ]]; then
  cp libPFParticle_rdict.pcm "$CMSSW_BASE/lib/$SCRAM_ARCH/"
else
  echo "⚠️  Warning: libPFParticle_rdict.pcm was not generated."
fi

echo "✅ Done: PFParticle library and dictionary ready."
cd "$CMSSW_BASE/src"


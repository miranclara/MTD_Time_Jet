import FWCore.ParameterSet.Config as cms

process = cms.Process("Test")

# Define the source (you need to specify the input file here)
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:/afs/cern.ch/work/m/mrkim/2025Mar26_PUPPITEST/CMSSW_15_0_0/src/patTuple_onlyJets.root')
)
#The analyzer module
process.jetTree = cms.EDAnalyzer("JetTreeProducer")

#Path to execute the analyzer
process.p = cms.Path(process.jetTree)


import FWCore.ParameterSet.Config as cms

process = cms.Process("JETTREE")

process.load("FWCore.MessageService.MessageLogger_cfi")

# Input source 
process.source = cms.Source("PoolSource",
#    fileNames = cms.untracked.vstring('file:patTuple_onlyJets.root')
    fileNames = cms.untracked.vstring('file:$CMSSW_BASE/src/patTuple_onlyJets.root')
)
# Max events
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)  # process all events(-1)
)
# JetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    jets = cms.InputTag("selectedPatJets")
)
# Output configuration
process.TFileService = cms.Service("TFileService",
    fileName = cms.string('jetTree.root')  # This is where the flat ROOT file will be stored
)
# Path
process.p = cms.Path(process.jetTree)



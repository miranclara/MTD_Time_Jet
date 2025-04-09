import FWCore.ParameterSet.Config as cms


from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('MyJetAnalysis',Phase2C17I13M9)

process.load("FWCore.MessageService.MessageLogger_cfi")

# Input source 
process.source = cms.Source("PoolSource",
#    fileNames = cms.untracked.vstring('file:patTuple_onlyJets.root')
    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_0_0/RelValTTbar_14TeV/MINIAODSIM/PU_141X_mcRun4_realistic_v3_STD_Run4D110_PU-v2/2580000/9c9737ed-bb08-48c5-9f58-ba4bb149c0d7.root')
)

# Max events
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(100)  # process all events(-1)
)

# JetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    jetTag = cms.InputTag("slimmedJetsPuppi")
)

# Output configuration
process.TFileService = cms.Service("TFileService",
  fileName = cms.string('jetTree.root'),  # This is where the flat ROOT file will be stored
  closeFileFast = cms.untracked.bool(True)
)
# Path
process.p = cms.Path(process.jetTree)

import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('MyJetAnalysis',Phase2C17I13M9)

process.load("FWCore.MessageService.MessageLogger_cfi")

# Input source 
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(), #Crab:For T3_CH_CERNBOX setting
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_0_0/RelValTTbar_14TeV/MINIAODSIM/PU_141X_mcRun4_realistic_v3_STD_Run4D110_PU-v2/2580000/9c9737ed-bb08-48c5-9f58-ba4bb149c0d7.root'),
#    fileNames = cms.untracked.vstring('/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/GEN-SIM-DIGI-RAW-MINIAOD/NoPU_111X_mcRun4_realistic_T15_v1-v1/100000/1750775B-F6D7-AE4A-8097-AE0181CE03A9.root'),
    inputCommands = cms.untracked.vstring(
        'keep *',
        'drop l1tTkPrimaryVertexs_L1TkPrimaryVertex__RECO'
    )
#    fileNames = cms.untracked.vstring(
#        "root://cms-xrd-global.cern.ch//store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/"
#        "QCD_Pt_300to470_TuneCP5_14TeV_pythia8/GEN-SIM-DIGI-RAW-MINIAOD/"
#        "NoPU_111X_mcRun4_realistic_T15_v1-v1/100000/"
#        "E4BCEE62-D6E4-114B-88C3-E3FF47AAA099.root"),
#    redirectorHints = cms.untracked.vstring(
#        "root://cms-xrd-global.cern.ch/"
#    )    
)
#when switching between AOD and MINIAOD
#process.options.allowUnscheduled = cms.untracked.bool(True)

# Max events
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(10)  # process all events(-1)
)

# iJetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    #Jets input tag
    jetTag = cms.InputTag("slimmedJetsPuppi"),
    #GenJets input tag
    genJetsTag  = cms.InputTag("slimmedGenJets"),
    # New: PackedCandidate collection (PF candidates)
    pf_collection_source = cms.InputTag("packedPFCandidates"),
    #Primary vertex input tag
#    pvTag = cms.InputTag("offlinePrimaryVertices"),#RECO/AOD 
    pvTag = cms.InputTag('offlineSlimmedPrimaryVertices'),#MINIAOD
    #Beam spot input tag
    bsTag = cms.InputTag("offlineBeamSpot"),
    # Generated particles input tag (z-position)
#    genParticlesTag = cms.InputTag("genParticles:xyz0"),
    genParticlesTag = cms.InputTag("prunedGenParticles"),
    # Generated vertex input tag
    genVertexTag = cms.InputTag("generatorSmeared")
#    doAllPFParticles = cms.bool(True) 
)

    #Allow fallback to continue processing
#process.options = cms.untracked.PSet(
#    allowUnscheduled = cms.untracked.bool(True),
#    TryToContinue = cms.untracked.vstring('ProductNotFound')
#)

# Output configuration
process.TFileService = cms.Service("TFileService",
  fileName = cms.string('jetTree_PU200.root'),  # This is where the flat ROOT file will be stored
  closeFileFast = cms.untracked.bool(True)
)
# Path
process.p = cms.Path(process.jetTree)

import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('MyJetAnalysis',Phase2C17I13M9)

#For Thread change
process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(0), #For single tread(1), CMSSW decide the default(0)
    numberOfStreams = cms.untracked.uint32(0)  # usually 0 disables streams, making it single-threaded
)

process.load("FWCore.MessageService.MessageLogger_cfi")

# Input source 
process.source = cms.Source("PoolSource",
#    fileNames = cms.untracked.vstring(), #Crab:For T3_CH_CERNBOX setting
#    fileNames = cms.untracked.vstring('/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/GEN-SIM-DIGI-RAW-MINIAOD/NoPU_111X_mcRun4_realistic_T15_v1-v1/100000/1750775B-F6D7-AE4A-8097-AE0181CE03A9.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre3/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2590000/f20b63ca-ab47-46de-a883-b90cd5279b65.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre3/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2590000/c6e7417e-082b-4280-bcf9-ada01d57ddaa.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre3/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2590000/f20b63ca-ab47-46de-a883-b90cd5279b65.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2580000/9ffe3c74-fa93-4d0c-b9f6-9c9c314fe349.root'),
    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/33677d35-d0c0-49b1-b4bb-1f5955972b7d.root',
'/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/d36375f7-f900-43b6-9089-7262d79b2393.root'),
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
    input = cms.untracked.int32(-1)  # process all events(-1)
#    input = cms.untracked.int32(20)  # For test run
)

# JetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    #Jets input tag
    jetTag = cms.InputTag("slimmedJetsPuppi"),
    #GenJets input tag
    genJetsTag  = cms.InputTag("slimmedGenJets"),
    # New: PackedCandidate collection (PF candidates)
    pf_collection_source = cms.InputTag("packedPFCandidates"),
    #Primary vertex input tag
    pvTag = cms.InputTag("offlinePrimaryVertices"),#RECO/AOD 
#    pvTag = cms.InputTag('offlineSlimmedPrimaryVertices4D'),#MINIAOD, 4D
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
#Run single-threaded for debugging
process.options.numberOfThreads = 1
process.options.numberOfStreams = 1


# Output configuration
process.TFileService = cms.Service("TFileService",
#  fileName = cms.string('jetTree_test.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_noPU_PFincldNEUTRAL.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_200PU_PFincldNEUTRAL.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string("root://eosuser.cern.ch//eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR.root"),  # This is where the flat ROOT file will be stored
  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_200PU_PFincldNEUTRAL_dzCut3D_newHR.root'),  # This is where the flat ROOT file will be stored
  closeFileFast = cms.untracked.bool(True)
)
# Path
process.p = cms.Path(process.jetTree)

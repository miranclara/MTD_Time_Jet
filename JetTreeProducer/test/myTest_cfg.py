import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('MyJetAnalysis',Phase2C17I13M9)

#For Thread change
process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(1), #For single tread(1), CMSSW decide the default(0)
    numberOfStreams = cms.untracked.uint32(0),  # usually 0 disables streams, making it single-threaded
    TryToContinue = cms.untracked.vstring('ProductNotFound')
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
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2580000/9ffe3c74-fa93-4d0c-b9f6-9c9c314fe349.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/33677d35-d0c0-49b1-b4bb-1f5955972b7d.root',
#'/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/d36375f7-f900-43b6-9089-7262d79b2393.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2580000/037b9a50-5171-4002-a42a-7ceafb195ad3.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/c89f421c-bda5-4c4f-9165-b04450d5fb73.root',
#'/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/ca5e7041-9b8e-4481-b071-a39dfe145d52.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/c89f421c-bda5-4c4f-9165-b04450d5fb73.root',
#'/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/ca5e7041-9b8e-4481-b071-a39dfe145d52.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2580000/037b9a50-5171-4002-a42a-7ceafb195ad3.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/c89f421c-bda5-4c4f-9165-b04450d5fb73.root',
# '/store/relval/CMSSW_15_1_0_pre5/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/ca5e7041-9b8e-4481-b071-a39dfe145d52.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre6/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2590000/5f219794-333b-4cbf-8d69-4554aa0ecc13.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre6/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2590000/c83dbd0b-6a6d-4051-ab23-fa6e4d9f57b9.root',
# '/store/relval/CMSSW_15_1_0_pre6/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2590000/8babae90-9211-4ee3-a638-59eafa4b08ac.root'),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_15_1_0_pre6/RelValQCD_Pt15To7000_Flat_14/GEN-SIM-RECO/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2590000/c65c1fc9-5581-4888-aa85-1b1ff589ea84.root'),
    fileNames = cms.untracked.vstring('/store/relval/CMSSW_16_1_0_pre2/RelValQCD_Pt15To7000_Flat_14/GEN-SIM-RECO/150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/2590000/25b00bb4-c8d4-4c39-8324-8f7cb59b5447.root'),
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
#    input = cms.untracked.int32(-1)  # process all events(-1)
    input = cms.untracked.int32(100)  # For test run
)

# JetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    #Jets input tag
    jetTag = cms.InputTag("slimmedJetsPuppi"),#AK4 PUPPI jets.
#    jetTag = cms.InputTag("slimmedJets"),#AK4 PFchs jets (PF candidates with CHS pileup subtraction)
    #GenJets input tag
    genJetsTag  = cms.InputTag("slimmedGenJets"),#already hard-scatter only
    # New: PackedCandidate collection (PF candidates)
    pf_collection_source = cms.InputTag("packedPFCandidates"),
    #Primary vertex input tag
#    pvTag = cms.InputTag("offlineSlimmedPrimaryVertices"),#RECO/AOD 
    pvTag = cms.InputTag('offlineSlimmedPrimaryVertices4D'),#MINIAOD, 4D
    #Beam spot input tag
    bsTag = cms.InputTag("offlineBeamSpot"),
    # Generated particles input tag (z-position)
#    genParticlesTag = cms.InputTag("genParticles:xyz0"),
    genParticlesTag = cms.InputTag("packedGenParticles"),#prunedGenParticles
    # Generated vertex input tag
    genVertexTag = cms.InputTag("generator")
#    genVertexTag = cms.InputTag("generatorSmeared")
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


#Output configuration
process.TFileService = cms.Service("TFileService",
  fileName = cms.string('jetTree_QCD_200PU_test.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_200PU_PFincldNEUTRAL_dzCut3D_newHR_PVcut_packedGEN_PFmatch_GenMatch_dRgenAsso_NewVZ_timeJet_dZ1dZ2_dtSig3_NEWdRbis_dzdtDecoupled.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR_PVcut_packedGEN_PFmatch_GenMatch_dRgenAsso_NewVZ_timeJet_dZcutFirst.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_200PU_PFincldNEUTRAL_dzCut3D_newHR_PVcut_packedGEN_PFmatch_GenMatch_dRgenAsso_dRTune.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_noPU_PFincldNEUTRAL.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_200PU_PFincldNEUTRAL.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string("root://eosuser.cern.ch//eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR.root"),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_noPU_PFincldNEUTRAL_dzCut3D_newHR_PVcut_packedGEN_PFmatch.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('/eos/user/m/mrkim/JetTreeOutput/jetTree_QCD_200PU_PFincldNEUTRAL_dzCut3D_newHR_PVcut_packedGEN_PFmatch.root'),  # This is where the flat ROOT file will be stored
  closeFileFast = cms.untracked.bool(True)
)
# Path
process.p = cms.Path(process.jetTree)

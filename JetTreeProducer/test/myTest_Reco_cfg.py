import os
import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
#from FWCore.ParameterSet.Types import FileInPath

process = cms.Process('MyJetAnalysis',Phase2C17I13M9)
from FWCore.Utilities.FileUtils import loadListFromFile


#For Thread change
process.options = cms.untracked.PSet(
    numberOfThreads = cms.untracked.uint32(1), #For single tread(1), CMSSW decide the default(0)
    numberOfStreams = cms.untracked.uint32(0),  # usually 0 disables streams, making it single-threaded
    TryToContinue = cms.untracked.vstring('ProductNotFound')
)

process.load("FWCore.MessageService.MessageLogger_cfi")
process.load("SimTracker.TrackAssociation.trackingParticleRecoTrackAsssociation_cfi")

# Input source 
this_dir = os.path.dirname(os.path.abspath(__file__))

inputFileList = os.path.join(
    this_dir,
#    "MyAnalysis/JetTreeProducer/test/inputFiles_noPU.txt"
)

process.source = cms.Source(
    "PoolSource",
    skipBadFiles = cms.untracked.bool(True),
#    fileNames = cms.untracked.vstring('/store/relval/CMSSW_17_0_0_pre1/RelValQCD_Pt15To7000_Flat_14/GEN-SIM-RECO/PU_150X_mcRun4_realistic_v1_STD_D121_RegeneratedGS_PU-v1/2590000/97f5a31f-03c2-4c35-a2d2-d2578427df77.root'),
#  inputCommands = cms.untracked.vstring(
#          'keep *',
#          'drop l1tTkPrimaryVertexs_L1TkPrimaryVertex__RECO'
#      )

    fileNames = cms.untracked.vstring(
#        *loadListFromFile("MyAnalysis/JetTreeProducer/test/inputFiles_200PU.txt")
        *loadListFromFile("MyAnalysis/JetTreeProducer/test/inputFiles_noPU.txt")
#        *loadListFromFile("inputFiles_200PU.txt")
    )
)

#when switching between AOD and MINIAOD
#process.options.allowUnscheduled = cms.untracked.bool(True)

# Max events
process.maxEvents = cms.untracked.PSet(
#    input = cms.untracked.int32(-1)  # process all events(-1)
    input = cms.untracked.int32(20)  # For test run
)

# JetTreeProducer configuration
process.jetTree = cms.EDAnalyzer("JetTreeProducer_Reco",
    jets = cms.InputTag("ak4PFJets"),

    pfcands = cms.InputTag("particleFlow"),

    vertices = cms.InputTag("offlinePrimaryVertices"),

    trackTimeTag = cms.InputTag("tofPID","t0"),
    trackTimeErrTag = cms.InputTag("tofPID","sigmat0"),
    genJetsTag = cms.InputTag("ak4GenJetsNoNu")
    #Jets input tag
#    jetTag = cms.InputTag("slimmedJetsPuppi"),#AK4 PUPPI jets.
#    jetTag = cms.InputTag("slimmedJets"),#AK4 PFchs jets (PF candidates with CHS pileup subtraction)
    #GenJets input tag
#    genJetsTag  = cms.InputTag("slimmedGenJets"),#already hard-scatter only
    # New: PackedCandidate collection (PF candidates)
#    pf_collection_source = cms.InputTag("packedPFCandidates"),
    #Primary vertex input tag
#    pvTag = cms.InputTag("offlineSlimmedPrimaryVertices"),#RECO/AOD 
#    pvTag = cms.InputTag('offlineSlimmedPrimaryVertices4D'),#MINIAOD, 4D
    #Beam spot input tag
#    bsTag = cms.InputTag("offlineBeamSpot"),
    # Generated particles input tag (z-position)
#    genParticlesTag = cms.InputTag("genParticles:xyz0"),
#    genParticlesTag = cms.InputTag("packedGenParticles"),#prunedGenParticles
    # Generated vertex input tag
#    genVertexTag = cms.InputTag("generator")
#    genVertexTag = cms.InputTag("generatorSmeared")
#    doAllPFParticles = cms.bool(True) 
)#End of process.jetTree = cms.EDAnalyzer(

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
  fileName = cms.string('test.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_200PU_test.root'),  # This is where the flat ROOT file will be stored
#  fileName = cms.string('jetTree_QCD_noPU_test.root'),  # This is where the flat ROOT file will be stored
  closeFileFast = cms.untracked.bool(True)
)
# Path
process.p = cms.Path(process.jetTree)

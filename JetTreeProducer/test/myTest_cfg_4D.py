import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C17I13M9_cff import Phase2C17I13M9
process = cms.Process('MyJetAnalysis',Phase2C17I13M9)

process.load("FWCore.MessageService.MessageLogger_cfi")

# Input
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        '/store/relval/CMSSW_15_1_0_pre4/RelValQCD_Pt15To7000_Flat_14/MINIAODSIM/PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/2580000/33677d35-d0c0-49b1-b4bb-1f5955972b7d.root'
    ),
    inputCommands = cms.untracked.vstring('keep *')
)
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(100))

# Re-run Puppi with 4D PV
from CommonTools.PileupAlgos.Puppi_cff import puppi as _puppi
process.puppi4D = _puppi.clone(
    candName   = "packedPFCandidates",
    vertexName = "offlineSlimmedPrimaryVertices4D",
    useExistingWeights = False
)

# Recluster jets from puppi4D
from RecoJets.JetProducers.ak4PFJets_cfi import ak4PFJets
process.ak4PuppiJets4D = ak4PFJets.clone(
    src = "puppi4D",
    rParam = 0.4
)

# PATify jets (optional, but useful if your analyzer expects pat::Jet)
from PhysicsTools.PatAlgos.tools.jetTools import addJetCollection
addJetCollection(
    process,
    labelName = 'Puppi4D',
    jetSource = cms.InputTag('ak4PuppiJets4D'),
    pvSource  = cms.InputTag('offlineSlimmedPrimaryVertices4D'),
    pfCandidates = cms.InputTag('packedPFCandidates'),
    jetCorrections = ('AK4PFPuppi', ['L2Relative','L3Absolute'], 'None')
)

# Analyzer: use the *new* 4D jets instead of slimmedJetsPuppi
process.jetTree = cms.EDAnalyzer("JetTreeProducer",
    jetTag = cms.InputTag("selectedUpdatedPatJetsPuppi4D"),  # <-- NEW jets
    genJetsTag  = cms.InputTag("slimmedGenJets"),
    pf_collection_source = cms.InputTag("packedPFCandidates"),
    pvTag = cms.InputTag("offlineSlimmedPrimaryVertices4D"),
    bsTag = cms.InputTag("offlineBeamSpot"),
    genParticlesTag = cms.InputTag("prunedGenParticles"),
    genVertexTag = cms.InputTag("generatorSmeared")
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string("jetTree_QCD_200PU_PUPPI4D.root")
)

process.p = cms.Path(process.puppi4D + process.ak4PuppiJets4D + process.jetTree)



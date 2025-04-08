## import skeleton process
from PhysicsTools.PatAlgos.patTemplate_cfg import *
process.setName_("MYJET")
#process.Tracer = cms.Service("Tracer")

## load tau sequences up to selectedPatJets
process.load("PhysicsTools.PatAlgos.producersLayer1.jetProducer_cff")
patAlgosToolsTask.add(process.makePatJetsTask)

process.load("PhysicsTools.PatAlgos.selectionLayer1.jetSelector_cfi")
patAlgosToolsTask.add(process.selectedPatJets)

## make sure to keep the created objects
process.out.outputCommands = ['keep *_selectedPat*_*_*']

## ------------------------------------------------------
#  In addition you usually want to change the following
#  parameters:
## ------------------------------------------------------
#
#   process.GlobalTag.globaltag =  ...    ##  (according to https://twiki.cern.ch/twiki/bin/view/CMS/SWGuideFrontierConditions)
#                                         ##
from PhysicsTools.PatAlgos.patInputFiles_cff import filesRelValProdTTbarAODSIM
process.source.fileNames = filesRelValProdTTbarAODSIM
#process.source.fileNames = ['/store/relval/CMSSW_7_4_0_pre8/RelValProdTTbar_13/AODSIM/MCRUN2_74_V7-v1/00000/44E1E4BA-50BD-E411-A57A-002618943949.root']<span data-mce-mark="1"><span data-mce-mark="1"><span data-mce-mark="1">
process.source.fileNames = cms.untracked.vstring(
#    '/store/mc/Phase2HLTTDRSummer20ReRECOMiniAOD/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/GEN-SIM-DIGI-RAW-MINIAOD/NoPU_111X_mcRun4_realistic_T15_v1-v1/100000/A904237D-D806-8D4F-9BC7-2F38CB8EBD32.root'
#'/store/relval/CMSSW_15_0_0/RelValQCD_Pt20toInfMuEnrichPt15_14/MINIAODSIM/PU_141X_mcRun4_realistic_v3_STD_Run4D110_PU-v3/2580000/2f470ab7-6e82-44c8-bcd3-4aaf830246f1.root'
#'/store/mc/Run3Summer22EEMiniAODv4/ZZto4L-2Jets_QCD_TuneCP5_13p6TeV_madgraph-pythia8/MINIAODSIM/130X_mcRun3_2022_realistic_postEE_v6-v3/80000/3999c1cc-4b67-47cc-81b4-3010f5898ef9.root'
'/store/mc/Run3Summer22MiniAODv4/ZZto4L-2Jets_QCD_TuneCP5_13p6TeV_madgraph-pythia8/MINIAODSIM/130X_mcRun3_2022_realistic_v5-v3/80000/9c0cda7c-e4ed-4613-82b3-c31614a1e168.root'
    # or remote file:
    # 'root://xrootd-cms.infn.it//store/user/youruser/yourfile.root'
)
#                                         ##
process.maxEvents.input = 100
#                                         ##
#   process.out.outputCommands = [ ... ]  ##  (e.g. taken from PhysicsTools/PatAlgos/python/patEventContent_cff.py)
#                                         ##
process.out.fileName = 'patTuple_onlyJets.root'
#                                         ##
#   process.options.wantSummary = False   ##  (to suppress the long output at the end of the job)

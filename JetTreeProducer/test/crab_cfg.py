from CRABClient.UserUtilities import config
config = config()

config.General.requestName = 'JetTime_analysis'  
config.General.workArea = 'crab_projects'        # ✅ Where CRAB stores metadata
config.General.transferOutputs = True
config.General.transferLogs = True

config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'myTest_cfg.py'        # ✅ Should exist and work with `cmsRun`

# ⚠️ Dataset string is missing slashes — must start with '/' and have 3 parts
#config.Data.inputDataset = '/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/Phase2HLTTDRSummer20ReRECOMiniAOD-NoPU_111X_mcRun4_realistic_T15_v1-v1/GEN-SIM-DIGI-RAW-MINIAOD'
#config.Data.inputDataset = '/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/Phase2HLTTDRSummer20ReRECOMiniAOD-PU140_111X_mcRun4_realistic_T15_v1-v1/GEN-SIM-DIGI-RAW-MINIAOD'
#config.Data.inputDataset = '/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/Phase2HLTTDRSummer20ReRECOMiniAOD-PU200_111X_mcRun4_realistic_T15_v1-v1/GEN-SIM-DIGI-RAW-MINIAOD'
#config.Data.inputDataset = '/RelValQCD_Pt15To7000_Flat_14/CMSSW_15_1_0_pre3-PU_150X_mcRun4_realistic_v1_STD_Run4D110_PU-v1/MINIAODSIM'
config.Data.inputDataset = '/RelValQCD_Pt15To7000_Flat_14/CMSSW_15_1_0_pre3-150X_mcRun4_realistic_v1_STD_RegeneratedGS_Run4D110_noPU-v1/MINIAODSIM'

#config.Data.inputDataset = '/QCD_Pt_300to470_TuneCP5_14TeV_pythia8/*/GEN-SIM-DIGI-RAW-MINIAOD'

config.Data.inputDBS = 'global'
#config.Data.splitting = 'FileBased' #failed
config.Data.splitting = 'Automatic'
config.Data.unitsPerJob = 180

config.JobType.outputFiles = ['jetTree_QCD_noPU.root']

config.Data.outLFNDirBase = '/store/user/mrkim/CRAB/'#CRAB maps your LFN (like /store/user/mrkim/...) to CERNBox's actual EOS location using an internal catalog
#config.Data.outLFNDirBase = '/eos/user/m/mrkim/CRAB/'#Wrong for CERNBOX,Reason is: Incorrect 'Data.outLFNDirBase' parameter <class 'str'> /eos/user/m/mrkim/CRAB/,Error Id: b369f89030c091d72f93273b7f7f43ee

config.Data.publication = False
config.Site.storageSite = 'T3_CH_CERNBOX'


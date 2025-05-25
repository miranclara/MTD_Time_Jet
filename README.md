## How to Build and Run

```bash
cmsrel CMSSW_15_0_1
cd CMSSW_15_0_1/src/
git clone -b new-1501-rebuild git@github.com:miranclara/MTD_Time_Jet.git .
cd $CMSSW_BASE/src
cmsenv
scram b clean
source fix_pfparticle.sh
scram b -j 8
edmPluginRefresh
cmsRun MyAnalysis/JetTreeProducer/test/myTest_cfg.py


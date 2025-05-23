## How to Build and Run

```bash
cd $CMSSW_BASE/src
cmsenv
scram b clean
source fix_pfparticle.sh
scram b -j 8
edmPluginRefresh
cmsRun MyAnalysis/JetTreeProducer/test/myTest_cfg.py


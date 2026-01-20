## Installation Instructions


```bash
To clone this repository, you can use either SSH or HTTPS:
Make sure you have an SSH key set up and added to your GitHub account.
## 🛠️ Installation Instructions

To clone **this specific branch** (`1510_pre4`) and set up the environment:

### Step 1: Set up the CMSSW environment
cmsrel CMSSW_15_1_0_pre4
cd CMSSW_15_1_0_pre4/src
cmsenv

###Step 2: Clone this repository with custom branch
(Option A) Clone directly into the current directory:
git init
git remote add origin git@github.com:miranclara/MTD_Time_Jet.git
git fetch origin 1510_pre4
git checkout -b 1510_pre4 origin/1510_pre4

(Option B) Clone into a temporary folder, then move it:
git clone --branch 1510_pre4 git@github.com:miranclara/MTD_Time_Jet.git temp_clone
mv temp_clone/* .
mv temp_clone/.??* .
rmdir temp_clone

###Step 3: Build and Run
scram b clean
scram b -j 8
cmsRun MyAnalysis/JetTreeProducer/test/myTest_cfg.py

###Step : Crab submit
voms-proxy-init --voms cms
crab submit -c MyAnalysis/JetTreeProducer/test/crab_cfg.py
# Monitor:
crab status -d MyAnalysis/JetTreeProducer/test/crab_projects/crab_JetTime_analysis
# Stream live logs:
crab tail -d MyAnalysis/JetTreeProducer/test/crab_projects/crab_JetTime_analysis
# Get outputs when done:
crab getoutput -d MyAnalysis/JetTreeProducer/test/crab_projects/crab_JetTime_analysis

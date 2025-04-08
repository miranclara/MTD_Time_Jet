## Installation Instructions

### Step 1: Set up the CMSSW environment

```bash
cmsrel CMSSW_15_0_0
cd CMSSW_15_0_0/src
cmsenv

###Step 2: Clone the repository into a temporary folder
git clone git@github.com:miranclara/MTD_Time_Jet.git temp_clone

###Step 3: Move its contents into the current src/ directory
mv temp_clone/* .
mv temp_clone/.??* .
rmdir temp_clone

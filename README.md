## Installation Instructions


```bash
To clone this repository, you can use either SSH or HTTPS:
Make sure you have an SSH key set up and added to your GitHub account.

### Step 1: Set up the CMSSW environment
cmsrel CMSSW_15_0_0
cd CMSSW_15_0_0/src
cmsenv

###Step 2: Clone the repository into current directory(.)
git clone git@github.com:miranclara/MTD_Time_Jet.git .

###Step 2-1: Clone the repository into a temporary folder
git clone git@github.com:miranclara/MTD_Time_Jet.git temp_clone

###Step 3-1: Mve its contents into the current src/ directory(Only when use Step 2-1)
mv temp_clone/* .
mv temp_clone/.??* .
rmdir temp_clone

# MTD_Time_Jet
cmsrel CMSSW_15_0_0
cd CMSSW_15_0_0/src
cmsenv

# Clone the repository into a temporary folder
git clone git@github.com:miranclara/MTD_Time_Jet.git temp_clone

# Move its contents into the current src/ directory
mv temp_clone/* .
mv temp_clone/.??* .
rmdir temp_clone

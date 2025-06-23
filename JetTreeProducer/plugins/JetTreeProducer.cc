#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/JetReco/interface/GenJet.h" 
#include "DataFormats/Math/interface/deltaR.h"  
#include "fastjet/ClusterSequence.hh"
#include "fastjet/PseudoJet.hh"
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/JetReco/interface/JetCollection.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/BeamSpot/interface/BeamSpot.h"
//#include "DataFormats/Math/interface/XYZPointF.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
//#include "DataFormats/HepMCCandidate/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"

#include "TTree.h"
#include "TFile.h"
//#include "TFileService.h"

#include <memory>

struct PrimaryVertex {
    float x;
    float y;
    float z;
    float t;
    float chi2;
    float ndof;
    int nTracks;
};


class JetTreeProducer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit JetTreeProducer(const edm::ParameterSet&);
  ~JetTreeProducer() override = default;
  //static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  
  
private:
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;
  
  edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJetToken_;

  // Add tokens for packedCandidate, primary vertex, beam spot, and generateid particles
  edm::EDGetTokenT<std::vector<pat::PackedCandidate>> pf_collection_token;
  edm::EDGetTokenT<std::vector<reco::Vertex>> pvsToken_;
  edm::EDGetTokenT<reco::BeamSpot> bsToken_;
  //edm::EDGetTokenT<math::XYZPointF> genpToken_;
  edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
  edm::EDGetTokenT<edm::HepMCProduct> genvertexToken_;
  
  // --  Output tree
//  bool doAllPFParticles;
//  bool isAssigned_tight = (std::abs(dz) < 0.1 && dzsig < 5);
//  bool isAssigned_loose = (std::abs(dz) < 0.2 && dzsig < 5);


  TTree* tree_;
  std::vector<float> jetPt_, genPt_;
  std::vector<float> jetResponse_;
  std::vector<float> jetAbsEta_;
//  float jetResponse_ULv15;
  std::vector<float> jetPt_pflaw_;
  std::vector<float> genPt_pflaw_;
  std::vector<float> jetPt_tight_;
  std::vector<float> genPt_tight_;
  std::vector<float> jetPt_loose_;
  std::vector<float> genPt_loose_;
  std::vector<float> jetResponse_PR_pflaw_;
  std::vector<float> jetResponse_PR_tight_;
  std::vector<float> jetResponse_PR_loose_;
  std::vector<float> jetAbsEta_pflaw_;
  std::vector<float> jetAbsEta_tight_;
  std::vector<float> jetAbsEta_loose_;
  std::vector<float> jetPt_MTD_, jetAbsEta_MTD_, jetTime_MTD_, jetTimeError_MTD_;//For MTD-based jet clustering
  std::vector<float> pf_vertex,pf_pt, pf_eta, pf_phi, pf_energy,pf_charge,pf_puppiWeight,pf_puppiWeightNoLep;//For <pat::PackedCandidate> collection
std::vector<float> pf_dxy, pf_dz, pf_dzError, pf_dzSig, pf_time, pf_timeError, pf_dtSig;//For <pat::PackedCandidate> collection
  std::vector<float> genparticles_z_;
//  float jetResponse_4D;
//  std::vector<PrimaryVertex> primaryVertices;

//std::vector<float> jet_pt;
  float pt_, eta_, phi_, mass_;//For jet collection
//  std::vector<PFParticle> pfparticles;//For PackedCandidate
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_; // For primary vertex position
  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
//  float genparticles_z_; // For generated particles z-position
  float genvertex_z_;
//  float pf_vx, pf_vy, pf_vz;//For <pat::PackedCandidate> collection
//  int pf_pdgId,pf_isTimeValid;//For <pat::PackedCandidate> collection
  std::vector<float> pf_vx, pf_vy, pf_vz;
  std::vector<int> pf_pdgId, pf_fromPV;
  float efficiency_tight_, purity_tight_,efficiency_loose_, purity_loose_;

};

JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig)//:
//	: doAllPFParticles(iConfig.getParameter<bool>("doAllPFParticles"))
{
  jetsToken_ = consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jetTag"));
  genJetToken_  = consumes<std::vector<reco::GenJet>>(iConfig.getParameter<edm::InputTag>("genJetsTag"));
  // Initialize new tokens for packedCandidate primary vertex, beam spot, and generated particles
  pf_collection_token = consumes<std::vector<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("pf_collection_source"));
  pvsToken_ = consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("pvTag"));
  bsToken_ = consumes<reco::BeamSpot>(edm::InputTag("offlineBeamSpot"));
//  genpToken_ = consumes<math::XYZPointF>(iConfig.getParameter<edm::InputTag>("genParticlesTag"));
  genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(iConfig.getParameter<edm::InputTag>("genParticlesTag"));
  genvertexToken_ = consumes<edm::HepMCProduct>(edm::InputTag("generatorSmeared"));

}

void JetTreeProducer::beginJob() {

  edm::Service<TFileService> fs_;
  tree_ = fs_->make<TTree>("JetTree", "JetTree");

  tree_->Branch("jetPt", &jetPt_);
  tree_->Branch("genPt", &genPt_);
  tree_->Branch("jetResponse", &jetResponse_);
  tree_->Branch("jetAbsEta", &jetAbsEta_);  
  tree_->Branch("jetResponse_PR_tight", &jetResponse_PR_pflaw_);
  tree_->Branch("jetResponse_PR_tight", &jetResponse_PR_tight_);
  tree_->Branch("jetResponse_PR_loose", &jetResponse_PR_loose_);
  tree_->Branch("jetAbsEta_tight", &jetAbsEta_pflaw_);
  tree_->Branch("jetAbsEta_tight", &jetAbsEta_tight_);
  tree_->Branch("jetAbsEta_loose", &jetAbsEta_loose_);
  tree_->Branch("jetPt_tight", &jetPt_pflaw_);
  tree_->Branch("genPt_tight", &genPt_pflaw_);
  tree_->Branch("jetPt_tight", &jetPt_tight_);
  tree_->Branch("genPt_tight", &genPt_tight_);
  tree_->Branch("jetPt_loose", &jetPt_loose_);
  tree_->Branch("genPt_loose", &genPt_loose_);

  tree_->Branch("jetPt_MTD", &jetPt_MTD_);
  tree_->Branch("jetAbsEta_MTD", &jetAbsEta_MTD_);
  tree_->Branch("jetTime_MTD", &jetTime_MTD_);
  tree_->Branch("jetTimeError_MTD", &jetTimeError_MTD_);

//  tree_->Branch("primaryVertices", &primaryVertices);


   // Branches for jet kinematics 
  tree_->Branch("pt", &pt_, "pt/F");
  tree_->Branch("eta", &eta_, "eta/F");
  tree_->Branch("phi", &phi_, "phi/F");
  tree_->Branch("mass", &mass_, "mass/F");

  // Branches for packedCandidate
//  tree_->Branch("PFParticles", &pfparticles);

  // Branches for primary vertex
  tree_->Branch("pvs_x", &pvs_x_, "pvs_x/F");
  tree_->Branch("pvs_y", &pvs_y_, "pvs_y/F");
  tree_->Branch("pvs_z", &pvs_z_, "pvs_z/F");
  tree_->Branch("pvs_t", &pvs_t_, "pvs_t/F");
  // Branches for beam spot
  tree_->Branch("beamspot_x", &beamspot_x_, "beamspot_x/F");
  tree_->Branch("beamspot_y", &beamspot_y_, "beamspot_y/F");
  tree_->Branch("beamspot_z", &beamspot_z_, "beamspot_z/F");
  // Branches for generated particles
  tree_->Branch("genparticles_z", &genparticles_z_,"genparticles_z/F");
  tree_->Branch("genvertex_z", &genvertex_z_, "genvertex_z/F");

   // Branches for <pat::PackedCandidate> collection 
  tree_->Branch("pf_pt", &pf_pt);
  tree_->Branch("pf_eta", &pf_eta);
  tree_->Branch("pf_phi", &pf_phi);
  tree_->Branch("pf_energy", &pf_energy);
  tree_->Branch("pf_charge", &pf_charge);
  tree_->Branch("pf_puppiWeight", &pf_puppiWeight);
  tree_->Branch("pf_puppiWeightNoLep", &pf_puppiWeightNoLep);
  tree_->Branch("pf_dz", &pf_dz);
  tree_->Branch("pf_dzError", &pf_dzError);
  tree_->Branch("pf_dzSig", &pf_dzSig);
  tree_->Branch("pf_time", &pf_time);
  tree_->Branch("pf_timeError", &pf_timeError);
  tree_->Branch("pf_dtSig", &pf_dtSig);
  tree_->Branch("pf_pdgId", &pf_pdgId);
  tree_->Branch("pf_vx", &pf_vx);
  tree_->Branch("pf_vy", &pf_vy);
  tree_->Branch("pf_vz", &pf_vz);
  tree_->Branch("pf_fromPV", &pf_fromPV);
  tree_->Branch("efficiency_tight", &efficiency_tight_);
  tree_->Branch("purity_tight", &purity_tight_);
  tree_->Branch("efficiency_loose", &efficiency_loose_);
  tree_->Branch("purity_loose", &purity_loose_);
}

void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
//  pfparticles.clear();
  pf_pt.clear();
  pf_eta.clear();
  pf_phi.clear();
  pf_energy.clear();
  pf_charge.clear();
  pf_puppiWeight.clear();
  pf_puppiWeightNoLep.clear();
  pf_dxy.clear();
  pf_dz.clear();
  pf_dzError.clear();
  pf_dzSig.clear();
  pf_time.clear();
  pf_timeError.clear();
  pf_dtSig.clear();
  pf_vx.clear();
  pf_vy.clear();
  pf_vz.clear();
  pf_pdgId.clear();
  pf_fromPV.clear();
  
  jetPt_.clear();
  genparticles_z_.clear();
  jetResponse_.clear();
  jetAbsEta_.clear(); 
  jetPt_pflaw_.clear();
  jetPt_tight_.clear();
  genPt_tight_.clear();
  jetPt_loose_.clear();
  genPt_loose_.clear();
  jetResponse_PR_pflaw_.clear();
  jetResponse_PR_tight_.clear();
  jetResponse_PR_loose_.clear();
  jetAbsEta_pflaw_.clear();
  jetAbsEta_tight_.clear();
  jetAbsEta_loose_.clear();

  jetPt_MTD_.clear();
  jetAbsEta_MTD_.clear();
  jetTime_MTD_.clear();
  jetTimeError_MTD_.clear();

  int N_HS_total = 0;
//  int N_selected = 0;
  int N_selected_tight = 0;
  int N_selected_HS_tight = 0;
  int N_selected_loose = 0;
  int N_selected_HS_loose = 0;


// Retrieve jet collection
  edm::Handle<std::vector<pat::Jet>> jets;
  iEvent.getByToken(jetsToken_, jets);
  // Retrieve Genjet collection
  edm::Handle<std::vector<reco::GenJet>> genJets;
  iEvent.getByToken(genJetToken_, genJets);

  // Retrieve primary vertex collection
  std::vector<reco::Vertex> reco_pvs;
  edm::Handle<std::vector<reco::Vertex>> pv_handle;
  iEvent.getByToken(pvsToken_, pv_handle); 
  if (pv_handle.isValid()) {
    reco_pvs = *pv_handle;//reco_pvs is a direct copy of all the primary vertices in this event
  }

  // Retrieve gen vertex
  edm::Handle<edm::HepMCProduct> genvertex;
  iEvent.getByToken(genvertexToken_, genvertex);
  bool hasGenZ = false;
  if (genvertex.isValid()) {
    const HepMC::GenEvent* evt = genvertex->GetEvent();
    if (evt && evt->vertices_size() > 0) {
      const HepMC::GenVertex* firstVertex = *(evt->vertices_begin());
      genvertex_z_ = firstVertex->position().z(); //position() returns a 4-vector (x, y, z, t)
      hasGenZ = true;
    }
  }


  // ⛔ Early event rejection if gen vertex is valid and reco_pvs(primary vertices) is not empty
  // =Only accept the event if the first reco PV is literally the one closest to the gen vertex.
  if (hasGenZ && !reco_pvs.empty()) {
    double dz_first = std::abs(reco_pvs[0].z() - genvertex_z_);
    double minDist = dz_first;
    int closestPVIndex = 0;

    for (size_t i = 1; i < reco_pvs.size(); ++i) {
      double dz = std::abs(reco_pvs[i].z() - genvertex_z_);
      if (dz < minDist) {
        minDist = dz;
        closestPVIndex = static_cast<int>(i);
      }
    }//End of for (size_t i = 1; i < reco_pvs.size()

    if (closestPVIndex != 0) {
      edm::LogInfo("JetTreeProducer") << "Skipping event: first reco PV is not closest to gen vertex";
      return; // Early skip
    }
  }//End of if (hasGenZ && !reco_pvs.empty())

  // Retrieve PackedCandidates
  edm::Handle<std::vector<pat::PackedCandidate>> pfColl_handle;
  iEvent.getByToken(pf_collection_token, pfColl_handle);
  const auto& pf_coll = *(pfColl_handle.product());

  // Retrieve beam spot
  edm::Handle<reco::BeamSpot> beamspot;
  iEvent.getByToken(bsToken_, beamspot);

  // Retrieve gen particles (optional)
  edm::Handle<std::vector<reco::GenParticle>> genp;
  iEvent.getByToken(genParticlesToken_, genp);

  // Fill jets (limit to leading 5 jets)
  int jetIndex = 0;
  for (const auto& jet : *jets) {

    const reco::GenJet* matchedGenJet = nullptr;
    float minDR = 0.3;
    for (const auto& genJet : *genJets) {
       float dR = reco::deltaR(jet, genJet);
       if (dR < minDR) {
         minDR = dR;
         matchedGenJet = &genJet;
      }//End of if (dR < minDR) 
    }//End of for (const auto& genJet : *genJets)
 
    if (matchedGenJet) {
      pt_ = jet.pt();
      eta_ = jet.eta();
      phi_ = jet.phi();
      mass_ = jet.mass();
//      jetResponse_ = (matchedGenJet->pt() > 0) ? jet.pt() / matchedGenJet->pt() : -1.0;
//      jetAbsEta_   = std::abs(jet.eta());
      jetResponse_.push_back(jet.pt() / matchedGenJet->pt());
      jetAbsEta_.push_back(std::abs(jet.eta()));
      jetPt_.push_back(jet.pt());
      genPt_.push_back(matchedGenJet->pt());

  //    tree_->Fill();  // fill the tree with this matched jet
    }//End of if (matchedGenJet)

    jetIndex++;
    if (jetIndex >= 5) break;// For the leading 5 jets
  }//End of for (const auto& jet : *jets)


  // Fill PackedCandidates
  std::vector<fastjet::PseudoJet> fjInputs_raw;
  std::vector<fastjet::PseudoJet> fjInputs_tight;
  std::vector<fastjet::PseudoJet> fjInputs_loose;
  std::vector<fastjet::PseudoJet> fjInputs_MTD;
  std::vector<const pat::PackedCandidate*> pf_for_MTD;
//  primaryVertices.clear();

  for (const auto& pf : pf_coll) {
    pf_pt.push_back(pf.pt());
    pf_eta.push_back(pf.eta());
    pf_phi.push_back(pf.phi());
    pf_energy.push_back(pf.energy());
    pf_charge.push_back(pf.charge());
    pf_puppiWeight.push_back(pf.puppiWeight());
    pf_puppiWeightNoLep.push_back(pf.puppiWeightNoLep());
// vertex is always available, but dz/dzError are not
    pf_vx.push_back(pf.vertex().x());
    pf_vy.push_back(pf.vertex().y());
    pf_vz.push_back(pf.vertex().z());
    pf_pdgId.push_back(pf.pdgId());
    pf_fromPV.push_back(pf.fromPV());

    if (pf.hasTrackDetails()) {
      bool isHS = (pf.fromPV() > 1);  // fromPV: 3 = tightly associated to PV
      if (isHS) ++N_HS_total;

      float dz     = pf.dz();
      float dzErr  = pf.dzError();
      float dzSig  = (dzErr > 0) ? dz / dzErr : 999;

      float t = pf.time();
      float tErr = pf.timeError();
      float dt = t - pvs_t_;
      float dtSig = (tErr > 0) ? dt / tErr : 999;


      pf_dxy.push_back(pf.dxy());
      pf_dz.push_back(dz);
      pf_dzError.push_back(dzErr);
      pf_dzSig.push_back(dzSig);  
      pf_time.push_back(pf.time());
      pf_timeError.push_back(pf.timeError());
      pf_dtSig.push_back(dtSig);

      // Common PseudoJet for dz-based clustering
      fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
      pj.set_user_index(0);//this one used for tight/loose only, Not used for MTD
   
      //dz:mean -3.1 X 10(-6), sigma:0.010cm
      //dzSig: mean -2.1X 10 (-6), sigma: 0.073
      fjInputs_raw.push_back(pj);//no cut (for control)
      if (std::abs(dz) < 0.03 && dzSig < 0.2) {
        fjInputs_tight.push_back(pj);
        ++N_selected_tight;
        if (isHS) ++N_selected_HS_tight;
      }
      if (std::abs(dz) < 0.05 && dzSig < 0.5) {
        fjInputs_loose.push_back(pj);
        ++N_selected_loose;
        if (isHS) ++N_selected_HS_loose;
      }
        // MTD-based selection
//      if (pf.isTimeValid() && pf.timeError() < 0.05) {//pat::PackedCandidate does not have a method called .isTimeValid()
      if (pf.timeError() > 0 && pf.timeError() < 0.05) {
        fastjet::PseudoJet pj_mtd(pf.px(), pf.py(), pf.pz(), pf.energy());
        pj_mtd.set_user_index(pf_for_MTD.size());//index back to PackedCandidate
        fjInputs_MTD.push_back(pj_mtd);
        pf_for_MTD.push_back(&pf);
      }

    } else {
      // Fill with defaults to avoid division by zero, preserve structure
      pf_dxy.push_back(0);
      pf_dz.push_back(0);
      pf_dzError.push_back(1e6);  // avoid division by zero
      pf_dzSig.push_back(0);
      pf_time.push_back(0);
      pf_timeError.push_back(1e6);
      pf_dtSig.push_back(1e6);
    } 

  }//End of for (const auto& pf : pf_coll)

float efficiency_tight = (N_HS_total > 0) ? float(N_selected_HS_tight) / N_HS_total : 0;
float purity_tight     = (N_selected_tight > 0) ? float(N_selected_HS_tight) / N_selected_tight : 0;
float efficiency_loose = (N_HS_total > 0) ? float(N_selected_HS_loose) / N_HS_total : 0;
float purity_loose     = (N_selected_loose > 0) ? float(N_selected_HS_loose) / N_selected_loose : 0;

edm::LogVerbatim("JetTreeProducer") 
  << "Efficiency_tight = " << efficiency_tight<< ", Purity_tight = " << purity_tight
  << " (with cuts: dzSig < 0.03, dtSig < 0.2)";

edm::LogVerbatim("JetTreeProducer") 
  << "Efficiency_loose = " << efficiency_loose<< ", Purity_loose = " << purity_loose
  << " (with cuts: dzSig < 0.05, dtSig < 0.5)";

efficiency_tight_ = efficiency_tight;
purity_tight_ = purity_tight;
efficiency_loose_ = efficiency_loose;
purity_loose_ = purity_loose;

// Run the jet clustering algorithm on each collection
  fastjet::JetDefinition jetDef(fastjet::antikt_algorithm, 0.4);
  auto cs_raw = fastjet::ClusterSequence(fjInputs_raw, jetDef);
  auto pfrawJets = fastjet::sorted_by_pt(cs_raw.inclusive_jets(20.0));
  
  auto cs_tight = fastjet::ClusterSequence(fjInputs_tight, jetDef);
  auto tightJets = fastjet::sorted_by_pt(cs_tight.inclusive_jets(20.0));

  auto cs_loose = fastjet::ClusterSequence(fjInputs_loose, jetDef);
  auto looseJets = fastjet::sorted_by_pt(cs_loose.inclusive_jets(20.0));

  auto cs_mtd = fastjet::ClusterSequence(fjInputs_MTD, jetDef);
  auto mtdJets = fastjet::sorted_by_pt(cs_mtd.inclusive_jets(20.0));


//  jetResponse_PR_tight_ = -1;
//  jetResponse_PR_loose_ = -1;
//  jetAbsEta_ = -1; 

//GenJet matching logic1  
  auto computeResponse = [&](const fastjet::PseudoJet& recoJet) -> float {
    const reco::GenJet* matchedGenJet = nullptr;
    float minDR = 0.3;
    for (const auto& genJet : *genJets) {
      float dR = reco::deltaR(recoJet.eta(), recoJet.phi(), genJet.eta(), genJet.phi());
      if (dR < minDR) {
        minDR = dR;
        matchedGenJet = &genJet;
      }
    }
    if (matchedGenJet && matchedGenJet->pt() > 0){
      return recoJet.pt() / matchedGenJet->pt();
      
    }
      return -1;
  };

  // no cut (for control)
  for (const auto& jet : pfrawJets) {
    float response = computeResponse(jet);
    jetResponse_PR_pflaw_.push_back(response);
    jetAbsEta_pflaw_.push_back(std::abs(jet.eta()));
    jetPt_pflaw_.push_back(jet.pt());

    //GenJet matching logic2: get matching GenJet pT
    const reco::GenJet* matchedGenJet = nullptr;
    float minDR = 0.3;
    for (const auto& genJet : *genJets) {
      float dR = reco::deltaR(jet.eta(), jet.phi(), genJet.eta(), genJet.phi());
      if (dR < minDR) {
        minDR = dR;
        matchedGenJet = &genJet;
      }
    }
    genPt_pflaw_.push_back((matchedGenJet) ? matchedGenJet->pt() : -1);

    if (jetResponse_PR_tight_.size() >= 5) break;
  }//for (const auto& jet : pfrawJets)

  for (const auto& jet : tightJets) {
    float response = computeResponse(jet);
    jetResponse_PR_tight_.push_back(response);
    jetAbsEta_tight_.push_back(std::abs(jet.eta()));
    jetPt_tight_.push_back(jet.pt());

    //GenJet matching logic2: get matching GenJet pT
    const reco::GenJet* matchedGenJet = nullptr;
    float minDR = 0.3;
    for (const auto& genJet : *genJets) {
      float dR = reco::deltaR(jet.eta(), jet.phi(), genJet.eta(), genJet.phi());
      if (dR < minDR) {
        minDR = dR;
        matchedGenJet = &genJet;
      }
    }
    genPt_tight_.push_back((matchedGenJet) ? matchedGenJet->pt() : -1);

    if (jetResponse_PR_tight_.size() >= 5) break;
  }//for (const auto& jet : tightJets)


  for (const auto& jet : looseJets) {
    float response = computeResponse(jet);
    jetResponse_PR_loose_.push_back(response);
    jetAbsEta_loose_.push_back(std::abs(jet.eta()));
    jetPt_loose_.push_back(jet.pt());

    //GenJet matching logic2:  get matching GenJet pT
    const reco::GenJet* matchedGenJet = nullptr;
    float minDR = 0.3;
    for (const auto& genJet : *genJets) {
      float dR = reco::deltaR(jet.eta(), jet.phi(), genJet.eta(), genJet.phi());
      if (dR < minDR) {
        minDR = dR;
        matchedGenJet = &genJet;
      }
    }
    genPt_loose_.push_back((matchedGenJet) ? matchedGenJet->pt() : -1);

    if (jetResponse_PR_loose_.size() >= 5) break;
  }//End of for (const auto& jet : looseJets)

  //Compute per-jet timing (pT-weighted average)
  for (const auto& jet : mtdJets) {
    float sumPt = 0.0, sumTime = 0.0, sumTime2 = 0.0;

    for (const auto& idx : jet.constituents()) {
      int userIdx = idx.user_index();
      if (userIdx >= 0 && userIdx < static_cast<int>(pf_for_MTD.size())) {
        const auto* pf = pf_for_MTD[userIdx];
        float weight = pf->pt();
        sumPt += weight;
        sumTime += weight * pf->time();
        sumTime2 += weight * pf->time() * pf->time();
      }
    }
    float avgTime = (sumPt > 0) ? sumTime / sumPt : -999;
    float rmsTime = (sumPt > 0) ? std::sqrt(sumTime2 / sumPt - avgTime * avgTime) : -1;

    jetPt_MTD_.push_back(jet.pt());
    jetAbsEta_MTD_.push_back(std::abs(jet.eta()));
    jetTime_MTD_.push_back(avgTime);
    jetTimeError_MTD_.push_back(rmsTime);

    if (jetPt_MTD_.size() >= 5) break;
  }//End of for (const auto& jet : mtdJets)

  //Store all primary vertices in a vector
    std::vector<PrimaryVertex> primaryVertices;
    for (const auto& vtx:reco_pvs) {
        PrimaryVertex pv;
        pv.x = vtx.x();
        pv.y = vtx.y();
        pv.z = vtx.z();
        pv.t = vtx.t();
        pv.chi2 = vtx.chi2();
        pv.ndof = vtx.ndof();
        pv.nTracks = vtx.nTracks();
        primaryVertices.push_back(pv);
   }

  // Fill first primary vertex variables
  if (!reco_pvs.empty()) {
    const reco::Vertex& firstPV = reco_pvs[0];

    pvs_x_ = firstPV.x();
    pvs_y_ = firstPV.y();
    pvs_z_ = firstPV.z();
    pvs_t_ = firstPV.t();
  //  tree_->Fill();
  }

  // Fill beam spot information
  if (beamspot.isValid()) {
    beamspot_x_ = beamspot->x0();
    beamspot_y_ = beamspot->y0();
    beamspot_z_ = beamspot->z0();
  //  tree_->Fill();
  }

  // Fill generator particle z-positions
  if (genp.isValid()) {
    for (const auto& particle : *genp) {
      genparticles_z_.push_back(particle.vz());
    }
  }


  // Fill generator vertex z-position (already done above, but now write it to tree)
  if (hasGenZ) {
  //  tree_->Fill();
  }
tree_->Fill();
}//End of void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {


void JetTreeProducer::endJob() {
  //file_->cd();
  //  tree_->Write();
  //  file_->Close();
}


//void JetTreeProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
//  edm::ParameterSetDescription desc;
//  descriptions.add("JetTreeProducer", desc);
//  desc.add<edm::InputTag>("jetTag", edm::InputTag("slimmedJetsPuppi"));
//}

//define this as a plug-in
DEFINE_FWK_MODULE(JetTreeProducer);

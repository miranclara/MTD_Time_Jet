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
  std::vector<float> jetPt_tight_;
  std::vector<float> genPt_tight_;
  std::vector<float> jetPt_loose_;
  std::vector<float> genPt_loose_;
  std::vector<float> jetResponse_PR_tight_;
  std::vector<float> jetResponse_PR_loose_;
  std::vector<float> jetAbsEta_tight_;
  std::vector<float> jetAbsEta_loose_;

//  float jetResponse_4D;
//  std::vector<PrimaryVertex> primaryVertices;

//std::vector<float> jet_pt;
  float pt_, eta_, phi_, mass_;//For jet collection
//  std::vector<PFParticle> pfparticles;//For PackedCandidate
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_; // For primary vertex position
  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
//  float genparticles_z_; // For generated particles z-position
  std::vector<float> genparticles_z_;
  float genvertex_z_;
  float pf_vertex,pf_pt, pf_eta, pf_phi, pf_energy,pf_charge,pf_puppiWeight,pf_puppiWeightNoLep,pf_dxy,pf_dz,pf_dzError,pf_dzSig,pf_time,pf_timeError;//For <pat::PackedCandidate> collection
  float pf_vx, pf_vy, pf_vz;//For <pat::PackedCandidate> collection
  int pf_pdgId,pf_isTimeValid;//For <pat::PackedCandidate> collection

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
  tree_->Branch("jetResponse_PR_tight", &jetResponse_PR_tight_);
  tree_->Branch("jetResponse_PR_loose", &jetResponse_PR_loose_);
  tree_->Branch("jetAbsEta_tight", &jetAbsEta_tight_);
  tree_->Branch("jetAbsEta_loose", &jetAbsEta_loose_);
  tree_->Branch("jetPt_tight", &jetPt_tight_);
  tree_->Branch("genPt_tight", &genPt_tight_);
  tree_->Branch("jetPt_loose", &jetPt_loose_);
  tree_->Branch("genPt_loose", &genPt_loose_);
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
  tree_->Branch("pf_pt", &pf_pt, "pf_pt/F");
  tree_->Branch("pf_eta", &pf_eta, "pf_eta/F");
  tree_->Branch("pf_phi", &pf_phi, "pf_phi/F");
  tree_->Branch("pf_energy", &pf_energy, "pf_energy/F");
  tree_->Branch("pf_charge", &pf_charge, "pf_charge/F");
  tree_->Branch("pf_puppiWeight", &pf_puppiWeight, "pf_puppiWeight/F");
  tree_->Branch("pf_puppiWeightNoLep", &pf_puppiWeightNoLep, "pf_puppiWeightNoLep/F");
  tree_->Branch("pf_dz", &pf_dz, "pf_dz/F");
  tree_->Branch("pf_dzError", &pf_dzError, "pf_dzError/F");
  tree_->Branch("pf_dzSig", &pf_dzSig, "pf_dzSig/F");
  tree_->Branch("pf_time", &pf_time, "pf_time/F");
  tree_->Branch("pf_pdgId", &pf_pdgId, "pf_pdgId/I");
  tree_->Branch("pf_vx", &pf_vx, "pf_vx/F");
  tree_->Branch("pf_vy", &pf_vy, "pf_vy/F");
  tree_->Branch("pf_vz", &pf_vz, "pf_vz/F");
}

void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
//  pfparticles.clear();
  genparticles_z_.clear();
  jetResponse_.clear();
  jetAbsEta_.clear(); 
  jetPt_tight_.clear();
  genPt_tight_.clear();
  jetPt_loose_.clear();
  genPt_loose_.clear();
  jetResponse_PR_tight_.clear();
  jetResponse_PR_loose_.clear();
  jetAbsEta_tight_.clear();
  jetAbsEta_loose_.clear();

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
  std::vector<fastjet::PseudoJet> fjInputs_tight;
  std::vector<fastjet::PseudoJet> fjInputs_loose;
//  primaryVertices.clear();

  for (const auto& pf : pf_coll) {
    pf_pt = pf.pt();
    pf_eta = pf.eta();
    pf_phi = pf.phi();
    pf_energy = pf.energy();
    pf_charge = pf.charge();
    pf_puppiWeight = pf.puppiWeight();
    pf_puppiWeightNoLep = pf.puppiWeightNoLep();
// vertex is always available, but dz/dzError are not
    pf_vx = pf.vertex().x();
    pf_vy = pf.vertex().y();
    pf_vz = pf.vertex().z();

    if (pf.hasTrackDetails()) {
      pf_dxy=pf.dxy();
      pf_dz=pf.dz();
      pf_dzError=pf.dzError();
      pf_dzSig = (pf_dzError > 0) ? pf_dz / pf_dzError : 0;  
//      pf_dzSig = pf.dz() / pf.dzError();
      pf_time = pf.time();
      pf_timeError = pf.timeError();

      fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
      pj.set_user_index(0);

      if (std::abs(pf_dz) < 0.1 && pf_dzSig < 5) {
      fjInputs_tight.push_back(pj);
      }
      if (std::abs(pf_dz) < 0.2 && pf_dzSig < 5) {
      fjInputs_loose.push_back(pj);
      }

    } else {
      pf_dxy=0;
      pf_dz = 0;
      pf_dzError = 1e6;  // avoid division by zero
      pf_dzSig = 0;
      pf_time = 0;
      pf_timeError = 1e6;
    }

    pf_pdgId = pf.pdgId();
//    tree_->Fill();
  }//End of for (const auto& pf : pf_coll)

// Run the jet clustering algorithm on each collection
  fastjet::JetDefinition jetDef(fastjet::antikt_algorithm, 0.4);
  auto cs_tight = fastjet::ClusterSequence(fjInputs_tight, jetDef);
  auto tightJets = fastjet::sorted_by_pt(cs_tight.inclusive_jets(20.0));

  auto cs_loose = fastjet::ClusterSequence(fjInputs_loose, jetDef);
  auto looseJets = fastjet::sorted_by_pt(cs_loose.inclusive_jets(20.0));

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

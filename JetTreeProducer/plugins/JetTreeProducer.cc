#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/PatCandidates/interface/Jet.h"

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

struct PFParticle {
        float pt, eta, phi, energy, charge, puppiWeight, puppiWeightNoLep, dzSig, time;
        enum EParticleID { eX, eH, eE, eMu, eGamma, eH0, eH_HF, eEgamma_HF } particleID;
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

  // Add tokens for packedCandidate, primary vertex, beam spot, and generateid particles
  edm::EDGetTokenT<std::vector<pat::PackedCandidate>> pf_collection_token;
  edm::EDGetTokenT<std::vector<reco::Vertex>> pvsToken_;
  edm::EDGetTokenT<reco::BeamSpot> bsToken_;
  //edm::EDGetTokenT<math::XYZPointF> genpToken_;
  edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesToken_;
  edm::EDGetTokenT<edm::HepMCProduct> genvertexToken_;
  
  // --  Output tree
  edm::Service<TFileService> fs_;
  bool doAllPFParticles;

  TTree* tree_;
  
//std::vector<float> jet_pt;
  float pt_, eta_, phi_, mass_;//For jet collection
  std::vector<PFParticle> pfparticles;//For PackedCandidate
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_; // For primary vertex position
  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
//  float genparticles_z_; // For generated particles z-position
  std::vector<float> genparticles_z_;
  float genvertex_z_;


};

JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig)//:
	: doAllPFParticles(iConfig.getParameter<bool>("doAllPFParticles"))
{
  jetsToken_ = consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jetTag"));

  // Initialize new tokens for packedCandidate primary vertex, beam spot, and generated particles
  pf_collection_token = consumes<std::vector<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("pf_collection_source"));
  pvsToken_ = consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("pvTag"));
  bsToken_ = consumes<reco::BeamSpot>(edm::InputTag("offlineBeamSpot"));
//  genpToken_ = consumes<math::XYZPointF>(iConfig.getParameter<edm::InputTag>("genParticlesTag"));
  genParticlesToken_ = consumes<std::vector<reco::GenParticle>>(iConfig.getParameter<edm::InputTag>("genParticlesTag"));
  genvertexToken_ = consumes<edm::HepMCProduct>(edm::InputTag("generatorSmeared"));

}

void JetTreeProducer::beginJob() {
  
  tree_ = fs_->make<TTree>("JetTree", "JetTree");
   // Branches for jet kinematics 
  tree_->Branch("pt", &pt_, "pt/F");
  tree_->Branch("eta", &eta_, "eta/F");
  tree_->Branch("phi", &phi_, "phi/F");
  tree_->Branch("mass", &mass_, "mass/F");

  // Branches for packedCandidate
  tree_->Branch("PFParticles", &pfparticles);

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
  tree_->Branch("genparticles_z", &genparticles_z_);
  tree_->Branch("genvertex_z", &genvertex_z_, "genvertex_z/F");
}

void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
  pfparticles.clear();
  genparticles_z_.clear();
  edm::Handle<std::vector<pat::Jet>> jets;
  iEvent.getByToken(jetsToken_, jets);

  // Retrieve  PackedCandidates,primary vertices, beam spot, and generated particles
 
  std::vector<reco::Vertex> reco_pvs;
  edm::Handle<std::vector<reco::Vertex>> pv_handle;
  iEvent.getByToken(pvsToken_, pv_handle); 
  if (pv_handle.isValid()) {
    reco_pvs = *pv_handle;
  }
  edm::Handle<reco::BeamSpot> beamspot;
  iEvent.getByToken(bsToken_, beamspot);

//  edm::Handle<math::XYZPointF> genp;
//  iEvent.getByToken(genpToken_, genp);
   edm::Handle<std::vector<reco::GenParticle>> genp;
   iEvent.getByToken(genParticlesToken_, genp);

  edm::Handle<edm::HepMCProduct> genvertex;
  iEvent.getByToken(genvertexToken_, genvertex);

 
  int jetIndex = 0;
  for (const auto& jet : *jets) {
    pt_ = jet.pt();
    eta_ = jet.eta();
    phi_ = jet.phi();
    mass_ = jet.mass();
    tree_->Fill();
    jetIndex++;
 	
   	if (jetIndex >= 5) break;  // limit to first 5 jets
        std::cout << "==== pat::Jet ====" << std::endl;
        std::cout << "  pt: " << jet.pt() << std::endl;
        std::cout << "  eta: " << jet.eta() << std::endl;
        std::cout << "  phi: " << jet.phi() << std::endl;
        std::cout << "  mass: " << jet.mass() << std::endl;
        std::cout << "  energy: " << jet.energy() << std::endl;
        std::cout << "  jet area: " << jet.jetArea() << std::endl;
        std::cout << "  charged hadron energy fraction: " << jet.chargedHadronEnergyFraction() << std::endl;
        std::cout << "  neutral hadron energy fraction: " << jet.neutralHadronEnergyFraction() << std::endl;
        std::cout << "  photon energy fraction: " << jet.photonEnergyFraction() << std::endl;
        std::cout << "  electron energy fraction: " << jet.electronEnergyFraction() << std::endl;

        std::cout << "  Number of constituents: " << jet.numberOfDaughters() << std::endl;
        std::cout << "  Charged multiplicity: " << jet.chargedMultiplicity() << std::endl;

        // B-tagging discriminators (may vary depending on data tier)
        if (jet.bDiscriminator("pfDeepCSVDiscriminatorsJetTags:probb"))
            std::cout << "  DeepCSV probb: " << jet.bDiscriminator("pfDeepCSVDiscriminatorsJetTags:probb") << std::endl;
	if (jet.hasUserFloat("DeepCSV_b")) {
        	float score = jet.userFloat("DeepCSV_b");
            std::cout << "  DeepCSV_b = " << score << std::endl;
    	}

        if (jet.bDiscriminator("pfDeepFlavourJetTags:probb"))
            std::cout << "  DeepFlavour probb: " << jet.bDiscriminator("pfDeepFlavourJetTags:probb") << std::endl;
        // Check for any userFloats (common in MiniAOD)
        std::vector<std::string> userFloatNames = jet.userFloatNames();
        for (const auto& name : userFloatNames) {
            std::cout << "  userFloat(" << name << ") = " << jet.userFloat(name) << std::endl;
        }
        std::vector<std::string> userIntNames = jet.userIntNames();
        for (const auto& name : userIntNames) {
            std::cout << "  userInt(" << name << ") = " << jet.userInt(name) << std::endl;
        }
        std::cout << std::endl;

  }//End of for (const auto& jet : *jets)

  //Storee PackedCandidates
    if (doAllPFParticles) {
        edm::Handle<std::vector<pat::PackedCandidate>> pfColl_handle;
        iEvent.getByToken(pf_collection_token, pfColl_handle);

        const auto& pf_coll = *(pfColl_handle.product());

        for (const auto& pf : pf_coll) {
            PFParticle part;
            part.pt = pf.pt();
            part.eta = pf.eta();
            part.phi = pf.phi();
            part.energy = pf.energy();
            part.charge = pf.charge();
            part.puppiWeight = pf.puppiWeight();
            part.puppiWeightNoLep = pf.puppiWeightNoLep();

            if (pf.hasTrackDetails()) {
                part.dzSig = pf.dz() / pf.dzError();
                part.time = pf.time();
            } else {
                part.dzSig = 0;
                part.time = 0;
            }

            reco::PFCandidate reco_pf;
            switch (reco_pf.translatePdgIdToType(pf.pdgId())) {
                case reco::PFCandidate::X: part.particleID = PFParticle::eX; break;
                case reco::PFCandidate::h: part.particleID = PFParticle::eH; break;
                case reco::PFCandidate::e: part.particleID = PFParticle::eE; break;
                case reco::PFCandidate::mu: part.particleID = PFParticle::eMu; break;
                case reco::PFCandidate::gamma: part.particleID = PFParticle::eGamma; break;
                case reco::PFCandidate::h0: part.particleID = PFParticle::eH0; break;
                case reco::PFCandidate::h_HF: part.particleID = PFParticle::eH_HF; break;
                case reco::PFCandidate::egamma_HF: part.particleID = PFParticle::eEgamma_HF; break;
                default: part.particleID = PFParticle::eX; break;
            }

            pfparticles.push_back(part);
        }
    }
  
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
  //If available, store the first vertex into the flat tree variables
  if (!reco_pvs.empty()) {
          const reco::Vertex& firstPV = reco_pvs[0];
          pvs_x_ = firstPV.x();
          pvs_y_ = firstPV.y();
          pvs_z_ = firstPV.z();
          pvs_t_ = firstPV.t();
    
  }
  // Fill beam spot information
  if (beamspot.isValid()) {
    beamspot_x_ = beamspot->x0();
    beamspot_y_ = beamspot->y0();
    beamspot_z_ = beamspot->z0();
  }

  // Fill generated particle z-position
  if ( genp.isValid()) {
    for (const auto& particle : *genp) {
        genparticles_z_.push_back(particle.vz());  // vz() gives the z vertex position
    }//    genparticles_z_ = genp->z();
  }

  // Fill generated vertex z-position
  if (genvertex.isValid()) {
     const HepMC::GenEvent* evt = genvertex->GetEvent();
    if (evt && evt->vertices_size() > 0) {
        const HepMC::GenVertex* firstVertex = *(evt->vertices_begin());
        genvertex_z_ = firstVertex->position().z();
    }
  }

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

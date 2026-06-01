#include <memory>
#include <vector>
#include <iostream>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertex.h"

//#include "SimTracker/TrackAssociation/interface/TrackAssociatorBase.h"
//#include "SimTracker/TrackAssociation/interface/TrackAssociatorByHits.h"
#include "SimDataFormats/Associations/interface/TrackAssociation.h"
#include "SimDataFormats/Associations/interface/TrackToTrackingParticleAssociator.h"
#include "fastjet/PseudoJet.hh"
#include "fastjet/JetDefinition.hh"
#include "fastjet/ClusterSequence.hh"

#include "TTree.h"


class JetTreeProducer_Reco : public edm::one::EDAnalyzer<edm::one::SharedResources>
{
public:
    explicit JetTreeProducer_Reco(const edm::ParameterSet&);
    ~JetTreeProducer_Reco() override {};

private:
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void beginJob() override;


    // =========================================
    // TOKENS
    // =========================================

    edm::EDGetTokenT<reco::PFJetCollection> jetToken_;

    edm::EDGetTokenT<reco::PFCandidateCollection> pfToken_;

    edm::EDGetTokenT<reco::VertexCollection> vertexToken_;

    edm::EDGetTokenT<TrackingParticleCollection> trackingParticleToken_;

    edm::EDGetTokenT<reco::RecoToSimCollection> recoToSimToken_;

    edm::EDGetTokenT<edm::ValueMap<float>> trackTimeToken_;

    edm::EDGetTokenT<edm::ValueMap<float>> trackTimeErrToken_;

    // =========================================
    // TREE
    // =========================================

    TTree* tree_;


    // =========================================
    // EVENT VARIABLES
    // =========================================

    int nPF_;

    std::vector<float> pf_pt;
    std::vector<float> pf_eta;
    std::vector<float> pf_phi;
    std::vector<int>   pf_charge;

    std::vector<int> pf_isHS_truth;
    std::vector<int> pf_isPU_truth;

    std::vector<float> pf_truth_vz;

    std::vector<float> pf_dz;
    std::vector<float> pf_dzError;
    std::vector<float> pf_dzSig;
    std::vector<float> pf_time;
    std::vector<float> pf_timeError;
    std::vector<float> pf_timeSig;
    std::vector<float> pf_dt;
    std::vector<float> pf_dtError;
    std::vector<float> pf_dtSig;
    std::vector<int> pf_keepAlways;//was bool
    std::vector<int> pf_keepDisplaced;//was bool
    std::vector<int> pf_hasTimeCompatibleWithPV;//New add
    std::vector<int> pf_isInMTD;//was bool
    std::vector<int> pf_isCharged;//was bool
    std::vector<int> pf_hasValidTime;//was bool
    std::vector<int> pf_isHS; // 1 = HS, 0 = PU, -1 = unmatched

};


// ============================================================
// CONSTRUCTOR
// ============================================================

JetTreeProducer_Reco::JetTreeProducer_Reco(const edm::ParameterSet& iConfig)
    :

    jetToken_(consumes<reco::PFJetCollection>(
        iConfig.getParameter<edm::InputTag>("jets"))),

    pfToken_(consumes<reco::PFCandidateCollection>(
        iConfig.getParameter<edm::InputTag>("pfcands"))),

    vertexToken_(consumes<reco::VertexCollection>(
        iConfig.getParameter<edm::InputTag>("vertices"))),

    trackingParticleToken_(consumes<TrackingParticleCollection>(
        edm::InputTag("mix", "MergedTrackTruth"))),

    recoToSimToken_(consumes<reco::RecoToSimCollection>(
    edm::InputTag("trackingParticleRecoTrackAsssociation"))),

    trackTimeToken_(consumes<edm::ValueMap<float>>(
        iConfig.getParameter<edm::InputTag>("trackTimeTag"))),

    trackTimeErrToken_(consumes<edm::ValueMap<float>>(
        iConfig.getParameter<edm::InputTag>("trackTimeErrTag")))
{
    usesResource("TFileService");
}


// ============================================================
// beginJob()
// ============================================================

void JetTreeProducer_Reco::beginJob()
{
    edm::Service<TFileService> fs;

    tree_ = fs->make<TTree>("JetTree", "JetTree");

    tree_->Branch("nPF", &nPF_);

    tree_->Branch("pf_pt", &pf_pt);
    tree_->Branch("pf_eta", &pf_eta);
    tree_->Branch("pf_phi", &pf_phi);
    tree_->Branch("pf_charge", &pf_charge);

    tree_->Branch("pf_isHS_truth", &pf_isHS_truth);
    tree_->Branch("pf_isPU_truth", &pf_isPU_truth);

    tree_->Branch("pf_truth_vz", &pf_truth_vz);

    tree_->Branch("pf_dz", &pf_dz);
    tree_->Branch("pf_dzError", &pf_dzError);
    tree_->Branch("pf_dzSig", &pf_dzSig);
    tree_->Branch("pf_time", &pf_time);
    tree_->Branch("pf_timeError", &pf_timeError);
    tree_->Branch("pf_timeSig", &pf_timeSig);
    tree_->Branch("pf_dt", &pf_dt);
    tree_->Branch("pf_dtError", &pf_dtError);
    tree_->Branch("pf_dtSig", &pf_dtSig);
    tree_->Branch("pf_isInMTD", &pf_isInMTD);
    tree_->Branch("pf_isCharged", &pf_isCharged);
}


// ============================================================
// analyze()
// ============================================================

void JetTreeProducer_Reco::analyze(const edm::Event& iEvent,
                                   const edm::EventSetup& iSetup)
{

    // =========================================
    // CLEAR VECTORS
    // =========================================

    pf_pt.clear();
    pf_eta.clear();
    pf_phi.clear();
    pf_charge.clear();

    pf_isHS_truth.clear();
    pf_isPU_truth.clear();

    pf_truth_vz.clear();

    pf_dz.clear();
    pf_dzError.clear();
    pf_dzSig.clear();
    pf_time.clear();
    pf_timeError.clear();
    pf_timeSig.clear();
    pf_dt.clear();
    pf_dtError.clear();
    pf_dtSig.clear();
    pf_isInMTD.clear();
    pf_isCharged.clear();


    // =========================================
    // HANDLES
    // =========================================

    edm::Handle<reco::PFJetCollection> jets;
    iEvent.getByToken(jetToken_, jets);

    edm::Handle<reco::PFCandidateCollection> pfcands;
    iEvent.getByToken(pfToken_, pfcands);

    edm::Handle<reco::VertexCollection> vertices;
    iEvent.getByToken(vertexToken_, vertices);

    edm::Handle<TrackingParticleCollection> trackingParticles;
    iEvent.getByToken(trackingParticleToken_, trackingParticles);

    edm::Handle<reco::RecoToSimCollection> recoToSimHandle;
    iEvent.getByToken(recoToSimToken_, recoToSimHandle);

    edm::Handle<edm::ValueMap<float>> trackTimeMap;
    iEvent.getByToken(trackTimeToken_, trackTimeMap);

    edm::Handle<edm::ValueMap<float>> trackTimeErrMap;
    iEvent.getByToken(trackTimeErrToken_, trackTimeErrMap);

   // Fill PFCandidateCollection
    std::vector<fastjet::PseudoJet> fjInputs_raw;
    std::vector<fastjet::PseudoJet> fjInputs_fromPV3;
    std::vector<fastjet::PseudoJet> fjInputs_tight;
    std::vector<fastjet::PseudoJet> fjInputs_loose;
    std::vector<fastjet::PseudoJet> fjInputs_time;
    std::vector<fastjet::PseudoJet> fjInputs_time4D;
    std::vector<const reco::PFCandidateCollection*> pf_for_time;
    std::vector<const reco::PFCandidateCollection*> pf_for_allCollections;
    pf_for_allCollections.reserve(pfcands->size());
    pf_for_time.reserve(pfcands->size());
 
    fjInputs_raw.clear();
    fjInputs_fromPV3.clear();
    fjInputs_tight.clear();
    fjInputs_loose.clear();
    fjInputs_time.clear();
    fjInputs_time4D.clear();
    pf_for_time.clear();

    // =========================================
    // PRIMARY VERTEX
    // =========================================

    reco::Vertex pv;

    if (!vertices->empty()) {
        pv = vertices->at(0);
    }


    // =========================================
    // LOOP OVER PF CANDIDATES
    // =========================================
    int pfcands_index = 0;
    for (size_t iPF = 0; iPF < pfcands->size(); ++iPF)
    {
        const reco::PFCandidate& pf = pfcands->at(iPF);
        float eta=pf.eta();
        bool isInMTD = (std::abs(eta) <= 3.0);
        bool isCharged=(pf.charge() !=0);
        bool isNeutral=(pf.charge()==0);

        // =========================================
        // BASIC PF INFO
        // =========================================

        pf_pt.push_back(pf.pt());
        pf_eta.push_back(pf.eta());
        pf_phi.push_back(pf.phi());
        pf_charge.push_back(pf.charge());

        // =========================================
        // DEFAULT TRUTH VALUES
        // =========================================

        int isHS = -1;
        int isPU = -1;

        float truth_vz = -999.f;
        float dz = -999.f;
        float dzErr= -999.f;
        float dzSig= -999.f;


        // =========================================
        // ONLY CHARGED PF HAS TRACKS
        // =========================================
        if (!isInMTD) continue;
        fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
        pj.set_user_index(static_cast<int>(iPF)); //KEY of global intexing,Index matching
        fjInputs_raw.push_back(pj);//keep global-indexed PF for raw jets, Index matching
        if (isCharged&&pf.trackRef().isAvailable())
        {
            reco::TrackRef trkRef = pf.trackRef();

            bool hasValidTime = false;
            bool pvHasValidTime=false;
            float t      = -999.f;
            float tErr   = -999.f;
            float tSig   = -999.f;

            float dt     = 999.f;
            float dtErr  = 999.f;
            float dtSig  = 999.f;
                
            float pv_t = pv.t();
//            float pv_t = 0.f;
            float pvs_TimeErr_ = 0.f;


            if (trkRef.isNonnull())
            {

                // =========================================
                // dz wrt PV
                // = reconstructed track displacement wrt reco PV
                // =========================================
                //extrapolated track z-position at beamline, small |dz| → likely from primary interaction (HS)
                dz = trkRef->dz(pv.position());//dz=z_track−zPV,Reco-level displacement wrt PV
                 dzErr  = std::sqrt(
                trkRef->dzError() * trkRef->dzError() +
                pv.zError() * pv.zError()
                );

                dzSig  = (dzErr > 0.f) ? dz / dzErr : 999.f;
                bool isDisplaced = (std::abs(dz) > 0.05f);
                 
                // =========================================
                // dt wrt PV
                // = reconstructed trackparticle time  wrt reco PV
                // =========================================
                t    = (*trackTimeMap)[trkRef];//t_track
                tErr = (*trackTimeErrMap)[trkRef];

                if (tErr > 0.f && std::isfinite(t))
                {
                    tSig = t / tErr;

                    hasValidTime =
                    (
                        isInMTD &&
                        tErr > 0.f &&
                        tErr < 0.1f
                    );
   
                    pvHasValidTime =  (
                          std::isfinite(pvs_TimeErr_) &&
                          pvs_TimeErr_ < 1e6
                          );//// choose a sensible threshold of pvs_t=real value.

                    if (hasValidTime && pvHasValidTime)
                    {
                        dt = t - pv_t;
                        dtErr = std::sqrt(
                            tErr * tErr +
                            pvs_TimeErr_ * pvs_TimeErr_
                        );
                        dtSig = (dtErr > 0.f) ? dt / dtErr : 999.f;
//                        std::cout
//                             << "t = "      << t
//                             << "  pv_t = " << pv_t
//                             << "  dt = "   << dt
//                             << "  dtErr = "<< dtErr
//                             << "  dtSig = "<< dtSig
//                             << std::endl;

                    }//End of (hasValidTime && pvHasValidTime)
                }//End of if (tErr > 0.f && std::isfinite(t))   

                // =========================================
                // BUILD TrackRefVector
                // =========================================

                reco::TrackRefVector trkVec;
                trkVec.push_back(trkRef);


                // =========================================
                // RECO → SIM ASSOCIATION
                // pv.z(): reconstructed primary vertex z
                // dzPV:reco track displacement wrt PV
                // truth_vz:true MC production vertex z
                // =========================================
                if (recoToSimHandle.isValid())
                {
                    //A TrackingParticle is a GEANT-level truth object representing a simulated charged particle.
                    edm::RefToBase<reco::Track> trackBaseRef(trkRef);
                    auto found = recoToSimHandle->find(trackBaseRef);//Which simulated TrackingParticle produced this reconstructed track?
                    if (found != recoToSimHandle->end())
                    {
                        const auto& tpVec = found->val;

                        if (!tpVec.empty())
                        {
                            TrackingParticleRef tpRef =
                                tpVec.begin()->first;

                            if (!tpRef->parentVertex().isNull())
                            {
                                TrackingVertexRef tvRef =
                                    tpRef->parentVertex();

                                truth_vz = tvRef->position().z();//Actual MC truth production z of the prticle,This stores the true simulated production z-vertex.
                            }//End of if (!tpRef->parentVertex().isNull())

                            if (tpRef->eventId().event() == 0)
                            {
                                isHS = 1;
                                isPU = 0;
                            }//End of if (tpRef->eventId().event() == 0)
                            else
                            {
                                isHS = 0;
                                isPU = 1;
                            }//End of else
                        }//End of if (!tpVec.empty())
                    }//End of if (found != recoToSimHandle->end())
                }//End of if (recoToSimHandle.isValid())


        // =========================================
        // STORE RESULTS
        // =========================================

        pf_isHS_truth.push_back(isHS);
        pf_isPU_truth.push_back(isPU);

        pf_truth_vz.push_back(truth_vz);

        pf_dz.push_back(dz);
        pf_dzError.push_back(dzErr);
        pf_dzSig.push_back(dzSig);
        pf_time.push_back(t);
        pf_timeError.push_back(tErr);
        pf_timeSig.push_back(tSig);
        pf_dt.push_back(dt);
        pf_dtError.push_back(dtErr);
        pf_dtSig.push_back(dtSig);
      }//End of if (trkRef.isNonnull())
    }//End of if (pf.charge() != 0)
  }//End of for (size_t i = 0; i < pfcands->size(); ++i)

    // =========================================
    // FINAL COUNTER
    // =========================================

    nPF_ = pf_pt.size();


    // =========================================
    // FILL TREE
    // =========================================

    tree_->Fill();
}//End of void JetTreeProducer_Reco::analyze()

// ============================================================
// DEFINE MODULE
// ============================================================

DEFINE_FWK_MODULE(JetTreeProducer_Reco);

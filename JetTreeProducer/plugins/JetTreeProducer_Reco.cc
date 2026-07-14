#include <memory>
#include <typeinfo>
#include <vector>
#include <iostream>
#include <algorithm>//for sort
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
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

// ======================================================
// Helper 0: Extract PF indices from reco::PFJet
// ======================================================
inline std::vector<unsigned int> getPFIndicesFromPFJet(const reco::PFJet& jet) {
    std::vector<unsigned int> pf_indices_this_jet;
    pf_indices_this_jet.reserve(jet.getJetConstituents().size());
    //pf_indices_this_jet.reserve(jet.numberOfDaughters());

    for (const auto& candPtr : jet.getJetConstituents()) {
//        std::cout
//        << "Jet constituents = "
//        << jet.getJetConstituents().size()
//        << std::endl;

       // Skip invalid constituent references.
       if (candPtr.isNull()) continue;
       if (!candPtr.isAvailable()) continue;

        // key() is the index into the event-level reco::PFCandidate collection.
       pf_indices_this_jet.push_back(candPtr.key());

//      std::cout
//        << "key = " << candPtr.key()
//        << std::endl;
    }        

    return pf_indices_this_jet;
}


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

    edm::EDGetTokenT<std::vector<reco::GenJet>> genJetToken_;//Add genJet Token

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
    std::vector<int> pf_timingRegion;//int
    std::vector<int> pf_isCharged;//was bool
    std::vector<int> pf_hasValidTime;//was bool
    std::vector<int> pf_isHS; // 1 = HS, 0 = PU, -1 = unmatched


    std::vector<float> jetDeltaR_puppi_all_;
    std::vector<int> jetMatched_puppi_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_puppi_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_puppi_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_puppi_all_;
    std::vector<int> jetIsPU_puppi_all_;
    std::vector<int> jetIsAmbiguous_puppi_all_;
    std::vector<int> jetIsUnknown_puppi_all_;
    std::vector<float> jetPt_puppi_all_;
    std::vector<float> jetAbsEta_puppi_all_;
    std::vector<float> jetPhi_puppi_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_puppi_all_;
    std::vector<float> genPt_puppi_all_;
    std::vector<float> genEta_puppi_all_;
    std::vector<float> genEtaDen_puppi_all_;
    std::vector<float> jetResponse_PR_puppi_all_;
    std::vector<float> puFracPt_algo_puppi_all_;
    std::vector<float> puFracCount_algo_puppi_all_;
    std::vector<float> puFracPt_truth_puppi_all_;
    std::vector<float> puFracCount_truth_puppi_all_;

    std::vector<std::vector<unsigned int>> pf_indices_puppi_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_puppi_;
    int totalRecoJetsClean_puppi;
    int totalPUJets_puppi;
    int totalGenJets_puppi;
    int nGenMatched_puppi;
    int nMatchedReco_puppi;

    float effLossAmbig_puppi_;
    float effWithAmbig_puppi_;//defined but not used
    float puJetFraction_puppi_all_;
    float efficiency_puppi_;
    float mistag_puppi_;
    float purity_puppi_;

    std::vector<float> jetDeltaR_pfraw_all_;
    std::vector<int> jetMatched_pfraw_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_pfraw_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_pfraw_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_pfraw_all_;
    std::vector<int> jetIsPU_pfraw_all_;
    std::vector<int> jetIsAmbiguous_pfraw_all_;
    std::vector<int> jetIsUnknown_pfraw_all_;
    std::vector<float> jetPt_pfraw_all_;
    std::vector<float> jetAbsEta_pfraw_all_;
    std::vector<float> jetPhi_pfraw_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_pfraw_all_;
    std::vector<float> genPt_pfraw_all_;
    std::vector<float> genEta_pfraw_all_;
    std::vector<float> genEtaDen_pfraw_all_;
    std::vector<float> jetResponse_PR_pfraw_all_;
    std::vector<float> puFracPt_algo_pfraw_all_;
    std::vector<float> puFracCount_algo_pfraw_all_;
    std::vector<float> puFracPt_truth_pfraw_all_;
    std::vector<float> puFracCount_truth_pfraw_all_;

    std::vector<std::vector<unsigned int>> pf_indices_pfraw_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_pfraw_;
    int totalRecoJetsClean_pfraw;
    int totalPUJets_pfraw;
    int totalGenJets_pfraw;
    int nGenMatched_pfraw;
    int nMatchedReco_pfraw;

    float effLossAmbig_pfraw_;
    float effWithAmbig_pfraw_;//defined but not used
    float puJetFraction_pfraw_all_;
    float efficiency_pfraw_;
    float mistag_pfraw_;
    float purity_pfraw_;

    
    std::vector<float> jetDeltaR_loose_all_;
    std::vector<int> jetMatched_loose_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_loose_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_loose_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_loose_all_;
    std::vector<int> jetIsPU_loose_all_;
    std::vector<int> jetIsAmbiguous_loose_all_;
    std::vector<int> jetIsUnknown_loose_all_;
    std::vector<float> jetPt_loose_all_;
    std::vector<float> jetAbsEta_loose_all_;
    std::vector<float> jetPhi_loose_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_loose_all_;
    std::vector<float> genPt_loose_all_;
    std::vector<float> genEta_loose_all_;
    std::vector<float> genEtaDen_loose_all_;
    std::vector<float> jetResponse_PR_loose_all_;
    std::vector<float> puFracPt_algo_loose_all_;
    std::vector<float> puFracCount_algo_loose_all_;
    std::vector<float> puFracPt_truth_loose_all_;
    std::vector<float> puFracCount_truth_loose_all_;

    std::vector<std::vector<unsigned int>> pf_indices_loose_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_loose_;
    int totalRecoJetsClean_loose;
    int totalPUJets_loose;
    int totalGenJets_loose;
    int nGenMatched_loose;
    int nMatchedReco_loose;

    float effLossAmbig_loose_;
    float effWithAmbig_loose_;//defined but not used
    float puJetFraction_loose_all_;
    float efficiency_loose_;
    float mistag_loose_;
    float purity_loose_;


    std::vector<float> jetDeltaR_tight_all_;
    std::vector<int> jetMatched_tight_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_tight_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_tight_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_tight_all_;
    std::vector<int> jetIsPU_tight_all_;
    std::vector<int> jetIsAmbiguous_tight_all_;
    std::vector<int> jetIsUnknown_tight_all_;
    std::vector<float> jetPt_tight_all_;
    std::vector<float> jetAbsEta_tight_all_;
    std::vector<float> jetPhi_tight_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_tight_all_;
    std::vector<float> genPt_tight_all_;
    std::vector<float> genEta_tight_all_;
    std::vector<float> genEtaDen_tight_all_;
    std::vector<float> jetResponse_PR_tight_all_;
    std::vector<float> puFracPt_algo_tight_all_;
    std::vector<float> puFracCount_algo_tight_all_;
    std::vector<float> puFracPt_truth_tight_all_;
    std::vector<float> puFracCount_truth_tight_all_;

    std::vector<std::vector<unsigned int>> pf_indices_tight_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_tight_;
    int totalRecoJetsClean_tight;
    int totalPUJets_tight;
    int totalGenJets_tight;
    int nGenMatched_tight;
    int nMatchedReco_tight;

    float effLossAmbig_tight_;
    float effWithAmbig_tight_;//defined but not used
    float puJetFraction_tight_all_;
    float efficiency_tight_;
    float mistag_tight_;
    float purity_tight_;


    std::vector<float> jetDeltaR_time_all_;
    std::vector<int> jetMatched_time_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_time_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_time_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_time_all_;
    std::vector<int> jetIsPU_time_all_;
    std::vector<int> jetIsAmbiguous_time_all_;
    std::vector<int> jetIsUnknown_time_all_;
    std::vector<float> jetPt_time_all_;
    std::vector<float> jetAbsEta_time_all_;
    std::vector<float> jetPhi_time_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_time_all_;
    std::vector<float> genPt_time_all_;
    std::vector<float> genEta_time_all_;
    std::vector<float> genEtaDen_time_all_;
    std::vector<float> jetResponse_PR_time_all_;
    std::vector<float> puFracPt_algo_time_all_;
    std::vector<float> puFracCount_algo_time_all_;
    std::vector<float> puFracPt_truth_time_all_;
    std::vector<float> puFracCount_truth_time_all_;

    std::vector<std::vector<unsigned int>> pf_indices_time_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_time_;
    int totalRecoJetsClean_time;
    int totalPUJets_time;
    int totalGenJets_time;
    int nGenMatched_time;
    int nMatchedReco_time;

    float effLossAmbig_time_;
    float effWithAmbig_time_;//defined but not used
    float puJetFraction_time_all_;
    float efficiency_time_;
    float mistag_time_;
    float purity_time_;


    std::vector<float> jetDeltaR_time4D_all_;
    std::vector<int> jetMatched_time4D_all_;     // RECO ?~F~R GEN match flag
    std::vector<int> genJetMatched_time4D_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> genMatchedDen_time4D_all_;  // GEN ?~F~R RECO match flag
    std::vector<int> jetIsHS_time4D_all_;
    std::vector<int> jetIsPU_time4D_all_;
    std::vector<int> jetIsAmbiguous_time4D_all_;
    std::vector<int> jetIsUnknown_time4D_all_;
    std::vector<float> jetPt_time4D_all_;
    std::vector<float> jetAbsEta_time4D_all_;
    std::vector<float> jetPhi_time4D_all_;
    std::vector<std::vector<unsigned int>> jet_pfIndices_time4D_all_;
    std::vector<float> genPt_time4D_all_;
    std::vector<float> genEta_time4D_all_;
    std::vector<float> genEtaDen_time4D_all_;
    std::vector<float> jetResponse_PR_time4D_all_;
    std::vector<float> puFracPt_algo_time4D_all_;
    std::vector<float> puFracCount_algo_time4D_all_;
    std::vector<float> puFracPt_truth_time4D_all_;
    std::vector<float> puFracCount_truth_time4D_all_;

    std::vector<std::vector<unsigned int>> pf_indices_time4D_all_;//For <pat::PackedCandidate> collection
    std::vector<float> puFrac_time4D_;
    int totalRecoJetsClean_time4D;
    int totalPUJets_time4D;
    int totalGenJets_time4D;
    int nGenMatched_time4D;
    int nMatchedReco_time4D;

    float effLossAmbig_time4D_;
    float effWithAmbig_time4D_;//defined but not used
    float puJetFraction_time4D_all_;
    float efficiency_time4D_;
    float mistag_time4D_;
    float purity_time4D_;

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
        iConfig.getParameter<edm::InputTag>("trackTimeErrTag"))),

    genJetToken_(consumes<std::vector<reco::GenJet>>(
        iConfig.getParameter<edm::InputTag>("genJetsTag")))

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
    tree_->Branch("pf_hasValidTime", &pf_hasValidTime);

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
    tree_->Branch("pf_timingRegion", &pf_timingRegion);
    tree_->Branch("pf_isCharged", &pf_isCharged);

    tree_->Branch("jetDeltaR_puppi_all", &jetDeltaR_puppi_all_);
    tree_->Branch("jetMatched_puppi_all", &jetMatched_puppi_all_);
    tree_->Branch("genJetMatched_puppi_all", &genJetMatched_puppi_all_);
    tree_->Branch("jetIsHS_puppi_all", &jetIsHS_puppi_all_);
    tree_->Branch("jetIsPU_puppi_all", &jetIsPU_puppi_all_);
    tree_->Branch("jetIsAmbiguous_puppi_all", &jetIsAmbiguous_puppi_all_);
    tree_->Branch("jetIsUnknown_puppi_all", &jetIsUnknown_puppi_all_);
    tree_->Branch("jetResponse_PR_puppi_all", &jetResponse_PR_puppi_all_);
    tree_->Branch("jetPt_puppi_all", &jetPt_puppi_all_);
    tree_->Branch("jetPhi_puppi_all", &jetPhi_puppi_all_);
    tree_->Branch("jet_pfIndices_puppi_all", &jet_pfIndices_puppi_all_);
    tree_->Branch("genPt_puppi_all", &genPt_puppi_all_);
    tree_->Branch("genEta_puppi_all", &genEta_puppi_all_);
    tree_->Branch("genEtaDen_puppi_all", &genEtaDen_puppi_all_);
    tree_->Branch("genMatchedDen_puppi_all", &genMatchedDen_puppi_all_);
    tree_->Branch("jetAbsEta_puppi_all", &jetAbsEta_puppi_all_);
    tree_->Branch("puFracPt_algo_puppi_all", &puFracPt_algo_puppi_all_);
    tree_->Branch("puFracCount_algo_puppi_all", &puFracCount_algo_puppi_all_);
    tree_->Branch("puFracPt_truth_puppi_all", &puFracPt_truth_puppi_all_);
    tree_->Branch("puFracCount_truth_puppi_all", &puFracCount_truth_puppi_all_);
    //tree_->Branch("totalRecoJetsClean_puppi", &totalRecoJetsClean_puppi,"totalRecoJetsClean_puppi/I");
    tree_->Branch("totalRecoJetsClean_puppi",&totalRecoJetsClean_puppi);
    tree_->Branch("totalGenJets_puppi", &totalGenJets_puppi);
    tree_->Branch("nGenMatched_puppi", &nGenMatched_puppi);
    tree_->Branch("nMatchedReco_puppi", &nMatchedReco_puppi);
    tree_->Branch("totalPUJets_puppi", &totalPUJets_puppi);
    tree_->Branch("puJetFraction_puppi_all", &puJetFraction_puppi_all_,"puJetFraction_puppi_all/F");
    tree_->Branch("efficiency_puppi", &efficiency_puppi_,"efficiency_puppi/F");
    tree_->Branch("effLossAmbig_puppi", &effLossAmbig_puppi_,"effLossAmbig_puppi/F");
    tree_->Branch("effWithAmbig_puppi", &effWithAmbig_puppi_,"effWithAmbig_puppi/F");
    tree_->Branch("mistag_puppi", &mistag_puppi_,"mistag_puppi/F");
    tree_->Branch("purity_puppi", &purity_puppi_,"purity_puppi/F");
    tree_->Branch("pf_indices_puppi_all", &pf_indices_puppi_all_);

    tree_->Branch("jetDeltaR_pfraw_all", &jetDeltaR_pfraw_all_);
    tree_->Branch("jetMatched_pfraw_all", &jetMatched_pfraw_all_);
    tree_->Branch("genJetMatched_pfraw_all", &genJetMatched_pfraw_all_);
    tree_->Branch("jetIsHS_pfraw_all", &jetIsHS_pfraw_all_);
    tree_->Branch("jetIsPU_pfraw_all", &jetIsPU_pfraw_all_);
    tree_->Branch("jetIsAmbiguous_pfraw_all", &jetIsAmbiguous_pfraw_all_);
    tree_->Branch("jetIsUnknown_pfraw_all", &jetIsUnknown_pfraw_all_);
    tree_->Branch("jetResponse_PR_pfraw_all", &jetResponse_PR_pfraw_all_);
    tree_->Branch("jetPt_pfraw_all", &jetPt_pfraw_all_);
    tree_->Branch("jetPhi_pfraw_all", &jetPhi_pfraw_all_);
    tree_->Branch("jet_pfIndices_pfraw_all", &jet_pfIndices_pfraw_all_);
    tree_->Branch("genPt_pfraw_all", &genPt_pfraw_all_);
    tree_->Branch("genEta_pfraw_all", &genEta_pfraw_all_);
    tree_->Branch("genEtaDen_pfraw_all", &genEtaDen_pfraw_all_);
    tree_->Branch("genMatchedDen_pfraw_all", &genMatchedDen_pfraw_all_);
    tree_->Branch("jetAbsEta_pfraw_all", &jetAbsEta_pfraw_all_);
    tree_->Branch("puFracPt_algo_pfraw_all", &puFracPt_algo_pfraw_all_);
    tree_->Branch("puFracCount_algo_pfraw_all", &puFracCount_algo_pfraw_all_);
    tree_->Branch("puFracPt_truth_pfraw_all", &puFracPt_truth_pfraw_all_);
    tree_->Branch("puFracCount_truth_pfraw_all", &puFracCount_truth_pfraw_all_);
    //tree_->Branch("totalRecoJetsClean_pfraw", &totalRecoJetsClean_pfraw,"totalRecoJetsClean_pfraw/I");
    tree_->Branch("totalRecoJetsClean_pfraw",&totalRecoJetsClean_pfraw);
    tree_->Branch("totalGenJets_pfraw", &totalGenJets_pfraw);
    tree_->Branch("nGenMatched_pfraw", &nGenMatched_pfraw);
    tree_->Branch("nMatchedReco_pfraw", &nMatchedReco_pfraw);
    tree_->Branch("totalPUJets_pfraw", &totalPUJets_pfraw);
    tree_->Branch("puJetFraction_pfraw_all", &puJetFraction_pfraw_all_,"puJetFraction_pfraw_all/F");
    tree_->Branch("efficiency_pfraw", &efficiency_pfraw_,"efficiency_pfraw/F");
    tree_->Branch("effLossAmbig_pfraw", &effLossAmbig_pfraw_,"effLossAmbig_pfraw/F");
    tree_->Branch("effWithAmbig_pfraw", &effWithAmbig_pfraw_,"effWithAmbig_pfraw/F");
    tree_->Branch("mistag_pfraw", &mistag_pfraw_,"mistag_pfraw/F");
    tree_->Branch("purity_pfraw", &purity_pfraw_,"purity_pfraw/F");
    tree_->Branch("pf_indices_pfraw_all", &pf_indices_pfraw_all_);


    tree_->Branch("jetDeltaR_loose_all", &jetDeltaR_loose_all_);
    tree_->Branch("jetMatched_loose_all", &jetMatched_loose_all_);
    tree_->Branch("genJetMatched_loose_all", &genJetMatched_loose_all_);
    tree_->Branch("jetIsHS_loose_all", &jetIsHS_loose_all_);
    tree_->Branch("jetIsPU_loose_all", &jetIsPU_loose_all_);
    tree_->Branch("jetIsAmbiguous_loose_all", &jetIsAmbiguous_loose_all_);
    tree_->Branch("jetIsUnknown_loose_all", &jetIsUnknown_loose_all_);
    tree_->Branch("jetResponse_PR_loose_all", &jetResponse_PR_loose_all_);
    tree_->Branch("jetPt_loose_all", &jetPt_loose_all_);
    tree_->Branch("jetPhi_loose_all", &jetPhi_loose_all_);
    tree_->Branch("jet_pfIndices_loose_all", &jet_pfIndices_loose_all_);
    tree_->Branch("genPt_loose_all", &genPt_loose_all_);
    tree_->Branch("genEta_loose_all", &genEta_loose_all_);
    tree_->Branch("genEtaDen_loose_all", &genEtaDen_loose_all_);
    tree_->Branch("genMatchedDen_loose_all", &genMatchedDen_loose_all_);
    tree_->Branch("jetAbsEta_loose_all", &jetAbsEta_loose_all_);
    tree_->Branch("puFracPt_algo_loose_all", &puFracPt_algo_loose_all_);
    tree_->Branch("puFracCount_algo_loose_all", &puFracCount_algo_loose_all_);
    tree_->Branch("puFracPt_truth_loose_all", &puFracPt_truth_loose_all_);
    tree_->Branch("puFracCount_truth_loose_all", &puFracCount_truth_loose_all_);
    //tree_->Branch("totalRecoJetsClean_loose", &totalRecoJetsClean_loose,"totalRecoJetsClean_loose/I");
    tree_->Branch("totalRecoJetsClean_loose",&totalRecoJetsClean_loose);
    tree_->Branch("totalGenJets_loose", &totalGenJets_loose);
    tree_->Branch("nGenMatched_loose", &nGenMatched_loose);
    tree_->Branch("nMatchedReco_loose", &nMatchedReco_loose);
    tree_->Branch("totalPUJets_loose", &totalPUJets_loose);
    tree_->Branch("puJetFraction_loose_all", &puJetFraction_loose_all_,"puJetFraction_loose_all/F");
    tree_->Branch("efficiency_loose", &efficiency_loose_,"efficiency_loose/F");
    tree_->Branch("effLossAmbig_loose", &effLossAmbig_loose_,"effLossAmbig_loose/F");
    tree_->Branch("effWithAmbig_loose", &effWithAmbig_loose_,"effWithAmbig_loose/F");
    tree_->Branch("mistag_loose", &mistag_loose_,"mistag_loose/F");
    tree_->Branch("purity_loose", &purity_loose_,"purity_loose/F");
    tree_->Branch("pf_indices_loose_all", &pf_indices_loose_all_);


    tree_->Branch("jetDeltaR_tight_all", &jetDeltaR_tight_all_);
    tree_->Branch("jetMatched_tight_all", &jetMatched_tight_all_);
    tree_->Branch("genJetMatched_tight_all", &genJetMatched_tight_all_);
    tree_->Branch("jetIsHS_tight_all", &jetIsHS_tight_all_);
    tree_->Branch("jetIsPU_tight_all", &jetIsPU_tight_all_);
    tree_->Branch("jetIsAmbiguous_tight_all", &jetIsAmbiguous_tight_all_);
    tree_->Branch("jetIsUnknown_tight_all", &jetIsUnknown_tight_all_);
    tree_->Branch("jetResponse_PR_tight_all", &jetResponse_PR_tight_all_);
    tree_->Branch("jetPt_tight_all", &jetPt_tight_all_);
    tree_->Branch("jetPhi_tight_all", &jetPhi_tight_all_);
    tree_->Branch("jet_pfIndices_tight_all", &jet_pfIndices_tight_all_);
    tree_->Branch("genPt_tight_all", &genPt_tight_all_);
    tree_->Branch("genEta_tight_all", &genEta_tight_all_);
    tree_->Branch("genEtaDen_tight_all", &genEtaDen_tight_all_);
    tree_->Branch("genMatchedDen_tight_all", &genMatchedDen_tight_all_);
    tree_->Branch("jetAbsEta_tight_all", &jetAbsEta_tight_all_);
    tree_->Branch("puFracPt_algo_tight_all", &puFracPt_algo_tight_all_);
    tree_->Branch("puFracCount_algo_tight_all", &puFracCount_algo_tight_all_);
    tree_->Branch("puFracPt_truth_tight_all", &puFracPt_truth_tight_all_);
    tree_->Branch("puFracCount_truth_tight_all", &puFracCount_truth_tight_all_);
    //tree_->Branch("totalRecoJetsClean_tight", &totalRecoJetsClean_tight,"totalRecoJetsClean_tight/I");
    tree_->Branch("totalRecoJetsClean_tight",&totalRecoJetsClean_tight);
    tree_->Branch("totalGenJets_tight", &totalGenJets_tight);
    tree_->Branch("nGenMatched_tight", &nGenMatched_tight);
    tree_->Branch("nMatchedReco_tight", &nMatchedReco_tight);
    tree_->Branch("totalPUJets_tight", &totalPUJets_tight);
    tree_->Branch("puJetFraction_tight_all", &puJetFraction_tight_all_,"puJetFraction_tight_all/F");
    tree_->Branch("efficiency_tight", &efficiency_tight_,"efficiency_tight/F");
    tree_->Branch("effLossAmbig_tight", &effLossAmbig_tight_,"effLossAmbig_tight/F");
    tree_->Branch("effWithAmbig_tight", &effWithAmbig_tight_,"effWithAmbig_tight/F");
    tree_->Branch("mistag_tight", &mistag_tight_,"mistag_tight/F");
    tree_->Branch("purity_tight", &purity_tight_,"purity_tight/F");
    tree_->Branch("pf_indices_tight_all", &pf_indices_tight_all_);


    tree_->Branch("jetDeltaR_time_all", &jetDeltaR_time_all_);
    tree_->Branch("jetMatched_time_all", &jetMatched_time_all_);
    tree_->Branch("genJetMatched_time_all", &genJetMatched_time_all_);
    tree_->Branch("jetIsHS_time_all", &jetIsHS_time_all_);
    tree_->Branch("jetIsPU_time_all", &jetIsPU_time_all_);
    tree_->Branch("jetIsAmbiguous_time_all", &jetIsAmbiguous_time_all_);
    tree_->Branch("jetIsUnknown_time_all", &jetIsUnknown_time_all_);
    tree_->Branch("jetResponse_PR_time_all", &jetResponse_PR_time_all_);
    tree_->Branch("jetPt_time_all", &jetPt_time_all_);
    tree_->Branch("jetPhi_time_all", &jetPhi_time_all_);
    tree_->Branch("jet_pfIndices_time_all", &jet_pfIndices_time_all_);
    tree_->Branch("genPt_time_all", &genPt_time_all_);
    tree_->Branch("genEta_time_all", &genEta_time_all_);
    tree_->Branch("genEtaDen_time_all", &genEtaDen_time_all_);
    tree_->Branch("genMatchedDen_time_all", &genMatchedDen_time_all_);
    tree_->Branch("jetAbsEta_time_all", &jetAbsEta_time_all_);
    tree_->Branch("puFracPt_algo_time_all", &puFracPt_algo_time_all_);
    tree_->Branch("puFracCount_algo_time_all", &puFracCount_algo_time_all_);
    tree_->Branch("puFracPt_truth_time_all", &puFracPt_truth_time_all_);
    tree_->Branch("puFracCount_truth_time_all", &puFracCount_truth_time_all_);
    //tree_->Branch("totalRecoJetsClean_time", &totalRecoJetsClean_time,"totalRecoJetsClean_time/I");
    tree_->Branch("totalRecoJetsClean_time",&totalRecoJetsClean_time);
    tree_->Branch("totalGenJets_time", &totalGenJets_time);
    tree_->Branch("nGenMatched_time", &nGenMatched_time);
    tree_->Branch("nMatchedReco_time", &nMatchedReco_time);
    tree_->Branch("totalPUJets_time", &totalPUJets_time);
    tree_->Branch("puJetFraction_time_all", &puJetFraction_time_all_,"puJetFraction_time_all/F");
    tree_->Branch("efficiency_time", &efficiency_time_,"efficiency_time/F");
    tree_->Branch("effLossAmbig_time", &effLossAmbig_time_,"effLossAmbig_time/F");
    tree_->Branch("effWithAmbig_time", &effWithAmbig_time_,"effWithAmbig_time/F");
    tree_->Branch("mistag_time", &mistag_time_,"mistag_time/F");
    tree_->Branch("purity_time", &purity_time_,"purity_time/F");
    tree_->Branch("pf_indices_time_all", &pf_indices_time_all_);


    tree_->Branch("jetDeltaR_time4D_all", &jetDeltaR_time4D_all_);
    tree_->Branch("jetMatched_time4D_all", &jetMatched_time4D_all_);
    tree_->Branch("genJetMatched_time4D_all", &genJetMatched_time4D_all_);
    tree_->Branch("jetIsHS_time4D_all", &jetIsHS_time4D_all_);
    tree_->Branch("jetIsPU_time4D_all", &jetIsPU_time4D_all_);
    tree_->Branch("jetIsAmbiguous_time4D_all", &jetIsAmbiguous_time4D_all_);
    tree_->Branch("jetIsUnknown_time4D_all", &jetIsUnknown_time4D_all_);
    tree_->Branch("jetResponse_PR_time4D_all", &jetResponse_PR_time4D_all_);
    tree_->Branch("jetPt_time4D_all", &jetPt_time4D_all_);
    tree_->Branch("jetPhi_time4D_all", &jetPhi_time4D_all_);
    tree_->Branch("jet_pfIndices_time4D_all", &jet_pfIndices_time4D_all_);
    tree_->Branch("genPt_time4D_all", &genPt_time4D_all_);
    tree_->Branch("genEta_time4D_all", &genEta_time4D_all_);
    tree_->Branch("genEtaDen_time4D_all", &genEtaDen_time4D_all_);
    tree_->Branch("genMatchedDen_time4D_all", &genMatchedDen_time4D_all_);
    tree_->Branch("jetAbsEta_time4D_all", &jetAbsEta_time4D_all_);
    tree_->Branch("puFracPt_algo_time4D_all", &puFracPt_algo_time4D_all_);
    tree_->Branch("puFracCount_algo_time4D_all", &puFracCount_algo_time4D_all_);
    tree_->Branch("puFracPt_truth_time4D_all", &puFracPt_truth_time4D_all_);
    tree_->Branch("puFracCount_truth_time4D_all", &puFracCount_truth_time4D_all_);
    //tree_->Branch("totalRecoJetsClean_time4D", &totalRecoJetsClean_time4D,"totalRecoJetsClean_time4D/I");
    tree_->Branch("totalRecoJetsClean_time4D",&totalRecoJetsClean_time4D);
    tree_->Branch("totalGenJets_time4D", &totalGenJets_time4D);
    tree_->Branch("nGenMatched_time4D", &nGenMatched_time4D);
    tree_->Branch("nMatchedReco_time4D", &nMatchedReco_time4D);
    tree_->Branch("totalPUJets_time4D", &totalPUJets_time4D);
    tree_->Branch("puJetFraction_time4D_all", &puJetFraction_time4D_all_,"puJetFraction_time4D_all/F");
    tree_->Branch("efficiency_time4D", &efficiency_time4D_,"efficiency_time4D/F");
    tree_->Branch("effLossAmbig_time4D", &effLossAmbig_time4D_,"effLossAmbig_time4D/F");
    tree_->Branch("effWithAmbig_time4D", &effWithAmbig_time4D_,"effWithAmbig_time4D/F");
    tree_->Branch("mistag_time4D", &mistag_time4D_,"mistag_time4D/F");
    tree_->Branch("purity_time4D", &purity_time4D_,"purity_time4D/F");
    tree_->Branch("pf_indices_time4D_all", &pf_indices_time4D_all_);


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
    pf_hasValidTime.clear();
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
    pf_timingRegion.clear();
    pf_isCharged.clear();


    pf_indices_puppi_all_.clear();
    jetDeltaR_puppi_all_.clear();
    jetMatched_puppi_all_.clear();
    genJetMatched_puppi_all_.clear();
    jetIsHS_puppi_all_.clear();
    jetIsPU_puppi_all_.clear();
    jetIsAmbiguous_puppi_all_.clear();
    jetIsUnknown_puppi_all_.clear();
    jetPt_puppi_all_.clear();
    jetPhi_puppi_all_.clear();
    jet_pfIndices_puppi_all_.clear();
    genPt_puppi_all_.clear();
    genEta_puppi_all_.clear();
    genEtaDen_puppi_all_.clear();
    genMatchedDen_puppi_all_.clear();
    jetResponse_PR_puppi_all_.clear();
    jetAbsEta_puppi_all_.clear();
    puFracPt_algo_puppi_all_.clear();
    puFracCount_algo_puppi_all_.clear();
    puFracPt_truth_puppi_all_.clear();
    puFracCount_truth_puppi_all_.clear();

    pf_indices_pfraw_all_.clear();
    jetDeltaR_pfraw_all_.clear();
    jetMatched_pfraw_all_.clear();
    genJetMatched_pfraw_all_.clear();
    jetIsHS_pfraw_all_.clear();
    jetIsPU_pfraw_all_.clear();
    jetIsAmbiguous_pfraw_all_.clear();
    jetIsUnknown_pfraw_all_.clear();
    jetPt_pfraw_all_.clear();
    jetPhi_pfraw_all_.clear();
    jet_pfIndices_pfraw_all_.clear();
    genPt_pfraw_all_.clear();
    genEta_pfraw_all_.clear();
    genEtaDen_pfraw_all_.clear();
    genMatchedDen_pfraw_all_.clear();
    jetResponse_PR_pfraw_all_.clear();
    jetAbsEta_pfraw_all_.clear();
    puFracPt_algo_pfraw_all_.clear();
    puFracCount_algo_pfraw_all_.clear();
    puFracPt_truth_pfraw_all_.clear();
    puFracCount_truth_pfraw_all_.clear();


    pf_indices_loose_all_.clear();
    jetDeltaR_loose_all_.clear();
    jetMatched_loose_all_.clear();
    genJetMatched_loose_all_.clear();
    jetIsHS_loose_all_.clear();
    jetIsPU_loose_all_.clear();
    jetIsAmbiguous_loose_all_.clear();
    jetIsUnknown_loose_all_.clear();
    jetPt_loose_all_.clear();
    jetPhi_loose_all_.clear();
    jet_pfIndices_loose_all_.clear();
    genPt_loose_all_.clear();
    genEta_loose_all_.clear();
    genEtaDen_loose_all_.clear();
    genMatchedDen_loose_all_.clear();
    jetResponse_PR_loose_all_.clear();
    jetAbsEta_loose_all_.clear();
    puFracPt_algo_loose_all_.clear();
    puFracCount_algo_loose_all_.clear();
    puFracPt_truth_loose_all_.clear();
    puFracCount_truth_loose_all_.clear();


    pf_indices_tight_all_.clear();
    jetDeltaR_tight_all_.clear();
    jetMatched_tight_all_.clear();
    genJetMatched_tight_all_.clear();
    jetIsHS_tight_all_.clear();
    jetIsPU_tight_all_.clear();
    jetIsAmbiguous_tight_all_.clear();
    jetIsUnknown_tight_all_.clear();
    jetPt_tight_all_.clear();
    jetPhi_tight_all_.clear();
    jet_pfIndices_tight_all_.clear();
    genPt_tight_all_.clear();
    genEta_tight_all_.clear();
    genEtaDen_tight_all_.clear();
    genMatchedDen_tight_all_.clear();
    jetResponse_PR_tight_all_.clear();
    jetAbsEta_tight_all_.clear();
    puFracPt_algo_tight_all_.clear();
    puFracCount_algo_tight_all_.clear();
    puFracPt_truth_tight_all_.clear();
    puFracCount_truth_tight_all_.clear();


    pf_indices_time_all_.clear();
    jetDeltaR_time_all_.clear();
    jetMatched_time_all_.clear();
    genJetMatched_time_all_.clear();
    jetIsHS_time_all_.clear();
    jetIsPU_time_all_.clear();
    jetIsAmbiguous_time_all_.clear();
    jetIsUnknown_time_all_.clear();
    jetPt_time_all_.clear();
    jetPhi_time_all_.clear();
    jet_pfIndices_time_all_.clear();
    genPt_time_all_.clear();
    genEta_time_all_.clear();
    genEtaDen_time_all_.clear();
    genMatchedDen_time_all_.clear();
    jetResponse_PR_time_all_.clear();
    jetAbsEta_time_all_.clear();
    puFracPt_algo_time_all_.clear();
    puFracCount_algo_time_all_.clear();
    puFracPt_truth_time_all_.clear();
    puFracCount_truth_time_all_.clear();

    pf_indices_time4D_all_.clear();
    jetDeltaR_time4D_all_.clear();
    jetMatched_time4D_all_.clear();
    genJetMatched_time4D_all_.clear();
    jetIsHS_time4D_all_.clear();
    jetIsPU_time4D_all_.clear();
    jetIsAmbiguous_time4D_all_.clear();
    jetIsUnknown_time4D_all_.clear();
    jetPt_time4D_all_.clear();
    jetPhi_time4D_all_.clear();
    jet_pfIndices_time4D_all_.clear();
    genPt_time4D_all_.clear();
    genEta_time4D_all_.clear();
    genEtaDen_time4D_all_.clear();
    genMatchedDen_time4D_all_.clear();
    jetResponse_PR_time4D_all_.clear();
    jetAbsEta_time4D_all_.clear();
    puFracPt_algo_time4D_all_.clear();
    puFracCount_algo_time4D_all_.clear();
    puFracPt_truth_time4D_all_.clear();
    puFracCount_truth_time4D_all_.clear();

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

    edm::Handle<std::vector<reco::GenJet>> genJetsHandle;
    iEvent.getByToken(genJetToken_, genJetsHandle);

    if (!genJetsHandle.isValid()) {
         edm::LogWarning("JetTreeProducer_Reco")
             << "GenJet collection not found.";
         return;
    }
    const auto& genJets = *genJetsHandle;


    //----------------------------------
    // FastJet input collections
    //----------------------------------
    std::vector<fastjet::PseudoJet> fjInputs_raw;
    std::vector<fastjet::PseudoJet> fjInputs_tight;
    std::vector<fastjet::PseudoJet> fjInputs_loose;
    std::vector<fastjet::PseudoJet> fjInputs_time;
    std::vector<fastjet::PseudoJet> fjInputs_time4D;

    fjInputs_raw.clear();
    fjInputs_tight.clear();
    fjInputs_loose.clear();
    fjInputs_time.clear();
    fjInputs_time4D.clear();

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
    int nInvalid=0;
    int nValid=0;

    for (size_t iPF = 0; iPF < pfcands->size(); ++iPF)
    {
//        std::cout
//          << "Number of PF candidates = "
//          << pfcands->size()
//          << std::endl;

        const reco::PFCandidate& pf = pfcands->at(iPF);
        fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
        pj.set_user_index(static_cast<int>(iPF)); //KEY of global intexing,Index matching
        fjInputs_raw.push_back(pj);//keep global-indexed PF for raw jets, Index matching

        float eta=pf.eta();
        bool isInMTD = (std::abs(eta) <= 3.0);
        bool isCharged=(pf.charge() !=0);
        bool isNeutral=(pf.charge()==0);
        int timingRegion = -1;

        if (std::abs(eta) < 1.5)
            timingRegion = 0;   // BTL

        else if (std::abs(eta) < 3.0)
            timingRegion = 1;   // ETL

        else
            timingRegion = 2;   // Outside MTD

        pf_timingRegion.push_back(timingRegion);

        // =========================================
        // BASIC PF INFO
        // =========================================

        pf_pt.push_back(pf.pt());
        pf_eta.push_back(pf.eta());
        pf_phi.push_back(pf.phi());
        pf_charge.push_back(pf.charge());
        pf_isInMTD.push_back(isInMTD);

        // =========================================
        // DEFAULT TRUTH VALUES
        // =========================================

        int isHS = -1;
        int isPU = -1;

        float truth_vz = -999.f;
        float dz = -999.f;
        float dzErr= -999.f;
        float dzSig= -999.f;

        bool hasValidTime = false;
        bool pvHasValidTime=false;
//        bool isDisplaced=false;
        float t      = -999.f;
        float tErr   = -999.f;
        float tSig   = -999.f;

        float dt     = 999.f;
        float dtErr  = 999.f;
        float dtSig  = 999.f;

        bool passesBaseCut       = false;
        bool passesLooseCut      = false;
        bool passesTightCut      = false;

        bool passesTimeSelection = false;
        bool passes4D            = false;
                
        float pv_t = pv.t();
        float pvs_TimeErr_ = pv.tError();

        // =========================================
        // ONLY CHARGED PF HAS TRACKS
        // =========================================
//        if (!isInMTD) continue;

        if (isInMTD&&isCharged&&pf.trackRef().isAvailable())
        {
            reco::TrackRef trkRef = pf.trackRef();

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
//                isDisplaced = (std::abs(dz) > 0.05f);
                float dzCutBase=1.0;//0.05/0.14     
                float dzCutLoose=0.5;//0.05/0.01     
                float dzCutTight=0.1;//0.03/0.002     
 
                float dzErrCutBase=0.0225;//0.05/0.0225     
                float dzErrCutLoose=0.017;//0.05/0.017
                float dzErrCutTight=0.014;//0.03/0.014     
 
                float dzSigCutBase=3;//0.2/3    
                float dzSigCutLoose=2.15;//0.2/2.15    
                float dzSigCutTight=1.65;//0.5/1.65

//                passesBaseCut = (std::abs(dz) < dzCutBase && std::abs(dzSig) < dzSigCutBase);//3D loos selection
                passesLooseCut = (std::abs(dz) < dzCutLoose && std::abs(dzSig) < dzSigCutLoose);//3D loos selection
                passesTightCut = (std::abs(dz) < dzCutTight && std::abs(dzSig) < dzSigCutTight);//3D tight selection

                // =========================================
                // dt wrt PV
                // = reconstructed trackparticle time  wrt reco PV
                // =========================================
                t    = (*trackTimeMap)[trkRef];//t_track
                tErr = (*trackTimeErrMap)[trkRef];//t_trackErr

                hasValidTime =
                (
                    isInMTD &&
                    std::isfinite(t)&&
                    std::isfinite(tErr)&&
                    tErr>0
                );

                 if (fabs(t) < 1e-6)
                 {
//                   std::cout
//                   << "t=" << t
//                   << " tErr=" << tErr
//                   << " hasValidTime=" << hasValidTime
//                   << std::endl;
                 }

                  if(hasValidTime){
                      nValid++;
                      tSig = t / tErr;
                  }
                  else{
                    nInvalid++;
                  }
   
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


                      float dtCutBase=1.0;//0.03
                      float dtCutLoose=0.4;//0.03
                      float dtCutTight=0.2;//0.03
 
                      float dtErrCutBase=0.071;//0.03
                      float dtErrCutLoose=0.044;//0.03
                      float dtErrCutTight=0.039;//0.03
 
                      float dtSigCutBase=3;//0.2/2
                      float dtSigCutLoose=2.15;//0.2/1.5
                      float dtSigCutTight=1.65;//0.5/1

                      passesTimeSelection= hasValidTime && pvHasValidTime &&(std::abs(dt)<dtCutLoose)&&(std::abs(dtSig)<dtSigCutBase);//track +time
                      passes4D = passesTightCut&&hasValidTime && pvHasValidTime&&(std::abs(dz) < dzCutLoose)&&(std::abs(dt)<dtCutTight)&&(std::abs(dtSig)<dtSigCutBase);//tract_resolution+time_resolution          

                  }//End of (hasValidTime && pvHasValidTime)

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
      }//End of if (trkRef.isNonnull())
  }//End of if (isInMTD&&isCharged&&pf.trackRef().isAvailable()

    // =========================================
    // STORE RESULTS
    // =========================================

    pf_isHS_truth.push_back(isHS);
    pf_isPU_truth.push_back(isPU);
    pf_isCharged.push_back(isCharged);

    pf_truth_vz.push_back(truth_vz);
    pf_hasValidTime.push_back(hasValidTime);
    pf_dz.push_back(dz);
    pf_dzError.push_back(dzErr);
    pf_dzSig.push_back(dzSig);
    pf_time.push_back(t);
    pf_timeError.push_back(tErr);
    pf_timeSig.push_back(tSig);
    pf_dt.push_back(dt);
    pf_dtError.push_back(dtErr);
    pf_dtSig.push_back(dtSig);

    // -----------------------------------
    // Fill FastJet collections
    // -----------------------------------

    if (passesLooseCut)
        fjInputs_loose.push_back(pj);

    if (passesTightCut)
        fjInputs_tight.push_back(pj);

    if (passesTimeSelection)
        fjInputs_time.push_back(pj);

    if (passes4D)
        fjInputs_time4D.push_back(pj);
  }//End of for (size_t i = 0; i < pfcands->size(); ++i)

//std::cout
//    << "raw="    << fjInputs_raw.size()
//    << " loose=" << fjInputs_loose.size()
//    << " tight=" << fjInputs_tight.size()
//    << " time="  << fjInputs_time.size()
//    << " time4D="<< fjInputs_time4D.size()
//    << std::endl;

// Run the jet clustering algorithm on each collection
fastjet::JetDefinition jetDef(fastjet::antikt_algorithm, 0.4);
auto cs_raw = fastjet::ClusterSequence(fjInputs_raw, jetDef);
auto pfrawJets = fastjet::sorted_by_pt(cs_raw.inclusive_jets(10.0));//return only jets with pt ≥ 10 GeV.

auto cs_tight = fastjet::ClusterSequence(fjInputs_tight, jetDef);
auto tightJets = fastjet::sorted_by_pt(cs_tight.inclusive_jets(10.0));

auto cs_loose = fastjet::ClusterSequence(fjInputs_loose, jetDef);
auto looseJets = fastjet::sorted_by_pt(cs_loose.inclusive_jets(10.0));

auto cs_time = fastjet::ClusterSequence(fjInputs_time, jetDef);
auto timeJets = fastjet::sorted_by_pt(cs_time.inclusive_jets(10.0));

auto cs_time4D = fastjet::ClusterSequence(fjInputs_time4D, jetDef);
auto timeJets4D = fastjet::sorted_by_pt(cs_time4D.inclusive_jets(10.0));

//-----------The index of the original PF candidate------------//
//,which makes immediate access, such as pf_pt[idx], No additional matching is needed.
for (const auto& jet : pfrawJets)
{
    std::vector<fastjet::PseudoJet> constituents = jet.constituents();
    for (const auto& c : constituents)
    {
        int idx = c.user_index();

        if (idx < 0) continue;

        float pt  = pf_pt[idx];

        // temporary check
//        std::cout
//        << "idx = " << idx
//        << "  pt = " << pf_pt[idx]
//        << std::endl;

}
}

// =========================================
// Loop over raw jets
// =========================================

for (const auto& jet : pfrawJets)
{
    int nConst = 0;
    int nCharged = 0;
    int nNeutral = 0;
    int nHS = 0;
    int nPU = 0;

    float sumPt = 0.f;
    float sumDz = 0.f;
    float sumDt = 0.f;

    std::vector<fastjet::PseudoJet> constituents = jet.constituents();

    for (const auto& c : constituents)
    {
        int idx = c.user_index();

        if (idx < 0 || idx >= (int)pf_pt.size())
            continue;

        ++nConst;

        if (pf_isCharged[idx])
            ++nCharged;
        else
            ++nNeutral;

        if (pf_isHS_truth[idx] == 1)
            ++nHS;

        if (pf_isPU_truth[idx] == 1)
            ++nPU;

        sumPt += pf_pt[idx];

        if (pf_dz[idx] > -900.f)
            sumDz += pf_dz[idx];

        if (pf_dt[idx] < 900.f)
            sumDt += pf_dt[idx];
    }//End of for (const auto& c : constituents)

//    float fracHS = 0.f;
//    float fracPU = 0.f;

//    if ((nHS + nPU) > 0)
//    {
//        fracHS = float(nHS) / float(nHS + nPU);
//        fracPU = float(nPU) / float(nHS + nPU);
//    }

//    float meanDz = 0.f;
//    float meanDt = 0.f;

//    if (nConst > 0)
//    {
//        meanDz = sumDz / float(nConst);
//        meanDt = sumDt / float(nConst);
//    }

//    std::cout
//        << "Jet pT = " << jet.pt()
//        << " nConst = " << nConst
//        << " nHS = " << nHS
//        << " nPU = " << nPU
//        << " HSfrac = " << fracHS
//        << " PUfrac = " << fracPU
//        << " meanDz = " << meanDz
//        << " meanDt = " << meanDt
//        << std::endl;
}//End of for (const auto& jet : pfrawJets)


//===== Generic std::vector<reco::PFJet> Loop Template=====
auto processJetCollection = [&](const std::vector<reco::PFJet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,
                                std::vector<int>& jetMatched_all_,     // RECO → GEN match flag
                                std::vector<int>& genJetMatched_all_,  // GEN → RECO match flag
                                std::vector<int>& genMatchedDen_all_,  // GEN → RECO match flag
                                std::vector<int>& jetIsHS_all_,
                                std::vector<int>& jetIsPU_all_,
                                std::vector<int>& jetIsAmbiguous_all_,
                                std::vector<int>& jetIsUnknown_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetPhi_all_,
                                std::vector<std::vector<unsigned int>>& jet_pfIndices_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& genEta_all_,
                                std::vector<float>& genEtaDen_all_,
                                std::vector<float>& puFracPt_algo_all_,
                                std::vector<float>& puFracCount_algo_all_,
                                std::vector<float>& puFracPt_truth_all_,
                                std::vector<float>& puFracCount_truth_all_,
                                std::vector<float>& jetDeltaR_all_,
                                std::vector<std::vector<unsigned int>>& pf_indices_general_all_,//per-jet PF indices
                                int& totalRecoJetsClean,
                                int& totalPUJets,
                                int& totalGenJets,
                                int& nGenMatched,
                                int& nMatchedReco,
                                float& puJetFraction_all_,
                                float& efficiency_out,
                                float& effLossAmbig,
                                float& effWithAmbig,
                                float& mistag,
                                float& purity_out) {


//Build matchCandidates

//Greedy one-to-one assignment

//assignedGenIndex
//assignedRecoIndex
//assignedDR

//HS / PU classification

//Fill ntuple

//Initialize the vector and the denominator
totalRecoJetsClean = 0;
totalPUJets        = 0;
totalGenJets       = 0;
nGenMatched        = 0;
nMatchedReco       = 0;

puJetFraction_all_ = -1.f;
efficiency_out     = -1.f;
effLossAmbig       = -1.f;
effWithAmbig       = -1.f;
mistag             = -1.f;
purity_out         = -1.f;

static size_t maxRecoJetsMatched = 0;
static size_t maxGenJetsMatched = 0;
genJetMatched_all_.assign(genJets.size(), 0);
struct MatchCandidate
{
    int jetIndex;
    int genIndex;
    float dR;
};

std::vector<MatchCandidate> matchCandidates;
std::vector<std::vector<MatchCandidate>> recoCandidates;

for (const auto& genJet : genJets)
{
    if (genJet.pt() > 20.)
        ++totalGenJets;
}

genMatchedDen_all_.assign(genJets.size(), 0);
std::vector<int> genJetAlreadyUsed(genJets.size(),0);

recoCandidates.clear();
recoCandidates.resize(jetsIn.size());
for (const auto& jet : jetsIn)
{
    std::vector<unsigned int> pf_indices_this_jet;
    pf_indices_this_jet = getPFIndicesFromPFJet(jet);

//    std::cout
//      << "PF indices found = "
//      << pf_indices_this_jet.size()
//      << std::endl;


    jet_pfIndices_all_.push_back(pf_indices_this_jet);
    pf_indices_general_all_.push_back(pf_indices_this_jet);

    jetPt_all_.push_back(jet.pt());
    jetAbsEta_all_.push_back(std::abs(jet.eta()));
    jetPhi_all_.push_back(jet.phi());
    int thisJetIndex = jetPt_all_.size() - 1;

    int nHS = 0;
    int nPU = 0;

    float sumPtHS = 0.f;
    float sumPtPU = 0.f;
    float sumPtUnknown = 0.f;

    float sumPtTot = 0.f;
    int nAlgoKeep = 0;
    int nAlgoReject = 0;

    float sumPtKeep = 0.f;
    float sumPtReject = 0.f;

    for (unsigned int idx : pf_indices_this_jet)
    {   if (idx >= pf_pt.size())
           continue;
        ++nAlgoKeep;
        sumPtKeep += pf_pt[idx];


        if (idx >= pf_pt.size())
            continue;

        sumPtTot += pf_pt[idx];

        if (pf_isHS_truth[idx] == 1)
        {
            ++nHS;
            sumPtHS += pf_pt[idx];
        }//End of if (pf_isHS_truth[idx] == 1)

        else if (pf_isPU_truth[idx] == 1)
        {
            ++nPU;
            sumPtPU += pf_pt[idx];
        }//End of if (pf_isPU_truth[idx] == 1)
        else
        {
            sumPtUnknown += pf_pt[idx];
        }
    }//End of for (unsigned int idx : pf_indices_this_jet), End of PF constituent loop

    // Compute jet-level quantities AFTER all constituents have been counted
    // puFracCount_truth and puFracPt_truth are jet-level quantities, so they must be computed once per jet,after the constituent loop finishes.
    float puFracCount_truth =
        (nHS + nPU > 0) ?
        float(nPU) / float(nHS + nPU) :
        -1.f;

    float puFracPt_truth =
        (sumPtTot > 0.f) ?
        sumPtPU / sumPtTot :
        -1.f;

//    std::cout
//    << "Jet summary:"
//    << " nPF=" << pf_indices_this_jet.size()
//    << " sumPtTot=" << sumPtTot
//    << " sumPtHS=" << sumPtHS
//    << " sumPtPU=" << sumPtPU
//    << " sumPtUnknown=" << sumPtUnknown
//    << std::endl;

    //--constituent-based HS/PU jet definition, Not generator-level definition---
    //constituentHSJet=HS fraction > 50%: Initial criteria of classification
    //isUnknownJet:they genuinely cannot be classified using the track-based truth matching.
    //isUnknownJet: no track,jets dominated by neutral particles,
    //isUnknownJet: neutral PF candidate,
    //isUnknownJet:outside MTD,|η| > , calorimeter-only jets,
    //isUnknownJet: no TrackingParticle association,
    //isUnknownJet: Those PF candidates have no truth information.

    const float eps = 1e-6;
    bool isUnknownJet = (sumPtHS + sumPtPU < eps);
    bool constituentHSJet = (!isUnknownJet && sumPtHS >= sumPtPU);
//    bool constituentPUJet = (!isUnknownJet && !constituentHSJet);

//one-to-one nearest-neighbor matching need, where each GenJet can be matched to at most one RECO jet.

    //GEN loop:Find Matched  GENjet
    for (size_t iGen = 0; iGen < genJets.size(); ++iGen)
    {
        const auto& genJet = genJets[iGen];

        if (genJet.pt() < 20.) continue;

        float dR = reco::deltaR(jet.eta(), jet.phi(),
                            genJet.eta(), genJet.phi());
        //All valid RECO-GEN pairs are now stored
        if (dR < 1.0)
        {   //every RECO jet has its own candidate list
            recoCandidates[thisJetIndex].push_back(
            {
                thisJetIndex,
                static_cast<int>(iGen),
                dR
            });

        }//End of  if (dR < 1.0)

    }//for (size_t iGen = 0; iGen < genJets.size(); ++iGen)

//    jetDeltaR_all_.push_back(bestDR);
    jetDeltaR_all_.push_back(-1.f);


//    jetMatched_all_.push_back(bestGenJet ? 1 : 0);
     jetMatched_all_.push_back(0);

//    genPt_all_.push_back(bestGenJet ? bestGenJet->pt() : -1.f);
//    genEta_all_.push_back(bestGenJet ? std::abs(bestGenJet->eta()) : -1.f);
    genPt_all_.push_back(-1.f);
    genEta_all_.push_back(-1.f);

//    float response = -1.f;
//    if (bestGenJet && bestGenJet->pt() > 0.f)
//        response = jet.pt() / bestGenJet->pt();

//    jetResponse_all_.push_back(response);
    jetResponse_all_.push_back(-1.f);
    //----------------------------
    // RECO -> GEN:old one-to-many jet matching
    //----------------------------
     jetMatched_all_.back() = 0;
    //----------------------------
    // Gen Jet->RECO:old one-to-many jet  matching 
    //----------------------------

    // Store one value per jet
    jetIsUnknown_all_.push_back(isUnknownJet ? 1 : 0);
    puFracCount_truth_all_.push_back(puFracCount_truth);
    puFracPt_truth_all_.push_back(puFracPt_truth);

    if (isUnknownJet)
    {
//         std::cout
//            << "Unknown jet"
//            << std::endl;
    }


  }//End of for (const auto& jet : jetsIn)

  maxRecoJetsMatched =
      std::max(maxRecoJetsMatched, jetMatched_all_.size());

  maxGenJetsMatched =
      std::max(maxGenJetsMatched, genJets.size());

//  std::cout << "Current maximum Reco jets = "
//            << maxRecoJetsMatched
//            << ", Gen jets = "
//            << maxGenJetsMatched
//            << std::endl;
/*
  if (jetMatched_all_.size() > 15 || genJets.size() > 15)
  {
    std::cout << "\n========== Matching Statistics ==========\n";
    std::cout << "Reco jets entering matching : "
            << jetMatched_all_.size() << std::endl;
    std::cout << "Gen jets entering matching  : "
            << genJets.size() << std::endl;
  }
*/
//Sort each RECO jet's candidate list by deltaR
for (auto& candList : recoCandidates)
{
    std::sort(
        candList.begin(),
        candList.end(),
        [](const MatchCandidate& a,
           const MatchCandidate& b)
        {
            return a.dR < b.dR;
        });
}

// Assignment maps
std::vector<int> assignedGenIndex(jetMatched_all_.size(), -1);
std::vector<int> assignedRecoIndex(genJets.size(), -1);
std::vector<float> assignedDR(jetMatched_all_.size(), -1.f);

// Cost matrix used for global one-to-one assignment.
// costMatrix[iGen][iReco] = ΔR if the pair is allowed,
// otherwise a very large value.
// Placeholder for future minimum-cost matching.
//std::vector<std::vector<float>> costMatrix(
//    genJets.size(),
//    std::vector<float>(jetMatched_all_.size(), 1e9f));

//Perform the one RECO-to-one GEN assignment:
//Greedy minimum deltaR assignment.
//Uses the alternative candidates that It'has been storing.
for (size_t iReco = 0; iReco < recoCandidates.size(); ++iReco)
{
    for (const auto& cand : recoCandidates[iReco])
    {
        if (assignedRecoIndex[cand.genIndex] != -1)
            continue;

        assignedGenIndex[iReco] = cand.genIndex;
        assignedRecoIndex[cand.genIndex] = iReco;
        assignedDR[iReco] = cand.dR;

        jetMatched_all_[iReco] = 1;
        genJetMatched_all_[cand.genIndex] = 1;

        break;
    }
}//End of for (size_t iReco = 0; iReco < recoCandidates.size(); ++iReco)

//The new classification loop
const float dR_HS = 0.2f;
const float dR_PU = 0.4f;

jetIsHS_all_.clear();
jetIsPU_all_.clear();
jetIsAmbiguous_all_.clear();

totalRecoJetsClean = 0;
totalPUJets = 0;

for (size_t iReco = 0; iReco < jetMatched_all_.size(); ++iReco)
{
    bool matchedHSJet = false;
    bool matchedPUJet = false;
    bool matchedAmbiguousJet = false;

    int iGen = assignedGenIndex[iReco];

    if (iGen == -1)
    {
        matchedPUJet = true;
    }
    else
    {   
      float dR = assignedDR[iReco];
        if (dR <= dR_HS)
            matchedHSJet = true;
        else if (dR >= dR_PU)
            matchedPUJet = true;
        else
            matchedAmbiguousJet = true;
    }


    if (matchedHSJet)
        ++totalRecoJetsClean;
    else if (matchedPUJet)
        ++totalPUJets;

    jetIsHS_all_.push_back(matchedHSJet);
    jetIsPU_all_.push_back(matchedPUJet);
    jetIsAmbiguous_all_.push_back(matchedAmbiguousJet);
}//End of for (size_t iReco = 0; iReco < jetMatched_all_.size(); ++iReco)

genEtaDen_all_.clear();
for (size_t iReco = 0; iReco < assignedGenIndex.size(); ++iReco)
{
    int iGen = assignedGenIndex[iReco];

    if (iGen < 0)
        continue;

    genPt_all_[iReco] = genJets[iGen].pt();

    genEta_all_[iReco] = std::abs(genJets[iGen].eta());

    jetDeltaR_all_[iReco] = assignedDR[iReco];

    if (genJets[iGen].pt() > 0.f)
    {
        jetResponse_all_[iReco] =
            jetPt_all_[iReco] / genJets[iGen].pt();
    }
}

genMatchedDen_all_.clear();


for (size_t iGen = 0; iGen < genJets.size(); ++iGen)
{
    if (genJets[iGen].pt() < 20.) continue;

    genEtaDen_all_.push_back(std::abs(genJets[iGen].eta()));

    genMatchedDen_all_.push_back(
        genJetMatched_all_[iGen]
    );   
}

//int nGenMatched = 0;

  //range-based for loop//
  //count the matches produced by the new one-to-one assignment.
  for (int matched : genJetMatched_all_)
  {
      if (matched)
          ++nGenMatched;//efficiency= matched GenJets/total GenJets
  }

//---Purity(Reco->Gen)--------
  for (int matched : jetMatched_all_)
  {
      if (matched)
          ++nMatchedReco;//purity,mistag
  }//End of for (int matched : jetMatched_all_)

  purity_out =
  (totalRecoJetsClean > 0)//condition
  ?
  float(nMatchedReco)/totalRecoJetsClean
  :
  -1.f;

  //----MisTag--------------------  
  mistag =
  (totalRecoJetsClean > 0)
  ?
  float(totalRecoJetsClean-nMatchedReco)
  /
  totalRecoJetsClean
  :
  -1.f;

  //--Efficiciency---
  efficiency_out =
      (totalGenJets > 0)
          ? float(nGenMatched) / totalGenJets
          : -1.f;

  //---PU jet fraction---
  puJetFraction_all_ =
  (totalRecoJetsClean+totalPUJets >0)
  ?
  float(totalPUJets)
  /
  (totalRecoJetsClean+totalPUJets)
  :
  -1.f;
};//end of auto processJetCollection = [&](){


auto processFastJetCollection = [&](const std::vector<fastjet::PseudoJet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,
                                std::vector<int>& jetMatched_all_,     // RECO → GEN match flag
                                std::vector<int>& genJetMatched_all_,  // GEN → RECO match flag
                                std::vector<int>& genMatchedDen_all_,  // GEN → RECO match flag

                                std::vector<int>& jetIsHS_all_,
                                std::vector<int>& jetIsPU_all_,
                                std::vector<int>& jetIsAmbiguous_all_,
                                std::vector<int>& jetIsUnknown_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetPhi_all_,
                                std::vector<std::vector<unsigned int>>& jet_pfIndices_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& genEta_all_,
                                std::vector<float>& genEtaDen_all_,
                                std::vector<float>& puFracPt_algo_all_,
                                std::vector<float>& puFracCount_algo_all_,
                                std::vector<float>& puFracPt_truth_all_,
                                std::vector<float>& puFracCount_truth_all_,
                                std::vector<float>& jetDeltaR_all_,
                                std::vector<std::vector<unsigned int>>& pf_indices_general_all_,//per-jet PF indices
                                int& totalRecoJetsClean,
                                int& totalPUJets,
                                int& totalGenJets,
                                int& nGenMatched,
                                int& nMatchedReco,
                                float& puJetFraction_all_,
                                float& efficiency_out,
                                float& effLossAmbig,
                                float& effWithAmbig,
                                float& mistag,
                                float& purity_out) {

//Build matchCandidates

//Greedy one-to-one assignment

//assignedGenIndex
//assignedRecoIndex
//assignedDR

//HS / PU classification

//Fill ntuple

//Initialize the vector and the denominator
totalRecoJetsClean = 0;
totalPUJets        = 0;
totalGenJets       = 0;
nGenMatched        = 0;
nMatchedReco       = 0;

puJetFraction_all_ = -1.f;
efficiency_out     = -1.f;
effLossAmbig       = -1.f;
effWithAmbig       = -1.f;
mistag             = -1.f;
purity_out         = -1.f;

static size_t maxRecoJetsMatched = 0;
static size_t maxGenJetsMatched = 0;
genJetMatched_all_.assign(genJets.size(), 0);
struct MatchCandidate
{
    int jetIndex;
    int genIndex;
    float dR;
};

std::vector<MatchCandidate> matchCandidates;
std::vector<std::vector<MatchCandidate>> recoCandidates;

for (const auto& genJet : genJets)
{
    if (genJet.pt() > 20.)
        ++totalGenJets;
}

genMatchedDen_all_.assign(genJets.size(), 0);
std::vector<int> genJetAlreadyUsed(genJets.size(),0);

recoCandidates.clear();
recoCandidates.resize(jetsIn.size());
for (const auto& jet : jetsIn)
{
    std::vector<unsigned int> pf_indices_this_jet;

    for (const auto& constituent : jet.constituents())
    {
        int idx = constituent.user_index();

        if (idx < 0 || idx >= static_cast<int>(pf_pt.size()))
            continue;

        pf_indices_this_jet.push_back(static_cast<unsigned int>(idx));
    }//End of for (const auto& constituent : jet.constituents())

    jet_pfIndices_all_.push_back(pf_indices_this_jet);
    pf_indices_general_all_.push_back(pf_indices_this_jet);

    jetPt_all_.push_back(jet.pt());
    jetAbsEta_all_.push_back(std::abs(jet.eta()));
    jetPhi_all_.push_back(jet.phi());
    int thisJetIndex = jetPt_all_.size() - 1;

    int nHS = 0;
    int nPU = 0;

    float sumPtHS = 0.f;
    float sumPtPU = 0.f;
    float sumPtUnknown = 0.f;

    float sumPtTot = 0.f;
    int nAlgoKeep = 0;
    int nAlgoReject = 0;

    float sumPtKeep = 0.f;
    float sumPtReject = 0.f;

    for (unsigned int idx : pf_indices_this_jet)
    {   if (idx >= pf_pt.size())
           continue;

        ++nAlgoKeep;
        sumPtKeep += pf_pt[idx];


        if (idx >= pf_pt.size())
            continue;

        sumPtTot += pf_pt[idx];

        if (pf_isHS_truth[idx] == 1)
        {
            ++nHS;
            sumPtHS += pf_pt[idx];
        }//End of if (pf_isHS_truth[idx] == 1)

        else if (pf_isPU_truth[idx] == 1)
        {
            ++nPU;
            sumPtPU += pf_pt[idx];
        }//End of if (pf_isPU_truth[idx] == 1)
        else
        {
            sumPtUnknown += pf_pt[idx];
        }
    }//End of for (unsigned int idx : pf_indices_this_jet), End of PF constituent loop

    // Compute jet-level quantities AFTER all constituents have been counted
    // puFracCount_truth and puFracPt_truth are jet-level quantities, so they must be computed once per jet,after the constituent loop finishes.
    float puFracCount_truth =
        (nHS + nPU > 0) ?
        float(nPU) / float(nHS + nPU) :
        -1.f;

    float puFracPt_truth =
        (sumPtTot > 0.f) ?
        sumPtPU / sumPtTot :
        -1.f;

//    std::cout
//    << "Jet summary:"
//    << " nPF=" << pf_indices_this_jet.size()
//    << " sumPtTot=" << sumPtTot
//    << " sumPtHS=" << sumPtHS
//    << " sumPtPU=" << sumPtPU
//    << " sumPtUnknown=" << sumPtUnknown
//    << std::endl;

    //--constituent-based HS/PU jet definition, Not generator-level definition---
    //constituentHSJet=HS fraction > 50%
    const float eps = 1e-6;
    bool isUnknownJet = (sumPtHS + sumPtPU < eps);
    bool constituentHSJet = (!isUnknownJet && sumPtHS >= sumPtPU);
//    bool constituentPUJet = (!isUnknownJet && !constituentHSJet);

//one-to-one nearest-neighbor matching need, where each GenJet can be matched to at most one RECO jet.

    //GEN loop:Find Matched  GENjet
    for (size_t iGen = 0; iGen < genJets.size(); ++iGen)
    {
        const auto& genJet = genJets[iGen];

        if (genJet.pt() < 20.) continue;

        float dR = reco::deltaR(jet.eta(), jet.phi(),
                            genJet.eta(), genJet.phi());
        //All valid RECO-GEN pairs are now stored
        if (dR < 1.0)
        {   //every RECO jet has its own candidate list
            recoCandidates[thisJetIndex].push_back(
            {
                thisJetIndex,
                static_cast<int>(iGen),
                dR
            });

        }//End of  if (dR < 1.0)

    }//for (size_t iGen = 0; iGen < genJets.size(); ++iGen)

//    jetDeltaR_all_.push_back(bestDR);
    jetDeltaR_all_.push_back(-1.f);


//    jetMatched_all_.push_back(bestGenJet ? 1 : 0);
     jetMatched_all_.push_back(0);

//    genPt_all_.push_back(bestGenJet ? bestGenJet->pt() : -1.f);
//    genEta_all_.push_back(bestGenJet ? std::abs(bestGenJet->eta()) : -1.f);
    genPt_all_.push_back(-1.f);
    genEta_all_.push_back(-1.f);

//    float response = -1.f;
//    if (bestGenJet && bestGenJet->pt() > 0.f)
//        response = jet.pt() / bestGenJet->pt();

//    jetResponse_all_.push_back(response);
    jetResponse_all_.push_back(-1.f);
    //----------------------------
    // RECO -> GEN:old one-to-many jet matching
    //----------------------------
     jetMatched_all_.back() = 0;
    //----------------------------
    // Gen Jet->RECO:old one-to-many jet  matching 
    //----------------------------

    // Store one value per jet
    jetIsUnknown_all_.push_back(isUnknownJet ? 1 : 0);
    puFracCount_truth_all_.push_back(puFracCount_truth);
    puFracPt_truth_all_.push_back(puFracPt_truth);

    if (isUnknownJet)
    {
//        std::cout
//            << "Unknown jet"
//            << std::endl;
    }


  }//End of for (const auto& jet : jetsIn)

  maxRecoJetsMatched =
      std::max(maxRecoJetsMatched, jetMatched_all_.size());

  maxGenJetsMatched =
      std::max(maxGenJetsMatched, genJets.size());

//  std::cout << "Current maximum Reco jets = "
//            << maxRecoJetsMatched
//            << ", Gen jets = "
//            << maxGenJetsMatched
//            << std::endl;
/*
  if (jetMatched_all_.size() > 15 || genJets.size() > 15)
  {
    std::cout << "\n========== Matching Statistics ==========\n";
    std::cout << "Reco jets entering matching : "
            << jetMatched_all_.size() << std::endl;
    std::cout << "Gen jets entering matching  : "
            << genJets.size() << std::endl;
  }
*/

//Sort each RECO jet's candidate list by deltaR
for (auto& candList : recoCandidates)
{
    std::sort(
        candList.begin(),
        candList.end(),
        [](const MatchCandidate& a,
           const MatchCandidate& b)
        {
            return a.dR < b.dR;
        });
}

// Assignment maps
std::vector<int> assignedGenIndex(jetMatched_all_.size(), -1);
std::vector<int> assignedRecoIndex(genJets.size(), -1);
std::vector<float> assignedDR(jetMatched_all_.size(), -1.f);

// Cost matrix used for global one-to-one assignment.
// costMatrix[iGen][iReco] = ΔR if the pair is allowed,
// otherwise a very large value.
// Placeholder for future minimum-cost matching.
//std::vector<std::vector<float>> costMatrix(
//    genJets.size(),
//    std::vector<float>(jetMatched_all_.size(), 1e9f));

//Perform the one RECO-to-one GEN assignment:
//Greedy minimum deltaR assignment.
//Uses the alternative candidates that It'has been storing.
for (size_t iReco = 0; iReco < recoCandidates.size(); ++iReco)
{
    for (const auto& cand : recoCandidates[iReco])
    {
        if (assignedRecoIndex[cand.genIndex] != -1)
            continue;

        assignedGenIndex[iReco] = cand.genIndex;
        assignedRecoIndex[cand.genIndex] = iReco;
        assignedDR[iReco] = cand.dR;

        jetMatched_all_[iReco] = 1;
        genJetMatched_all_[cand.genIndex] = 1;

        break;
    }
}//End of for (size_t iReco = 0; iReco < recoCandidates.size(); ++iReco)

//The new classification loop
const float dR_HS = 0.2f;
const float dR_PU = 0.4f;

jetIsHS_all_.clear();
jetIsPU_all_.clear();
jetIsAmbiguous_all_.clear();

totalRecoJetsClean = 0;
totalPUJets = 0;

for (size_t iReco = 0; iReco < jetMatched_all_.size(); ++iReco)
{
    bool matchedHSJet = false;
    bool matchedPUJet = false;
    bool matchedAmbiguousJet = false;

    int iGen = assignedGenIndex[iReco];

    if (iGen == -1)
    {
        matchedPUJet = true;
    }
    else
    {   
      float dR = assignedDR[iReco];
        if (dR <= dR_HS)
            matchedHSJet = true;
        else if (dR >= dR_PU)
            matchedPUJet = true;
        else
            matchedAmbiguousJet = true;
    }


    if (matchedHSJet)
        ++totalRecoJetsClean;
    else if (matchedPUJet)
        ++totalPUJets;

    jetIsHS_all_.push_back(matchedHSJet);
    jetIsPU_all_.push_back(matchedPUJet);
    jetIsAmbiguous_all_.push_back(matchedAmbiguousJet);
}//End of for (size_t iReco = 0; iReco < jetMatched_all_.size(); ++iReco)

genEtaDen_all_.clear();
for (size_t iReco = 0; iReco < assignedGenIndex.size(); ++iReco)
{
    int iGen = assignedGenIndex[iReco];

    if (iGen < 0)
        continue;

    genPt_all_[iReco] = genJets[iGen].pt();

    genEta_all_[iReco] = std::abs(genJets[iGen].eta());

    jetDeltaR_all_[iReco] = assignedDR[iReco];

    if (genJets[iGen].pt() > 0.f)
    {
        jetResponse_all_[iReco] =
            jetPt_all_[iReco] / genJets[iGen].pt();
    }
}

genMatchedDen_all_.clear();


for (size_t iGen = 0; iGen < genJets.size(); ++iGen)
{
    if (genJets[iGen].pt() < 20.) continue;

    genEtaDen_all_.push_back(std::abs(genJets[iGen].eta()));

    genMatchedDen_all_.push_back(
        genJetMatched_all_[iGen]
    );   
}

//int nGenMatched = 0;

  //range-based for loop//
  //count the matches produced by the new one-to-one assignment.
  for (int matched : genJetMatched_all_)
  {
      if (matched)
          ++nGenMatched;//efficiency= matched GenJets/total GenJets
  }

//---Purity(Reco->Gen)--------
  for (int matched : jetMatched_all_)
  {
      if (matched)
          ++nMatchedReco;//purity,mistag
  }//End of for (int matched : jetMatched_all_)

  purity_out =
  (totalRecoJetsClean > 0)//condition
  ?
  float(nMatchedReco)/totalRecoJetsClean
  :
  -1.f;

  //----MisTag--------------------  
  mistag =
  (totalRecoJetsClean > 0)
  ?
  float(totalRecoJetsClean-nMatchedReco)
  /
  totalRecoJetsClean
  :
  -1.f;

  //--Efficiciency---
  efficiency_out =
      (totalGenJets > 0)
          ? float(nGenMatched) / totalGenJets
          : -1.f;

  //---PU jet fraction---
  puJetFraction_all_ =
  (totalRecoJetsClean+totalPUJets >0)
  ?
  float(totalPUJets)
  /
  (totalRecoJetsClean+totalPUJets)
  :
  -1.f;

};//End of auto processFastJetCollection = [&]()


processJetCollection(*jets,  genJets, jetMatched_puppi_all_, genJetMatched_puppi_all_, genMatchedDen_puppi_all_,
    jetIsHS_puppi_all_,jetIsPU_puppi_all_,jetIsAmbiguous_puppi_all_,jetIsUnknown_puppi_all_, jetPt_puppi_all_, jetAbsEta_puppi_all_, jetPhi_puppi_all_, jet_pfIndices_puppi_all_, jetResponse_PR_puppi_all_,genPt_puppi_all_, genEta_puppi_all_, genEtaDen_puppi_all_,puFracPt_algo_puppi_all_, puFracCount_algo_puppi_all_, puFracPt_truth_puppi_all_, puFracCount_truth_puppi_all_,jetDeltaR_puppi_all_,pf_indices_puppi_all_,
    totalRecoJetsClean_puppi, totalPUJets_puppi,totalGenJets_puppi,nGenMatched_puppi,nMatchedReco_puppi, puJetFraction_puppi_all_,efficiency_puppi_, effLossAmbig_puppi_,effWithAmbig_puppi_,mistag_puppi_, purity_puppi_);


processFastJetCollection(pfrawJets,  genJets, jetMatched_pfraw_all_, genJetMatched_pfraw_all_, genMatchedDen_pfraw_all_,
    jetIsHS_pfraw_all_, jetIsPU_pfraw_all_,jetIsAmbiguous_pfraw_all_,jetIsUnknown_pfraw_all_,jetPt_pfraw_all_, jetAbsEta_pfraw_all_, jetPhi_pfraw_all_, jet_pfIndices_pfraw_all_, jetResponse_PR_pfraw_all_,genPt_pfraw_all_, genEta_pfraw_all_, genEtaDen_pfraw_all_, puFracPt_algo_pfraw_all_, puFracCount_algo_pfraw_all_,puFracPt_truth_pfraw_all_, puFracCount_truth_pfraw_all_,jetDeltaR_pfraw_all_,pf_indices_pfraw_all_,
    totalRecoJetsClean_pfraw, totalPUJets_pfraw,totalGenJets_pfraw,nGenMatched_pfraw,nMatchedReco_pfraw, puJetFraction_pfraw_all_, efficiency_pfraw_, effLossAmbig_pfraw_, effWithAmbig_pfraw_, mistag_pfraw_, purity_pfraw_);


processFastJetCollection(looseJets,  genJets, jetMatched_loose_all_, genJetMatched_loose_all_, genMatchedDen_loose_all_,
    jetIsHS_loose_all_,jetIsPU_loose_all_,jetIsAmbiguous_loose_all_,jetIsUnknown_loose_all_, jetPt_loose_all_, jetAbsEta_loose_all_,jetPhi_loose_all_, jet_pfIndices_loose_all_, jetResponse_PR_loose_all_,genPt_loose_all_,genEta_loose_all_, genEtaDen_loose_all_, puFracPt_algo_loose_all_, puFracCount_algo_loose_all_,puFracPt_truth_loose_all_, puFracCount_truth_loose_all_,jetDeltaR_loose_all_,pf_indices_loose_all_,
    totalRecoJetsClean_loose, totalPUJets_loose,totalGenJets_loose,nGenMatched_loose,nMatchedReco_loose, puJetFraction_loose_all_, efficiency_loose_, effLossAmbig_loose_, effWithAmbig_loose_, mistag_loose_,purity_loose_);


processFastJetCollection(tightJets,  genJets, jetMatched_tight_all_, genJetMatched_tight_all_, genMatchedDen_tight_all_,
    jetIsHS_tight_all_,jetIsPU_tight_all_,jetIsAmbiguous_tight_all_,jetIsUnknown_tight_all_,jetPt_tight_all_, jetAbsEta_tight_all_, jetPhi_tight_all_, jet_pfIndices_tight_all_, jetResponse_PR_tight_all_,genPt_tight_all_, genEta_tight_all_, genEtaDen_tight_all_, puFracPt_algo_tight_all_, puFracCount_algo_tight_all_,puFracPt_truth_tight_all_, puFracCount_truth_tight_all_,jetDeltaR_tight_all_,pf_indices_tight_all_,
    totalRecoJetsClean_tight, totalPUJets_tight,totalGenJets_tight,nGenMatched_tight,nMatchedReco_tight, puJetFraction_tight_all_, efficiency_tight_, effLossAmbig_tight_, effWithAmbig_tight_, mistag_tight_, purity_tight_);


processFastJetCollection(timeJets,  genJets, jetMatched_time_all_, genJetMatched_time_all_, genMatchedDen_time_all_,
    jetIsHS_time_all_,jetIsPU_time_all_,jetIsAmbiguous_time_all_,jetIsUnknown_time_all_, jetPt_time_all_, jetAbsEta_time_all_, jetPhi_time_all_, jet_pfIndices_time_all_, jetResponse_PR_time_all_,genPt_time_all_,genEta_time_all_, genEtaDen_time_all_, puFracPt_algo_time_all_, puFracCount_algo_time_all_,puFracPt_truth_time_all_, puFracCount_truth_time_all_, jetDeltaR_time_all_,pf_indices_time_all_,
    totalRecoJetsClean_time, totalPUJets_time, totalGenJets_time,nGenMatched_time,nMatchedReco_time,puJetFraction_time_all_, efficiency_time_, effLossAmbig_time_, effWithAmbig_time_, mistag_time_, purity_time_);


processFastJetCollection(timeJets4D,  genJets, jetMatched_time4D_all_, genJetMatched_time4D_all_, genMatchedDen_time4D_all_,
    jetIsHS_time4D_all_,jetIsPU_time4D_all_,jetIsAmbiguous_time4D_all_,jetIsUnknown_time4D_all_, jetPt_time4D_all_, jetAbsEta_time4D_all_, jetPhi_time4D_all_, jet_pfIndices_time4D_all_, jetResponse_PR_time4D_all_,genPt_time4D_all_, genEta_time4D_all_, genEtaDen_time4D_all_, puFracPt_algo_time4D_all_, puFracCount_algo_time4D_all_,puFracPt_truth_time4D_all_, puFracCount_truth_time4D_all_, jetDeltaR_time4D_all_,pf_indices_time4D_all_,
    totalRecoJetsClean_time4D, totalPUJets_time4D, totalGenJets_time4D,nGenMatched_time4D,nMatchedReco_time4D,puJetFraction_time4D_all_, efficiency_time4D_, effLossAmbig_time4D_, effWithAmbig_time4D_, mistag_time4D_,purity_time4D_);



//std::cout << "Valid timing   : " << nValid << std::endl;
//std::cout << "Invalid timing : " << nInvalid << std::endl;

double fracInvalid =
    100.0 * nInvalid / (nValid + nInvalid);

std::cout << "Invalid fraction = "
          << fracInvalid
          << " %" << std::endl;
//std::cout
//<< "Stored values : "
//<< " efficiency = " << efficiency_pfraw_
//<< " purity = " << purity_pfraw_
//<< " mistag = " << mistag_pfraw_
//<< " PUfrac = " << puJetFraction_pfraw_all_
//<< std::endl;
    
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

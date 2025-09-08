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
#include <typeinfo>
#define DEBUG_PUCONTENT

struct PrimaryVertex {
    float x;
    float y;
    float z;
    float t;
    float chi2;
    float ndof;
    int nTracks;
};

//GenJet matching + jet classification (ΔR + pT cut, clean HS/PU)
namespace {
  // Return best gen-jet match that passes both ΔR and pt cuts (or nullptr)
  inline const reco::GenJet* //returns a pointer to the best-matching gen jet (or nullptr if no match).
  bestGenMatch(const reco::Jet& j,//the reconstructed jet we want to match.
               const std::vector<reco::GenJet>& gens,//all gen jets in the event.
               double dRMax,//maximum allowed ΔR (cone size, e.g. 0.3).
               double minGenPt,//skip gen jets below this pT threshold
               double* outDR = nullptr,//optional output parameter, filled with the best ΔR found.
               int* outIdx = nullptr) //optional output parameter, index of the matched gen jet in the gens vector.
  {
    const reco::GenJet* best = nullptr;
    double bestDR = 1e9;
    int bestIdx = -1;

    const double eta = j.eta();
    const double phi = j.phi();

    for (size_t i = 0; i < gens.size(); ++i) {//Loop over all gen jets.
      const auto& g = gens[i];
      if (g.pt() <= minGenPt) continue;//Skip too-soft ones (g.pt() <= minGenPt).
      const double dR = reco::deltaR(eta, phi, g.eta(), g.phi());//Compute ΔR
      if (dR < dRMax && dR < bestDR) {//Keep the closest match within dRMax.
        bestDR = dR;
        best = &g;//return the pointer (or nullptr)of gen jet in the event
        bestIdx = static_cast<int>(i);//plus fill out outDR and outIdx if requested.
      }//End of if (dR < dRMax && dR < bestDR)
    }//End of for (size_t i = 0; i < gens.size(); ++i)

    if (outDR)  *outDR  = (best ? bestDR : -1.0);
    if (outIdx) *outIdx = bestIdx;
    return best;
  }//End of inline const reco::GenJet*  bestGenMatc()

  inline const reco::GenJet* //returns a pointer to the best-matching gen jet (or nullptr if no match).
  bestGenMatch(const fastjet::PseudoJet& j,//the reconstructed jet we want to match.
               const std::vector<reco::GenJet>& gens,//all gen jets in the event.
               double dRMax,//maximum allowed ΔR (cone size, e.g. 0.3).
               double minGenPt,//skip gen jets below this pT threshold
               double* outDR = nullptr,//optional output parameter, filled with the best ΔR found.
               int* outIdx = nullptr) //optional output parameter, index of the matched gen jet in the gens vector.
  {
    const reco::GenJet* best = nullptr;
    double bestDR = 1e9;
    int bestIdx = -1;

    const double eta = j.eta();
    const double phi = j.phi();

    for (size_t i = 0; i < gens.size(); ++i) {//Loop over all gen jets.
      const auto& g = gens[i];
      if (g.pt() <= minGenPt) continue;//Skip too-soft ones (g.pt() <= minGenPt).
      const double dR = reco::deltaR(eta, phi, g.eta(), g.phi());//Compute ΔR
      if (dR < dRMax && dR < bestDR) {//Keep the closest match within dRMax.
        bestDR = dR;
        best = &g;//return the pointer (or nullptr)of gen jet in the event
        bestIdx = static_cast<int>(i);//plus fill out outDR and outIdx if requested.
      }
    }

    if (outDR)  *outDR  = (best ? bestDR : -1.0);
    if (outIdx) *outIdx = bestIdx;
    return best;
  }//End of inline const reco::GenJet*  bestGenMatc()

  struct JetTruthMatch {//Just a container (like a small record) to hold matching results.
    bool   isHS=false;//whether the jet is a "hard-scatter" jet (true if matched to a gen jet).
    float  dR=-1.0;//ΔR distance to the matched gen jet (or -1 if none).
    float  genPt=-1.0;//matched gen jet’s pT (or -1).
    int    genIndex=-1;//index in the gens vector (or -1).
    const reco::GenJet* matchedGen=nullptr; 
  };

//This is a wrapper function that uses 'bestGenMatch' and fills a 'JetTruthMatch struct'.
  inline JetTruthMatch
  classifyJetHS(const reco::Jet& j,//the reco jet.
                const std::vector<reco::GenJet>& gens,//the gen jets.
                double dRMax = 0.3,//threshold
                double minGenPt = 10.0)//threshold
  {
    double dR = -1.0;
    int idx = -1;
    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
    JetTruthMatch out;//It builds a JetTruthMatch out strut
    out.isHS     = (g != nullptr);//if matched to some gen jet, then it’s "hard-scatter".
    out.dR       = static_cast<float>(dR);//best ΔR or -1.
    out.genPt    = (g ? g->pt() : -1.0f);//if valid, else -1.
    out.genIndex = idx;// index in gens vector,Indicate the matched gen jet positon in the vector
    out.matchedGen      =  g;//pointer to actual GenJet
    return out;//Returns the JetTruthMatch struct.
  }//It just call .classifyJetHS' and get a neat struct with all info

  inline JetTruthMatch
  classifyJetHS(const fastjet::PseudoJet& j,//the PseudoJet.
                const std::vector<reco::GenJet>& gens,//the gen jets.
                double dRMax = 0.3,//threshold
                double minGenPt = 10.0)//threshold
  {
    double dR = -1.0;
    int idx = -1;
    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
    JetTruthMatch out;//It builds a JetTruthMatch out strut
    out.isHS     = (g != nullptr);//if matched to some gen jet, then it’s "hard-scatter".
    out.dR       = static_cast<float>(dR);//best ΔR or -1.
    out.genPt    = (g ? g->pt() : -1.0f);//if valid, else -1.
    out.genIndex = idx;//from the loop.
    out.matchedGen      =  g;
    return out;//Returns the JetTruthMatch struct.
  }//It just call .classifyJetHS' and get a neat struct with all info
}//End of namespace

//Constituent-level PU fractions (MiniAOD-friendly, reco-level)
//Helper: classify a PF as PU (reco-level)
//On MiniAOD you typically don’t have full pileup truth for each GenParticle. The most stable way to get PU-content per jet is reco-level vertex association for charged constituents, and timing/PUPPI heuristics for neutrals.
namespace {
  // Quality cut values you can tune
  constexpr float kMaxDz      = 0.03;  // cm
  constexpr float kMaxDzSig   = 2.0;   // unitless
  constexpr float kTimeNSigma = 3.0;   // |(t - tPV)/σ|

  inline bool isChargedPU(const pat::PackedCandidate &pf)
  {
    // Charged hadrons, electrons, muons with a track
    if (pf.charge() == 0) return false;

  // conservative charged-PU test using available API:
  // - use fromPV() which returns an int (3 means "from PV", 2 ambiguous, etc.)
  // - use dz() rather than dzSig() if dzSig() is not available

  const int kMaxDzAbs = 0.5; // tune as needed (was using dzSig limit before)
  const int fromPV = pf.fromPV(); // 3 is highest quality (fromPV==3 => from PV)
  const bool goodPVQual = (fromPV == 3 || fromPV == 2); // treat 2 as acceptable (adjust if needed)

  // some candidates have undefined dz; treat large dz as PU
  const bool hasDz = (!std::isnan(pf.dz()) && std::abs(pf.dz()) < 1e6);
  const bool goodDz = hasDz ? (std::abs(pf.dz()) < kMaxDzAbs) : false;

  // charge check (charged candidates only)
  if (pf.charge() == 0) return false;
  // final heuristic: require either good PV association or small dz
  return !(goodPVQual || goodDz);

  }//End of inline bool isChargedPU(const pat::PackedCandidate &pf)

  inline bool isNeutralPU(const pat::PackedCandidate& pf, float pvTime, bool useTimingFallbackPuppi)
  {
    if (pf.charge() != 0) return false;

    // If timing is present, use timing significance w.r.t PV
    // NOTE: accessors depend on your stored branches; change to your names/getters.
    const bool hasTime    = (pf.timeError() > 0.0f);
    if (hasTime) {
      const float tSig = std::abs((pf.time() - pvTime) / pf.timeError());
      if (tSig > kTimeNSigma) return true;          // likely PU
      else return false;                            // likely HS
    }

    if (useTimingFallbackPuppi) {
      // Fallback heuristic: very small PUPPI weight → likely PU neutral
      // (threshold can be tuned; 0.1 is common as a "very soft/PU" proxy)
      if (pf.puppiWeight() < 0.1f && pf.pt() < 5.0f) return true;
    }

    // Without timing or good proxy, we can't confidently tag neutral as PU
    return false;
  }//End of inline bool isNeutralPU(const pat::PackedCandidate& pf, float pvTime, bool useTimingFallbackPuppi)

  struct PUContentReco {//A data container that defines to summarize PU content[count fractions (nPU/nTot),pt fractions (sumPtPU/sumPtTot)] at the jet level
    int   nTot = 0;//Total number of PF candidates in this jet (all constituents)
    int   nPU  = 0;//Number of PF candidates identified as pileup.
    float sumPtTot = 0.f;//Sum of the transverse momentum of all PF candidates in this jet.
    float sumPtPU  = 0.f;//Sum of the transverse momentum of only the PU PF candidates.
  };//End of struct PUContentReco 
}//End of namespace

// Common helper: update PUContentReco given one PF candidate
inline void updatePUContentReco(const pat::PackedCandidate* pf,
                                PUContentReco& acc,
                                float pvTime,
                                bool useTimingFallbackPuppi)
{
  if (!pf) {
   #ifdef DEBUG_PUCONTENT
    std::cout << "[PUContentReco] Skipped null PF candidate" << std::endl;
   #endif 
   return;
  }

  // Update totals
  ++acc.nTot;
  acc.sumPtTot += pf->pt();

  // PU classification
  bool isPU = (pf->charge() != 0)
                ? isChargedPU(*pf)
                : isNeutralPU(*pf, pvTime, useTimingFallbackPuppi);

  if (isPU) {
    ++acc.nPU;
    acc.sumPtPU += pf->pt();
    #ifdef DEBUG_PUCONTENT
        std::cout << "[PUContentReco] PU PF pt=" << pf->pt()
                  << " eta=" << pf->eta()
                  << " phi=" << pf->phi()
                  << " charge=" << pf->charge()
                  << std::endl;
    #endif
  }
}//End of void updatePUContentReco

//Function: compute PU fractions for one jet
PUContentReco computePUContentRecoJet(//A function that returns PUContentRec
    const pat::Jet& jet,//the reco jet whose PF composition we are studying.
    const std::vector<const pat::PackedCandidate*>& /*pfAll*/,//the full PF candidate collection by index
    float pvTime,//the primary-vertex time, needed for neutral timing PU ID.
    bool useTimingFallbackPuppi = true)//if true, neutrals with missing timing fall back to Puppi weight classification.
{
  PUContentReco acc;//Start an empty accumulator struct.

  const size_t nConst = jet.numberOfDaughters();
  for (size_t i = 0; i < nConst; ++i) {//Loop over all PF constituents of this jet.
    const auto& c = *jet.daughter(i);


    // Recover PackedCandidate pointer via sourceCandidatePtr
    const pat::PackedCandidate* pf = nullptr;
      if (c.numberOfSourceCandidatePtrs() > 0) {
        auto ptr = c.sourceCandidatePtr(0);
        pf = dynamic_cast<const pat::PackedCandidate*>(ptr.get());
      }//If no index is available, try to backtrack to the PF candidate pointer.

    #ifdef DEBUG_PUCONTENT
    if (!pf) {
      std::cout << "[PUContentRecoJet] Failed to recover PF from daughter "
                << i << " of jet pt=" << jet.pt()
                << " eta=" << jet.eta() << std::endl;
    }
    #endif

    updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
    }
  return acc;//he function returns
}//End of PUContentReco computePUContentRecoJet()

// ----------------------------------------------------------------------
//Differences from PUContentReco computePUContentRecoJet(),your pat::Jet version:
//Uses jet.constituents() instead of numberOfDaughters() / daughter(i).
//Relies exclusively on user_index (pf_coll_index) to map back to PF candidates.
//No need for sourceCandidatePtr fallback, since fastjet::PseudoJet doesn’t carry that.
// Compute PU content for a fastjet::PseudoJet assuming each constituent
// PseudoJet carries pf_coll_index in user_index() that indexes into pfAll.



PUContentReco computePUContentFastJet(
    const fastjet::PseudoJet& jet,
    const std::vector<const pat::PackedCandidate*>& pfAll,
    float pvTime,
    bool useTimingFallbackPuppi = true)
{
    PUContentReco acc; // zeros by default

    for (const auto &c : jet.constituents()) {
        int idx = c.user_index(); // pf_coll_index used when the PseudoJet was created
        if (idx < 0 || idx >= static_cast<int>(pfAll.size())){ 

        #ifdef DEBUG_PUCONTENT
        std::cout << "[PUContentFastJet] Invalid index=" << idx
                  << " for jet pt=" << jet.pt()
                  << " eta=" << jet.eta() << std::endl;
        #endif
         continue;
        }

        const pat::PackedCandidate* pf = pfAll[idx];
        updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
      }
      return acc;
}//End of PUContentReco computePUContentFastJet()

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
  
  TTree* tree_;

// For jet-level matching diagnostics
std::vector<float> jetDeltaR_puppi_all_;
std::vector<float> jetDeltaR_pfraw_all_;
std::vector<float> jetDeltaR_tight_all_;
std::vector<float> jetDeltaR_loose_all_;
std::vector<float> jetDeltaR_MTD_all_;
std::vector<float> jetTimeSig_MTD_all__;

// For jet-level efficiency & purity per collection
float efficiency_puppi_, purity_puppi_;
float efficiency_pfraw_, purity_pfraw_;//defined but not used
float efficiency_MTDJets_, purity_MTDJets_;//defined but not used
float efficiency_tight_, purity_tight_,efficiency_loose_, purity_loose_;

// Event-level PU jet counts
int nPUJets_puppi_, nRecoJets_puppi_;
int nPUJets_pfraw_, nRecoJets_pfraw_;
int nPUJets_tight_, nRecoJets_tight_;
int nPUJets_loose_, nRecoJets_loose_;
int nPUJets_MTD_, nRecoJets_MTD_;

// For jet-level timing (MTD jets)
  std::vector<float> jetTimeSig_MTD_all_;   // (tjet - PVt)/σ_tjet


  std::vector<int> jetIsHS_puppi_all_;
  std::vector<float> jetPt_puppi_all_;
  std::vector<float> jetAbsEta_puppi_all_;
  std::vector<float> jetResponse_PR_puppi_all_;
  std::vector<float> genPt_puppi_all_;
  std::vector<float> puFracPt_puppi_all_;
  std::vector<float> puFracCount_puppi_all_;
  std::vector<float> puJetFraction_puppi_all_;

/*
  std::vector<float> jetPt_puppi_5leading_;
  std::vector<float> jetAbsEta_puppi_5leading_;
  std::vector<float> jetResponse_PR_puppi_5leading_;
  std::vector<float> genPt_puppi_5leading_;
  std::vector<float> puFracPt_puppi_5leading_;
  std::vector<float> puFracCount_puppi_5leading_;
  std::vector<float> puJetFraction_puppi_5leading_;
*/

//  float jetResponse_ULv15;
  std::vector<int> jetIsHS_pfraw_all_;
  std::vector<float> jetPt_pfraw_all_;
  std::vector<float> genPt_pfraw_all_;
  std::vector<float> jetResponse_PR_pfraw_all_;
  std::vector<float> jetAbsEta_pfraw_all_;
  std::vector<float> puFracPt_pfraw_all_;
  std::vector<float> puFracCount_pfraw_all_;
  std::vector<float> puJetFraction_pfraw_all_;
/*
  std::vector<float> jetPt_pfraw_5leading_;
  std::vector<float> genPt_pfraw_5leading_;
  std::vector<float> jetResponse_PR_pfraw_5leading_;
  std::vector<float> jetAbsEta_pfraw_5leading_;
  std::vector<float> puFracPt_pfraw_5leading_;
  std::vector<float> puFracCount_pfraw_5leading_;
  std::vector<float> puJetFraction_pfraw_5leading_;
*/
  std::vector<int> jetIsHS_tight_all_;
  std::vector<float> jetPt_tight_all_;
  std::vector<float> genPt_tight_all_;
  std::vector<float> jetResponse_PR_tight_all_;
  std::vector<float> jetAbsEta_tight_all_;
  std::vector<float> puFracPt_tight_all_;
  std::vector<float> puFracCount_tight_all_;
  std::vector<float> puJetFraction_tight_all_;

/* 
  std::vector<float> jetPt_tight_5leading_;
  std::vector<float> genPt_tight_5leading_;
  std::vector<float> jetResponse_PR_tight_5leading_;
  std::vector<float> jetAbsEta_tight_5leading_;
  std::vector<float> puFracPt_tight_5leading_;
  std::vector<float> puFracCount_tight_5leading_;
  std::vector<float> puJetFraction_tight_5leading_;
*/

  std::vector<int> jetIsHS_loose_all_;
  std::vector<float> jetPt_loose_all_;
  std::vector<float> genPt_loose_all_;
  std::vector<float> jetResponse_PR_loose_all_;
  std::vector<float> jetAbsEta_loose_all_;
  std::vector<float> puFracPt_loose_all_;
  std::vector<float> puFracCount_loose_all_;
  std::vector<float> puJetFraction_loose_all_;

/*  
  std::vector<float> jetPt_loose_5leading_;
  std::vector<float> genPt_loose_5leading_;
  std::vector<float> jetResponse_PR_loose_5leading_;
  std::vector<float> jetAbsEta_loose_5leading_;
  std::vector<float> puFracPt_loose_5leading_;
  std::vector<float> puFracCount_loose_5leading_;
  std::vector<float> puJetFraction_loose_5leading_;
*/

  //For MTD-based jet clustering
  std::vector<int> jetIsHS_MTD_all_;
  std::vector<float> jetPt_MTD_all_;
  std::vector<float> genPt_MTD_all_;
  std::vector<float> jetResponse_PR_MTD_all_;
  std::vector<float> jetAbsEta_MTD_all_;
  std::vector<float> puFracPt_MTD_all_;
  std::vector<float> puFracCount_MTD_all_;
  std::vector<float> puJetFraction_MTD_all_;
  std::vector<float> jetTime_MTD_all_;
  std::vector<float> jetTimeError_MTD_all_;

/*
  std::vector<float> jetPt_MTD_5leading_;
  std::vector<float> genPt_MTD_5leading_;
  std::vector<float> jetResponse_PR_MTD_5leading_;
  std::vector<float> jetAbsEta_MTD_5leading_;
  std::vector<float> puFracPt_MTD_5leading_;
  std::vector<float> puFracCount_MTD_5leading_;
  spf_vertextd::vector<float> puJetFraction_MTD_5leading_;
  std::vector<float> jetTime_MTD_5leading_;
  std::vector<float> jetTimeError_MTD_5leading_;
*/

  std::vector<float> pf_vertex,pf_pt, pf_eta, pf_phi, pf_energy;
  std::vector<float> pf_charge,pf_puppiWeight,pf_puppiWeightNoLep;//For <pat::PackedCandidate> collection
  std::vector<float> pf_dxy, pf_dz, pf_dzError, pf_dzSig;
  std::vector<float> pf_time, pf_timeError, pf_dtSig;//For <pat::PackedCandidate> collection
  std::vector<float> pf_vx, pf_vy, pf_vz;

  std::vector<int> pf_pdgId;
  std::vector<int> pf_fromPV;          // Store fromPV() intege
  std::vector<unsigned char> pf_passesTightCut;//was bool
  std::vector<unsigned char> pf_passesLooseCut;//was bool
  std::vector<unsigned char> pf_keepAlways;//was bool
  std::vector<unsigned char> pf_keepDisplaced;//was bool
  std::vector<unsigned char> pf_isInMTD;//was bool
  std::vector<unsigned char> pf_isCharged;//was bool
  std::vector<unsigned char> pf_hasValidTime;//was bool
  std::vector<int> pf_isHS; // 1 = HS, 0 = PU, -1 = unmatched
  std::vector<std::vector<unsigned int>> pf_indices_MTD_, pf_indices_tight_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_loose_, pf_indices_pfraw_;//For <pat::PackedCandidate> collection
  std::vector<float> genparticles_z_;// For generated particles z-position

//  std::vector<PrimaryVertex> primaryVertices;

  float pt_, eta_, phi_, mass_;//For jet collection
//  std::vector<PFParticle> pfparticles;//For PackedCandidate
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_,pvs_TimeErr_; // For primary vertex position
  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
  float genvertex_z_;
//  float pf_vx, pf_vy, pf_vz;//For <pat::PackedCandidate> collection
//  int pf_pdgId,pf_isTimeValid;//For <pat::PackedCandidate> collection

//  std::vector<float> puFrac_pfraw_, puFrac_tight_, puFrac_loose_,puFrac_MTD_;;
// add to class members (inside class definition)

      
  int totalRecoJets = 0;
  int totalPUJets = 0;
  float puJetFraction_all = 0;

  int totalRecoJets_puppi = 0; 
  int totalPUJets_puppi = 0; 
  float  puJetFraction_puppi_all= 0;

  int totalRecoJets_pfraw = 0; 
  int totalPUJets_pfraw = 0; 
  float puJetFraction_pfraw_all =0;

  int totalRecoJets_tight = 0; 
  int totalPUJets_tight = 0; 
  float puJetFraction_tight_all= 0;

  int totalRecoJets_loose = 0 ;
  int totalPUJets_loose = 0 ; 
  float puJetFraction_loose_all =0; 

  int totalRecoJets_MTD = 0; 
  int totalPUJets_MTD = 0; 
  float puJetFraction_MTD_all =0;
};


JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig)//:
//	: doAllPFParticles(iConfig.getParameter<bool>("doAllPFParticles"))
{
  jetsToken_ = consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jetTag"));
  genJetToken_  = consumes<std::vector<reco::GenJet>>(iConfig.getParameter<edm::InputTag>("genJetsTag"));
  // Initialize new tokens for packedCandida/te primary vertex, beam spot, and generated particles
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
  // ΔR branches
  tree_->Branch("jetDeltaR_puppi_all", &jetDeltaR_puppi_all_);
  tree_->Branch("jetDeltaR_pfraw_all", &jetDeltaR_pfraw_all_);
  tree_->Branch("jetDeltaR_tight_all", &jetDeltaR_tight_all_);
  tree_->Branch("jetDeltaR_loose_all", &jetDeltaR_loose_all_);
  tree_->Branch("jetDeltaR_MTD_all",   &jetDeltaR_MTD_all_);
  tree_->Branch("jetTimeSig_MTD_all",   &jetTimeSig_MTD_all_);

  // Efficiency & purity
  tree_->Branch("efficiency_puppi", &efficiency_puppi_, "efficiency_puppi/F");
  tree_->Branch("purity_puppi",     &purity_puppi_,     "purity_puppi/F");
  tree_->Branch("efficiency_pfraw", &efficiency_pfraw_, "efficiency_pfraw/F");
  tree_->Branch("purity_pfraw",     &purity_pfraw_,     "purity_pfraw/F");
  tree_->Branch("efficiency_MTDJets", &efficiency_MTDJets_, "efficiency_MTDJets/F");
  tree_->Branch("purity_MTDJets",     &purity_MTDJets_,     "purity_MTDJets/F");

  // PU jet counts
  tree_->Branch("nPUJets_puppi",   &nPUJets_puppi_,   "nPUJets_puppi/I");
  tree_->Branch("nRecoJets_puppi", &nRecoJets_puppi_, "nRecoJets_puppi/I");
  tree_->Branch("nPUJets_pfraw",   &nPUJets_pfraw_,   "nPUJets_pfraw/I");
  tree_->Branch("nRecoJets_pfraw", &nRecoJets_pfraw_, "nRecoJets_pfraw/I");
  tree_->Branch("nPUJets_tight",   &nPUJets_tight_,   "nPUJets_tight/I");
  tree_->Branch("nRecoJets_tight", &nRecoJets_tight_, "nRecoJets_tight/I");
  tree_->Branch("nPUJets_loose",   &nPUJets_loose_,   "nPUJets_loose/I");
  tree_->Branch("nRecoJets_loose", &nRecoJets_loose_, "nRecoJets_loose/I");
  tree_->Branch("nPUJets_MTD",     &nPUJets_MTD_,     "nPUJets_MTD/I");
  tree_->Branch("nRecoJets_MTD",   &nRecoJets_MTD_,   "nRecoJets_MTD/I");

  // Jet timing
  tree_->Branch("jetTimeSig_MTD_all", &jetTimeSig_MTD_all_);

  tree_->Branch("jetIsHS_puppi_all", &jetIsHS_puppi_all_);
  tree_->Branch("jetPt_puppi_all", &jetPt_puppi_all_);
  tree_->Branch("genPt_puppi_all", &genPt_puppi_all_);
  tree_->Branch("jetResponse_PR_puppi_all", &jetResponse_PR_puppi_all_);
  tree_->Branch("jetAbsEta_puppi_all", &jetAbsEta_puppi_all_);  
  tree_->Branch("puFracPt_puppi_all", &puFracPt_puppi_all_);
  tree_->Branch("puFracCount_puppi_all", &puFracCount_puppi_all_);  
  tree_->Branch("puJetFraction_puppi_all", &puJetFraction_puppi_all_);  
/*  
  tree_->Branch("jetPt_puppi_5leading", &jetPt_puppi_5leading_);
  tree_->Branch("genPt_puppi_5leading", &genPt_puppi_5leading_);
  tree_->Branch("jetResponse_puppi_5leading", &jetResponse_PR_puppi_5leading_);
  tree_->Branch("jetAbsEta_puppi_5leading", &jetAbsEta_puppi_5leading_);  
  tree_->Branch("puFracPt_puppi_5leading", &puFracPt_puppi_5leading_);
  tree_->Branch("puFracCount_puppi_5leading", &puFracCount_puppi_5leading_);  
  tree_->Branch("puJetFraction_puppi_5leading", &puJetFraction_puppi_5leading_);  
*/
  
  tree_->Branch("jetIsHS_pfraw_all", &jetIsHS_pfraw_all_);
  tree_->Branch("jetResponse_PR_pfraw_all", &jetResponse_PR_pfraw_all_);
  tree_->Branch("jetPt_pfraw_all", &jetPt_pfraw_all_);
  tree_->Branch("genPt_pfraw_all", &genPt_pfraw_all_);
  tree_->Branch("jetAbsEta_pfraw_all", &jetAbsEta_pfraw_all_); 
  tree_->Branch("puFracPt_pfraw_all", &puFracPt_pfraw_all_);
  tree_->Branch("puFracCount_pfraw_all", &puFracCount_pfraw_all_);  
  tree_->Branch("puJetFraction_pfraw_all", &puJetFraction_pfraw_all_);  
/* 
  tree_->Branch("jetResponse_PR_pfraw_5leading", &jetResponse_PR_pfraw_5leading_);
  tree_->Branch("jetPt_pfraw_5leading", &jetPt_pfraw_5leading_);
  tree_->Branch("genPt_pfraw_5leading", &genPt_pfraw_5leading_);
  tree_->Branch("jetAbsEta_pfraw_5leading", &jetAbsEta_pfraw_5leading_); 
  tree_->Branch("puFracPt_pfraw_5leading", &puFracPt_pfraw_5leading_);
  tree_->Branch("puFracCount_pfraw_5leading", &puFracCount_pfraw_5leading_);  
  tree_->Branch("puJetFraction_pfraw_5leading", &puJetFraction_pfraw_5leading_);  
*/

  tree_->Branch("jetIsHS_tight_all", &jetIsHS_tight_all_);
  tree_->Branch("jetPt_tight_all", &jetPt_tight_all_);
  tree_->Branch("genPt_tight_all", &genPt_tight_all_);
  tree_->Branch("jetResponse_PR_tight_all", &jetResponse_PR_tight_all_);
  tree_->Branch("jetAbsEta_tight_all", &jetAbsEta_tight_all_);
  tree_->Branch("puFracPt_tight_all", &puFracPt_tight_all_);
  tree_->Branch("puFracCount_tight_all", &puFracCount_tight_all_);  
  tree_->Branch("puJetFraction_tight_all", &puJetFraction_tight_all_);  

/*  
  tree_->Branch("jetPt_tight_5leading", &jetPt_tight_5leading_);
  tree_->Branch("genPt_tight_5leading", &genPt_tight_5leading_);
  tree_->Branch("jetResponse_PR_tight_5leading", &jetResponse_PR_tight_5leading_);
  tree_->Branch("jetAbsEta_tight_5leading", &jetAbsEta_tight_5leading_);
  tree_->Branch("puFracPt_tight_5leading", &puFracPt_tight_5leading_);
  tree_->Branch("puFracCount_tight_5leading", &puFracCount_tight_5leading_);  
  tree_->Branch("puJetFraction_tight_5leading", &puJetFraction_tight_5leading_);  
*/

  tree_->Branch("jetIsHS_loose_all", &jetIsHS_loose_all_);
  tree_->Branch("jetResponse_PR_loose_all", &jetResponse_PR_loose_all_);
  tree_->Branch("jetAbsEta_loose_all", &jetAbsEta_loose_all_);
  tree_->Branch("jetPt_loose_all", &jetPt_loose_all_);
  tree_->Branch("genPt_loose_all", &genPt_loose_all_);
  tree_->Branch("puFracPt_loose_all", &puFracPt_loose_all_);
  tree_->Branch("puFracCount_loose_all", &puFracCount_loose_all_);  
  tree_->Branch("puJetFraction_loose_all", &puJetFraction_loose_all_);  

/*
  tree_->Branch("jetResponse_PR_loose_5leading", &jetResponse_PR_loose_5leading_);
  tree_->Branch("jetAbsEta_loose_5leading", &jetAbsEta_loose_5leading_);
  tree_->Branch("jetPt_loose_5leading", &jetPt_loose_5leading_);
  tree_->Branch("genPt_loose_5leading", &genPt_loose_5leading_);
  tree_->Branch("puFracPt_loose_5leading", &puFracPt_tight_5leading_);
  tree_->Branch("puFracCount_loose_5leading", &puFracCount_tight_5leading_);  
  tree_->Branch("puJetFraction_loose_5leading", &puJetFraction_tight_5leading_);  
*/

  tree_->Branch("jetIsHS_MTD_all", &jetIsHS_MTD_all_);
  tree_->Branch("jetResponse_PR_MTD_all", &jetResponse_PR_MTD_all_);
  tree_->Branch("jetAbsEta_MTD_all", &jetAbsEta_MTD_all_);
  tree_->Branch("jetPt_MTD_all", &jetPt_MTD_all_);
  tree_->Branch("genPt_MTD_all", &genPt_MTD_all_);
  tree_->Branch("jetTime_MTD_all", &jetTime_MTD_all_);
  tree_->Branch("jetTimeError_MTD_all", &jetTimeError_MTD_all_);
  tree_->Branch("puFracPt_MTD_all", &puFracPt_MTD_all_);
  tree_->Branch("puFracCount_MTD_all", &puFracCount_MTD_all_);  
  tree_->Branch("puJetFraction_MTD_all", &puJetFraction_MTD_all_);  

/*
  tree_->Branch("jetResponse_PR_MTD_5leading", &jetResponse_PR_MTD_5leading_);
  tree_->Branch("jetAbsEta_MTD_5leading", &jetAbsEta_MTD_5leading_);
  tree_->Branch("jetPt_MTD_5leading", &jetPt_MTD_5leading_);
  tree_->Branch("genPt_MTD_5leading", &genPt_MTD_5leading_);
  tree_->Branch("jetTime_MTD_5leading", &jetTime_MTD_5leading_);
  tree_->Branch("jetTimeError_MTD_5leading", &jetTimeError_MTD_5leading_);
  tree_->Branch("puFracPt_MTD_5leading", &puFracPt_MTD_5leading_);
  tree_->Branch("puFracCount_MTD_5leading", &puFracCount_MTD_5leading_);  
  tree_->Branch("puJetFraction_MTD_5leading", &puJetFraction_MTD_5leading_);  
*/

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
  tree_->Branch("pvs_TimeErr", &pvs_TimeErr_, "pvs_TimeErr/F");
  // Branches for beam spot
  tree_->Branch("beamspot_x", &beamspot_x_, "beamspot_x/F");
  tree_->Branch("beamspot_y", &beamspot_y_, "beamspot_y/F");
  tree_->Branch("beamspot_z", &beamspot_z_, "beamspot_z/F");
  // Branches for generated particles
  tree_->Branch("genparticles_z", &genparticles_z_);
  tree_->Branch("genvertex_z", &genvertex_z_, "genvertex_z/F");

   // Branches for <pat::PackedCandidate> collection 
  tree_->Branch("pf_vertex", &pf_vertex);
  tree_->Branch("pf_pt", &pf_pt);
  tree_->Branch("pf_eta", &pf_eta);
  tree_->Branch("pf_phi", &pf_phi);
  tree_->Branch("pf_energy", &pf_energy);
  tree_->Branch("pf_charge", &pf_charge);
  tree_->Branch("pf_puppiWeight", &pf_puppiWeight);
  tree_->Branch("pf_puppiWeightNoLep", &pf_puppiWeightNoLep);
  tree_->Branch("pf_dxy", &pf_dxy);
  tree_->Branch("pf_dz", &pf_dz);
  tree_->Branch("pf_dzError", &pf_dzError);
  tree_->Branch("pf_dzSig", &pf_dzSig);
  tree_->Branch("pf_time", &pf_time);
  tree_->Branch("pf_timeError", &pf_timeError);
  tree_->Branch("pf_dtSig", &pf_dtSig);

  tree_->Branch("pf_isInMTD", &pf_isInMTD);
  tree_->Branch("pf_isCharged", &pf_isCharged);
  tree_->Branch("pf_hasValidTime", &pf_hasValidTime);

  tree_->Branch("pf_passesTightCut", &pf_passesTightCut);
  tree_->Branch("pf_passesLooseCut", &pf_passesLooseCut);
  tree_->Branch("pf_keepAlways", &pf_keepAlways);
  tree_->Branch("pf_keepDisplaced",  &pf_keepDisplaced);

  tree_->Branch("pf_vx", &pf_vx);
  tree_->Branch("pf_vy", &pf_vy);
  tree_->Branch("pf_vz", &pf_vz);

  tree_->Branch("pf_pdgId", &pf_pdgId);  
  tree_->Branch("pf_isHS", &pf_isHS);
  tree_->Branch("pf_fromPV", &pf_fromPV);
  tree_->Branch("efficiency_tight", &efficiency_tight_,"efficiency_tight/F");
  tree_->Branch("purity_tight", &purity_tight_,"purity_tight/F");
  tree_->Branch("efficiency_loose", &efficiency_loose_,"efficiency_loose/F");
  tree_->Branch("purity_loose", &purity_loose_,"purity_loose/F");
  tree_->Branch("pf_indices_pfraw", &pf_indices_pfraw_);
  tree_->Branch("pf_indices_tight", &pf_indices_tight_);
  tree_->Branch("pf_indices_loose", &pf_indices_loose_);
  tree_->Branch("pf_indices_MTD", &pf_indices_MTD_);
  
//  tree_->Branch("puFrac_pfraw", &puFrac_pfraw_);
//  tree_->Branch("puFrac_tight", &puFrac_tight_);
//  tree_->Branch("puFrac_loose", &puFrac_loose_);
//  tree_->Branch("puFrac_MTD", &puFrac_MTD_);

}//End of void JetTreeProducer::beginJob()

// ===================
//  Helper Functions
// ===================
// ======================================================
// Helper 0: Extract PF indices from pat::Jet
// ======================================================
inline std::vector<unsigned int>
getPFIndicesFromPatJet(const pat::Jet& jet) {
    std::vector<unsigned int> pf_indices_this_jet;
    pf_indices_this_jet.reserve(jet.numberOfDaughters());

    for (const auto& candPtr : jet.getJetConstituents()) {
        // Try casting to PackedCandidate
        const auto* pfcand = dynamic_cast<const pat::PackedCandidate*>(candPtr.get());
        if (!pfcand) continue;

        // Use key() as PF index (aligned with event-level PF collection)
        unsigned int pfIdx = candPtr.key();
        pf_indices_this_jet.push_back(pfIdx);

        // If you have a custom userInt index stored earlier:
        // unsigned int pfIdx = pfcand->userInt("pfIdx");
    }

    return pf_indices_this_jet;
}

// ======================================================
// Helper 00: Extract PF indices from fastjet::PseudoJet
// ======================================================
inline std::vector<unsigned int> getPFIndicesFromPseudoJet(
    const fastjet::PseudoJet& jet,
    const std::vector<const pat::PackedCandidate*>& pfAll)
{
    std::vector<unsigned int> out;
    std::vector<fastjet::PseudoJet> consts = jet.constituents();

    for (const auto &c : consts) {
        int idx = c.user_index();
        if (idx < 0 || static_cast<size_t>(idx) >= pfAll.size()) {
            edm::LogWarning("getPFIndicesFromPseudoJet")
            << "Invalid user_index=" << idx
            << " pfAll.size()=" << pfAll.size();
            continue;//skip this constituent instead of dereferencing
        }
        const pat::PackedCandidate* pf = pfAll[idx];
        out.push_back(static_cast<unsigned int>(idx));
    }
    return out;
}//End of getPFIndicesFromPseudoJet()

//---Helper Function1:  PF candidate  ↔   GenParticle matching---
//Find the generator-level particle that this specific PF candidate came from.It works at the constituent particle level, not jet level.
//Adjust minDR or relax the pdgId requirement if needed.
const reco::GenParticle* matchToGen(const pat::PackedCandidate& pf,
                                    const std::vector<reco::GenParticle>& genParticles) {
  float minDR = 0.05; // tighter matching for individual particles
  const reco::GenParticle* bestMatch = nullptr;

  for (const auto& gen : genParticles) {
    // Require same particle type (optional: comment out if you want looser matching)
    if (std::abs(pf.pdgId()) != std::abs(gen.pdgId())) continue;

    float dR = reco::deltaR(pf.eta(), pf.phi(), gen.eta(), gen.phi());
    if (dR < minDR) {
      minDR = dR;
      bestMatch = &gen;
    }
  }
  return bestMatch;
}


// --- Helper Function3: Jet ↔ GenJet Jetresponse calcultation---
inline float computeResponse(const pat::Jet& recoJet,
                              const reco::GenJet* matchedGenJet) {
    if (!matchedGenJet || matchedGenJet->pt() <= 0) return -1.0f;
    return recoJet.pt() / matchedGenJet->pt();
}

inline float computeResponse(const fastjet::PseudoJet& fjJet,
                              const reco::GenJet* matchedGenJet) {
    if (!matchedGenJet || matchedGenJet->pt() <= 0) return -1.0f;
    return fjJet.pt() / matchedGenJet->pt();
}


void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
//  pfparticles.clear();

  pf_vertex.clear();
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

  pf_isHS.clear();
  pf_fromPV.clear();
  pf_isCharged.clear();
  pf_hasValidTime.clear();
 
  pf_indices_pfraw_.clear();
  pf_indices_tight_.clear();
  pf_indices_loose_.clear();
  pf_indices_MTD_.clear();
  pf_passesTightCut.clear();
  pf_passesLooseCut.clear();
  pf_keepAlways.clear();
  pf_keepDisplaced.clear();
  
  jetDeltaR_puppi_all_.clear();
  jetDeltaR_pfraw_all_.clear();
  jetDeltaR_tight_all_.clear();
  jetDeltaR_loose_all_.clear();
  jetDeltaR_MTD_all_.clear();
  jetTimeSig_MTD_all_.clear();

  efficiency_puppi_ = purity_puppi_ = -1;//defined but not used
  efficiency_pfraw_ = purity_pfraw_ = -1;//defined but not used
  efficiency_MTDJets_ = purity_MTDJets_ = -1;

  nPUJets_puppi_ = nRecoJets_puppi_ = 0;
  nPUJets_pfraw_ = nRecoJets_pfraw_ = 0;
  nPUJets_tight_ = nRecoJets_tight_ = 0;
  nPUJets_loose_ = nRecoJets_loose_ = 0;
  nPUJets_MTD_   = nRecoJets_MTD_   = 0;

  jetIsHS_puppi_all_.clear();
  jetPt_puppi_all_.clear();
  genPt_puppi_all_.clear();
  jetResponse_PR_puppi_all_.clear();
  jetAbsEta_puppi_all_.clear(); 
  puFracPt_puppi_all_.clear(), 
  puFracCount_puppi_all_.clear();
  puJetFraction_puppi_all_.clear();

/*  
  jetPt_puppi_5leading_.clear();
  genPt_puppi_5leading_.clear();
  jetResponse_PR_puppi_5leading_.clear();
  jetAbsEta_puppi_5leading_.clear(); 
  puFracPt_puppi_5leading_.clear(), 
  puFracCount_puppi_5leading_.clear();
  puJetFraction_puppi_5leading_.clear();
*/

  jetIsHS_pfraw_all_.clear();
  jetPt_pfraw_all_.clear();
  genPt_pfraw_all_.clear();
  jetResponse_PR_pfraw_all_.clear();
  jetAbsEta_pfraw_all_.clear();
  puFracPt_pfraw_all_.clear(), 
  puFracCount_pfraw_all_.clear();
  puJetFraction_pfraw_all_.clear();

/*
  jetPt_pfraw_5leading_.clear();
  genPt_pfraw_5leading_.clear();
  jetResponse_PR_pfraw_5leading_.clear();
  jetAbsEta_pfraw_5leading_.clear();
  puFracPt_pfraw_5leading_.clear(), 
  puFracCount_pfraw_5leading_.clear();
  puJetFraction_pfraw_5leading_.clear();
*/

  jetIsHS_tight_all_.clear();
  jetPt_tight_all_.clear();
  genPt_tight_all_.clear();
  jetResponse_PR_tight_all_.clear();
  jetAbsEta_tight_all_.clear();
  puFracPt_tight_all_.clear(), 
  puFracCount_tight_all_.clear();
  puJetFraction_tight_all_.clear();

/*
  jetPt_tight_5leading_.clear();
  genPt_tight_5leading_.clear();
  jetResponse_PR_tight_5leading_.clear();
  jetAbsEta_tight_5leading_.clear();
  puFracPt_tight_5leading_.clear(), 
  puFracCount_tight_5leading_.clear();
  puJetFraction_tight_5leading_.clear();
*/

  
  jetIsHS_loose_all_.clear();
  jetPt_loose_all_.clear();
  genPt_loose_all_.clear();
  jetResponse_PR_loose_all_.clear();
  jetAbsEta_loose_all_.clear();
  puFracPt_loose_all_.clear(), 
  puFracCount_loose_all_.clear();
  puJetFraction_loose_all_.clear();

/*  
  jetPt_loose_5leading_.clear();
  genPt_loose_5leading_.clear();
  jetResponse_PR_loose_5leading_.clear();
  jetAbsEta_loose_5leading_.clear();
  puFracPt_loose_5leading_.clear(), 
  puFracCount_loose_5leading_.clear();
  puJetFraction_loose_5leading_.clear();
*/

  jetIsHS_MTD_all_.clear();
  jetPt_MTD_all_.clear();
  genPt_MTD_all_.clear();
  jetResponse_PR_MTD_all_.clear();
  jetAbsEta_MTD_all_.clear();
  jetTime_MTD_all_.clear();
  jetTimeError_MTD_all_.clear();
  puFracPt_MTD_all_.clear(), 
  puFracCount_MTD_all_.clear();
  puJetFraction_MTD_all_.clear();

/*
  jetPt_MTD_5leading_.clear();
  genPt_MTD_5leading_.clear();
  jetResponse_PR_MTD_5leading_.clear();
  jetAbsEta_MTD_5leading_.clear();
  jetTime_MTD_5leading_.clear();
  jetTimeError_MTD_5leading_.clear();
  puFracPt_MTD_5leading_.clear(), 
  puFracCount_MTD_5leading_.clear();
  puJetFraction_MTD_5leading_.clear();
*/


  int N_HS_total = 0;
//  int N_selected = 0;
  int N_selected_tight = 0;
  int N_selected_HS_tight = 0;
  int N_selected_loose = 0;
  int N_selected_HS_loose = 0;
  int N_charged = 0;
  int N_neutral = 0; 

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

  // â Early event rejection if gen vertex is valid and reco_pvs(primary vertices) is not empty
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


  // Fill PackedCandidates
  std::vector<fastjet::PseudoJet> fjInputs_raw;
  std::vector<fastjet::PseudoJet> fjInputs_tight;
  std::vector<fastjet::PseudoJet> fjInputs_loose;
  std::vector<fastjet::PseudoJet> fjInputs_MTD;
  std::vector<const pat::PackedCandidate*> pf_for_MTD;
  std::vector<const pat::PackedCandidate*> pf_for_allCollections;//?  Is it necessary for me?
  pf_for_allCollections.reserve(pf_coll.size());
//  primaryVertices.clear();
  
  //---PF Particle Loop---
  int pf_coll_index = 0;
  for (const auto& pf : pf_coll) {
    pf_for_allCollections.push_back(&pf);
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
    
    float eta = pf.eta(); 
    bool isInMTD = (std::abs(eta) <= 3.0);
    const bool isCharged = (pf.charge() != 0);
    const bool isNeutral = (pf.charge() == 0);
    const int fromPV = pf.fromPV();
    const bool keepAlways = (fromPV == 3);
    pf_isCharged.push_back(isCharged ? 1u : 0u);

    // inside PF candidate loop, before any use:
    float dtSig = 999.0f;               // sentinel (invalid)
    const float dtSigCut = 3.0f;        // tune this threshold (was missing)
 
    bool isDisplaced = false;
    bool hasValidTime = false;
    bool hasTimingInfo = false;

    // Common PseudoJet for dz-based clustering
    fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
    int pf_coll_index = &pf - &pf_coll[0]; // index relative to pf_coll
    pj.set_user_index(pf_coll_index);//link PF index,this one used for tight/loose only, Not used for MTD  
    ++pf_coll_index;
//    pj.set_user_index(pfIndex);
//    pj.set_user_info(new MyUserInfo(pfIndex));// optional richer info
    fjInputs_raw.push_back(pj);//all PFS, no cut (for control)


    if (pf.hasTrackDetails() && isCharged) {
       ++N_charged;
      bool isHS = (pf.fromPV() > 1);  // fromPV: 3 = tightly associated to PV
      if (isHS) ++N_HS_total;

      float dz     = pf.dz();
      float dzErr  = pf.dzError();
      float dzSig  = (dzErr > 0) ? dz / dzErr : 999;
      
      pf_dxy.push_back(pf.dxy());
      pf_dz.push_back(dz);
      pf_dzError.push_back(dzErr);
      pf_dzSig.push_back(dzSig);  
 
      // compute only if timing is valid:
[[maybe_unused]]      bool hasTimingInfo = false;

      float t = pf.time();//nanoseconds
      float tErr = pf.timeError();//nanoseconds
      
      // Treat as "valid" only if error is positive and not too large
      pf_isInMTD.push_back(isInMTD);      
//      bool isDisplaced = (std::abs(dz) > 0.05);//To keep displaced tracks
      isDisplaced = (std::abs(dz) > 0.05);//To keep displaced tracks


      // === Robust validity check ===
[[maybe_unused]]      hasTimingInfo = (tErr > 0 && tErr < 0.2 && std::abs(t) < 100);
      hasValidTime = (
          isInMTD &&        //Ensures only trust time info(in MTD acceptance)
          tErr > 0 &&
          tErr < 0.2 &&     // Conservative threshold (30â50 ps typical)
          std::abs(t) < 100 // sanity check: time should be < 100 ns
      );
      pf_hasValidTime.push_back(hasValidTime ? 1u : 0u);//
     

      if (hasValidTime) {
      //Safe to use time
        float dt = t - pvs_t_;
       // float dtSig = (tErr > 0) ? dt / tErr : 999;
        float dtSig = dt / std::sqrt(tErr*tErr + pvs_TimeErr_*pvs_TimeErr_);//

        pf_time.push_back(pf.time());
        pf_timeError.push_back(pf.timeError());
        pf_dtSig.push_back(dtSig);
  
        // Optional: log debug info
//        edm::LogVerbatim("JetTreeProducer::TimeDebug") 
//          << "PF with eta=" << eta_ << ", pt=" << pt 
//          << " has time=" << t << " ns, error=" << timeErr 
//          << " ns, dtSig=" << dtSig;
      } else {
        // Invalid or missing time
        pf_time.push_back(-999);
        pf_timeError.push_back(999);
        pf_dtSig.push_back(999);

//        edm::LogVerbatim("JetTreeProducer::TimeDebug")
//          << "PF with eta=" << eta_ << ", pt=" << pt 
//          << " has INVALID time (time=" << t
//          << ", error=" << tErr << ")";
      }//end of if (hasValidTime) else 

      //dz:mean -3.1 X 10(-6), sigma:0.010cm
      //dzSig: mean -2.1X 10 (-6), sigma: 0.073 
      float dzCut=0.05;     
      float dzSigCut=0.5;     
      bool passesTightCut = (std::abs(dz) < 0.03 && dzSig < 0.2);//3D selection
      bool passesLooseCut = (std::abs(dz) < 0.05 && dzSig < 0.5);//3D selection
      bool keepDisplaced = (isDisplaced && hasValidTime);//4D selection
      bool passes3D = (std::abs(dz) < dzCut) && (std::abs(dzSig) < dzSigCut);
      bool passes4D = passes3D && hasValidTime && (std::abs(dtSig) < dtSigCut);      

      pf_passesTightCut.push_back(passesTightCut ? 1u : 0u);
      pf_passesLooseCut.push_back(passesLooseCut ? 1u : 0u);
      pf_keepAlways.push_back(keepAlways ? 1u : 0u);
      pf_keepDisplaced.push_back(keepDisplaced ? 1u : 0u);


      if (keepAlways || passesTightCut) {
//      if (keepAlways || passesTightCut || keepDisplaced &&(hasValidTime && pf.timeError() < 0.2)) {//To save track has large dz,4D selection
        fjInputs_tight.push_back(pj);
        ++N_selected_tight;
        if (isHS) ++N_selected_HS_tight;
      }
      if (keepAlways || passesLooseCut) {
//      if (keepAlways || passesLooseCut || keepDisplaced &&(hasValidTime && pf.timeError() < 0.2)) {//To save track has large dz, 4D selection
        fjInputs_loose.push_back(pj);
        ++N_selected_loose;
        if (isHS) ++N_selected_HS_loose;
      }
        // MTD-based selection
//      if (pf.isTimeValid() && pf.timeError() < 0.05) {//pat::PackedCandidate does not have a method called .isTimeValid()
      if (hasValidTime && pf.timeError() < 0.2) {
      
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
      pf_isInMTD.push_back(false);
       
      //Neutral candidate -no dz/dzSig
      //CMSSW_15_1_0_pre4/Optionally:include all neutrals in tight/loose
      ++N_neutral;
      fjInputs_tight.push_back(pj);
      fjInputs_loose.push_back(pj);
    }//End of if (hasValidTime && pf.timeError() < 0.2) else 


      //outside the MTD geometry &seem to have time info:Indicate a reconstruction or simulation artifact.
//      if (!isInMTD && hasTimingInfo) {
      if (hasValidTime) {
//        edm::LogWarning("TimingCheck")
//        << "PF with eta = " << eta
//        << " has timing info (t = " << t << " ns, error = " << tErr << " ns)"
//        << " outside MTD acceptance!";
      }//end of if (!isInMTD && hasTimingInfo)
//    ++pf_coll_index;
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

//Jet-level MTD time
for (const auto& jet : mtdJets) {
    float sumPt = 0, sumWeightedT = 0, sumVar = 0;
    for (const auto& c : jet.constituents()) {
        int idx = c.user_index();
        if (idx < 0 || (size_t)idx >= pf_for_MTD.size()) continue;
        const pat::PackedCandidate* pf = pf_for_allCollections[idx];
//        const auto* pf = pf_for_MTD[idx];
        if (!pf) continue;
        if (pf->timeError() <= 0 || pf->timeError() > 0.2) continue;
        float w = pf->pt();
        sumPt += w;
        sumWeightedT += w * pf->time();
        sumVar += w*w * (pf->timeError()*pf->timeError());
    }
    float jetT = (sumPt>0) ? sumWeightedT/sumPt : -999;
    float jetTerr = (sumPt>0) ? std::sqrt(sumVar)/sumPt : 999;

    // Save
    jetTime_MTD_all_.push_back(jetT);
    jetTimeError_MTD_all_.push_back(jetTerr);

    // Jet time significance
    float jetTsig = (jetTerr>0) ? (jetT - pvs_t_)/jetTerr : 999;
    jetTimeSig_MTD_all_.push_back(jetTsig);
}


//===== Generic std::vector<pat::Jet> Loop Template=====
auto processJetCollection = [&](const std::vector<pat::Jet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,

                                std::vector<int> jetIsHS_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& puFracPt_all_,
                                std::vector<float>& puFracCount_all_,
                                std::vector<float>& jetDeltaR_all_,
/*
                                std::vector<float>& jetPt_5leading_,
                                std::vector<float>& jetAbsEta_5leading_,
                                std::vector<float>& jetResponse_5leading_,
                                std::vector<float>& genPt_5leading_,
                                std::vector<float>& puFracPt_5leading_,
                                std::vector<float>& puFracCount_5leading_,
*/
                                int& totalRecoJets,
                                int& totalPUJets,
                                float& puJetFraction_all) {
    int idx = 0;
    totalRecoJets = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;
        nMatchedReco = totalRecoJets - totalPUJets;

    // Compute nGenJetsHS with the same minGenPt you use above
    int nGenJetsHS = 0; // apply pt>10 cut here
     for (const auto& g : genJets) if (g.pt() > 10.0) nGenJetsHS++;

    for (const auto& jet : jetsIn) {
        totalRecoJets++;

        // Get PF indices for this jet
        std::vector<unsigned int> pf_indices_this_jet =  getPFIndicesFromPatJet(jet);
         pf_indices_tight_.push_back(pf_indices_this_jet);

        // Jet ↔ GenJet matching
//        const reco::GenJet* matchedGenJet = matchGenJet(jet, genJets,0.3);
         const auto truth = classifyJetHS(jet, genJets, /*dRMax=*/0.3, /*minGenPt=*/10.0);

//          auto truth = classifyJetHS(jet, genJets, 0.3); // truth is JetTruthMatch
//          const reco::GenJet* matchedGenJet = truth.gen; // or truth.matchedGen (use correct field)
            const reco::GenJet* matchedGenJet = truth.matchedGen;          
           
[[maybe_unused]]           float minDR = truth.dR; // example: distance saved in the struct
           bool isHSjet = truth.isHS;
           bool isPUjet = !isHSjet;
//        bool isHSjet = (matchedGenJet && matchedGenJet->pt() > 10.0); // threshold
//        bool isPUJet = (matchedGenJet == nullptr);
        if (matchedGenJet) nMatchedReco++;
     
        // --- NEW: store ΔR to matched gen jet
        minDR = (matchedGenJet) ?
        reco::deltaR(jet.eta(), jet.phi(), matchedGenJet->eta(), matchedGenJet->phi()) : -1;
//        jetDeltaR_all_.push_back(minDR);  // use the correct vector per collection
        jetDeltaR_all_.push_back(truth.dR);  // use the correct vector per collection

        // HS/PU counting (strict)
        if (!truth.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, matchedGenJet);

        // PU content
//        PUContent pu = computePUContent(jet, pf_pt, pf_isHS);
        PUContentReco pu = computePUContentRecoJet(jet, pf_for_allCollections, pvs_t_, /*useTimingFallbackPuppi=*/true);

        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        genPt_all_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
        jetResponse_all_.push_back(response);
        jetIsHS_all_.push_back(truth.isHS ? 1 : 0);
//        puFracCount_all_.push_back(pu.fracCount);
//        puFracPt_all_.push_back(pu.fracPt);

       // Example: pfAll is the vector of pointers you already have in memory.
       // Adjust to your actual container.
       /// This gives you PU fraction by count and by pT for each jet, in a way that works on MiniAOD (no need for full pileup GenParticles).
//Charged classification is robust (PV association). Neutral classification uses timing when available, falling back to a conservative PUPPI weight heuristic.

       const float puFracCount = (pu.nTot > 0)       ? float(pu.nPU)     / pu.nTot     : -1.f;
       const float puFracPt    = (pu.sumPtTot > 0.f) ? pu.sumPtPU        / pu.sumPtTot : -1.f;

       // Save to your existing branches for the corresponding collection:
//       puFracCount_currentCollection_.push_back(puFracCount);
//       puFracPt_currentCollection_.push_back(puFracPt);
         puFracCount_all_.push_back(puFracCount);
         puFracPt_all_.push_back(puFracPt);
       

/*
        // Save LEADING 5 jets
        if (idx < 5) {
            jetPt_5leading_.push_back(jet.pt());
            jetAbsEta_5leading_.push_back(std::abs(jet.eta()));
            genPt_5leading_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
            jetResponse_5leading_.push_back(response);
            puFracCount_5leading_.push_back(pu.fracCount);
            puFracPt_5leading_.push_back(pu.fracPt);
        }
*/
        idx++;
    }

    puJetFraction_all = (totalRecoJets > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJets
        : -1.0f;
   
//    Jet efficiency/purity: Count HS vs PU jets explicitly 
    const int nRecoJetsHS = totalRecoJets - totalPUJets;
    const float efficiency = (nGenJetsHS > 0)     ? float(nRecoJetsHS)/nGenJetsHS : -1.0f;
    const float purity     = (totalRecoJets > 0)   ? float(nRecoJetsHS)/totalRecoJets : -1.0f;

[[maybe_unused]]    float efficiency_out = -1;
[[maybe_unused]]    float purity_out = -1;
[[maybe_unused]]    int nPUJets_out = 0;
[[maybe_unused]]    int nRecoJets_out = 0;

    // Assign to the right collection’s variables
[[maybe_unused]]    efficiency_out = efficiency;
[[maybe_unused]]    purity_out     = purity;
[[maybe_unused]]    nPUJets_out    = totalPUJets;
[[maybe_unused]]    nRecoJets_out  = totalRecoJets;
};



//===== Generic std::vector<pat::Jet> Loop Template=====
auto processFastJetCollection = [&](const std::vector<fastjet::PseudoJet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,

                                std::vector<int> jetIsHS_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& puFracPt_all_,
                                std::vector<float>& puFracCount_all_,
                                std::vector<float>& jetDeltaR_all_,
/*
                                std::vector<float>& jetPt_5leading_,
                                std::vector<float>& jetAbsEta_5leading_,
                                std::vector<float>& jetResponse_5leading_,
                                std::vector<float>& genPt_5leading_,
                                std::vector<float>& puFracPt_5leading_,
                                std::vector<float>& puFracCount_5leading_,
*/
                                int& totalRecoJets,
                                int& totalPUJets,
                                float& puJetFraction_all) {
    int idx = 0;
    totalRecoJets = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;
        nMatchedReco = totalRecoJets - totalPUJets;

    // Compute nGenJetsHS with the same minGenPt you use above
    int nGenJetsHS = 0; // apply pt>10 cut here
     for (const auto& g : genJets) if (g.pt() > 10.0) nGenJetsHS++;

    for (const auto& jet : jetsIn) {
        totalRecoJets++;

        // Get PF indices for this jet
//        std::vector<unsigned int> pf_indices_this_jet =  getPFIndicesFromPseudoJet(jet);
        auto pf_indices_this_jet = getPFIndicesFromPseudoJet(jet, pf_for_allCollections);

         pf_indices_tight_.push_back(pf_indices_this_jet);

        // Jet ↔ GenJet matching
//        const reco::GenJet* matchedGenJet = matchGenJet(jet, genJets,0.3);
         const auto truth = classifyJetHS(jet, genJets, /*dRMax=*/0.3, /*minGenPt=*/10.0);
//          auto truth = classifyJetHS(jet, genJets, 0.3); // truth is JetTruthMatch
//          const reco::GenJet* matchedGenJet = truth.gen; // or truth.matchedGen (use correct field)
            const reco::GenJet* matchedGenJet = truth.matchedGen;          
           
[[maybe_unused]]           float minDR = truth.dR; // example: distance saved in the struct
           bool isHSjet = truth.isHS;
           bool isPUjet = !isHSjet;
//        bool isHSjet = (matchedGenJet && matchedGenJet->pt() > 10.0); // threshold
//        bool isPUJet = (matchedGenJet == nullptr);
        if (matchedGenJet) nMatchedReco++;
     
        // --- NEW: store ΔR to matched gen jet
        minDR = (matchedGenJet) ?
        reco::deltaR(jet.eta(), jet.phi(), matchedGenJet->eta(), matchedGenJet->phi()) : -1;
//        jetDeltaR_all_.push_back(minDR);  // use the correct vector per collection
        jetDeltaR_all_.push_back(truth.dR);  // use the correct vector per collection

        // HS/PU counting (strict)
        if (!truth.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, matchedGenJet);

        // PU content
//        PUContent pu = computePUContent(jet, pf_pt, pf_isHS);
        PUContentReco pu = computePUContentFastJet(jet, pf_for_allCollections, pvs_t_, /*useTimingFallbackPuppi=*/true);

        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        genPt_all_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
        jetResponse_all_.push_back(response);
        jetIsHS_all_.push_back(truth.isHS ? 1 : 0);
//        puFracCount_all_.push_back(pu.fracCount);
//        puFracPt_all_.push_back(pu.fracPt);

       // Example: pfAll is the vector of pointers you already have in memory.
       // Adjust to your actual container.
       /// This gives you PU fraction by count and by pT for each jet, in a way that works on MiniAOD (no need for full pileup GenParticles).
//Charged classification is robust (PV association). Neutral classification uses timing when available, falling back to a conservative PUPPI weight heuristic.

       const float puFracCount = (pu.nTot > 0)       ? float(pu.nPU)     / pu.nTot     : -1.f;
       const float puFracPt    = (pu.sumPtTot > 0.f) ? pu.sumPtPU        / pu.sumPtTot : -1.f;

       // Save to your existing branches for the corresponding collection:
//       puFracCount_currentCollection_.push_back(puFracCount);
//       puFracPt_currentCollection_.push_back(puFracPt);
        puFracCount_all_.push_back(puFracCount);
         puFracPt_all_.push_back(puFracPt);
       

/*
        // Save LEADING 5 jets
        if (idx < 5) {
            jetPt_5leading_.push_back(jet.pt());
            jetAbsEta_5leading_.push_back(std::abs(jet.eta()));
            genPt_5leading_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
            jetResponse_5leading_.push_back(response);
            puFracCount_5leading_.push_back(pu.fracCount);
            puFracPt_5leading_.push_back(pu.fracPt);
        }
*/
        idx++;
    }

    puJetFraction_all = (totalRecoJets > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJets
        : -1.0f;
   
//    Jet efficiency/purity: Count HS vs PU jets explicitly 
    const int nRecoJetsHS = totalRecoJets - totalPUJets;
    const float efficiency = (nGenJetsHS > 0)     ? float(nRecoJetsHS)/nGenJetsHS : -1.0f;
    const float purity     = (totalRecoJets > 0)   ? float(nRecoJetsHS)/totalRecoJets : -1.0f;

[[maybe_unused]]    float efficiency_out = -1;
[[maybe_unused]]    float purity_out = -1;
[[maybe_unused]]    int nPUJets_out = 0;
[[maybe_unused]]    int nRecoJets_out = 0;

    // Assign to the right collection’s variables
[[maybe_unused]]    efficiency_out = efficiency;
[[maybe_unused]]    purity_out     = purity;
[[maybe_unused]]    nPUJets_out    = totalPUJets;
[[maybe_unused]]    nRecoJets_out  = totalRecoJets;
};


processJetCollection(*jets,  *genJets, 
	jetIsHS_puppi_all_, jetPt_puppi_all_, jetAbsEta_puppi_all_, jetResponse_PR_puppi_all_,genPt_puppi_all_, puFracPt_puppi_all_, puFracCount_puppi_all_,jetDeltaR_puppi_all_,
//	jetPt_puppi_5leading_, jetAbsEta_puppi_5leading_, jetResponse_PR_puppi_5leading_, genPt_puppi_5leading_, puFracPt_puppi_5leading_, puFracCount_puppi_5leading_,
	totalRecoJets_puppi, totalPUJets_puppi, puJetFraction_puppi_all);

processFastJetCollection(pfrawJets,  *genJets, 
	jetIsHS_pfraw_all_, jetPt_pfraw_all_, jetAbsEta_pfraw_all_, jetResponse_PR_pfraw_all_,genPt_pfraw_all_, puFracPt_pfraw_all_, puFracCount_pfraw_all_,jetDeltaR_pfraw_all_,
//	jetPt_pfraw_5leading_, jetAbsEta_pfraw_5leading_, jetResponse_PR_pfraw_5leading_, genPt_pfraw_5leading_, puFracPt_pfraw_5leading_, puFracCount_pfraw_5leading_,
	totalRecoJets_pfraw, totalPUJets_pfraw, puJetFraction_pfraw_all);

processFastJetCollection(tightJets,  *genJets, 
	jetIsHS_tight_all_,jetPt_tight_all_, jetAbsEta_tight_all_, jetResponse_PR_tight_all_,genPt_tight_all_, puFracPt_tight_all_, puFracCount_tight_all_,jetDeltaR_tight_all_,
//	jetPt_tight_5leading_, jetAbsEta_tight_5leading_, jetResponse_PR_tight_5leading_, genPt_tight_5leading_, puFracPt_tight_5leading_, puFracCount_tight_5leading_,
	totalRecoJets_tight, totalPUJets_tight, puJetFraction_tight_all);


processFastJetCollection(looseJets,  *genJets,
	jetIsHS_loose_all_, jetPt_loose_all_, jetAbsEta_loose_all_, jetResponse_PR_loose_all_,genPt_loose_all_, puFracPt_loose_all_, puFracCount_loose_all_,jetDeltaR_loose_all_,
//	jetPt_loose_5leading_, jetAbsEta_loose_5leading_, jetResponse_PR_loose_5leading_, genPt_loose_5leading_, puFracPt_loose_5leading_, puFracCount_loose_5leading_,
	totalRecoJets_loose, totalPUJets_loose, puJetFraction_loose_all);

/*
processFastJetCollection(mtdJets,  *genJets, 
	jetIsHS_MTD_all_, jetPt_MTD_all_, jetAbsEta_MTD_all_, jetResponse_PR_MTD_all_,genPt_MTD_all_, puFracPt_MTD_all_, puFracCount_MTD_all_, jetDeltaR_MTD_all_,
//	jetPt_MTD_5leading_, jetAbsEta_MTD_5leading_, jetResponse_PR_MTD_5leading_, genPt_MTD_5leading_, puFracPt_MTD_5leading_, puFracCount_MTD_5leading_,
	totalRecoJets_MTD, totalPUJets_MTD, puJetFraction_MTD_all);
*/

//  jetResponse_PR_tight_ = -1;
//  jetResponse_PR_loose_ = -1;
//  jetAbsEta_ = -1; 

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
    pvs_TimeErr_=firstPV.tError();
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
edm::LogPrint("JetTreeProducer") 
     << "Filling event with "
     << pf_keepAlways.size() << " PFs";

edm::LogWarning("particle check")
    << "charged particles = " << N_charged
    << ", neutral particles = " << N_neutral;

edm::LogInfo("JetTreeProducer") << "About to Fill tree, sizes: "
    << " pf_pt=" << pf_pt.size()
    << " pf_indices_pfraw=" << pf_indices_pfraw_.size()
    << " pf_indices_pfraw[0].size=" << (pf_indices_pfraw_.empty() ? -1 : (int)pf_indices_pfraw_[0].size());

std::cout << "PF candidates: " << pf_pt.size()
          << " fromPV: " << pf_fromPV.size()
          << " keepAlways: " << pf_keepAlways.size()
          << " hasValidTime: " << pf_hasValidTime.size()
          << std::endl;

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

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
#include "DataFormats/PatCandidates/interface/PackedGenParticle.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

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

//Test of genvertex
//Test of genvertex

//GenJet matching + jet classification (ΔR + pT cut, clean HS/PU)
namespace {


// ===================
//  Helper Functions
// ===================
// ======================================================
// Helper 0: Extract PF indices from pat::Jet
// ======================================================
inline std::vector<unsigned int> getPFIndicesFromPatJet(const pat::Jet& jet) {
    std::vector<unsigned int> pf_indices_this_jet;
    pf_indices_this_jet.reserve(jet.numberOfDaughters());

    for (const auto& candPtr : jet.getJetConstituents()) {

        // Basic nullptr / availability checks on edm::Ptr:
        // - skip null pointers (no reference stored)
        // - skip pointers whose product is not available in this event (avoids ProductNotFound)
        if (candPtr.isNull()) continue;
        // isAvailable() tells if the product backing the Ptr exists in this event
        if (!candPtr.isAvailable()) {
            edm::LogWarning("getPFIndicesFromPatJet")
                << "Jet constituent Ptr refers to a product not available in this event (ProductID invalid); skipping.";
            continue;
        }

        // Now safe to call candPtr.get()
        const auto* pfcand = dynamic_cast<const pat::PackedCandidate*>(candPtr.get());
        if (!pfcand) continue;

        // Use key() as PF index (aligned with event-level PF collection)
        unsigned int pfIdx = candPtr.key();
        pf_indices_this_jet.push_back(pfIdx);
    }

    return pf_indices_this_jet;
}


// ======================================================
// Helper 0-0: Extract PF indices from fastjet::PseudoJet
// ======================================================

//Old Version
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
}//Old Version:End of getPFIndicesFromPseudoJet()

//---Helper Function1:  PF candidate  ↔   GenParticle matching---
// --- Updated PF candidate ↔ GenParticle matching ---
// Step 1: Require gen particle to be from hard scatter (status flags)
// Step 2: Among those, find best ΔR match
//const pat::PackedGenParticle* matchToGen(const pat::PackedCandidate& pf,
std::pair<const pat::PackedGenParticle*, float> matchToGen(const pat::PackedCandidate& pf,
                                         const std::vector<pat::PackedGenParticle>& genParticles,
                                         float maxDR = 0.4) {//0.05
  const pat::PackedGenParticle* bestMatch = nullptr;
//  float bestDR = maxDR;
  float bestDR = (pf.charge() == 0) ? 1.0 : 0.4;

  for (const auto& gen : genParticles) {
    // --- Physics requirement: must be HS / from PV ---
//    if (!(gen.fromHardProcessFinalState()||gen.isPromptFinalState()||gen.isDirectHardProcessTauDecayProductFinalState() ||gen.isDirectPromptTauDecayProductFinalState())) continue;
//    if (!(gen.isHardProcess())) continue;//AOD,reco::GenParticle only

    if (pf.charge() != 0) {
      if (!(gen.fromHardProcessFinalState() || gen.isPromptFinalState() ||
            gen.isDirectHardProcessTauDecayProductFinalState() ||
            gen.isDirectPromptTauDecayProductFinalState()))
        continue;
    }
    // --- For neutrals: allow all stable final copies
    else {
      if (!gen.statusFlags().isLastCopy()) continue;
    }

    // --- Relaxed pdgId match for neutrals
    if (pf.charge() == 0) {
      if (!(abs(gen.pdgId()) == 22 || abs(gen.pdgId()) == 111 || abs(gen.pdgId()) == 130 || abs(gen.pdgId()) == 2112))
        continue;
    } else {
      if (abs(pf.pdgId()) != abs(gen.pdgId())) continue;
    }
    // --- (Optional) require same particle type:TOO STRIC ---
//    if (std::abs(pf.pdgId()) != std::abs(gen.pdgId())) continue;

    // --- ΔR matching ---
    float dR = reco::deltaR(pf.eta(), pf.phi(), gen.eta(), gen.phi());
    if (dR < bestDR) {
      bestDR = dR;
      bestMatch = &gen;
    }
  }
  return std::make_pair(bestMatch, bestDR);  // nullptr if no good match
}//End of  matchToGen

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



  // Return best gen-jet match that passes both ΔR and pt cuts (or nullptr)
  inline const reco::GenJet* //returns a pointer to the best-matching gen jet (or nullptr if no match).
  bestGenMatch(const reco::Jet& j,
             const std::vector<reco::GenJet>& gens,
             double dRMax,
             double minGenPt,
             double maxDz,          // NEW: max allowed |vz - genvertex_z|
             double genvertex_z,    // NEW: generator PV z position
             double* outDR = nullptr,
             int* outIdx = nullptr)  
 {
  // ✅ ADDED SAFEGUARDS inside bestGenMatch
  if (gens.empty()) {
    edm::LogWarning("JetTreeProducer")
      << "⚠️ bestGenMatch called with empty genJet collection.";
    if (outDR)  *outDR = -1.0;
    if (outIdx) *outIdx = -1;
    return nullptr;
  }//End of ✅ ADDED SAFEGUARDS inside bestGenMatch

    const reco::GenJet* best = nullptr;
    double bestDR = 1e9;
    int bestIdx = -1;

    const double eta = j.eta();
    const double phi = j.phi();

    for (size_t i = 0; i < gens.size(); ++i) {//Loop over all gen jets.
      const auto& g = gens[i];
      if (g.pt() <= minGenPt) continue;//Skip too-soft ones (g.pt() <= minGenPt).
  
       // Require gen jet to come from generator PV
//      if (std::abs(g.vz() - genvertex_z) > maxDz) continue;
          
       // ---TEST: more robust dz filter ---//
    double vzGen = 0.0;
    bool hasVz = (std::abs(g.vz()) > 1e-3);
    
    if (!g.getJetConstituents().empty()){
      const auto& firstPtr = g.getJetConstituents().at(0);
      if (firstPtr.isNull()) {
          edm::LogWarning("JetTreeProducer")
              << "GenJet first constituent Ptr is null; cannot read vz. Using default.";
      } else if (!firstPtr.isAvailable()) {
          edm::LogWarning("JetTreeProducer")
              << "GenJet first constituent Ptr product not available; cannot read vz. Using default.";
      } else {
          vzGen = firstPtr->vz();
      }
    }    
    else
        vzGen = g.vz();
    if (hasVz && maxDz < 900.0 && std::abs(vzGen - genvertex_z) > maxDz)
        continue;      
       //---TEST:For GenJet verification---//       

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
  bestGenMatch(const fastjet::PseudoJet& j,
             const std::vector<reco::GenJet>& gens,
             double dRMax,
             double minGenPt,
             double maxDz,
             double genvertex_z,
             double* outDR = nullptr,
             int* outIdx = nullptr) 
  {
    const reco::GenJet* best = nullptr;
    double bestDR = 1e9;
    int bestIdx = -1;

    const double eta = j.eta();
    const double phi = j.phi();

    for (size_t i = 0; i < gens.size(); ++i) {//Loop over all gen jets.
      const auto& g = gens[i];
      if (g.pt() <= minGenPt) continue;//Skip too-soft ones (g.pt() <= minGenPt).
      if (std::abs(g.vz() - genvertex_z) > maxDz) continue;

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
                double dRMax,//threshold
                double minGenPt,//threshold
                double maxDz,          // loose PV consistency window
                double genvertex_z_)    //generator PV z position

  {
 // ✅ ADDED SAFEGUARDS inside classifyJetHS
   JetTruthMatch out;//It builds a JetTruthMatch out strut
  if (gens.empty()) {
    edm::LogWarning("JetTreeProducer") 
      << "⚠️ classifyJetHS called with empty genJet vector.";
    out.isHS = false;
    out.dR = -1.0;
    out.genPt = -1.0f;
    out.genIndex = -1;
    out.matchedGen = nullptr;
    return out;
  }//End of Test safe

    double dR = -1.0;
    int idx = -1;

    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, maxDz, genvertex_z_, &dR, &idx);
//    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
    out.isHS     = (g != nullptr);//only true if match exists AND passed PV cut
    out.dR       = static_cast<float>(dR);//best ΔR or -1.
    out.genPt    = (g ? g->pt() : -1.0f);//if valid, else -1.
    out.genIndex = idx;// index in gens vector,Indicate the matched gen jet positon in the vector
    out.matchedGen      =  g;//pointer to actual GenJet
    return out;//Returns the JetTruthMatch struct.
  }//It just call .classifyJetHS' and get a neat struct with all info

  inline JetTruthMatch
  classifyJetHS(const fastjet::PseudoJet& j,//the PseudoJet.
                const std::vector<reco::GenJet>& gens,//the gen jets.
                double dRMax,//threshold
                double minGenPt,//threshold
                double maxDz,
                double genvertex_z_)
  {
    double dR = -1.0;
    int idx = -1;

    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, maxDz, genvertex_z_, &dR, &idx);
//    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
    JetTruthMatch out;//It builds a JetTruthMatch out strut
    out.isHS     = (g != nullptr);//only true if match exists AND passed PV cut
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
  constexpr float kMaxDz      = 5.0;  // cm, 0.03->5
  constexpr float kMaxDzSig   = 30.0;   // unitless 2->30
  constexpr float kTimeNSigma = 10.0;   // |(t - tPV)/σ|,3->10 

  inline bool isChargedPU(const pat::PackedCandidate &pf)
  {
    // Charged hadrons, electrons, muons with a track
    if (pf.charge() == 0) return false;

  // conservative charged-PU test using available API:
  // - use fromPV() which returns an int (3 means "from PV", 2 ambiguous, etc.)
  // - use dz() rather than dzSig() if dzSig() is not available

  const int kMaxDzAbs = 5; // tune as needed (was using dzSig limit before)0.5->5
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

  inline bool isNeutralPU(const pat::PackedCandidate& pf, 
                          float pvTime, 
                          bool useTimingFallbackPuppi)
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
  };//End of struct PUContentReco(FOR ALGO) 
}//End of namespace

struct PFTruthContentReco {
  int nTot   = 0;   // all PFs in the jet
  int nPU    = 0;   // PFs matched to PU gen particles
  float sumPtTot = 0.f;
  float sumPtPU  = 0.f;
};//End of PFTruthContentReco(FOR TRUTH).

//=====================================================================
//  Canonical helper functions for PU content (cleaned & consistent)
//=====================================================================

//1. Algorithmic PU-content accumulator (per-PF)
inline void updatePUContentReco(const pat::PackedCandidate* pf,
                                PUContentReco& acc,
                                float pvTime,
//                                const std::vector<float>& pvs_t_,
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

// 2. Truth-based PU-content accumulator (per-PF), reusing cached truth flags.
//=====================================================================
//  Update PF Truth Content using precomputed truth flags
//  (no matchToGen call — reuses pf_isPU_truth)
//=====================================================================
inline void updatePFTruthContentReco(const pat::PackedCandidate* pf,
                                     PFTruthContentReco& acc,
                                     unsigned int idx,
                                     const std::vector<int>& pf_isPU_truth)
{
  if (!pf) return;
  ++acc.nTot;
  acc.sumPtTot += pf->pt();

  if (idx < pf_isPU_truth.size() && pf_isPU_truth[idx]) {
    ++acc.nPU;
    acc.sumPtPU += pf->pt();
  }
}//End of updatePFTruthContentReco()

//=====================================================================
//  FastJet PU-content calculator (algorithmic only, no truth inside)
//=====================================================================
inline PUContentReco computePUContentFastJet(const fastjet::PseudoJet& jet,
                                             const std::vector<const pat::PackedCandidate*>& pfAll,
                                             float pvTime,
                                             bool useTimingFallbackPuppi = true)
{
  PUContentReco acc;
  for (const auto& c : jet.constituents()) {
    if (c.user_index()<0) continue;
    int idx = c.user_index();
    if (idx < 0 || idx >= static_cast<int>(pfAll.size())) continue;
    
    const pat::PackedCandidate* pf = pfAll[idx];
    updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
  }
  return acc;
}

//Function: compute PU fractions for one jet
inline PUContentReco computePUContentRecoJet(//A function that returns PUContentRec
    const pat::Jet& jet,//the reco jet whose PF composition we are studying.
    const std::vector<const pat::PackedCandidate*>& /*pfAll*/,//the full PF candidate collection by index
    float pvTime,//the primary-vertex time, needed for neutral timing PU ID.
    bool useTimingFallbackPuppi = true)//if true, neutrals with missing timing fall back to Puppi weight classification.
{
  PUContentReco acc;//Start an empty accumulator struct.
    const size_t nConst = jet.numberOfDaughters();
    int recovered = 0;
    int failed = 0;
 
//  const size_t nConst = jet.numberOfDaughters();
  for (size_t i = 0; i < nConst; ++i) {//Loop over all PF constituents of this jet.
    const auto& c = *jet.daughter(i);


    // Recover PackedCandidate pointer via sourceCandidatePtr
    const pat::PackedCandidate* pf = nullptr;
      if (c.numberOfSourceCandidatePtrs() > 0) {
        auto ptr = c.sourceCandidatePtr(0);
        if (ptr.isNull()) {
        edm::LogWarning("JetTreeProducer") << "Found null Ptr at pf-pointer extraction; skipping.";
    } else if (!ptr.isAvailable()) {
        edm::LogWarning("JetTreeProducer") << "Found Ptr whose product is not available; skipping.";
    } else {
          pf = dynamic_cast<const pat::PackedCandidate*>(ptr.get());
          }
      }//If no index is available, try to backtrack to the PF candidate pointer.

      if (pf) {
          recovered++;
      } else {
          failed++;
      }
    updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
    }

 // One-line summary instead of spamming per-daughter
    if (failed > 0) {
    #ifdef DEBUG_PUCONTENT
            std::cout << "[PUContentRecoJet] Jet pt=" << jet.pt()
                  << " eta=" << jet.eta()
                  << " recovered " << recovered << "/" << nConst
                  << " PF daughters (" << failed << " failed)"
                  << std::endl;
    #endif
     }  

  return acc;//he function returns
}//End of PUContentReco computePUContentRecoJet()


class JetTreeProducer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit JetTreeProducer(const edm::ParameterSet&);
  ~JetTreeProducer() override = default;
  //static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  
  
private:
  std::pair<double, std::string> getGenVertexZ(const edm::Event& iEvent);
  void beginJob() override;
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  void endJob() override;
  
  // --- jet and genjet ---
  edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;
  edm::EDGetTokenT<std::vector<reco::GenJet>> genJetToken_;

// --- PF candidates and primary vertex ---
edm::EDGetTokenT<std::vector<pat::PackedCandidate>> pf_collection_token;
edm::EDGetTokenT<std::vector<reco::Vertex>> pvsToken_;
edm::EDGetTokenT<reco::BeamSpot> bsToken_;

// --- generator-level info (multi-source robust setup) ---
edm::EDGetTokenT<std::vector<reco::GenParticle>> genParticlesTokenAOD_;          // prunedGenParticles (AOD)
edm::EDGetTokenT<std::vector<pat::PackedGenParticle>> genParticlesTokenMiniAOD_; // packedGenParticles (MiniAOD)
edm::EDGetTokenT<edm::HepMCProduct> genvertexToken_;                             // generatorSmeared
edm::EDGetTokenT<GenEventInfoProduct> genEventInfoToken_;                        // generator metadata


  TTree* tree_;

// For jet-level matching diagnostics
std::vector<float> jetDeltaR_puppi_all_;
std::vector<float> jetDeltaR_pfraw_all_;
std::vector<float> jetDeltaR_fromPV3_all_;
std::vector<float> jetDeltaR_tight_all_;
std::vector<float> jetDeltaR_loose_all_;
std::vector<float> jetDeltaR_time_all_;

/*
// Event-level PU jet counts
int nPUJets_puppi_, nRecoJets_puppi_;
int nPUJets_pfraw_, nRecoJets_pfraw_;
int nPUJets_tight_, nRecoJets_tight_;
int nPUJets_loose_, nRecoJets_loose_;
int nPUJets_time_, nRecoJets_time_;
*/


  std::vector<int> jetIsHS_puppi_all_;
  std::vector<float> jetPt_puppi_all_;
  std::vector<float> jetAbsEta_puppi_all_;
  std::vector<float> jetResponse_PR_puppi_all_;
  std::vector<float> genPt_puppi_all_;
  std::vector<float> puFracPt_algo_puppi_all_;
  std::vector<float> puFracCount_algo_puppi_all_;
  std::vector<float> puFracPt_truth_puppi_all_;
  std::vector<float> puFracCount_truth_puppi_all_;


/*
  std::vector<float> jetPt_puppi_5leading_;
  std::vector<float> jetAbsEta_puppi_5leading_;
  std::vector<float> jetResponse_PR_puppi_5leading_;
  std::vector<float> genPt_puppi_5leading_;
  std::vector<float> puFracPt_puppi_5leading_;
  std::vector<float> puFracCount_puppi_5leading_;
*/


//  float jetResponse_ULv15;
  std::vector<int> jetIsHS_pfraw_all_;
  std::vector<float> jetPt_pfraw_all_;
  std::vector<float> genPt_pfraw_all_;
  std::vector<float> jetResponse_PR_pfraw_all_;
  std::vector<float> jetAbsEta_pfraw_all_;
  std::vector<float> puFracPt_algo_pfraw_all_;
  std::vector<float> puFracCount_algo_pfraw_all_;
  std::vector<float> puFracPt_truth_pfraw_all_;
  std::vector<float> puFracCount_truth_pfraw_all_;

/*
  std::vector<float> jetPt_pfraw_5leading_;
  std::vector<float> genPt_pfraw_5leading_;
  std::vector<float> jetResponse_PR_pfraw_5leading_;
  std::vector<float> jetAbsEta_pfraw_5leading_;
  std::vector<float> puFracPt_pfraw_5leading_;
  std::vector<float> puFracCount_pfraw_5leading_;
*/

  std::vector<int> jetIsHS_fromPV3_all_;
  std::vector<float> jetPt_fromPV3_all_;
  std::vector<float> genPt_fromPV3_all_;
  std::vector<float> jetResponse_PR_fromPV3_all_;
  std::vector<float> jetAbsEta_fromPV3_all_;
  std::vector<float> puFracPt_algo_fromPV3_all_;
  std::vector<float> puFracCount_algo_fromPV3_all_;
  std::vector<float> puFracPt_truth_fromPV3_all_;
  std::vector<float> puFracCount_truth_fromPV3_all_;

/*
  std::vector<float> jetPt_fromPV3_5leading_;
  std::vector<float> genPt_fromPV3_5leading_;
  std::vector<float> jetResponse_PR_fromPV3_5leading_;
  std::vector<float> jetAbsEta_fromPV3_5leading_;
  std::vector<float> puFracPt_fromPV3_5leading_;
  std::vector<float> puFracCount_fromPV3_5leading_;
*/
  
  std::vector<int> jetIsHS_tight_all_;
  std::vector<float> jetPt_tight_all_;
  std::vector<float> genPt_tight_all_;
  std::vector<float> jetResponse_PR_tight_all_;
  std::vector<float> jetAbsEta_tight_all_;
  std::vector<float> puFracPt_algo_tight_all_;
  std::vector<float> puFracCount_algo_tight_all_;
  std::vector<float> puFracPt_truth_tight_all_;
  std::vector<float> puFracCount_truth_tight_all_;

/* 
  std::vector<float> jetPt_tight_5leading_;
  std::vector<float> genPt_tight_5leading_;
  std::vector<float> jetResponse_PR_tight_5leading_;
  std::vector<float> jetAbsEta_tight_5leading_;
  std::vector<float> puFracPt_tight_5leading_;
  std::vector<float> puFracCount_tight_5leading_;
*/

  std::vector<int> jetIsHS_loose_all_;
  std::vector<float> jetPt_loose_all_;
  std::vector<float> genPt_loose_all_;
  std::vector<float> jetResponse_PR_loose_all_;
  std::vector<float> jetAbsEta_loose_all_;
  std::vector<float> puFracPt_algo_loose_all_;
  std::vector<float> puFracCount_algo_loose_all_;
  std::vector<float> puFracPt_truth_loose_all_;
  std::vector<float> puFracCount_truth_loose_all_;
 /*
  std::vector<float> jetPt_loose_5leading_;
  std::vector<float> genPt_loose_5leading_;
  std::vector<float> jetResponse_PR_loose_5leading_;
  std::vector<float> jetAbsEta_loose_5leading_;
  std::vector<float> puFracPt_loose_5leading_;
  std::vector<float> puFracCount_loose_5leading_;
*/

  //For MTD-based jet clustering
  std::vector<int> jetIsHS_time_all_;
  std::vector<float> jetPt_time_all_;
  std::vector<float> genPt_time_all_;
  std::vector<float> jetResponse_PR_time_all_;
  std::vector<float> jetAbsEta_time_all_;
  std::vector<float> puFracPt_algo_time_all_;
  std::vector<float> puFracCount_algo_time_all_;
  std::vector<float> puFracPt_truth_time_all_;
  std::vector<float> puFracCount_truth_time_all_;

/*
  std::vector<float> jetPt_time_5leading_;
  std::vector<float> genPt_time_5leading_;
  std::vector<float> jetResponse_PR_time_5leading_;
  std::vector<float> jetAbsEta_time_5leading_;
  std::vector<float> puFracPt_time_5leading_;
  std::vector<float> puFracCount_time_5leading_;
  std::vector<float> jetTime_time_5leading_;
  std::vector<float> jetTimeError_time_5leading_;
*/


// For jet-level timing (MTD jets)
  std::vector<float> jetTimeSig_time_all_;   // (tjet - PVt)/σ_tjet
  std::vector<float> jetTime_time_all_;
  std::vector<float> jetTimeError_time_all_;
  
  std::vector<float> pf_vertex,pf_pt, pf_eta, pf_phi, pf_energy;
  std::vector<float> pf_charge,pf_puppiWeight,pf_puppiWeightNoLep;//For <pat::PackedCandidate> collection
  std::vector<float> pf_pvQuality;//associatedPVIndex, For <pat::PackedCandidate> collection
  std::vector<int> pf_pvIndex;//associatedPVIndex, For <pat::PackedCandidate> collection
  std::vector<float> pf_dxy, pf_dz, pf_dzError, pf_dzSig;
  std::vector<float> pf_time, pf_timeError, pf_dt, pf_dtErr, pf_dtSig;//For <pat::PackedCandidate> collection
  std::vector<float> pf_vx, pf_vy, pf_vz;

  std::vector<int> pf_pdgId;
  std::vector<int> pf_fromPV;          // Store fromPV() intege
  std::vector<int> pf_passesTightCut;//was bool
  std::vector<int> pf_passesLooseCut;//was bool
  std::vector<int> pf_passes3D;//was bool
  std::vector<int> pf_passes4D;//was bool

  std::vector<int> pf_isHS_algo;//test
  std::vector<int> pf_isPU_algo;//test
  std::vector<int> pf_isHS_truth;//test
  std::vector<int> pf_isPU_truth;//test
  std::vector<float> pf_dR_truth_;//test
 
  std::vector<int> pf_keepAlways;//was bool
  std::vector<int> pf_keepDisplaced;//was bool
  std::vector<int> pf_hasTimeCompatibleWithPV;//New add
  std::vector<int> pf_isInMTD;//was bool
  std::vector<int> pf_isCharged;//was bool
  std::vector<int> pf_hasValidTime;//was bool
  std::vector<int> pf_isHS; // 1 = HS, 0 = PU, -1 = unmatched
  std::vector<std::vector<unsigned int>> pf_indices_general_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_time_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_tight_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_loose_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_pfraw_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_fromPV3_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_puppi_all_;//For <pat::PackedCandidate> collection
  std::vector<float> genparticles_z_;//segmentation fault. For generated particles z-position
  //For primary vetex
  std::vector<float> pv_x_, pv_y_, pv_z_, pv_t_, pv_chi2_, pv_ndof_;
  std::vector<int>   pv_nTracks_;


  float pt_, eta_, phi_, mass_;//For jet collection
//  std::vector<PFParticle> pfparticles;//For PackedCandidate,commented out
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_,pvs_TimeErr_; // For "FIRST" primary vertex position

  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
  float genvertex_z_;
  std::vector<float> puFrac_pfraw_,puFrac_fromPV3_, puFrac_tight_, puFrac_loose_,puFrac_time_;;
//  float pf_vx, pf_vy, pf_vz;//For <pat::PackedCandidate> collection
//  int pf_pdgId,pf_isTimeValid;//For <pat::PackedCandidate> collection

// add to class members (inside class definition)

  // For jet-level efficiency & purity per collection
  float efficiency_puppi_, purity_puppi_;
  float efficiency_pfraw_, purity_pfraw_;//defined but not used
  float efficiency_fromPV3_, purity_fromPV3_;//defined but not used
  float efficiency_tight_, purity_tight_;
  float efficiency_loose_, purity_loose_;
  float efficiency_time_, purity_time_;//defined but not used
      
 
  int totalRecoJets = 0;
  int totalPUJets = 0;
  float puJetFraction_all_ = 0;//Not saved

  int totalRecoJets_puppi = 0; 
  int totalPUJets_puppi = 0; 
  float  puJetFraction_puppi_all_ = 0;


  int totalRecoJets_pfraw = 0; 
  int totalPUJets_pfraw = 0; 
  float puJetFraction_pfraw_all_ =0;

  int totalRecoJets_fromPV3 = 0; 
  int totalPUJets_fromPV3 = 0; 
  float puJetFraction_fromPV3_all_ =0;
  
  int totalRecoJets_tight = 0; 
  int totalPUJets_tight = 0; 
  float puJetFraction_tight_all_ = 0;

  int totalRecoJets_loose = 0 ;
  int totalPUJets_loose = 0 ; 
  float puJetFraction_loose_all_ =0; 


  int totalRecoJets_time = 0; 
  int totalPUJets_time = 0; 
  float puJetFraction_time_all_ =0;

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

// --- multi-source genvertex---
genParticlesTokenAOD_     = consumes<std::vector<reco::GenParticle>>(edm::InputTag("prunedGenParticles"));
genParticlesTokenMiniAOD_ = consumes<std::vector<pat::PackedGenParticle>>(edm::InputTag("packedGenParticles"));
genvertexToken_           = consumes<edm::HepMCProduct>(edm::InputTag("generatorSmeared"));
genEventInfoToken_        = consumes<GenEventInfoProduct>(edm::InputTag("generator"));


}//End of JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig)

void JetTreeProducer::beginJob() {
  usesResource("TFileService");
  edm::Service<TFileService> fs_;
  tree_ = fs_->make<TTree>("JetTree", "JetTree");
 
 
// ΔR branches
  tree_->Branch("jetDeltaR_puppi_all", &jetDeltaR_puppi_all_);
  tree_->Branch("jetDeltaR_pfraw_all", &jetDeltaR_pfraw_all_);
  tree_->Branch("jetDeltaR_fromPV3_all", &jetDeltaR_fromPV3_all_);
  tree_->Branch("jetDeltaR_tight_all", &jetDeltaR_tight_all_);
  tree_->Branch("jetDeltaR_loose_all", &jetDeltaR_loose_all_);
  tree_->Branch("jetDeltaR_time_all",   &jetDeltaR_time_all_);

/*
  // PU jet counts
  tree_->Branch("nPUJets_puppi",   &nPUJets_puppi_,   "nPUJets_puppi/I");
  tree_->Branch("nRecoJets_puppi", &nRecoJets_puppi_, "nRecoJets_puppi/I");
  tree_->Branch("nPUJets_pfraw",   &nPUJets_pfraw_,   "nPUJets_pfraw/I");
  tree_->Branch("nRecoJets_pfraw", &nRecoJets_pfraw_, "nRecoJets_pfraw/I");
  tree_->Branch("nPUJets_tight",   &nPUJets_tight_,   "nPUJets_tight/I");
  tree_->Branch("nRecoJets_tight", &nRecoJets_tight_, "nRecoJets_tight/I");
  tree_->Branch("nPUJets_loose",   &nPUJets_loose_,   "nPUJets_loose/I");
  tree_->Branch("nRecoJets_loose", &nRecoJets_loose_, "nRecoJets_loose/I");
  tree_->Branch("nPUJets_MTD",     &nPUJets_time_,     "nPUJets_MTD/I");
  tree_->Branch("nRecoJets_MTD",   &nRecoJets_time_,   "nRecoJets_MTD/I");
*/

  tree_->Branch("jetIsHS_puppi_all", &jetIsHS_puppi_all_);
  tree_->Branch("jetPt_puppi_all", &jetPt_puppi_all_);
  tree_->Branch("genPt_puppi_all", &genPt_puppi_all_);
  tree_->Branch("jetResponse_PR_puppi_all", &jetResponse_PR_puppi_all_);
  tree_->Branch("jetAbsEta_puppi_all", &jetAbsEta_puppi_all_);  
  tree_->Branch("puFracPt_algo_puppi_all", &puFracPt_algo_puppi_all_);
  tree_->Branch("puFracCount_algo_puppi_all", &puFracCount_algo_puppi_all_);  
  tree_->Branch("puFracPt_truth_puppi_all", &puFracPt_truth_puppi_all_);
  tree_->Branch("puFracCount_truth_puppi_all", &puFracCount_truth_puppi_all_);  

  /*
  tree_->Branch("jetPt_puppi_5leading", &jetPt_puppi_5leading_);
  tree_->Branch("genPt_puppi_5leading", &genPt_puppi_5leading_);
  tree_->Branch("jetResponse_puppi_5leading", &jetResponse_PR_puppi_5leading_);
  tree_->Branch("jetAbsEta_puppi_5leading", &jetAbsEta_puppi_5leading_);  
  tree_->Branch("puFracPt_puppi_5leading", &puFracPt_puppi_5leading_);
  tree_->Branch("puFracCount_puppi_5leading", &puFracCount_puppi_5leading_);  
*/
  
  tree_->Branch("jetIsHS_pfraw_all", &jetIsHS_pfraw_all_);
  tree_->Branch("jetResponse_PR_pfraw_all", &jetResponse_PR_pfraw_all_);
  tree_->Branch("jetPt_pfraw_all", &jetPt_pfraw_all_);
  tree_->Branch("genPt_pfraw_all", &genPt_pfraw_all_);
  tree_->Branch("jetAbsEta_pfraw_all", &jetAbsEta_pfraw_all_); 
  tree_->Branch("puFracPt_algo_pfraw_all", &puFracPt_algo_pfraw_all_);
  tree_->Branch("puFracCount_algo_pfraw_all", &puFracCount_algo_pfraw_all_);  
  tree_->Branch("puFracPt_truth_pfraw_all", &puFracPt_truth_pfraw_all_);
  tree_->Branch("puFracCount_truth_pfraw_all", &puFracCount_truth_pfraw_all_);  

 /*
  tree_->Branch("jetResponse_PR_pfraw_5leading", &jetResponse_PR_pfraw_5leading_);
  tree_->Branch("jetPt_pfraw_5leading", &jetPt_pfraw_5leading_);
  tree_->Branch("genPt_pfraw_5leading", &genPt_pfraw_5leading_);
  tree_->Branch("jetAbsEta_pfraw_5leading", &jetAbsEta_pfraw_5leading_); 
  tree_->Branch("puFracPt_pfraw_5leading", &puFracPt_pfraw_5leading_);
  tree_->Branch("puFracCount_pfraw_5leading", &puFracCount_pfraw_5leading_);  
*/

  tree_->Branch("jetIsHS_fromPV3_all", &jetIsHS_fromPV3_all_);
  tree_->Branch("jetResponse_PR_fromPV3_all", &jetResponse_PR_fromPV3_all_);
  tree_->Branch("jetPt_fromPV3_all", &jetPt_fromPV3_all_);
  tree_->Branch("genPt_fromPV3_all", &genPt_fromPV3_all_);
  tree_->Branch("jetAbsEta_fromPV3_all", &jetAbsEta_fromPV3_all_); 
  tree_->Branch("puFracPt_algo_fromPV3_all", &puFracPt_algo_fromPV3_all_);
  tree_->Branch("puFracCount_algo_fromPV3_all", &puFracCount_algo_fromPV3_all_);  
  tree_->Branch("puFracPt_truth_fromPV3_all", &puFracPt_truth_fromPV3_all_);
  tree_->Branch("puFracCount_truth_fromPV3_all", &puFracCount_truth_fromPV3_all_);  

 /*
  tree_->Branch("jetResponse_PR_fromPV3_5leading", &jetResponse_PR_fromPV3_5leading_);
  tree_->Branch("jetPt_fromPV3_5leading", &jetPt_fromPV3_5leading_);
  tree_->Branch("genPt_fromPV3_5leading", &genPt_fromPV3_5leading_);
  tree_->Branch("jetAbsEta_fromPV3_5leading", &jetAbsEta_fromPV3_5leading_); 
  tree_->Branch("puFracPt_fromPV3_5leading", &puFracPt_fromPV3_5leading_);
  tree_->Branch("puFracCount_fromPV3_5leading", &puFracCount_fromPV3_5leading_);  
*/

  tree_->Branch("jetIsHS_tight_all", &jetIsHS_tight_all_);
  tree_->Branch("jetPt_tight_all", &jetPt_tight_all_);
  tree_->Branch("genPt_tight_all", &genPt_tight_all_);
  tree_->Branch("jetResponse_PR_tight_all", &jetResponse_PR_tight_all_);
  tree_->Branch("jetAbsEta_tight_all", &jetAbsEta_tight_all_);
  tree_->Branch("puFracPt_algo_tight_all", &puFracPt_algo_tight_all_);
  tree_->Branch("puFracCount_algo_tight_all", &puFracCount_algo_tight_all_);  
  tree_->Branch("puFracPt_truth_tight_all", &puFracPt_truth_tight_all_);
  tree_->Branch("puFracCount_truth_tight_all", &puFracCount_truth_tight_all_);  

/*  
  tree_->Branch("jetPt_tight_5leading", &jetPt_tight_5leading_);
  tree_->Branch("genPt_tight_5leading", &genPt_tight_5leading_);
  tree_->Branch("jetResponse_PR_tight_5leading", &jetResponse_PR_tight_5leading_);
  tree_->Branch("jetAbsEta_tight_5leading", &jetAbsEta_tight_5leading_);
  tree_->Branch("puFracPt_tight_5leading", &puFracPt_tight_5leading_);
  tree_->Branch("puFracCount_tight_5leading", &puFracCount_tight_5leading_);  
*/

  tree_->Branch("jetIsHS_loose_all", &jetIsHS_loose_all_);
  tree_->Branch("jetResponse_PR_loose_all", &jetResponse_PR_loose_all_);
  tree_->Branch("jetAbsEta_loose_all", &jetAbsEta_loose_all_);
  tree_->Branch("jetPt_loose_all", &jetPt_loose_all_);
  tree_->Branch("genPt_loose_all", &genPt_loose_all_);
  tree_->Branch("puFracPt_algo_loose_all", &puFracPt_algo_loose_all_);
  tree_->Branch("puFracCount_algo_loose_all", &puFracCount_algo_loose_all_);  
  tree_->Branch("puFracPt_truth_loose_all", &puFracPt_truth_loose_all_);
  tree_->Branch("puFracCount_truth_loose_all", &puFracCount_truth_loose_all_);  

/*
  tree_->Branch("jetResponse_PR_loose_5leading", &jetResponse_PR_loose_5leading_);
  tree_->Branch("jetAbsEta_loose_5leading", &jetAbsEta_loose_5leading_);
  tree_->Branch("jetPt_loose_5leading", &jetPt_loose_5leading_);
  tree_->Branch("genPt_loose_5leading", &genPt_loose_5leading_);
  tree_->Branch("puFracPt_loose_5leading", &puFracPt_tight_5leading_);
  tree_->Branch("puFracCount_loose_5leading", &puFracCount_tight_5leading_);  
*/

  tree_->Branch("jetIsHS_time_all", &jetIsHS_time_all_);
  tree_->Branch("jetResponse_PR_time_all", &jetResponse_PR_time_all_);
  tree_->Branch("jetAbsEta_time_all", &jetAbsEta_time_all_);
  tree_->Branch("jetPt_time_all", &jetPt_time_all_);
  tree_->Branch("genPt_time_all", &genPt_time_all_);
  tree_->Branch("puFracPt_algo_time_all", &puFracPt_algo_time_all_);
  tree_->Branch("puFracCount_algo_time_all", &puFracCount_algo_time_all_);  
  tree_->Branch("puFracPt_truth_time_all", &puFracPt_truth_time_all_);
  tree_->Branch("puFracCount_truth_time_all", &puFracCount_truth_time_all_);  

/*
  tree_->Branch("jetResponse_PR_time_5leading", &jetResponse_PR_time_5leading_);
  tree_->Branch("jetAbsEta_time_5leading", &jetAbsEta_time_5leading_);
  tree_->Branch("jetPt_time_5leading", &jetPt_time_5leading_);
  tree_->Branch("genPt_time_5leading", &genPt_time_5leading_);
  tree_->Branch("jetTime_time_5leading", &jetTime_time_5leading_);
  tree_->Branch("jetTimeError_time_5leading", &jetTimeError_time_5leading_);
  tree_->Branch("puFracPt_time_5leading", &puFracPt_time_5leading_);
  tree_->Branch("puFracCount_time_5leading", &puFracCount_time_5leading_);  
*/

  // Jet timing
  tree_->Branch("jetTimeSig_time_all", &jetTimeSig_time_all_);
  tree_->Branch("jetTime_time_all", &jetTime_time_all_);
  tree_->Branch("jetTimeError_time_all", &jetTimeError_time_all_);


   // Branches for jet kinematics 
  tree_->Branch("pt", &pt_, "pt/F");
  tree_->Branch("eta", &eta_, "eta/F");
  tree_->Branch("phi", &phi_, "phi/F");
  tree_->Branch("mass", &mass_, "mass/F");

  // Branches for packedCandidate
//  tree_->Branch("PFParticles", &pfparticles);//commented out

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
  tree_->Branch("genparticles_z", &genparticles_z_);//segmentation fault=>removed.
  tree_->Branch("genvertex_z", &genvertex_z_, "genvertex_z/F");

  //For primary vetex
  tree_->Branch("pv_x", &pv_x_);
  tree_->Branch("pv_y", &pv_y_);
  tree_->Branch("pv_z", &pv_z_);
  tree_->Branch("pv_t", &pv_t_);
  tree_->Branch("pv_chi2", &pv_chi2_);
  tree_->Branch("pv_ndof", &pv_ndof_);
  tree_->Branch("pv_nTracks", &pv_nTracks_);

 
   // Branches for <pat::PackedCandidate> collection 
  tree_->Branch("pf_vertex", &pf_vertex);
  tree_->Branch("pf_pt", &pf_pt);
  tree_->Branch("pf_eta", &pf_eta);
  tree_->Branch("pf_phi", &pf_phi);
  tree_->Branch("pf_energy", &pf_energy);
  tree_->Branch("pf_charge", &pf_charge);
  tree_->Branch("pf_puppiWeight", &pf_puppiWeight);
  tree_->Branch("pf_puppiWeightNoLep", &pf_puppiWeightNoLep);
  tree_->Branch("pf_pvQuality", &pf_pvQuality);
  tree_->Branch("pf_pvIndex", &pf_pvIndex);
  tree_->Branch("pf_dxy", &pf_dxy);
  tree_->Branch("pf_dz", &pf_dz);
  tree_->Branch("pf_dzError", &pf_dzError);
  tree_->Branch("pf_dzSig", &pf_dzSig);
  tree_->Branch("pf_time", &pf_time);
  tree_->Branch("pf_timeError", &pf_timeError);
  tree_->Branch("pf_dt", &pf_dt);
  tree_->Branch("pf_dtErr", &pf_dtErr);
  tree_->Branch("pf_dtSig", &pf_dtSig);

  tree_->Branch("pf_isInMTD", &pf_isInMTD);
  tree_->Branch("pf_isCharged", &pf_isCharged);
  tree_->Branch("pf_hasValidTime", &pf_hasValidTime);

  tree_->Branch("pf_passesTightCut", &pf_passesTightCut);
  tree_->Branch("pf_passesLooseCut", &pf_passesLooseCut);
  tree_->Branch("pf_passes3D", &pf_passes3D);
  tree_->Branch("pf_passes4D", &pf_passes4D);
  tree_->Branch("pf_keepAlways", &pf_keepAlways);
  tree_->Branch("pf_keepDisplaced",  &pf_keepDisplaced);
  tree_->Branch("pf_hasTimeCompatibleWithPV",  &pf_hasTimeCompatibleWithPV);

  tree_->Branch("pf_vx", &pf_vx);
  tree_->Branch("pf_vy", &pf_vy);
  tree_->Branch("pf_vz", &pf_vz);

  tree_->Branch("pf_pdgId", &pf_pdgId);  
  tree_->Branch("pf_isHS", &pf_isHS);
  tree_->Branch("pf_fromPV", &pf_fromPV);
 
 
  float efficiency_puppi_, purity_puppi_;
  float efficiency_pfraw_, purity_pfraw_;//defined but not used
  float efficiency_fromPV3_, purity_fromPV3_;//defined but not used
  float efficiency_tight_, purity_tight_;
  float efficiency_loose_, purity_loose_;
  float efficiency_time_, purity_time_;//defined but not used

  tree_->Branch("totalRecoJets_pfraw", &totalRecoJets_pfraw,"totalRecoJets_pfraw/I");
  tree_->Branch("totalRecoJets_fromPV3", &totalRecoJets_fromPV3,"totalRecoJets_fromPV3/I");
  tree_->Branch("totalRecoJets_tight", &totalRecoJets_tight,"totalRecoJets_tight/I");
  tree_->Branch("totalRecoJets_loose", &totalRecoJets_loose,"totalRecoJets_loose/I");
  tree_->Branch("totalRecoJets_time", &totalRecoJets_time,"totalRecoJets_time/I");
  

  tree_->Branch("totalPUJets_puppi", &totalPUJets_puppi,"totalPUJets_puppi/I");
  tree_->Branch("totalPUJets_pfraw", &totalPUJets_pfraw,"totalPUJets_pfraw/I");
  tree_->Branch("totalPUJets_fromPV3", &totalPUJets_fromPV3,"totalPUJets_fromPV3/I");
  tree_->Branch("totalPUJets_tight", &totalPUJets_tight,"totalPUJets_tight/I");
  tree_->Branch("totalPUJets_loose", &totalPUJets_loose,"totalPUJets_loose/I");
  tree_->Branch("totalPUJets_time", &totalPUJets_time,"totalPUJets_time/I");
  

  tree_->Branch("puJetFraction_puppi_all", &puJetFraction_puppi_all_,"puJetFraction_puppi_all/F");
  tree_->Branch("puJetFraction_pfraw_all", &puJetFraction_pfraw_all_,"puJetFraction_pfraw_all/F");
  tree_->Branch("puJetFraction_fromPV3_all", &puJetFraction_fromPV3_all_,"puJetFraction_fromPV3_all/F");
  tree_->Branch("puJetFraction_tight_all", &puJetFraction_tight_all_,"puJetFraction_tight_all/F");
  tree_->Branch("puJetFraction_loose_all", &puJetFraction_loose_all_,"puJetFraction_loose_all/F");
  tree_->Branch("puJetFraction_time_all", &puJetFraction_time_all_,"puJetFraction_time_all/F");

  tree_->Branch("efficiency_puppi", &efficiency_puppi_,"efficiency_puppi/F");
  tree_->Branch("purity_puppi", &purity_puppi_,"purity_puppi/F");
  tree_->Branch("efficiency_pfraw", &efficiency_pfraw_,"efficiency_pfraw/F");
  tree_->Branch("purity_pfraw", &purity_pfraw_,"purity_pfraw/F");
  tree_->Branch("efficiency_fromPV3", &efficiency_fromPV3_,"efficiency_fromPV3/F");
  tree_->Branch("purity_fromPV3", &purity_fromPV3_,"purity_fromPV3/F");
  tree_->Branch("efficiency_tight", &efficiency_tight_,"efficiency_tight/F");
  tree_->Branch("purity_tight", &purity_tight_,"purity_tight/F");
  tree_->Branch("efficiency_loose", &efficiency_loose_,"efficiency_loose/F");
  tree_->Branch("purity_loose", &purity_loose_,"purity_loose/F");
  tree_->Branch("efficiency_time", &efficiency_time_,"efficiency_time/F");
  tree_->Branch("purity_time", &purity_time_,"purity_time/F");

  tree_->Branch("pf_indices_puppi_all", &pf_indices_puppi_all_);
  tree_->Branch("pf_indices_pfraw_all", &pf_indices_pfraw_all_);
  tree_->Branch("pf_indices_fromPV3_all", &pf_indices_fromPV3_all_);
  tree_->Branch("pf_indices_tight_all", &pf_indices_tight_all_);
  tree_->Branch("pf_indices_loose_all", &pf_indices_loose_all_);
  tree_->Branch("pf_indices_time_all", &pf_indices_time_all_);
  
  tree_->Branch("puFrac_pfraw", &puFrac_fromPV3_);
  tree_->Branch("puFrac_pfraw", &puFrac_pfraw_);
  tree_->Branch("puFrac_tight", &puFrac_tight_);
  tree_->Branch("puFrac_loose", &puFrac_loose_);
  tree_->Branch("puFrac_MTD", &puFrac_time_);

  tree_->Branch("pf_isHS_algo", &pf_isHS_algo);//test
  tree_->Branch("pf_isPU_algo",  &pf_isPU_algo);//test
  tree_->Branch("pf_isHS_truth", &pf_isHS_truth);//test
  tree_->Branch("pf_isPU_truth", &pf_isPU_truth);//test
  tree_->Branch("pf_dR_truth", &pf_dR_truth_);

}//End of void JetTreeProducer::beginJob()

//Test of genvertex
// --- robust, member implementation ---
std::pair<double, std::string> JetTreeProducer::getGenVertexZ(const edm::Event& iEvent) {
    double genvertex_z_ = 0.0;
    std::string source = "fallback_zero";

    // 1. Try HepMCProduct (FullSIM case)
    edm::Handle<edm::HepMCProduct> hepmcH;
    if (iEvent.getByToken(genvertexToken_, hepmcH) && hepmcH.isValid()) {
        const HepMC::GenEvent* evt = hepmcH->GetEvent();
        if (evt && evt->signal_process_vertex()) {
            genvertex_z_ = evt->signal_process_vertex()->point3d().z();
            source = "HepMCProduct";
            return {genvertex_z_, source};
        }
    }

    // 2. Try GenEventInfoProduct (exists but no vertex info)
    edm::Handle<GenEventInfoProduct> genEvtInfoH;
    if (iEvent.getByToken(genEventInfoToken_, genEvtInfoH) && genEvtInfoH.isValid()) {
        edm::LogInfo("JetTreeProducer") << "GenEventInfoProduct is valid (no vertex info available)";
    }

    // 3. Try reco::GenParticle (AODSIM case)
    edm::Handle<std::vector<reco::GenParticle>> genParticlesAOD;
    if (iEvent.getByToken(genParticlesTokenAOD_, genParticlesAOD) && genParticlesAOD.isValid() && !genParticlesAOD->empty()) {
        genvertex_z_ = genParticlesAOD->at(0).vz();
        source = "GenParticle";
        return {genvertex_z_, source};
    }

    // 4. Try pat::PackedGenParticle (MiniAOD case)
    edm::Handle<std::vector<pat::PackedGenParticle>> genParticlesMini;
    if (iEvent.getByToken(genParticlesTokenMiniAOD_, genParticlesMini) && genParticlesMini.isValid() && !genParticlesMini->empty()) {
        genvertex_z_ = genParticlesMini->at(0).vz();
        source = "PackedGenParticle";
        return {genvertex_z_, source};
    }

    // 5. If all fail, fallback to zero
    edm::LogWarning("JetTreeProducer") << "No valid generator vertex found. Falling back to z=0.";
    return {genvertex_z_, source};
}
//Tes of gen vetex

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
  pf_pvQuality.clear();
  pf_pvIndex.clear();
  pf_dxy.clear();
  pf_dz.clear();
  pf_dzError.clear();
  pf_dzSig.clear();
  pf_time.clear();
  pf_timeError.clear();
  pf_dt.clear();
  pf_dtErr.clear();
  pf_dtSig.clear();
  pf_vx.clear();
  pf_vy.clear();
  pf_vz.clear();
  pf_pdgId.clear();

  pf_isHS_algo.clear();//test
  pf_isPU_algo.clear();//test
  pf_isHS_truth.clear();//test
  pf_isPU_truth.clear();//test
  pf_dR_truth_.clear();//test

  pf_isHS.clear();
  pf_fromPV.clear();
  pf_isCharged.clear();
  pf_hasValidTime.clear();

  jetTimeSig_time_all_.clear();
  jetTime_time_all_.clear();
  jetTimeError_time_all_.clear();
 

  pf_indices_general_all_.clear();
  pf_indices_pfraw_all_.clear();
  pf_indices_fromPV3_all_.clear();
  pf_indices_puppi_all_.clear();
  pf_indices_tight_all_.clear();
  pf_indices_loose_all_.clear();
  pf_indices_time_all_.clear();

  pf_passesTightCut.clear();
  pf_passesLooseCut.clear();
  pf_passes3D.clear();
  pf_passes4D.clear();
  pf_keepAlways.clear();
  pf_keepDisplaced.clear();
  pf_hasTimeCompatibleWithPV.clear();

 
  jetDeltaR_puppi_all_.clear();
  jetDeltaR_pfraw_all_.clear();
  jetDeltaR_fromPV3_all_.clear();
  jetDeltaR_tight_all_.clear();
  jetDeltaR_loose_all_.clear();
  jetDeltaR_time_all_.clear();

  jetIsHS_puppi_all_.clear();
  jetPt_puppi_all_.clear();
  genPt_puppi_all_.clear();
  jetResponse_PR_puppi_all_.clear();
  jetAbsEta_puppi_all_.clear(); 
  puFracPt_algo_puppi_all_.clear(); 
  puFracCount_algo_puppi_all_.clear();
  puFracPt_truth_puppi_all_.clear(); 
  puFracCount_truth_puppi_all_.clear();

 /*
  jetPt_puppi_5leading_.clear();
  genPt_puppi_5leading_.clear();
  jetResponse_PR_puppi_5leading_.clear();
  jetAbsEta_puppi_5leading_.clear(); 
  puFracPt_puppi_5leading_.clear(); 
  puFracCount_puppi_5leading_.clear();
*/

  jetIsHS_pfraw_all_.clear();
  jetPt_pfraw_all_.clear();
  genPt_pfraw_all_.clear();
  jetResponse_PR_pfraw_all_.clear();
  jetAbsEta_pfraw_all_.clear();
  puFracPt_algo_pfraw_all_.clear(); 
  puFracCount_algo_pfraw_all_.clear();
  puFracPt_truth_pfraw_all_.clear(); 
  puFracCount_truth_pfraw_all_.clear();


  /*
  jetPt_pfraw_5leading_.clear();
  genPt_pfraw_5leading_.clear();
  jetResponse_PR_pfraw_5leading_.clear();
  jetAbsEta_pfraw_5leading_.clear();
  puFracPt_pfraw_5leading_.clear(); 
  puFracCount_pfraw_5leading_.clear();
*/


  jetIsHS_fromPV3_all_.clear();
  jetPt_fromPV3_all_.clear();
  genPt_fromPV3_all_.clear();
  jetResponse_PR_fromPV3_all_.clear();
  jetAbsEta_fromPV3_all_.clear();
  puFracPt_algo_fromPV3_all_.clear(); 
  puFracCount_algo_fromPV3_all_.clear();
  puFracPt_truth_fromPV3_all_.clear(); 
  puFracCount_truth_fromPV3_all_.clear();


  /*
  jetPt_fromPV3_5leading_.clear();
  genPt_fromPV3_5leading_.clear();
  jetResponse_PR_fromPV3_5leading_.clear();
  jetAbsEta_fromPV3_5leading_.clear();
  puFracPt_fromPV3_5leading_.clear(); 
  puFracCount_fromPV3_5leading_.clear();
*/

  jetIsHS_tight_all_.clear();
  jetPt_tight_all_.clear();
  genPt_tight_all_.clear();
  jetResponse_PR_tight_all_.clear();
  jetAbsEta_tight_all_.clear();
  puFracPt_algo_tight_all_.clear(); 
  puFracCount_algo_tight_all_.clear();
  puFracPt_truth_tight_all_.clear(); 
  puFracCount_truth_tight_all_.clear();

/*
  jetPt_tight_5leading_.clear();
  genPt_tight_5leading_.clear();
  jetResponse_PR_tight_5leading_.clear();
  jetAbsEta_tight_5leading_.clear();
  puFracPt_tight_5leading_.clear(); 
  puFracCount_tight_5leading_.clear();
*/
  
  jetIsHS_loose_all_.clear();
  jetPt_loose_all_.clear();
  genPt_loose_all_.clear();
  jetResponse_PR_loose_all_.clear();
  jetAbsEta_loose_all_.clear();
  puFracPt_algo_loose_all_.clear(); 
  puFracCount_algo_loose_all_.clear();
  puFracPt_truth_loose_all_.clear(); 
  puFracCount_truth_loose_all_.clear();

  /*
  jetPt_loose_5leading_.clear();
  genPt_loose_5leading_.clear();
  jetResponse_PR_loose_5leading_.clear();
  jetAbsEta_loose_5leading_.clear();
  puFracPt_loose_5leading_.clear(); 
  puFracCount_loose_5leading_.clear();
*/

  jetIsHS_time_all_.clear();
  jetPt_time_all_.clear();
  genPt_time_all_.clear();
  jetResponse_PR_time_all_.clear();
  jetAbsEta_time_all_.clear();
  puFracPt_algo_time_all_.clear(); 
  puFracCount_algo_time_all_.clear();
  puFracPt_truth_time_all_.clear(); 
  puFracCount_truth_time_all_.clear();

/*
  jetPt_time_5leading_.clear();
  genPt_time_5leading_.clear();
  jetResponse_PR_time_5leading_.clear();
  jetAbsEta_time_5leading_.clear();
  jetTime_time_5leading_.clear();
  jetTimeError_time_5leading_.clear();
  puFracPt_time_5leading_.clear(); 
  puFracCount_time_5leading_.clear();
*/

  int N_HS_total = 0;
//  int N_selected = 0;
  int N_selected_fromPV3 = 0;
  int N_selected_tight = 0;
  int N_selected_HS_tight = 0;
  int N_selected_loose = 0;
  int N_selected_HS_loose = 0;
  int N_charged = 0;
  int N_neutral = 0; 

// Retrieve jet collection
  edm::Handle<std::vector<pat::Jet>> jets;
  iEvent.getByToken(jetsToken_, jets);
  if (!jets.isValid()) {
    edm::LogError("JetTreeProducer") << "Missing input: jetsToken. Skipping event.";
    return;
  }
  const auto& jetsColl = *jets;


  // Retrieve Genjet collection
  edm::Handle<std::vector<reco::GenJet>> genJetsHandle;
  iEvent.getByToken(genJetToken_,genJetsHandle);
  if (!genJetsHandle.isValid()) {
    edm::LogError("JetTreeProducer") << "Missing input: genJetsToken";
  }


// -------------------------
// ✅ ADDED SAFEGUARDS: genJets validity check
// -------------------------
if (!genJetsHandle.isValid()) {
  edm::LogWarning("JetTreeProducer") 
    << "⚠️ genJetsHandle not found in event. Skipping event " 
    << iEvent.id().event() << ".";
  return;  // stop processing this event
}

if (genJetsHandle->empty()) {
  edm::LogWarning("JetTreeProducer")
    << "⚠️ genJetsHandle is empty in event " 
    << iEvent.id().event() << ". Skipping event.";
  return;
}//End of  ✅ ADDED SAFEGUARDS: genJets validity check

// --- Create a reference to the vector ---
const auto& genJets = *genJetsHandle;// ✅ This gives you const std::vector<reco::GenJet>&  
  

// Retrieve primary vertex collection
  std::vector<reco::Vertex> reco_pvs;
  edm::Handle<std::vector<reco::Vertex>> pv_handle;
  iEvent.getByToken(pvsToken_, pv_handle); 
  if (pv_handle.isValid()) {
    reco_pvs = *pv_handle;//reco_pvs is a direct copy of all the primary vertices in this event
  }
 if (!pv_handle.isValid()) {
    edm::LogError("JetTreeProducer") << "Missing input: reco vertex";
  }

  // Retrieve gen vertex
  edm::Handle<GenEventInfoProduct> genEventInfoHandle;
//  edm::Handle<edm::HepMCProduct> genvertex;//Original Handel

auto [genvertex_z_val, genvertex_source] = getGenVertexZ(iEvent);
genvertex_z_ = genvertex_z_val;        // <-- important
edm::LogInfo("JetTreeProducer") << "Generator vertex source: " << genvertex_source 
                                << ", z = " << genvertex_z_;
bool hasGenZ = (genvertex_source != "fallback_zero");

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
  if (!pfColl_handle.isValid()) {
    edm::LogError("JetTreeProducer") << "Missing input: PackedCandidate collection (pf_collection_source). Skipping event.";
    return;
  }
  const auto& pf_coll = *pfColl_handle; // safe dereference only after isValid()


  // Retrieve beam spot
  edm::Handle<reco::BeamSpot> beamspot;
  iEvent.getByToken(bsToken_, beamspot);
  if (!beamspot.isValid()) {
    edm::LogWarning("JetTreeProducer") << "BeamSpot not found in event; beamspot_* will be set to 0.";
    beamspot_x_ = beamspot_y_ = beamspot_z_ = 0.0f;
  } else {
    beamspot_x_ = beamspot->x0();
    beamspot_y_ = beamspot->y0();
    beamspot_z_ = beamspot->z0();
  }

  // Retrieve gen particles (optional)
//  edm::Handle<std::vector<pat::PackedGenParticle>> genp;
//  iEvent.getByToken(genParticlesToken_, genp);
  // Retrieve packed genparticles (MiniAOD) safely
  //New Test
  // Retrieve packed genparticles (MiniAOD) safely
  edm::Handle<std::vector<pat::PackedGenParticle>> genpHandle;
  iEvent.getByToken(genParticlesTokenMiniAOD_, genpHandle);

  // Provide a safe reference (fallback to empty container if not present)
  static const std::vector<pat::PackedGenParticle> emptyGen; // static avoids realloc each event
  const std::vector<pat::PackedGenParticle>& genpVec =
      (genpHandle.isValid() ? *genpHandle : emptyGen);

  if (!genpHandle.isValid()) {
    edm::LogWarning("JetTreeProducer")
        << "PackedGenParticle collection not found in event; truth matching will be skipped for this event.";
  }//New Test
  

  // Fill PackedCandidates
  std::vector<fastjet::PseudoJet> fjInputs_raw;
  std::vector<fastjet::PseudoJet> fjInputs_fromPV3;
  std::vector<fastjet::PseudoJet> fjInputs_tight;
  std::vector<fastjet::PseudoJet> fjInputs_loose;
  std::vector<fastjet::PseudoJet> fjInputs_time;
  std::vector<const pat::PackedCandidate*> pf_for_time;
  std::vector<const pat::PackedCandidate*> pf_for_allCollections;
  pf_for_allCollections.reserve(pf_coll.size());
 
  
  fjInputs_raw.clear();
  fjInputs_fromPV3.clear();
  fjInputs_tight.clear();
  fjInputs_loose.clear();
  fjInputs_time.clear();


  PUContentReco puContentAlgo;
  PFTruthContentReco puContentTruth;

  float pvTime = 0.0f;                  // or your stored primary vertex time
//float pvTime = pvs_t_;
  bool useTimingFallbackPuppi = true;   // set according to your config

  //Define the global PF vector
  std::vector<const pat::PackedCandidate*> pf_all;
  pf_all.reserve(pf_coll.size());
  for (const auto& pf : pf_coll) {
    pf_all.push_back(&pf);
  }

 
  //---PF Particle Loop---


  int pf_coll_index = 0;
  for (size_t iPF = 0; iPF < pf_coll.size(); ++iPF) {

//  for (const auto& pf : pf_coll) {
//  for (size_t i = 0; i < pf_coll.size(); ++i)
    const auto& pf = pf_coll[iPF];

    //Diagnostic print of associatedPVIndex
    int pvIndex = pf.vertexRef().key();
    auto pvQuality = pf.pvAssociationQuality();

//    std::cout << "PF[" << iPF << "] PV index = " << pf.vertexRef().key()
//              << " quality = " << pf.pvAssociationQuality() << std::endl;
//    if (pvIndex != 0) { std::cout <<"isPU = true"<< std::endl;}
//    else {std::cout <<"isPU = false"<<std::endl;}
//    std::cout << "pT=" << pf.pt()
//          << " pvAssocQuality=" << pf.pvAssociationQuality()
//          << " fromPV=" << pf.fromPV()
//          << std::endl;
    //End of Diagnostic print of associatedPVIndex

    pf_for_allCollections.push_back(&pf);
    pf_pt.push_back(pf.pt());
    pf_eta.push_back(pf.eta());
    pf_phi.push_back(pf.phi());
    pf_energy.push_back(pf.energy());
    pf_charge.push_back(pf.charge());
    pf_puppiWeight.push_back(pf.puppiWeight());
    pf_puppiWeightNoLep.push_back(pf.puppiWeightNoLep());
    pf_pvQuality.push_back(pf.pvAssociationQuality());
    pf_pvIndex.push_back(pf.vertexRef().key());
// vertex is always available, but dz/dzError are not
    pf_vx.push_back(pf.vertex().x());
    pf_vy.push_back(pf.vertex().y());
    pf_vz.push_back(pf.vertex().z());
    pf_pdgId.push_back(pf.pdgId());
    pf_fromPV.push_back(pf.fromPV());
   
    //==========PF HS/PU check====================
    // inside pf loop, for each const pat::PackedCandidate& pf:
//    float pvTime = pvs_t_.empty() ? 0.f : pvs_t_[0];
    float pvTime = pvs_t_;  
    bool isPU_algo = (pf.charge() != 0)
                    ? isChargedPU(pf)
                    : isNeutralPU(pf, pvTime, /*useTimingFallbackPuppi=*/true);
   // store algorithmic labels
    pf_isPU_algo.push_back(isPU_algo ? 1 : 0);
    pf_isHS_algo.push_back(isPU_algo ? 0 : 1);


    // Truth classification via Gen match ---
    // PV-consistency check is already enforced inside matchToGen,
    // so no extra vz cut is needed here.

      //New Test         
      // Safe truth-match: only attempt if gen collection is present (genpVec non-empty)
      const pat::PackedGenParticle* matchedGen = nullptr;
      float dR_value = -1.f;

      if (!genpVec.empty()) {
      auto matchResult = matchToGen(pf, genpVec,(pf.charge() == 0 ? 1.0 : 0.4));
      matchedGen = matchResult.first;
      dR_value   = matchResult.second;
      }

     pf_dR_truth_.push_back(matchedGen ? dR_value : -1.f);

    bool isHS_truth = (matchedGen != nullptr);
    pf_isPU_truth.push_back(!isHS_truth);
    pf_isHS_truth.push_back(isHS_truth); 
    //==========PF HS/PU check End====================i
#ifdef DEBUG_PU_DIAG
  // === Temporary counters (declare static so they persist between events)
  static unsigned int totalPF_all = 0;
  static unsigned int totalPF_algoPU = 0;
  static unsigned int totalPF_truthPU = 0;
  static unsigned int totalPF_algoMis = 0;  // algorithmic misclassification
  static unsigned int totalPF_truthMis = 0; // truth-based misclassification
  static unsigned int nEventsChecked = 0;

  edm::Handle<std::vector<reco::Vertex>> pv_handle;
  iEvent.getByToken(pvsToken_, pv_handle);
  const size_t nPV = pv_handle.isValid() ? pv_handle->size() : 0;

  // --- Only run this diagnostic for no-PU samples (nPV <= 1)
  if (nPV <= 1) {

    // Local event counters
    unsigned int nPF_algoPU = 0, nPF_truthPU = 0, nPF_total = 0;
    unsigned int nMis_algo = 0, nMis_truth = 0;

    for (size_t iPF = 0; iPF < pfcands->size(); ++iPF) {
      const auto& pf = (*pfcands)[iPF];
      bool algoPU   = pf_isPU_algo[iPF];
      bool truthPU  = pf_isPU_truth[iPF];
      bool misAlgo  = (algoPU != truthPU); // mismatch between algo & truth classification

      ++nPF_total;
      if (algoPU) ++nPF_algoPU;
      if (truthPU) ++nPF_truthPU;
      if (misAlgo) ++nMis_algo;

      // Print first few PFs for inspection
      if (iPF < 5 && iEvent.id().event() < 10) {
        std::cout << "[DiagPF] evt=" << iEvent.id().event()
                  << " PF[" << iPF << "] pt=" << pf.pt()
                  << " eta=" << pf.eta()
                  << " charge=" << pf.charge()
                  << " fromPV=" << pf.fromPV()
                  << " pvAssocQ=" << pf.pvAssociationQuality()
                  << " algoPU=" << algoPU
                  << " truthPU=" << truthPU
                  << std::endl;
      }
    }

    // Event-level fractions
    float frac_algoPU = (nPF_total > 0) ? float(nPF_algoPU) / nPF_total : 0;
    float frac_truthPU = (nPF_total > 0) ? float(nPF_truthPU) / nPF_total : 0;
    float frac_mis = (nPF_total > 0) ? float(nMis_algo) / nPF_total : 0;

    std::cout << std::fixed << std::setprecision(3)
              << "[DiagSummary] evt=" << iEvent.id().event()
              << " nPF=" << nPF_total
              << " algoPU=" << frac_algoPU * 100 << "%"
              << " truthPU=" << frac_truthPU * 100 << "%"
              << " misAlgo=" << frac_mis * 100 << "%"
              << std::endl;

    // Update global counters
    totalPF_all += nPF_total;
    totalPF_algoPU += nPF_algoPU;
    totalPF_truthPU += nPF_truthPU;
    totalPF_algoMis += nMis_algo;
    ++nEventsChecked;
  }

  // --- Print global average every 50 events
  if (nEventsChecked % 50 == 0 && totalPF_all > 0) {
    float avg_algoPU  = 100.0f * totalPF_algoPU  / totalPF_all;
    float avg_truthPU = 100.0f * totalPF_truthPU / totalPF_all;
    float avg_mis     = 100.0f * totalPF_algoMis / totalPF_all;
    std::cout << "[DiagGlobal] after " << nEventsChecked << " events: "
              << " avg_algoPU=" << avg_algoPU << "%"
              << " avg_truthPU=" << avg_truthPU << "%"
              << " avg_mis=" << avg_mis << "%"
              << std::endl;
  }
#endif


    float eta = pf.eta(); 
    bool isInMTD = (std::abs(eta) <= 3.0);
    pf_isInMTD.push_back(isInMTD ? 1 : 0);      
    const bool isCharged = (pf.charge() != 0);
    const bool isNeutral = (pf.charge() == 0);
    const int fromPV = pf.fromPV();
    const bool keepAlways = (fromPV == 3);
    pf_isCharged.push_back(isCharged ? 1 : 0);

    // inside PF candidate loop, before any use:
    float dtSig = 999.0f;               // sentinel (invalid)
    const float dtSigCut = 3.0f;        // tune this threshold (was missing)
 
    bool isDisplaced = false;//To keep displaced tracks
    bool hasValidTime = false;
    bool hasTimingInfo = false;

    // Common PseudoJet for dz-based clustering
    fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
//    int pf_coll_index = &pf - &pf_coll[0]; // index relative to pf_coll
//    pj.set_user_index(pf_coll_index);// link PF index    
    pj.set_user_index(static_cast<int>(iPF)); // simpler and safe   
    fjInputs_raw.push_back(pj);//keep global-indexed PF for raw jets

    if (pf.hasTrackDetails() && isCharged) {//For charged particle
       ++N_charged;
//      bool isHS = (pf.fromPV() > 1);  // Not correct definidtion fo isHS, fromPV: 3 = tightly associated to PV
//      if (isHS) ++N_HS_total;

      float dz     = pf.dz();
      float dzErr  = pf.dzError();
      float dzSig  = (dzErr > 0) ? dz / dzErr : 999;
      isDisplaced = (std::abs(dz) > 0.05);//To keep displaced tracks
      
      pf_dxy.push_back(pf.dxy());
      pf_dz.push_back(dz);
      pf_dzError.push_back(dzErr);
      pf_dzSig.push_back(dzSig);  
 
      // compute only if timing is valid:
      float t = pf.time();//nanoseconds
      float tErr = pf.timeError();//nanoseconds
      
      // === Robust validity check ===
      // Treat as "valid" only if error is positive and not too large
      hasValidTime = (
          isInMTD &&        //Ensures only trust time info(in MTD acceptance)
          tErr > 0 &&
          tErr < 0.2 &&     // Conservative threshold (30â50 ps typical)
          std::abs(t) <10  // sanity check: time should be < 10 ns, change from 100 to 10=>Selection result not changed. 
      );
      pf_hasValidTime.push_back(hasValidTime ? 1 : 0);//
     

      if (hasValidTime) {
      //Safe to use time
        float dt = t - pvs_t_;
        float dtErr =std::sqrt( tErr*tErr + pvs_TimeErr_*pvs_TimeErr_ );
        float dtSig = dt / std::sqrt(tErr*tErr + pvs_TimeErr_*pvs_TimeErr_);//

        pf_time.push_back(pf.time());
        pf_timeError.push_back(pf.timeError());
        pf_dt.push_back(dt);
        pf_dtErr.push_back(dtErr);
        pf_dtSig.push_back(dtSig);
  
        // Optional: log debug info
//        edm::LogWarning("TimingCheck")
//        << "PF with eta = " << eta
//        << " has timing info (t = " << t << " ns, error = " << tErr << " ns)"
//        << " ns, dtSig=" << dtSig;
//        << " outside MTD acceptance!";
      } else {
        // Invalid or missing time
        pf_time.push_back(-999);
        pf_timeError.push_back(999);
        pf_dt.push_back(999);
        pf_dtErr.push_back(999);
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
      bool passesTightCut = (std::abs(dz) < 0.03 && dzSig < 0.2);//3D tight selection
      bool passesLooseCut = (std::abs(dz) < 0.05 && dzSig < 0.5);//3D loos selection
      bool keepDisplaced = (isDisplaced && hasValidTime);//4D selection
      bool hasTimeCompatibleWithPV= hasValidTime && (std::abs(dtSig) < dtSigCut);
      bool passes3D = (std::abs(dz) < dzCut) && (std::abs(dzSig) < dzSigCut);
      bool passes4D = passes3D && hasValidTime && (std::abs(dtSig) < dtSigCut);      

      pf_passesTightCut.push_back(passesTightCut ? 1 : 0);
      pf_passesLooseCut.push_back(passesLooseCut ? 1 : 0);
      pf_keepAlways.push_back(keepAlways ? 1 : 0);
      pf_keepDisplaced.push_back(keepDisplaced ? 1 : 0);
      pf_hasTimeCompatibleWithPV.push_back(hasTimeCompatibleWithPV ? 1 : 0);
      pf_passes3D.push_back(passesLooseCut ? 1 : 0);
      pf_passes4D.push_back(passesLooseCut ? 1 : 0);


      if (keepAlways) {
//      if (keepAlways&&hasValidTime && (std::abs(dtSig) < dtSigCut)) {//To save track has large dz,4D selection
        fjInputs_fromPV3.push_back(pj);
        ++N_selected_fromPV3;
        
       // if (isHS) ++N_selected_HS_tight;
      }
      if (passesTightCut) {
//      if (passesTightCut&&hasValidTime && (std::abs(dtSig) < dtSigCut) {//To save track has large dz,4D selection
        fjInputs_tight.push_back(pj);
        ++N_selected_tight;
       // if (isHS) ++N_selected_HS_tight;
      }
      if (passesLooseCut) {
//      if (passesLooseCut&&hasValidTime && (std::abs(dtSig) < dtSigCut) {//To save track has large dz, 4D selection
        fjInputs_loose.push_back(pj);
        ++N_selected_loose;
      //  if (isHS) ++N_selected_HS_loose;
      }
        // MTD-based selection
//      if (pf.isTimeValid() && pf.timeError() < 0.05) {//pat::PackedCandidate does not have a method called .isTimeValid()
//      if (hasValidTime) {
      if (hasTimeCompatibleWithPV) {

      // create a local copy if it need separate UserInfo, but keep the same index
//        pj_time.set_user_index(pf_for_time.size());//local index bug
//        pj_time.set_user_index(static_cast<int>(iPF));//index back to PackedCandidate
//        fastjet::PseudoJet pj_time(pf.px(), pf.py(), pf.pz(), pf.energy());

        // Reuse the same PseudoJet (already has global user_index)
        fastjet::PseudoJet pj_time = pj; // copy keeps iPF index
        fjInputs_time.push_back(pj_time);
        pf_for_time.push_back(&pf);
      }
    } else {
      // Fill with defaults to avoid division by zero, preserve structure
      pf_dxy.push_back(0);
      pf_dz.push_back(0);
      pf_dzError.push_back(1e6);  // avoid division by zero
      pf_dzSig.push_back(0);
      pf_time.push_back(0);
      pf_timeError.push_back(1e6);
      pf_dt.push_back(1e6);
      pf_dtErr.push_back(1e6);
      pf_dtSig.push_back(1e6);

      fjInputs_fromPV3.push_back(pj);
      fjInputs_tight.push_back(pj);
      fjInputs_loose.push_back(pj);
      fjInputs_time.push_back(pj);
      pf_isInMTD.push_back(0);
       
      //Neutral candidate -no dz/dzSig
      //CMSSW_15_1_0_pre4/Optionally:include all neutrals in tight/loose
      ++N_neutral;
    }//End of if (hasValidTime) else 
    //outside the MTD geometry &seem to have time info:Indicate a reconstruction or simulation artifact.
    ++pf_coll_index;
  }//End of for (const auto& pf : pf_coll)


/*
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
*/


// Run the jet clustering algorithm on each collection
  fastjet::JetDefinition jetDef(fastjet::antikt_algorithm, 0.4);
  auto cs_raw = fastjet::ClusterSequence(fjInputs_raw, jetDef);
  auto pfrawJets = fastjet::sorted_by_pt(cs_raw.inclusive_jets(10.0));//eturn only jets with pt ≥ 20 GeV.
  
  auto cs_fromPV3= fastjet::ClusterSequence(fjInputs_fromPV3, jetDef);
  auto fromPV3Jets = fastjet::sorted_by_pt(cs_fromPV3.inclusive_jets(10.0));
  
  auto cs_tight = fastjet::ClusterSequence(fjInputs_tight, jetDef);
  auto tightJets = fastjet::sorted_by_pt(cs_tight.inclusive_jets(10.0));

  auto cs_loose = fastjet::ClusterSequence(fjInputs_loose, jetDef);
  auto looseJets = fastjet::sorted_by_pt(cs_loose.inclusive_jets(10.0));

  auto cs_time = fastjet::ClusterSequence(fjInputs_time, jetDef);
  auto timeJets = fastjet::sorted_by_pt(cs_time.inclusive_jets(10.0));


  // diagnostic: check user_index min/max for each fjInputs_* before clustering
  auto diag_index_range = [](const std::vector<fastjet::PseudoJet>& v, const char* name){
    if(v.empty()){
      edm::LogInfo("JetTreeProducer") << name << " empty";
      return;
    }
    int minI = INT_MAX, maxI = INT_MIN;
    for (const auto &pj : v) {
      int u = pj.user_index();
      minI = std::min(minI, u);
      maxI = std::max(maxI, u);
    }  
    edm::LogInfo("JetTreeProducer") << name << " count=" << v.size()
                                 << " user_index range = [" << minI << "," << maxI << "]";
  };

  diag_index_range(fjInputs_raw, "fjInputs_raw");
  diag_index_range(fjInputs_fromPV3, "fjInputs_fromPV3");
  diag_index_range(fjInputs_tight, "fjInputs_tight");
  diag_index_range(fjInputs_loose, "fjInputs_loose");
  diag_index_range(fjInputs_time, "fjInputs_time");


//Jet-level MTD time: PU jet rejection studies using MTD timing at the jet level.
//Computes per-jet timing observables using the MTD (Minimum Timing Detector) information stored in the PF candidates
//Jets from the hard scatter should have times consistent with the PV (jetTsig ≈ 0).
//Pileup jets can have displaced times (jetTsig large)
for (const auto& jet : timeJets) {
    float sumPt = 0, sumWeightedT = 0, sumVar = 0;
    for (const auto& c : jet.constituents()) {
        int idx = c.user_index();//c is a fastjet PseudoJet,c.user_index() gives you back the index of the original PF candidate.
        if (idx < 0 || (size_t)idx >= pf_for_time.size()) continue;
        const auto* pf = pf_for_time[idx];

        if (!pf) continue;
        //Only keep PFs with valid timing
        if (pf->timeError() <= 0 || pf->timeError() > 0.2) continue;//reject bad timing resolution

        // Weight = PF transverse momentum
        float w = pf->pt();
        sumPt += w;//Σ (pT)
        sumWeightedT += w * pf->time();//Σ (pT × time)
        sumVar += w*w * (pf->timeError()*pf->timeError());//Σ (pT² × σ_time²)
    }
    // Jet time significance: distance from PV time in units of error
    float jetT = (sumPt>0) ? sumWeightedT/sumPt : -999;// weighted jet time
    float jetTerr = (sumPt>0) ? std::sqrt(sumVar)/sumPt : 999;//uncertainty on jet time,estimated error
    // Jet time significance=distance between jet time and primary vertex time
    float jetTsig = (jetTerr>0) ? fabs(jetT - pvs_t_)/jetTerr : 999;

    // Save
    jetTime_time_all_.push_back(jetT);
    jetTimeError_time_all_.push_back(jetTerr);
    jetTimeSig_time_all_.push_back(jetTsig);
}//End of for (const auto& jet : timeJets)



//===== Generic std::vector<pat::Jet> Loop Template=====
auto processJetCollection = [&](const std::vector<pat::Jet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,

                                std::vector<int>& jetIsHS_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& puFracPt_algo_all_,
                                std::vector<float>& puFracCount_algo_all_,
                                std::vector<float>& puFracPt_truth_all_,
                                std::vector<float>& puFracCount_truth_all_,
                                std::vector<float>& jetDeltaR_all_,
                                std::vector<std::vector<unsigned int>>& pf_indices_general_all_,//per-jet PF indices
/*
                                std::vector<float>& jetPt_5leading_,
                                std::vector<float>& jetAbsEta_5leading_,
                                std::vector<float>& jetResponse_5leading_,
                                std::vector<float>& genPt_5leading_,
                                std::vector<float>& puFracPt_5leading_,
                                std::vector<float>& puFracCount_5leading_,
*/
				//per-collection summary variables
                                int& totalRecoJets,
                                int& totalPUJets,
                                float& puJetFraction_all_,
				float& efficiency_out,
				float& purity_out) {
    int idx = 0;
    totalRecoJets = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;
    int nMatchedR = 0;

    // Compute nGenJetsHS with the same minGenPt useed above
    int nGenJetsHS = 0; // apply pt>10 cut here
    for (const auto& g : genJets) {
      if (g.pt() > 10.0) nGenJetsHS++;

    }

    for (const auto& jet : jetsIn) {
        totalRecoJets++;

        // Get PF indices for this jet
        std::vector<unsigned int> pf_indices_this_jet =  getPFIndicesFromPatJet(jet);
         pf_indices_general_all_.push_back(pf_indices_this_jet);

        // Jet ↔ GenJet matching/ classificaiton
        auto  truthMatch = classifyJetHS(jet, genJets,
                                0.3,        // dRMax, 0.3
                                10.0,       // minGenPt 10
                                0.3,        // maxDz (loose PV cut)0.3
                                genvertex_z_);  // generator PV

         const reco::GenJet* matchedGenJet = truthMatch.matchedGen;          
           
           float minDR = truthMatch.dR; // example: distance saved in the struct
           bool isHSjet = truthMatch.isHS;
           bool isPUjet = !isHSjet;//jetIsHS_all_
        if (matchedGenJet) nMatchedR++;
     
        // --- NEW: store ΔR to matched gen jet
        jetDeltaR_all_.push_back(truthMatch.dR);  // use the correct vector per collection
        
        // HS/PU counting (strict)
        if (!truthMatch.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, matchedGenJet);

	//=====================================================================
	//  Compute per-jet PU fractions (algorithmic and truth-based)
	//=====================================================================
	PUContentReco algoAcc;
	PFTruthContentReco truthAcc;

//	auto pfIndices = getPFIndicesFromPatJet(jet,pf_for_allCollections);
	auto pfIndices = getPFIndicesFromPatJet(jet);
	for (unsigned int idx : pfIndices) {
  	if (idx >= pf_for_allCollections.size()) continue;
  	const pat::PackedCandidate* pf = pf_for_allCollections[idx];

  	// algorithmic classification
//  	float pvTime = pvs_t_.empty() ? 0.f : pvs_t_[0];
        float pvTime = pvs_t_;  	
  	updatePUContentReco(pf, algoAcc, pvTime, useTimingFallbackPuppi);

  	// truth classification (reuse cached pf_isPU_truth)
  	updatePFTruthContentReco(pf, truthAcc, idx, pf_isPU_truth);
	}

	// Calculate fractions safely
	float puFracCount_algo  = (algoAcc.nTot     > 0) ? float(algoAcc.nPU) / algoAcc.nTot     : -1.f;
	float puFracPt_algo     = (algoAcc.sumPtTot > 0) ? algoAcc.sumPtPU   / algoAcc.sumPtTot  : -1.f;
	float puFracCount_truth = (truthAcc.nTot    > 0) ? float(truthAcc.nPU)/ truthAcc.nTot    : -1.f;
	float puFracPt_truth    = (truthAcc.sumPtTot> 0) ? truthAcc.sumPtPU  / truthAcc.sumPtTot : -1.f;


        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        genPt_all_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
        jetResponse_all_.push_back(response);
        jetIsHS_all_.push_back(truthMatch.isHS ? 1 : 0);
        puFracCount_algo_all_.push_back(puFracCount_algo);//algorithm-based
        puFracPt_algo_all_.push_back(puFracPt_algo);//algorithm-based
        puFracCount_truth_all_.push_back(puFracCount_truth);//truth-based
        puFracPt_truth_all_.push_back(puFracPt_truth);//truth-based

       //Charged classification is robust (PV association). Neutral classification uses timing when available, falling back to a conservative PUPPI weight heuristic.
       
/*
        // Save LEADING 5 jets
        if (idx < 5) {
            jetPt_5leading_.push_back(jet.pt());
            jetAbsEta_5leading_.push_back(std::abs(jet.eta()));
            genPt_5leading_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
            jetResponse_5leading_.push_back(response);
            puFracCount_5leading_.push_back(pu.fracCount);
            puFracPt_5leading_.push_back(pu.fracPt);
        }//if (idx < 5)
*/
        idx++;
    }

    //PU jet fraction
    puJetFraction_all_ = (totalRecoJets > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJets
        : -1.0f;
   
    //Jet efficiency/purity: Count HS vs PU jets explicitly 
    //efficiency = fraction of gen jets reconstructed.
    //purity = fraction of  jetIsHS_all_.push_back(truthMatch.isHS ? 1 : 0);reco jets that are HS.    
    const int nRecoJetsHS = totalRecoJets - totalPUJets;
    efficiency_out = (nGenJetsHS > 0)     ? float(nRecoJetsHS)/nGenJetsHS : -1.0f;
    purity_out     = (totalRecoJets > 0)   ? float(nRecoJetsHS)/totalRecoJets : -1.0f;

};



//===== Generic std::vector<fastjet::PseudoJet> Loop Template=====
auto processFastJetCollection = [&](const std::vector<fastjet::PseudoJet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,

                                std::vector<int>& jetIsHS_all_,
                                std::vector<float>& jetPt_all_,
                                std::vector<float>& jetAbsEta_all_,
                                std::vector<float>& jetResponse_all_,
                                std::vector<float>& genPt_all_,
                                std::vector<float>& puFracPt_algo_all_,
                                std::vector<float>& puFracCount_algo_all_,
                                std::vector<float>& puFracPt_truth_all_,
                                std::vector<float>& puFracCount_truth_all_,
                                std::vector<float>& jetDeltaR_all_,
                                std::vector<std::vector<unsigned int>>& pf_indices_general_all_,//per-jet PF indices
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
                                float& puJetFraction_all_,
				float& efficiency_out,
				float& purity_out) {
    int idx = 0;
    totalRecoJets = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;

    // Compute nGenJetsHS with the same minGenPt(pt > 10)
    int nGenJetsHS = 0; // apply pt>10 cut here
    for (const auto& g : genJets) {
      if (g.pt() > 10.0) nGenJetsHS++;
    }

    for (const auto& jet : jetsIn) {
        totalRecoJets++;

        // Get PF indices for this jet
        auto pf_indices_this_jet = getPFIndicesFromPseudoJet(jet, pf_for_allCollections);
        pf_indices_general_all_.push_back(pf_indices_this_jet);

        auto truthMatch = classifyJetHS(jet, genJets,
                                0.3,        // dRMax
                                10.0,       // minGenPt
                                0.3,        // maxDz (loose PV cut)
                                genvertex_z_);  // generator PV
         const reco::GenJet* matchedGenJet = truthMatch.matchedGen;      
     
           float minDR = truthMatch.dR; // example: distance saved in the struct
           bool isHSjet = truthMatch.isHS;
           bool isPUjet = !isHSjet;

        if (matchedGenJet) nMatchedReco++;
     
        // --- NEW: store ΔR to matched gen jet
        jetDeltaR_all_.push_back(truthMatch.dR);  // use the correct vector per collection

        // HS/PU counting (strict)
        if (!truthMatch.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, matchedGenJet);

	//=====================================================================
	//  Compute per-fastjet PU fractions (algorithmic and truth-based)
	//=====================================================================
	PUContentReco algoAcc;
	PFTruthContentReco truthAcc;

	auto pfIndices = getPFIndicesFromPseudoJet(jet, pf_for_allCollections);
	for (unsigned int idx : pfIndices) {
  	if (idx >= pf_for_allCollections.size()) continue;
 	 const pat::PackedCandidate* pf = pf_for_allCollections[idx];

         // algorithmic classification
//        float pvTime = pvs_t_.empty() ? 0.f : pvs_t_[0];
        float pvTime = pvs_t_;
  	updatePUContentReco(pf, algoAcc, pvs_t_, useTimingFallbackPuppi);
        
        // truth update using cached per-PF truth flags
  	updatePFTruthContentReco(pf, truthAcc, idx, pf_isPU_truth);
	}

	float puFracCount_algo  = (algoAcc.nTot     > 0) ? float(algoAcc.nPU) / algoAcc.nTot     : -1.f;
	float puFracPt_algo     = (algoAcc.sumPtTot > 0) ? algoAcc.sumPtPU   / algoAcc.sumPtTot  : -1.f;
	float puFracCount_truth = (truthAcc.nTot    > 0) ? float(truthAcc.nPU)/ truthAcc.nTot    : -1.f;
	float puFracPt_truth    = (truthAcc.sumPtTot> 0) ? truthAcc.sumPtPU  / truthAcc.sumPtTot : -1.f;

        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        genPt_all_.push_back(matchedGenJet ? matchedGenJet->pt() : -1.0);
        jetResponse_all_.push_back(response);
        jetIsHS_all_.push_back(truthMatch.isHS ? 1 : 0);
	puFracCount_algo_all_.push_back(puFracCount_algo);
	puFracPt_algo_all_.push_back(puFracPt_algo);
	puFracCount_truth_all_.push_back(puFracCount_truth);
	puFracPt_truth_all_.push_back(puFracPt_truth);

/*
        // Save LEADING 5 jetstruth.isHS
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
    // PU jet fraction
    puJetFraction_all_ = (totalRecoJets > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJets
        : -1.0f;
   
//    Jet efficiency/purity: Count HS vs PU jets explicitly 
    const int nRecoJetsHS = totalRecoJets - totalPUJets;
    efficiency_out = (nGenJetsHS > 0)     ? float(nRecoJetsHS)/nGenJetsHS : -1.0f;
    purity_out     = (totalRecoJets > 0)   ? float(nRecoJetsHS)/totalRecoJets : -1.0f;
};

processJetCollection(*jets,  genJets, 
	jetIsHS_puppi_all_, jetPt_puppi_all_, jetAbsEta_puppi_all_, jetResponse_PR_puppi_all_,genPt_puppi_all_, puFracPt_algo_puppi_all_, puFracCount_algo_puppi_all_, puFracPt_truth_puppi_all_, puFracCount_truth_puppi_all_,jetDeltaR_puppi_all_,pf_indices_puppi_all_,
//	jetPt_puppi_5leading_, jetAbsEta_puppi_5leading_, jetResponse_PR_puppi_5leading_, genPt_puppi_5leading_, puFracPt_puppi_5leading_, puFracCount_puppi_5leading_,
	totalRecoJets_puppi, totalPUJets_puppi, puJetFraction_puppi_all_,efficiency_puppi_, purity_puppi_);


//const auto& pfrawJetsConst = pfrawJets;
processFastJetCollection(pfrawJets,  genJets, 
	jetIsHS_pfraw_all_, jetPt_pfraw_all_, jetAbsEta_pfraw_all_, jetResponse_PR_pfraw_all_,genPt_pfraw_all_, puFracPt_algo_pfraw_all_, puFracCount_algo_pfraw_all_,puFracPt_truth_pfraw_all_, puFracCount_truth_pfraw_all_,jetDeltaR_pfraw_all_,pf_indices_pfraw_all_,
//	jetPt_pfraw_5leading_, jetAbsEta_pfraw_5leading_, jetResponse_PR_pfraw_5leading_, genPt_pfraw_5leading_, puFracPt_pfraw_5leading_, puFracCount_pfraw_5leading_,
	totalRecoJets_pfraw, totalPUJets_pfraw, puJetFraction_pfraw_all_,efficiency_pfraw_, purity_pfraw_);

processFastJetCollection(fromPV3Jets,  genJets, 
	jetIsHS_fromPV3_all_, jetPt_fromPV3_all_, jetAbsEta_fromPV3_all_, jetResponse_PR_fromPV3_all_,genPt_fromPV3_all_, puFracPt_algo_fromPV3_all_, puFracCount_algo_fromPV3_all_,puFracPt_truth_fromPV3_all_, puFracCount_truth_fromPV3_all_,jetDeltaR_fromPV3_all_,pf_indices_fromPV3_all_,
//	jetPt_fromPV3_5leading_, jetAbsEta_fromPV3_5leading_, jetResponse_PR_fromPV3_5leading_, genPt_fromPV3_5leading_, puFracPt_fromPV3_5leading_, puFracCount_fromPV3_5leading_,
	totalRecoJets_fromPV3, totalPUJets_fromPV3, puJetFraction_fromPV3_all_,efficiency_fromPV3_, purity_fromPV3_);

processFastJetCollection(tightJets,  genJets, 
	jetIsHS_tight_all_,jetPt_tight_all_, jetAbsEta_tight_all_, jetResponse_PR_tight_all_,genPt_tight_all_, puFracPt_algo_tight_all_, puFracCount_algo_tight_all_,puFracPt_truth_tight_all_, puFracCount_truth_tight_all_,jetDeltaR_tight_all_,pf_indices_tight_all_,
//	jetPt_tight_5leading_, jetAbsEta_tight_5leading_, jetResponse_PR_tight_5leading_, genPt_tight_5leading_, puFracPt_tight_5leading_, puFracCount_tight_5leading_,
	totalRecoJets_tight, totalPUJets_tight, puJetFraction_tight_all_,efficiency_tight_, purity_tight_);


processFastJetCollection(looseJets,  genJets,
	jetIsHS_loose_all_, jetPt_loose_all_, jetAbsEta_loose_all_, jetResponse_PR_loose_all_,genPt_loose_all_, puFracPt_algo_loose_all_, puFracCount_algo_loose_all_,puFracPt_truth_loose_all_, puFracCount_truth_loose_all_,jetDeltaR_loose_all_,pf_indices_loose_all_,
//	jetPt_loose_5leading_, jetAbsEta_loose_5leading_, jetResponse_PR_loose_5leading_, genPt_loose_5leading_, puFracPt_loose_5leading_, puFracCount_loose_5leading_,
	totalRecoJets_loose, totalPUJets_loose, puJetFraction_loose_all_,efficiency_loose_, purity_loose_);


processFastJetCollection(timeJets,  genJets, 
	jetIsHS_time_all_, jetPt_time_all_, jetAbsEta_time_all_, jetResponse_PR_time_all_,genPt_time_all_, puFracPt_algo_time_all_, puFracCount_algo_time_all_,puFracPt_truth_time_all_, puFracCount_truth_time_all_, jetDeltaR_time_all_,pf_indices_time_all_,
//	jetPt_time_5leading_, jetAbsEta_time_5leading_, jetResponse_PR_time_5leading_, genPt_time_5leading_, puFracPt_time_5leading_, puFracCount_time_5leading_,
	totalRecoJets_time, totalPUJets_time, puJetFraction_time_all_,efficiency_time_, purity_time_);


//  jetResponse_PR_tight_ = -1;
//  jetResponse_PR_loose_ = -1;
//  jetAbsEta_ = -1; 

pv_x_.clear();
pv_y_.clear();
pv_z_.clear();
pv_t_.clear();
pv_chi2_.clear();
pv_ndof_.clear();
pv_nTracks_.clear(); 

//Store all primary vertices in a vector
    for (const auto& vtx:reco_pvs) {
    pv_x_.push_back(vtx.x());
    pv_y_.push_back(vtx.y());
    pv_z_.push_back(vtx.z());
    pv_t_.push_back(vtx.t());
    pv_chi2_.push_back(vtx.chi2());
    pv_ndof_.push_back(vtx.ndof());
    pv_nTracks_.push_back(vtx.nTracks());
   }


  // Fill first primary vertex variables
  if (!reco_pvs.empty()) {
    const reco::Vertex& firstPV = reco_pvs[0];

    pvs_x_ = firstPV.x();
    pvs_y_ = firstPV.y();
    pvs_z_ = firstPV.z();
    pvs_t_ = firstPV.t();
    pvs_TimeErr_=firstPV.tError();
  }

  // Fill beam spot information
  if (beamspot.isValid()) {
    beamspot_x_ = beamspot->x0();
    beamspot_y_ = beamspot->y0();
    beamspot_z_ = beamspot->z0();
  }

  // Fill generator particle z-positions
  if (genpVec.empty()) {
    for (const auto& particle : genpVec) {
      genparticles_z_.push_back(particle.vz());
    }
  }


  // Fill generator vertex z-position (already done above, but now write it to tree)
  if (hasGenZ) {
  }
edm::LogPrint("JetTreeProducer") 
     << "Filling event with "
     << pf_keepAlways.size() << " PFs";

edm::LogWarning("particle check")
    << "charged particles = " << N_charged
    << ", neutral particles = " << N_neutral;

edm::LogInfo("JetTreeProducer") << "About to Fill tree, sizes: "
    << " pf_pt=" << pf_pt.size()
    << " pf_indices_pfraw=" << pf_indices_pfraw_all_.size()
    << " pf_indices_pfraw[0].size=" << (pf_indices_pfraw_all_.empty() ? -1 : (int)pf_indices_pfraw_all_[0].size());

std::cout << "PF candidates: " << pf_pt.size()
          << " fromPV: " << pf_fromPV.size()
          << " keepAlways: " << pf_keepAlways.size()
          << " hasValidTime: " << pf_hasValidTime.size()
          << std::endl;

edm::LogInfo("JetTreeProducer") << "Found genJets: size = " << genJets.size();
for (size_t i=0; i < std::min<size_t>(genJets.size(), 5); ++i) {
    const auto& g = genJets[i];
    edm::LogInfo("JetTreeProducer") << "  GenJet[" << i << "]: pt=" << g.pt()
                                   << " eta=" << g.eta()
                                   << " phi=" << g.phi()
                                   << " vz=" << g.vz();
 }

tree_->Fill();
  // sanity-check mapping: each stored pf pointer should match pf_coll[i]
  // If this loop prints nothing, It is safe that  the mapping is correct.
  for (size_t i = 0; i < pf_for_allCollections.size(); ++i) {
    if (pf_for_allCollections[i] != &pf_coll[i]) {
      edm::LogWarning("JetTreeProducer") << "PF pointer mismatch at index " << i;
      // optionally print a few quantities to help debug
      if (pf_for_allCollections[i]) {
        edm::LogWarning("JetTreeProducer") << "pf_for_allCollections[i]->pt() = "
                                          << pf_for_allCollections[i]->pt();
      }
      edm::LogWarning("JetTreeProducer") << "pf_coll[i].pt() = " << pf_coll[i].pt();
    }
  }//End of for (size_t i = 0; i < pf_for_allCollections.size()
  std::vector<int> jetIsHS_puppi_all_;
  std::vector<float> jetPt_puppi_all_;
  std::vector<float> jetAbsEta_puppi_all_;
  std::vector<float> jetResponse_PR_puppi_all_;
  std::vector<float> genPt_puppi_all_;
  std::vector<float> puFracPt_algo_puppi_all_;
  std::vector<float> puFracCount_algo_puppi_all_;
  std::vector<float> puFracPt_truth_puppi_all_;
  std::vector<float> puFracCount_truth_puppi_all_;


  if (!jetPt_puppi_all_.empty()) {
      double sumHS = 0, sumPU = 0;
      int nHS = 0, nPU = 0;
      for (size_t i = 0; i < jetIsHS_puppi_all_.size(); ++i) {
          if (jetIsHS_puppi_all_[i]) {
              sumHS += puFracPt_truth_puppi_all_[i];
              ++nHS;
          } else {
              sumPU += puFracPt_truth_puppi_all_[i];
              ++nPU;
          }
      }
      edm::LogVerbatim("JetTreeProducer")
          << "[Diag] Average PU fraction (truth): HS jets = "
          << (nHS > 0 ? sumHS/nHS : -1)
          << ", PU jets = "
          << (nPU > 0 ? sumPU/nPU : -1);
  }//if (!jetPt_.empty())

 std::cout << "Number of PVs: " << pv_handle->size() << std::endl;

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

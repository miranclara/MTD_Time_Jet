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
#include "DataFormats/JetReco/interface/GenJetCollection.h"
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
#include "fastjet/ClusterSequence.hh"

#include "TTree.h"
#include "TFile.h"
//#include "TFileService.h"

#include <memory>
#include <typeinfo>
//#define DEBUG_PUCONTENT
//#define DEBUG_MATCH_DETAIL
//#define DEBUG_PU_DIAG
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
    const std::vector<const pat::PackedCandidate*>& pf_for_allCollections)
{
    std::vector<unsigned int> out;
    std::vector<fastjet::PseudoJet> consts = jet.constituents();

    for (const auto &c : consts) {
//         if (c.user_index()<0) continue;
        int idx = c.user_index();
        if (idx < 0 || static_cast<size_t>(idx) >= pf_for_allCollections.size()) {
            edm::LogWarning("getPFIndicesFromPseudoJet")
            << "Invalid user_index=" << idx
            << " pf_for_allCollections.size()=" << pf_for_allCollections.size();
            continue;//skip this constituent instead of dereferencing
        }
        const pat::PackedCandidate* pf = pf_for_allCollections[idx];
        if(!pf_for_allCollections[idx]) continue;
        out.push_back(static_cast<unsigned int>(idx));
    }
    return out;
}//Old Version:End of getPFIndicesFromPseudoJet()

//---Helper Function1:  PF candidate  ↔   GenParticle matching---
// --- Updated PF candidate ↔ GenParticle matching ---
// Step 1: Require gen particle to be from hard scatter (status flags)
// Step 2: Among those, find best ΔR match
//const pat::PackedGenParticle*    matchToGen(const pat::PackedCandidate& pf,
std::pair<const pat::PackedGenParticle*, float> matchToGen(const pat::PackedCandidate& pf,
                                         const std::vector<pat::PackedGenParticle>& genParticles,
                                         float maxDR_charged = 0.1) {//0.05
  const pat::PackedGenParticle* bestMatch = nullptr;
//  float maxDR_charged = 0.4;  // or 0.1, depending on your analysis
//  float maxDR_neutral = 0.8;

//  float maxDR_eff = (pf.charge() == 0) ? maxDR_neutral : maxDR_charged;// loose neutral, tight charged   
  float bestDR = maxDR_charged;
  bool debug = false;//Debugging Toggle
//  bool finalbest=false;//Debugging Toggle
//  float dRHolder=-999;
//  int DPGIdHolder=-999;

    // --- Only consider charged PF candidates ---
//    if (pf.charge() == 0) {
//        if (debug)
//            std::cout << "[matchToGen] Skipping neutral PF (pt=" << pf.pt()
//                      << ", eta=" << pf.eta() << ")\n";
//        return std::make_pair(nullptr, -1.0f);
//    }

//    if (genParticles.empty()) {
//        std::cout << "[matchToGen] WARNING: genParticles vector is empty! skipping match for PF pt="
//                  << pf.pt() << std::endl;
//        return std::make_pair(nullptr, maxDR_charged);
//    }

//--- Loop over GEN particles ---
  for (const auto& gen : genParticles) {

   //------------------------------------------------------------------
        // Charged PF: require (isHSorPrompt && PDG match)
        //------------------------------------------------------------------
//        if (pf.charge() != 0) {

//            bool isHSorPrompt =//Prompt leptons,Prompt photons,Tau decay products.
//                gen.fromHardProcessFinalState() ||
//                gen.isDirectPromptTauDecayProductFinalState();
//                gen.isPromptFinalState()||
//                gen.isDirectHardProcessTauDecayProductFinalState();

//            if (!isHSorPrompt) {
//                if (debug)
//                    std::cout << "[DiagMatch] Charged PF skip: isHSorPrompt=" << isHSorPrompt
//                              << " (PF pdgId=" << pf.pdgId()
//                              << ", GEN pdgId=" << gen.pdgId() << ")\n"
//                              << ", pt=" << gen.pt() << ") skipped: not prompt/tau\n";
//                continue;
//            }
            // --- PDG ID consistency (charged only) ---
//            bool passPDG = (pf.pdgId() == gen.pdgId());
//            if (std::abs(pf.pdgId()) != std::abs(gen.pdgId())) {
//              if (debug)
//                  std::cout << "[matchToGen] PDG mismatch: PF=" << pf.pdgId()
//                            << ", GEN=" << gen.pdgId() << "\n";
//              continue;
//            }

//        }//End of if (pf.charge() != 0)

        //------------------------------------------------------------------
        // Neutral PF: require (isLastCopy && PDG in allowed set)
        //------------------------------------------------------------------
//       else {
//            if (!gen.statusFlags().isLastCopy()) {
//                if (debug)
//                    std::cout << "[DiagMatch] Neutral PF skip: GEN not last copy (pdgId="
//                              << gen.pdgId() << ")\n";
//                continue;
//            }

//            bool passPDG = (std::abs(gen.pdgId()) == 22 ||   // photon
//                            std::abs(gen.pdgId()) == 111 ||  // pi0
//                            std::abs(gen.pdgId()) == 130 ||  // K_L
//                            std::abs(gen.pdgId()) == 2112);  // neutron
//
//            if (!passPDG) {
//                if (debug)
//                    std::cout << "[DiagMatch] Neutral PF skip: GEN pdgId="
//                              << gen.pdgId() << " not in allowed set\n";
//                continue;
//            }
//      }//End of else{}


        //------------------------------------------------------------------
        // ΔR condition (same for both charged & neutral)
        //------------------------------------------------------------------
        float dR = reco::deltaR(pf.eta(), pf.phi(), gen.eta(), gen.phi());
        if (dR > maxDR_charged) {
            if (debug)
                std::cout << "[DiagMatch] ΔR failed: " << dR
                          << " > " << maxDR_charged << " (PF pdgId=" << pf.pdgId()
                          << ", GEN pdgId=" << gen.pdgId() << ")\n";
            continue;
        }
        //------------------------------------------------------------------
        // Best-match update
        //------------------------------------------------------------------
        if (dR < bestDR) {
            bestDR = dR;
            bestMatch = &gen;
//            finalbest = true;
        }

  }//End of for(const auto& gen : genParticles):End of loop over genParticles
    if (debug && bestMatch)
        std::cout << "[matchToGen] ✅ Best match found: GEN pdgId=" << bestMatch->pdgId()
                  << ", ΔR=" << bestDR << "\n";

    if (debug && !bestMatch)
        std::cout << "[matchToGen] ❌ No match found for PF pdgId=" << pf.pdgId()
                  << " (pt=" << pf.pt() << ")\n";

  return std::make_pair(bestMatch, bestDR);  // nullptr if no good match
}//End of  matchToGen

// --- Helper Function3: Jet ↔ GenJet Jetresponse calcultation---
inline float computeResponse(const pat::Jet& recoJet,
                              const reco::GenJet* bestMatchedGenJet) {
    if (!bestMatchedGenJet || bestMatchedGenJet->pt() <= 0) return -1.0f;
    return recoJet.pt() / bestMatchedGenJet->pt();
}

inline float computeResponse(const fastjet::PseudoJet& fjJet,
                              const reco::GenJet* bestMatchedGenJet) {
    if (!bestMatchedGenJet || bestMatchedGenJet->pt() <= 0) return -1.0f;
    return fjJet.pt() / bestMatchedGenJet->pt();
}


// ======================================================================
// Publication-level weighted vz for GenJet
//  - Uses packedGenParticles (status==1) within ΔR < 0.25
//  - Uses charged-only particles (charge!=0)
//  - Weights by pT (optionally by pT^2: uncomment)
//  - Removes PU/displaced by requiring |vz - median(vz)| < 0.3 cm
// ======================================================================
inline double weightedGenJetVz(
    const reco::GenJet &g,
    const std::vector<pat::PackedGenParticle> &pgVec,
    double dRmax = 0.25)
{
    std::vector<double> vzList;
    std::vector<double> wList;

    // 1) collect constituents around genJet axis
    for (const auto &pg : pgVec) {
        if (pg.status() != 1) continue;
        if (pg.charge() == 0) continue;            // charged-only
        if (pg.pt() < 0.3) continue;               // avoid noisy pt

        double dr = reco::deltaR(g.eta(), g.phi(), pg.eta(), pg.phi());
        if (dr > dRmax) continue;

        vzList.push_back(pg.vz());
        wList.push_back(pg.pt());                  // pT weight

        //In case prefer pT^2 weight:
        // wList.push_back(pg.pt() * pg.pt());
    }

    if (vzList.empty()) {
        // fallback to genJet vz
        double vz = g.vz();
        if (std::abs(vz) < 1000.) return vz;
        return 1e6;
    }

    // 2) compute median vz (robust center)
    std::vector<double> vzCopy = vzList;
    std::sort(vzCopy.begin(), vzCopy.end());
    double medianVz = vzCopy[vzCopy.size() / 2];

    // 3) remove outliers more than 0.3 cm from median
    std::vector<double> vzClean;
    std::vector<double> wClean;

    for (size_t i = 0; i < vzList.size(); ++i) {
        if (std::abs(vzList[i] - medianVz) < 0.30) {
            vzClean.push_back(vzList[i]);
            wClean.push_back(wList[i]);
        }
    }

    if (vzClean.empty()) return medianVz;    // fallback

    // 4) weighted average
    double sumW = 0.0, sumVz = 0.0;
    for (size_t i = 0; i < vzClean.size(); ++i) {
        sumW  += wClean[i];
        sumVz += wClean[i] * vzClean[i];
    }

    if (sumW == 0.0) return medianVz;
    return sumVz / sumW;
}//End of inline double weightedGenJetVz

  // Return best gen-jet match that passes both ΔR and pt cuts (or nullptr)
  inline const reco::GenJet* //returns a pointer to the best-matching gen jet (or nullptr if no match).
  bestGenMatch(const reco::Jet& j,
             const std::vector<reco::GenJet>& gens,
             const std::vector<pat::PackedGenParticle>& packedGen,
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
//    double vzGen = 0.0;
//    bool hasVz = (std::abs(g.vz()) > 1e-3);
    
//    if (!g.getJetConstituents().empty()){
//      const auto& firstPtr = g.getJetConstituents().at(0);
//      if (firstPtr.isNull()) {
//          edm::LogWarning("JetTreeProducer")
//              << "GenJet first constituent Ptr is null; cannot read vz. Using default.";
//      } else if (!firstPtr.isAvailable()) {
//          edm::LogWarning("JetTreeProducer")
//              << "GenJet first constituent Ptr product not available; cannot read vz. Using default.";
//      } else {
//          vzGen = firstPtr->vz();
//      }
//    }    
//    else
//        vzGen = g.vz();
//    if (hasVz && maxDz < 900.0 && std::abs(vzGen - genvertex_z) > maxDz)
//
//        continue;   
      // ---NEW TEST: more robust dz filter ---//        
      double vzGen = weightedGenJetVz(g, packedGen);
      if (std::abs(vzGen - genvertex_z) > maxDz) continue;
            
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
             const std::vector<pat::PackedGenParticle>& packedGen,
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
//      if (std::abs(g.vz() - genvertex_z) > maxDz) continue;
      // === publication-level weighted vz for genJet ===
      double vzGen = weightedGenJetVz(g, packedGen);
      if (std::abs(vzGen - genvertex_z) > maxDz) continue;//pileup GEN jets from displaced vertices are rejected before matching.

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
    bool   isAmbiguous=false;   // NEW:0.2< dR <0.4 region
    float  dR=-1.0;//ΔR distance to the matched gen-jet (or -1 if none).
    float  genPt=-1.0;//matched gen jet’s pT (or -1).
    float  genEta=-999.0;//matched gen jet’s eta (or -999).
    int    genIndex=-1;//index in the gens vector (or -1).
    const reco::GenJet* matchedGen=nullptr; 
  };

//This is a wrapper function that uses 'bestGenMatch' and fills a 'JetTruthMatch struct'.
  inline JetTruthMatch
  classifyJetHS(const reco::Jet& j,//the reco jet.
                const std::vector<reco::GenJet>& gens,//the gen jets.
                const std::vector<pat::PackedGenParticle>& packedGen,
                double dRMax,
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
    out.genEta = -999.0f;
    out.genIndex = -1;
    out.matchedGen = nullptr;
    return out;
  }//End of Test safe

    double dR = -1.0;
    int idx = -1;

    const reco::GenJet* g = bestGenMatch(j, gens,packedGen, dRMax, minGenPt, maxDz, genvertex_z_, &dR, &idx);
//    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
//    out.isHS     = (g != nullptr);//only true if match exists AND a generator-level PV compatibility cut inside bestGenMatch:| z(gen particle) − genvertex_z_ | < maxDz
    out.dR       = static_cast<float>(dR);//best ΔR or -1.
    out.genPt    = (g ? g->pt() : -1.0f);//if valid, else -1.
    out.genEta    = (g ? g->eta() : -999.0f);//if valid, else -1.
    out.genIndex = idx;// index in gens vector,Indicate the matched gen jet positon in the vector
    out.matchedGen      =  g;//pointer to actual GenJet

    // --- NEW 3-region classification ---
    const double dR_HS = 0.2;
    const double dR_PU = 0.4;

    out.isHS = false;
    out.isAmbiguous = false;

    if (g != nullptr) {
        if (dR <= dR_HS) {
            out.isHS = true;          // HS
        }  
        else if (dR >= dR_PU) {
            out.isHS = false;         // PU
        }
        else {
            out.isAmbiguous = true;   // exclude region
        }
    }//End of if (g != nullptr)

    return out;//Returns the JetTruthMatch struct.
  }//It just call .classifyJetHS' and get a neat struct with all info

  inline JetTruthMatch
  classifyJetHS(const fastjet::PseudoJet& j,//the PseudoJet.
                const std::vector<reco::GenJet>& gens,//the gen jets.
                const std::vector<pat::PackedGenParticle>& packedGen,
                double dRMax,//threshold
                double minGenPt,//threshold
                double maxDz,
                double genvertex_z_)
  {
    double dR = -1.0;
    int idx = -1;

    const reco::GenJet* g = bestGenMatch(j, gens,packedGen, dRMax, minGenPt, maxDz, genvertex_z_, &dR, &idx);
//    const reco::GenJet* g = bestGenMatch(j, gens, dRMax, minGenPt, &dR, &idx);//It calls bestGenMatch(), gets the pointer,ΔR,and index.
    JetTruthMatch out;//It builds a JetTruthMatch out strut
//    out.isHS     = (g != nullptr);//only true if match exists AND a generator-level PV compatibility cut inside bestGenMatch:| z(gen particle) − genvertex_z_ | < maxDz 
    out.dR       = static_cast<float>(dR);//best ΔR or -1.
    out.genPt    = (g ? g->pt() : -1.0f);//if valid, else -1.
    out.genEta    = (g ? g->eta() : -999.0f);//if valid, else -1.
    out.genIndex = idx;//from the loop.
    out.matchedGen      =  g;

    // --- NEW 3-region classification ---
    const double dR_HS = 0.2;
    const double dR_PU = 0.4;

    out.isHS = false;
    out.isAmbiguous = false;

    if (g != nullptr) {
        if (dR <= dR_HS) {
            out.isHS = true;          // HS
        }  
        else if (dR >= dR_PU) {
            out.isHS = false;         // PU
        }
        else {
            out.isAmbiguous = true;   // exclude region
        }
    }//End of if (g != nullptr)

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
                                             const std::vector<const pat::PackedCandidate*>& pf_for_allCollections,
                                             float pvTime,
                                             bool useTimingFallbackPuppi = true)
{
  PUContentReco acc;
  for (const auto& c : jet.constituents()) {
    if (c.user_index()<0) continue;
    int idx = c.user_index();
    if (idx >= static_cast<int>(pf_for_allCollections.size())) continue;
    
    const pat::PackedCandidate* pf = pf_for_allCollections[idx];
    updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
  }
  return acc;
}

//Function: compute PU fractions for one jet:Updated version (index-based, removed pointer recovery)
inline PUContentReco computePUContentRecoJet(
    const pat::Jet& jet,
    const std::vector<const pat::PackedCandidate*>& pf_for_allCollections,
    float pvTime,
    bool useTimingFallbackPuppi = true)
{
    PUContentReco acc;

    const auto pfIndices = getPFIndicesFromPatJet(jet);

    int recovered = 0;
    int failed = 0;

    for (unsigned int idx : pfIndices) {

        if (idx >= pf_for_allCollections.size()) {
            failed++;
            continue;
        }

        const pat::PackedCandidate* pf = pf_for_allCollections[idx];

        if (pf) {
            recovered++;
        } else {
            failed++;
        }

        updatePUContentReco(pf, acc, pvTime, useTimingFallbackPuppi);
    }

#ifdef DEBUG_PUCONTENT
    if (failed > 0) {
        std::cout << "[PUContentRecoJet] Jet pt=" << jet.pt()
                  << " eta=" << jet.eta()
                  << " recovered " << recovered << "/" << pfIndices.size()
                  << " PF constituents (" << failed << " failed)"
                  << std::endl;
    }
#endif

    return acc;
}//End of PUContentReco computePUContentRecoJet{} 


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

private:
    // --- helper function (DECLARATION)
    void fillHSGenJets(const reco::GenJetCollection& genJets);

    // --- branches
    int nHSGenJets_;
    std::vector<float> hsGenJet_pt_;
    std::vector<float> hsGenJet_eta_;

  
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

//--- PF->GEN mapping ---
edm::EDGetTokenT<edm::Association<std::vector<reco::GenParticle>>> pfToGenAssocToken_;


  TTree* tree_;

// --- For ΔR threshold tuning ---
std::vector<float> pf_dR_custom_;              // actual ΔR for each PF
std::vector<unsigned char> pf_match05_;        // 1 if ΔR<0.05 else 0
std::vector<unsigned char> pf_match10_;
std::vector<unsigned char> pf_match15_;
std::vector<unsigned char> pf_match20_;
std::vector<unsigned char> pf_match25_;
std::vector<unsigned char> pf_match30_;

// For jet-level matching diagnostics
std::vector<float> jetDeltaR_puppi_all_;
std::vector<float> jetDeltaR_pfraw_all_;
std::vector<float> jetDeltaR_fromPV3_all_;
std::vector<float> jetDeltaR_tight_all_;
std::vector<float> jetDeltaR_loose_all_;
std::vector<float> jetDeltaR_time_all_;
std::vector<float> jetDeltaR_time4D_all_;

/*
// Event-level PU jet counts
int nPUJets_puppi_, nRecoJets_puppi_;
int nPUJets_pfraw_, nRecoJets_pfraw_;
int nPUJets_tight_, nRecoJets_tight_;
int nPUJets_loose_, nRecoJets_loose_;
int nPUJets_time_, nRecoJets_time_;
int nPUJets_time4D_, nRecoJets_time4D_;
*/
  
  std::vector<int> jetMatched_puppi_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_puppi_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_puppi_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_puppi_all_;
  std::vector<float> jetPt_puppi_all_;
  std::vector<float> jetAbsEta_puppi_all_;
  std::vector<float> jetPhi_puppi_all_;
  std::vector<std::vector<unsigned int>> jet_pfIndices_puppi_all_;
  std::vector<float> jetResponse_PR_puppi_all_;
  std::vector<float> genPt_puppi_all_;
  std::vector<float> genEta_puppi_all_;
  std::vector<float> genEtaDen_puppi_all_;
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
  std::vector<int> jetMatched_pfraw_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_pfraw_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_pfraw_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_pfraw_all_;
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

/*
  std::vector<float> jetPt_pfraw_5leading_;
  std::vector<float> genPt_pfraw_5leading_;
  std::vector<float> jetResponse_PR_pfraw_5leading_;
  std::vector<float> jetAbsEta_pfraw_5leading_;
  std::vector<float> puFracPt_pfraw_5leading_;
  std::vector<float> puFracCount_pfraw_5leading_;
*/

  std::vector<int> jetMatched_fromPV3_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_fromPV3_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_fromPV3_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_fromPV3_all_;
  std::vector<float> jetPt_fromPV3_all_;
  std::vector<float> jetAbsEta_fromPV3_all_;
  std::vector<float> jetPhi_fromPV3_all_;
  std::vector<std::vector<unsigned int>> jet_pfIndices_fromPV3_all_;
  std::vector<float> genPt_fromPV3_all_;
  std::vector<float> genEta_fromPV3_all_;
  std::vector<float> genEtaDen_fromPV3_all_;
  std::vector<float> jetResponse_PR_fromPV3_all_;
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
  
  std::vector<int> jetMatched_tight_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_tight_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_tight_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_tight_all_;
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

/* 
  std::vector<float> jetPt_tight_5leading_;
  std::vector<float> genPt_tight_5leading_;
  std::vector<float> jetResponse_PR_tight_5leading_;
  std::vector<float> jetAbsEta_tight_5leading_;
  std::vector<float> puFracPt_tight_5leading_;
  std::vector<float> puFracCount_tight_5leading_;
*/

  std::vector<int> jetMatched_loose_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_loose_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_loose_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_loose_all_;
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
 /*
  std::vector<float> jetPt_loose_5leading_;
  std::vector<float> genPt_loose_5leading_;
  std::vector<float> jetResponse_PR_loose_5leading_;
  std::vector<float> jetAbsEta_loose_5leading_;
  std::vector<float> puFracPt_loose_5leading_;
  std::vector<float> puFracCount_loose_5leading_;
*/

  //For MTD-based jet clustering 
  std::vector<int> jetMatched_time_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_time_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_time_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_time_all_;
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

  //For 3D + MTD-based jet clustering=4D
  std::vector<int> jetMatched_time4D_all_;     // RECO → GEN match flag
  std::vector<int> genJetMatched_time4D_all_;  // GEN → RECO match flag
  std::vector<int> genMatchedDen_time4D_all_;  // GEN → RECO match flag
  std::vector<int> jetIsHS_time4D_all_;
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

// For jet-level timing (3D+MTD jets=4D)
  std::vector<float> jetTime_time4D_all_;
  std::vector<float> jetTimeError_time4D_all_;
  std::vector<float> jetTimeSig_time4D_all_;   // (tjet - PVt)/σ_tjet
  
  std::vector<float> pf_vertex,pf_pt, pf_eta, pf_phi, pf_energy;
  std::vector<float> pf_charge,pf_puppiWeight,pf_puppiWeightNoLep;//For <pat::PackedCandidate> collection
  std::vector<float> pf_pvQuality;//associatedPVIndex, For <pat::PackedCandidate> collection
  std::vector<int> pf_pvIndex;//associatedPVIndex, For <pat::PackedCandidate> collection
  std::vector<float> pf_dxy, pf_dz, pf_dzError, pf_dzSig;
  std::vector<float> pf_time, pf_timeError, pf_timeSig, pf_dt, pf_dtErr, pf_dtSig;//For <pat::PackedCandidate> collection
  std::vector<float> pf_vx, pf_vy, pf_vz;

  std::vector<int> pf_genIndex_assoc_;//Index from offical pfToGenAssociation
  std::vector<float> pf_genPt_assoc_;
  std::vector<float> pf_genEta_assoc_;
  std::vector<float> pf_genPhi_assoc_;
  std::vector<int>   pf_genPdgId_assoc_;
  std::vector<float> pf_genDR_assoc_;

  std::vector<int> pf_genIndex_custom_;//Index from matchToGe
  std::vector<float> pf_genPt_custom_;
  std::vector<float> pf_genEta_custom_;
  std::vector<float> pf_genPhi_custom_;
  std::vector<int>   pf_genPdgId_custom_;
  std::vector<float> pf_genDR_custom_;

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

  std::vector<unsigned int> pf_indices_withTime_;//One vector per event,contains PF indices (iPF),only PFs with valid timing

  std::vector<std::vector<unsigned int>> pf_indices_general_all_;//redundent,For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_time_all_;//For <pat::PackedCandidate> collection
  std::vector<std::vector<unsigned int>> pf_indices_time4D_all_;//For <pat::PackedCandidate> collection
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
  float pvs_x_, pvs_y_, pvs_z_,pvs_t_,pvs_TimeErr_,pvs_tSig_; // For "FIRST" primary vertex position

  float beamspot_x_, beamspot_y_, beamspot_z_; // For beam spot position
  float genvertex_z_;
  std::vector<float> puFrac_pfraw_,puFrac_fromPV3_, puFrac_tight_, puFrac_loose_,puFrac_time_,puFrac_time4D_;
//  float pf_vx, pf_vy, pf_vz;//For <pat::PackedCandidate> collection
//  int pf_pdgId,pf_isTimeValid;//For <pat::PackedCandidate> collection

// add to class members (inside class definition)

  // For jet-level efficiency & purity per collection
  float efficiency_puppi_, effLossAmbig_puppi_, effWithAmbig_puppi_, mistag_puppi_, purity_puppi_;
  float efficiency_pfraw_, effLossAmbig_pfraw_, effWithAmbig_pfraw_, mistag_pfraw_, purity_pfraw_;//defined but not used
  float efficiency_fromPV3_, effLossAmbig_fromPV3_, effWithAmbig_fromPV3_, mistag_fromPV3_, purity_fromPV3_;//defined but not used
  float efficiency_tight_, effLossAmbig_tight_, effWithAmbig_tight_, mistag_tight_, purity_tight_;
  float efficiency_loose_, effLossAmbig_loose_, effWithAmbig_loose_, mistag_loose_, purity_loose_;
  float efficiency_time_, effLossAmbig_time_, effWithAmbig_time_, mistag_time_, purity_time_;//defined but not used
  float efficiency_time4D_, effLossAmbig_time4D_, effWithAmbig_time4D_, mistag_time4D_,purity_time4D_;//defined but not used
      
 
  int totalRecoJetsClean = 0;
  int totalPUJets = 0;
  float puJetFraction_all_ = 0;//Not saved

  int totalRecoJetsClean_puppi = 0; 
  int totalPUJets_puppi = 0; 
  float  puJetFraction_puppi_all_ = 0;


  int totalRecoJetsClean_pfraw = 0; 
  int totalPUJets_pfraw = 0; 
  float puJetFraction_pfraw_all_ =0;

  int totalRecoJetsClean_fromPV3 = 0; 
  int totalPUJets_fromPV3 = 0; 
  float puJetFraction_fromPV3_all_ =0;
  
  int totalRecoJetsClean_tight = 0; 
  int totalPUJets_tight = 0; 
  float puJetFraction_tight_all_ = 0;

  int totalRecoJetsClean_loose = 0 ;
  int totalPUJets_loose = 0 ; 
  float puJetFraction_loose_all_ =0; 

  int totalRecoJetsClean_time = 0; 
  int totalPUJets_time = 0; 
  float puJetFraction_time_all_ =0;

  int totalRecoJetsClean_time4D = 0; 
  int totalPUJets_time4D = 0; 
  float puJetFraction_time4D_all_ =0;

};//class JetTreeProducer : public edm::one::EDAnalyzer<edm::one::SharedResources> {


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
pfToGenAssocToken_ = consumes<edm::Association<std::vector<reco::GenParticle>>>(edm::InputTag("packedPFCandidateToGenAssociation"));


}//End of JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig)

void JetTreeProducer::beginJob() {
  usesResource("TFileService");
  edm::Service<TFileService> fs_;
  tree_ = fs_->make<TTree>("JetTree", "JetTree");



tree_->Branch("nHSGenJets", &nHSGenJets_, "nHSGenJets/I");
tree_->Branch("hsGenJet_pt",  &hsGenJet_pt_);
tree_->Branch("hsGenJet_eta", &hsGenJet_eta_);

// --- For ΔR threshold tuning ---
tree_->Branch("pf_dR_custom",   &pf_dR_custom_);
tree_->Branch("pf_match05",     &pf_match05_);
tree_->Branch("pf_match10",     &pf_match10_);
tree_->Branch("pf_match15",     &pf_match15_);
tree_->Branch("pf_match20",     &pf_match20_);
tree_->Branch("pf_match25",     &pf_match25_);
tree_->Branch("pf_match30",     &pf_match30_);
 
 
// ΔR branches
  tree_->Branch("jetDeltaR_puppi_all", &jetDeltaR_puppi_all_);
  tree_->Branch("jetDeltaR_pfraw_all", &jetDeltaR_pfraw_all_);
  tree_->Branch("jetDeltaR_fromPV3_all", &jetDeltaR_fromPV3_all_);
  tree_->Branch("jetDeltaR_tight_all", &jetDeltaR_tight_all_);
  tree_->Branch("jetDeltaR_loose_all", &jetDeltaR_loose_all_);
  tree_->Branch("jetDeltaR_time_all",   &jetDeltaR_time_all_);
  tree_->Branch("jetDeltaR_time4D_all",   &jetDeltaR_time4D_all_);

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

  tree_->Branch("jetMatched_puppi_all", &jetMatched_puppi_all_);
  tree_->Branch("genJetMatched_puppi_all", &genJetMatched_puppi_all_);
  tree_->Branch("jetIsHS_puppi_all", &jetIsHS_puppi_all_);
  tree_->Branch("jetPt_puppi_all", &jetPt_puppi_all_);
  tree_->Branch("jetPhi_puppi_all", &jetPhi_puppi_all_);
  tree_->Branch("jet_pfIndices_puppi_all", &jet_pfIndices_puppi_all_);
  tree_->Branch("genPt_puppi_all", &genPt_puppi_all_);
  tree_->Branch("genEta_puppi_all", &genEta_puppi_all_);
  tree_->Branch("genEtaDen_puppi_all", &genEtaDen_puppi_all_);
  tree_->Branch("genMatchedDen_puppi_all", &genMatchedDen_puppi_all_);
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
  
  tree_->Branch("jetMatched_pfraw_all", &jetMatched_pfraw_all_);
  tree_->Branch("genJetMatched_pfraw_all", &genJetMatched_pfraw_all_);
  tree_->Branch("jetIsHS_pfraw_all", &jetIsHS_pfraw_all_);
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

 /*
  tree_->Branch("jetResponse_PR_pfraw_5leading", &jetResponse_PR_pfraw_5leading_);
  tree_->Branch("jetPt_pfraw_5leading", &jetPt_pfraw_5leading_);
  tree_->Branch("genPt_pfraw_5leading", &genPt_pfraw_5leading_);
  tree_->Branch("jetAbsEta_pfraw_5leading", &jetAbsEta_pfraw_5leading_); 
  tree_->Branch("puFracPt_pfraw_5leading", &puFracPt_pfraw_5leading_);
  tree_->Branch("puFracCount_pfraw_5leading", &puFracCount_pfraw_5leading_);  
*/

  tree_->Branch("jetMatched_fromPV3_all", &jetMatched_fromPV3_all_);
  tree_->Branch("genJetMatched_fromPV3_all", &genJetMatched_fromPV3_all_);
  tree_->Branch("jetIsHS_fromPV3_all", &jetIsHS_fromPV3_all_);
  tree_->Branch("jetResponse_PR_fromPV3_all", &jetResponse_PR_fromPV3_all_);
  tree_->Branch("jetPt_fromPV3_all", &jetPt_fromPV3_all_);
  tree_->Branch("jetPhi_fromPV3_all", &jetPhi_fromPV3_all_);
  tree_->Branch("jet_pfIndices_fromPV3_all", &jet_pfIndices_fromPV3_all_);
  tree_->Branch("genPt_fromPV3_all", &genPt_fromPV3_all_);
  tree_->Branch("genEta_fromPV3_all", &genEta_fromPV3_all_);
  tree_->Branch("genEtaDen_fromPV3_all", &genEtaDen_fromPV3_all_);
  tree_->Branch("genMatchedDen_fromPV3_all", &genMatchedDen_fromPV3_all_);
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

  tree_->Branch("jetMatched_tight_all", &jetMatched_tight_all_);
  tree_->Branch("genJetMatched_tight_all", &genJetMatched_tight_all_);
  tree_->Branch("jetIsHS_tight_all", &jetIsHS_tight_all_);
  tree_->Branch("jetPt_tight_all", &jetPt_tight_all_);
  tree_->Branch("jetPhi_tight_all", &jetPhi_tight_all_);
  tree_->Branch("jet_pfIndices_tight_all", &jet_pfIndices_tight_all_);
  tree_->Branch("genPt_tight_all", &genPt_tight_all_);
  tree_->Branch("genEta_tight_all", &genEta_tight_all_);
  tree_->Branch("genEtaDen_tight_all", &genEtaDen_tight_all_);
  tree_->Branch("genMatchedDen_tight_all", &genMatchedDen_tight_all_);
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

  tree_->Branch("jetMatched_loose_all", &jetMatched_loose_all_);
  tree_->Branch("genJetMatched_loose_all", &genJetMatched_loose_all_);
  tree_->Branch("jetIsHS_loose_all", &jetIsHS_loose_all_);
  tree_->Branch("jetResponse_PR_loose_all", &jetResponse_PR_loose_all_);
  tree_->Branch("jetAbsEta_loose_all", &jetAbsEta_loose_all_);
  tree_->Branch("jetPt_loose_all", &jetPt_loose_all_);
  tree_->Branch("jetPhi_loose_all", &jetPhi_loose_all_);
  tree_->Branch("jet_pfIndices_loose_all", &jet_pfIndices_loose_all_);
  tree_->Branch("genPt_loose_all", &genPt_loose_all_);
  tree_->Branch("genEta_loose_all", &genEta_loose_all_);
  tree_->Branch("genEtaDen_loose_all", &genEtaDen_loose_all_);
  tree_->Branch("genMatchedDen_loose_all", &genMatchedDen_loose_all_);
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

  tree_->Branch("jetMatched_time_all", &jetMatched_time_all_);
  tree_->Branch("genJetMatched_time_all", &genJetMatched_time_all_);
  tree_->Branch("jetIsHS_time_all", &jetIsHS_time_all_);
  tree_->Branch("jetResponse_PR_time_all", &jetResponse_PR_time_all_);
  tree_->Branch("jetAbsEta_time_all", &jetAbsEta_time_all_);
  tree_->Branch("jetPt_time_all", &jetPt_time_all_);
  tree_->Branch("jetPhi_time_all", &jetPhi_time_all_);
  tree_->Branch("jet_pfIndices_time_all", &jet_pfIndices_time_all_);
  tree_->Branch("genPt_time_all", &genPt_time_all_);
  tree_->Branch("genEta_time_all", &genEta_time_all_);
  tree_->Branch("genEtaDen_time_all", &genEtaDen_time_all_);
  tree_->Branch("genMatchedDen_time_all", &genMatchedDen_time_all_);
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

  tree_->Branch("jetMatched_time4D_all", &jetMatched_time4D_all_);
  tree_->Branch("genJetMatched_time4D_all", &genJetMatched_time4D_all_);
  tree_->Branch("jetIsHS_time4D_all", &jetIsHS_time4D_all_);
  tree_->Branch("jetResponse_PR_time4D_all", &jetResponse_PR_time4D_all_);
  tree_->Branch("jetAbsEta_time4D_all", &jetAbsEta_time4D_all_);
  tree_->Branch("jetPt_time4D_all", &jetPt_time4D_all_);
  tree_->Branch("jetPhi_time4D_all", &jetPhi_time4D_all_);
  tree_->Branch("jet_pfIndices_time4D_all", &jet_pfIndices_time4D_all_);
  tree_->Branch("genPt_time4D_all", &genPt_time4D_all_);
  tree_->Branch("genEta_time4D_all", &genEta_time4D_all_);
  tree_->Branch("genEtaDen_time4D_all", &genEtaDen_time4D_all_);
  tree_->Branch("genMatchedDen_time4D_all", &genMatchedDen_time4D_all_);
  tree_->Branch("puFracPt_algo_time4D_all", &puFracPt_algo_time4D_all_);
  tree_->Branch("puFracCount_algo_time4D_all", &puFracCount_algo_time4D_all_);  
  tree_->Branch("puFracPt_truth_time4D_all", &puFracPt_truth_time4D_all_);
  tree_->Branch("puFracCount_truth_time4D_all", &puFracCount_truth_time4D_all_);  
  
  // Jet timing
  tree_->Branch("jetTime_time4D_all", &jetTime_time4D_all_);
  tree_->Branch("jetTimeError_time4D_all", &jetTimeError_time4D_all_);
  tree_->Branch("jetTimeSig_time4D_all", &jetTimeSig_time4D_all_);


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
  tree_->Branch("pvs_tSig", &pvs_tSig_, "pvs_tSig/F");
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
  tree_->Branch("pf_timeSig", &pf_timeSig);
  tree_->Branch("pf_dt", &pf_dt);
  tree_->Branch("pf_dtErr", &pf_dtErr);
  tree_->Branch("pf_dtSig", &pf_dtSig);


  tree_->Branch("pf_genIndex_assoc", &pf_genIndex_assoc_);
  tree_->Branch("pf_genPt_assoc", &pf_genPt_assoc_);
  tree_->Branch("pf_genEta_assoc", &pf_genEta_assoc_);
  tree_->Branch("pf_genPhi_assoc", &pf_genPhi_assoc_);
  tree_->Branch("pf_genPdgId_assoc", &pf_genPdgId_assoc_);
  tree_->Branch("pf_genDR_assoc", &pf_genDR_assoc_);

  tree_->Branch("pf_genIndex_custom", &pf_genIndex_custom_);
  tree_->Branch("pf_genPt_custom", &pf_genPt_custom_);
  tree_->Branch("pf_genEta_custom", &pf_genEta_custom_);
  tree_->Branch("pf_genPhi_custom", &pf_genPhi_custom_);
  tree_->Branch("pf_genPdgId_custom", &pf_genPdgId_custom_);
  tree_->Branch("pf_genDR_custom", &pf_genDR_custom_);

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
 
//  float efficiency_puppi_, mistag_puppi_, purity_puppi_;
//  float efficiency_pfraw_, mistag_pfraw_, purity_pfraw_;//defined but not used
//  float efficiency_fromPV3_, mistag_fromPV3_, purity_fromPV3_;//defined but not used
//  float efficiency_tight_, mistag_tight_, purity_tight_;
//  float efficiency_loose_, mistag_loose_, purity_loose_;
//  float efficiency_time_, mistag_time_, purity_time_;//defined but not used
//  float efficiency_time4D_, mistag_time4D_,purity_time4D_;//defined but not used

  tree_->Branch("totalRecoJetsClean_pfraw", &totalRecoJetsClean_pfraw,"totalRecoJetsClean_pfraw/I");
  tree_->Branch("totalRecoJetsClean_fromPV3", &totalRecoJetsClean_fromPV3,"totalRecoJetsClean_fromPV3/I");
  tree_->Branch("totalRecoJetsClean_tight", &totalRecoJetsClean_tight,"totalRecoJetsClean_tight/I");
  tree_->Branch("totalRecoJetsClean_loose", &totalRecoJetsClean_loose,"totalRecoJetsClean_loose/I");
  tree_->Branch("totalRecoJetsClean_time", &totalRecoJetsClean_time,"totalRecoJetsClean_time/I");
  
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
  tree_->Branch("puJetFraction_time4D_all", &puJetFraction_time4D_all_,"puJetFraction_time4D_all/F");

  tree_->Branch("efficiency_puppi", &efficiency_puppi_,"efficiency_puppi/F");
  tree_->Branch("effLossAmbig_puppi", &effLossAmbig_puppi_,"effLossAmbig_puppi/F");
  tree_->Branch("effWithAmbig_puppi", &effWithAmbig_puppi_,"effWithAmbig_puppi/F");
  tree_->Branch("mistag_puppi", &mistag_puppi_,"mistag_puppi/F");
  tree_->Branch("purity_puppi", &purity_puppi_,"purity_puppi/F");

  tree_->Branch("efficiency_pfraw", &efficiency_pfraw_,"efficiency_pfraw/F");
  tree_->Branch("effLossAmbig_pfraw", &effLossAmbig_pfraw_,"effLossAmbig_pfraw/F");
  tree_->Branch("effWithAmbig_pfraw", &effWithAmbig_pfraw_,"effWithAmbig_pfraw/F");
  tree_->Branch("mistag_pfraw", &mistag_pfraw_,"mistag_pfraw/F");
  tree_->Branch("purity_pfraw", &purity_pfraw_,"purity_pfraw/F");

  tree_->Branch("efficiency_fromPV3", &efficiency_fromPV3_,"efficiency_fromPV3/F");
  tree_->Branch("effLossAmbig_fromPV3", &effLossAmbig_fromPV3_,"effLossAmbig_fromPV3/F");
  tree_->Branch("effWithAmbig_fromPV3", &effWithAmbig_fromPV3_,"effWithAmbig_fromPV3/F");
  tree_->Branch("mistag_fromPV3", &mistag_fromPV3_,"mistag_fromPV3/F");
  tree_->Branch("purity_fromPV3", &purity_fromPV3_,"purity_fromPV3/F");

  tree_->Branch("efficiency_tight", &efficiency_tight_,"efficiency_tight/F");
  tree_->Branch("effLossAmbig_tight", &effLossAmbig_tight_,"effLossAmbig_tight/F");
  tree_->Branch("effWithAmbig_tight", &effWithAmbig_tight_,"effWithAmbig_tight/F");
  tree_->Branch("mistag_tight", &mistag_tight_,"mistag_tight/F");
  tree_->Branch("purity_tight", &purity_tight_,"purity_tight/F");

  tree_->Branch("efficiency_loose", &efficiency_loose_,"efficiency_loose/F");
  tree_->Branch("effLossAmbig_loose", &effLossAmbig_loose_,"effLossAmbig_loose/F");
  tree_->Branch("effWithAmbig_loose", &effWithAmbig_loose_,"effWithAmbig_loose/F");
  tree_->Branch("mistag_loose", &mistag_loose_,"mistag_loose/F");
  tree_->Branch("purity_loose", &purity_loose_,"purity_loose/F");

  tree_->Branch("efficiency_time", &efficiency_time_,"efficiency_time/F");
  tree_->Branch("effLossAmbig_time", &effLossAmbig_time_,"effLossAmbig_time/F");
  tree_->Branch("effWithAmbig_time", &effWithAmbig_time_,"effWithAmbig_time/F");
  tree_->Branch("mistag_time", &mistag_time_,"mistag_time/F");
  tree_->Branch("purity_time", &purity_time_,"purity_time/F");

  tree_->Branch("efficiency_time4D", &efficiency_time4D_,"efficiency_time4D/F");
  tree_->Branch("effLossAmbig_time4D", &effLossAmbig_time4D_,"effLossAmbig_time4D/F");
  tree_->Branch("effWithAmbig_time4D", &effWithAmbig_time4D_,"effWithAmbig_time4D/F");
  tree_->Branch("mistag_time4D", &mistag_time4D_,"mistag_time4D/F");
  tree_->Branch("purity_time4D", &purity_time4D_,"purity_time4D/F");

  tree_->Branch("pf_indices_withTime", &pf_indices_withTime_);
  
  tree_->Branch("pf_indices_puppi_all", &pf_indices_puppi_all_);
  tree_->Branch("pf_indices_pfraw_all", &pf_indices_pfraw_all_);
  tree_->Branch("pf_indices_fromPV3_all", &pf_indices_fromPV3_all_);
  tree_->Branch("pf_indices_tight_all", &pf_indices_tight_all_);
  tree_->Branch("pf_indices_loose_all", &pf_indices_loose_all_);
  tree_->Branch("pf_indices_time_all", &pf_indices_time_all_);
  tree_->Branch("pf_indices_time4D_all", &pf_indices_time4D_all_);
  
  tree_->Branch("puFrac_pfraw", &puFrac_fromPV3_);
  tree_->Branch("puFrac_pfraw", &puFrac_pfraw_);
  tree_->Branch("puFrac_tight", &puFrac_tight_);
  tree_->Branch("puFrac_loose", &puFrac_loose_);
  tree_->Branch("puFrac_time", &puFrac_time_);
  tree_->Branch("puFrac_time4D", &puFrac_time4D_);

  tree_->Branch("pf_isHS_algo", &pf_isHS_algo);//test
  tree_->Branch("pf_isPU_algo",  &pf_isPU_algo);//test
  tree_->Branch("pf_isHS_truth", &pf_isHS_truth);//test
  tree_->Branch("pf_isPU_truth", &pf_isPU_truth);//test
  tree_->Branch("pf_dR_truth", &pf_dR_truth_);//test
//  tree_->Branch("best_dR_in_event", &best_dR_in_event, "best_dR_in_event/F");//test

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
//Test of gen vetex

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
  pf_timeSig.clear();
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
  //best_dR_in_event.clear();//test

  pf_genIndex_assoc_.clear();
  pf_genPt_assoc_.clear();//PF->GEN association Info.
  pf_genEta_assoc_.clear();//PF->GEN association Info.
  pf_genPhi_assoc_.clear();//PF->GEN association Info.
  pf_genPdgId_assoc_.clear();//PF->GEN association Info.
  pf_genDR_assoc_.clear();//PF->GEN association Info.

  pf_genIndex_custom_.clear();
  pf_genPt_custom_.clear();
  pf_genEta_custom_.clear();
  pf_genPhi_custom_.clear();
  pf_genPdgId_custom_.clear();
  pf_genDR_custom_.clear();

  pf_isHS.clear();
  pf_fromPV.clear();
  pf_isCharged.clear();
  pf_hasValidTime.clear();

  //jetTime_time_all_.clear();
 // jetTimeError_time_all_.clear();
 // jetTimeSig_time_all_.clear();
 
  pf_indices_withTime_.clear();

  pf_indices_general_all_.clear();//redunent
  pf_indices_pfraw_all_.clear();
  pf_indices_fromPV3_all_.clear();
  pf_indices_puppi_all_.clear();
  pf_indices_tight_all_.clear();
  pf_indices_loose_all_.clear();
  pf_indices_time_all_.clear();
  pf_indices_time4D_all_.clear();

  pf_passesTightCut.clear();
  pf_passesLooseCut.clear();
  pf_passes3D.clear();
  pf_passes4D.clear();
  pf_keepAlways.clear();
  pf_keepDisplaced.clear();
  pf_hasTimeCompatibleWithPV.clear();

  pf_dR_custom_.clear();              // actual ΔR for each PF
  pf_match05_.clear();        // 1 if ΔR<0.05 else 0
  pf_match10_.clear();
  pf_match15_.clear();
  pf_match20_.clear();
  pf_match25_.clear();
  pf_match30_.clear();
 
  jetDeltaR_puppi_all_.clear();
  jetDeltaR_pfraw_all_.clear();
  jetDeltaR_fromPV3_all_.clear();
  jetDeltaR_tight_all_.clear();
  jetDeltaR_loose_all_.clear();
  jetDeltaR_time_all_.clear();
  jetDeltaR_time4D_all_.clear();

  jetMatched_puppi_all_.clear();
  genJetMatched_puppi_all_.clear();
  jetIsHS_puppi_all_.clear();
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

 /*
  jetPt_puppi_5leading_.clear();
  genPt_puppi_5leading_.clear();
  jetResponse_PR_puppi_5leading_.clear();
  jetAbsEta_puppi_5leading_.clear(); 
  puFracPt_puppi_5leading_.clear(); 
  puFracCount_puppi_5leading_.clear();
*/

  jetMatched_pfraw_all_.clear();
  genJetMatched_pfraw_all_.clear();
  jetIsHS_pfraw_all_.clear();
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


  /*
  jetPt_pfraw_5leading_.clear();
  genPt_pfraw_5leading_.clear();
  jetResponse_PR_pfraw_5leading_.clear();
  jetAbsEta_pfraw_5leading_.clear();
  puFracPt_pfraw_5leading_.clear(); 
  puFracCount_pfraw_5leading_.clear();
*/


  jetMatched_fromPV3_all_.clear();
  genJetMatched_fromPV3_all_.clear();
  jetIsHS_fromPV3_all_.clear();
  jetPt_fromPV3_all_.clear();
  jetPhi_fromPV3_all_.clear();
  jet_pfIndices_fromPV3_all_.clear();
  genPt_fromPV3_all_.clear();
  genEta_fromPV3_all_.clear();
  genEtaDen_fromPV3_all_.clear();
  genMatchedDen_fromPV3_all_.clear();
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

  jetMatched_tight_all_.clear();
  genJetMatched_tight_all_.clear();
  jetIsHS_tight_all_.clear();
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

/*
  jetPt_tight_5leading_.clear();
  genPt_tight_5leading_.clear();
  jetResponse_PR_tight_5leading_.clear();
  jetAbsEta_tight_5leading_.clear();
  puFracPt_tight_5leading_.clear(); 
  puFracCount_tight_5leading_.clear();
*/
  
  jetMatched_loose_all_.clear();
  genJetMatched_loose_all_.clear();
  jetIsHS_loose_all_.clear();
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

  /*
  jetPt_loose_5leading_.clear();
  genPt_loose_5leading_.clear();
  jetResponse_PR_loose_5leading_.clear();
  jetAbsEta_loose_5leading_.clear();
  puFracPt_loose_5leading_.clear(); 
  puFracCount_loose_5leading_.clear();
*/

  jetMatched_time_all_.clear();
  genJetMatched_time_all_.clear();
  jetIsHS_time_all_.clear();
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


  jetMatched_time4D_all_.clear();
  genJetMatched_time4D_all_.clear();
  jetIsHS_time4D_all_.clear();
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
  // True genjet counting
  if (!genJetsHandle.isValid()) {
      edm::LogError("JetTreeProducer") << "Missing input: genJetsToken";
      nHSGenJets_ = -1;//or 0   
      return; //If necessary
  }
  fillHSGenJets(*genJetsHandle);
 

 //PF->GEN association
  edm::Handle<edm::Association<std::vector<reco::GenParticle>>> pfToGenAssoc;
  iEvent.getByToken(pfToGenAssocToken_, pfToGenAssoc);
    if (!pfToGenAssoc.isValid()) {
//      edm::LogError("JetTreeProducer") << "Missing input: pfToGenAssocToken";
      edm::LogWarning("JetTreeProducer") << "packedPFCandidateToGenAssociation not found!";
    return;
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
  //!Truth-matched primary vertex requirement (MC-only)
  //!For physics analyses or data/MC comparisons, it should not be used
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


  if (!genpHandle.isValid()) {
      edm::LogError("JetTreeProducer") << "Missing packedGenParticles";
      return;
  }

 // Convert Handle → Reference (NECESSARY for weighted vz)
  const std::vector<pat::PackedGenParticle>& packedGenParticles = *genpHandle;


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
  std::vector<fastjet::PseudoJet> fjInputs_time4D;
  std::vector<const pat::PackedCandidate*> pf_for_time;
  std::vector<const pat::PackedCandidate*> pf_for_allCollections;
  pf_for_allCollections.reserve(pf_coll.size());
  pf_for_time.reserve(pf_coll.size()); 
  
  fjInputs_raw.clear();
  fjInputs_fromPV3.clear();
  fjInputs_tight.clear();
  fjInputs_loose.clear();
  fjInputs_time.clear();
  fjInputs_time4D.clear();
  pf_for_time.clear();

  PUContentReco puContentAlgo;
  PFTruthContentReco puContentTruth;

  //Define the global PF vector
  std::vector<const pat::PackedCandidate*> pf_all;
  pf_all.reserve(pf_coll.size());
  for (const auto& pf : pf_coll) {
    pf_all.push_back(&pf);
  }
// Diagnostic counters
  int nPF_total = 0, nPF_matched = 0, nPF_unmatched = 0;
  int nPF_charged = 0, nPF_charged_matched = 0;
  int nPF_neutral = 0, nPF_neutral_matched = 0;
  float best_dR_in_event = 999.f; 
 
  pv_x_.clear();
  pv_y_.clear();
  pv_z_.clear();
  pv_t_.clear();
  pv_chi2_.clear();
  pv_ndof_.clear();
  pv_nTracks_.clear(); 



  // Fill beam spot information
  if (beamspot.isValid()) {
    beamspot_x_ = beamspot->x0();
    beamspot_y_ = beamspot->y0();
    beamspot_z_ = beamspot->z0();
  }

  // Fill first primary vertex variables
  if (!reco_pvs.empty()) {
     const reco::Vertex& pv = reco_pvs[0];

     pvs_x_ = pv.x();
     pvs_y_ = pv.y();
     pvs_z_ = pv.z();
     pvs_t_ = pv.t();
     pvs_TimeErr_=pv.tError();
   }
  
  const reco::Vertex& pv = reco_pvs[0];//OK,3D point (math::XYZPoint),3D point,not a single z value.
  pvs_tSig_=pvs_t_/pvs_TimeErr_;

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

  // Fill generator particle z-positions
  if (genpVec.empty()) {
    for (const auto& particle : genpVec) {
      genparticles_z_.push_back(particle.vz());
    }
  }

//  float pvTime = 0.0f;                  // or your stored primary vertex time
  float pvTime = pvs_t_;
  bool useTimingFallbackPuppi = true;   // set according to your config

  //=========================  PF Particle Loop  ============================
  int pf_coll_index = 0;
  for (size_t iPF = 0; iPF < pf_coll.size(); ++iPF) {
  nPF_total++;

    if (iPF < 10) {  // limit output
    const auto& pf = pf_coll[iPF];
    std::cout << "[PF LOOP] iPF=" << iPF
              << " pt=" << pf.pt()
              << " eta=" << pf.eta()
              << " phi=" << pf.phi()
              << " time=" << pf.time()
              << std::endl;
    }//End of if (iPF < 10)
  

//  for (const auto& pf : pf_coll) {
//  for (size_t i = 0; i < pf_coll.size(); ++i)
    const auto& pf = pf_coll[iPF];

    float eta = pf.eta(); 
    bool isInMTD = (std::abs(eta) <= 3.0);
    pf_isInMTD.push_back(isInMTD ? 1 : 0);      
    const bool isCharged = (pf.charge() != 0);
    const bool isNeutral = (pf.charge() == 0);
    const int fromPV = pf.fromPV();
    const bool keepAlways = (fromPV == 3);
    pf_isCharged.push_back(isCharged ? 1 : 0);
   if (isCharged) nPF_charged++;
   else nPF_neutral++;

    // inside PF candidate loop, before any use:
    float dtSig = 999.0f;               // sentinel (invalid)
    const float dtSigCut = 5.0f;        // tune this threshold (was missing)
 
    bool isDisplaced = false;//To keep displaced tracks
    bool hasValidTime = false;
    bool hasTimingInfo = false;
    bool pvHasValidTime = false;
    bool hasValidDt = false;

    //Diagnostic print of associatedPVIndex
    int pvIndex = pf.vertexRef().key();
    auto pvQuality = pf.pvAssociationQuality();

    pf_for_allCollections.push_back(&pf);//Index matching
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

   //(1). Official CMS association(pfToGenAssoc):Official mapping of CMS
    // --- get associated GEN particle ---
    int official_gen_index = -1;
//    reco::GenParticleRef genRef = (*pfToGenAssoc)[pf];
      edm::Ref<std::vector<pat::PackedCandidate>> pfRef(pfColl_handle, iPF);
      reco::GenParticleRef genRef = (*pfToGenAssoc)[pfRef];

      if (genRef.isNonnull() && genRef.isAvailable()) {
//             const reco::GenParticle& gen = *genRef;
             const auto& gen = *genRef;
             float dR = reco::deltaR(pf.eta(), pf.phi(), gen.eta(), gen.phi());

          // optional: store to vectors
          pf_genPt_assoc_.push_back(gen.pt());
          pf_genEta_assoc_.push_back(gen.eta());
          pf_genPhi_assoc_.push_back(gen.phi());
          pf_genPdgId_assoc_.push_back(gen.pdgId());
          pf_genDR_assoc_.push_back(dR);
          official_gen_index = genRef.key();//The index in the official genParticles collection used inside CMS PF → GEN association
      } else {
  
          // store default “no match” values
          pf_genPt_assoc_.push_back(-1.f);
          pf_genEta_assoc_.push_back(-99.f);
          pf_genPhi_assoc_.push_back(-99.f);
          pf_genPdgId_assoc_.push_back(0);
          pf_genDR_assoc_.push_back(-1.f);
          official_gen_index = -1;
      }//End of if (genRef.isNonnull() && genRef.isAvailable())
      pf_genIndex_assoc_.push_back(official_gen_index);
    // ---END: get associated GEN particle ---
 
 
      //New Test         
      // Safe truth-match: only attempt if gen collection is present (genpVec non-empty)
      int custom_gen_index = -1;
      const pat::PackedGenParticle* matchedGen = nullptr;
      float dR_value = -1.f;

     // (2) Compute my oown custom match(matchToGen) for comparison
      if (!genpVec.empty()) {
      bool isHS_truth = false;
      bool isPU_truth = false;
//      auto matchResult = matchToGen(pf, genpVec,(pf.charge() == 0 ? 1.0 : 0.4));//Neutral:Chaged
      auto matchResult = matchToGen(pf, genpVec, 0.3);//0.3=maxD
      matchedGen = matchResult.first;
      dR_value   = matchResult.second;
      pf_dR_custom_.push_back(matchedGen ? dR_value : -1.f); 

//      isHS_truth = (matchedGen != nullptr);
      if (matchedGen) {
//          int pvFlag = matchedGen->fromPV();//!fromPV is not available in miniAOD packedGenParticle

          if(pf.fromPV()>1){ isHS_truth=true;}
          pf_genPt_custom_.push_back(matchedGen->pt());
          pf_genEta_custom_.push_back(matchedGen->eta());
          pf_genPhi_custom_.push_back(matchedGen->phi());
          pf_genPdgId_custom_.push_back(matchedGen->pdgId());
          pf_genDR_custom_.push_back(dR_value);
          // ---- compute index inside genpVec ----
          custom_gen_index = matchedGen - &genpVec[0];  // genVec Index saving

    } else {
          isPU_truth=true; 
          pf_genPt_custom_.push_back(-1.f);
          pf_genEta_custom_.push_back(-99.f);
          pf_genPhi_custom_.push_back(-99.f);
          pf_genPdgId_custom_.push_back(0);
          pf_genDR_custom_.push_back(-1.f);
          custom_gen_index = -1;

    }//End of if (matchedGen){}else{}
    pf_isHS_truth.push_back(isHS_truth);
    pf_isPU_truth.push_back(isPU_truth);

    // --- Fill boolean flags for various ΔR thresholds ---
    pf_match05_.push_back( (dR_value>=0.f && dR_value<0.05) );
    pf_match10_.push_back( (dR_value>=0.f && dR_value<0.10) );
    pf_match15_.push_back( (dR_value>=0.f && dR_value<0.15) );
    pf_match20_.push_back( (dR_value>=0.f && dR_value<0.20) );
    pf_match25_.push_back( (dR_value>=0.f && dR_value<0.25) );
    pf_match30_.push_back( (dR_value>=0.f && dR_value<0.30) );


      if (isHS_truth) {
      nPF_matched++;
      if (isCharged) nPF_charged_matched++;
      }
    
    //-----mindR of PF-Gen  ------ 
    if (matchedGen && dR_value >= 0.f) {
    if (dR_value < best_dR_in_event)
      best_dR_in_event = dR_value;
    }

   } else {
      // no gen collection
      edm::LogWarning("JetTreeProducer") << "No packedGenParticles found in this event.";
      dR_value = -99.f;
       // if genpVec empty, no match possible
        pf_genPt_custom_.push_back(-99.f);//-1.f
        pf_genEta_custom_.push_back(-99.f);
        pf_genPhi_custom_.push_back(-99.f);
        pf_genPdgId_custom_.push_back(0);
        pf_genDR_custom_.push_back(-99.f);//-1ff?
        custom_gen_index = -1;
        pf_isPU_truth.push_back(1);
        pf_isHS_truth.push_back(0);
        nPF_unmatched++;
      }//if (!genpVec.empty())
    
    // push the gen index
    pf_genIndex_custom_.push_back(custom_gen_index);
    
    //-----mindR of PF-Gen  ------ 
    if (matchedGen && dR_value >= 0.f) {
    if (dR_value < best_dR_in_event)
      best_dR_in_event = dR_value;
    }  

    //==========PF HS/PU check End====================i

    if (!isInMTD) continue;

    // Common PseudoJet for dz-based clustering
    fastjet::PseudoJet pj(pf.px(), pf.py(), pf.pz(), pf.energy());
    pj.set_user_index(static_cast<int>(iPF)); //KEY of global intexing,Index matching  
    fjInputs_raw.push_back(pj);//keep global-indexed PF for raw jets, Index matching
   //=>This PseudoJet corresponds to PF candidate at index i in pf_for_allCollections
    if (iPF < 10) {
        std::cout << "  -> PseudoJet user_index = "
                  << pj.user_index() << std::endl;
    }//End of if (iPF < 10)
    
    if (pf.hasTrackDetails() && isCharged) {//For charged particle
       ++N_charged;

      pf_keepAlways.push_back(keepAlways ? 1 : 0);
      if (keepAlways) {// const bool keepAlways = (fromPV == 3);
        fjInputs_fromPV3.push_back(pj);
        ++N_selected_fromPV3;        
      }//End of if (keepAlways)
      
      float dz     = pf.dz(pv.position());//z(track) − z(primary vertex):dz of track with respect to the track reference point:beam spot orbeam spot,It is NOT guaranteed to be the reconstructed primary vertex.
      float dzErr = std::sqrt(
          pf.dzError()*pf.dzError() +//track uncertainty
          pv.zError()*pv.zError()//vertex uncertainty
      );//
      float dzSig  = (dzErr > 0) ? dz / dzErr : 999;
      isDisplaced = (std::abs(dz) > 0.05);//To keep displaced tracks
      pf_dxy.push_back(pf.dxy());
      pf_dz.push_back(dz);
      pf_dzError.push_back(dzErr);
      pf_dzSig.push_back(dzSig);
      

      //PF track to Primary Vetex Association check
      //fromPV: 0=PU, 1=loosely PV, 2:from PV, 3:highly quality PV
      //if (pf.pvAssociationQuality() < pat::PackedCandidate::UsedInFitTight)continue;

      if(pf.fromPV()<=1)continue;//Fitering PF candidates using vertex association.

      //dz:mean -3.1 X 10(-6), sigma:0.010cm
      //dzSig: mean -2.1X 10 (-6), sigma: 0.073 
      float dzCutBase=0.14;//0.05/0.14     
      float dzCutLoose=0.5;//0.05/0.01     
      float dzCutTight=0.1;//0.03/0.002     

      float dzErrCutBase=0.0225;//0.05/0.0225     
      float dzErrCutLoose=0.017;//0.05/0.017
      float dzErrCutTight=0.014;//0.03/0.014     

      float dzSigCutBase=3;//0.2/3    
      float dzSigCutLoose=2.15;//0.2/2.15    
      float dzSigCutTight=1.65;//0.5/1.65
      bool passesBaseCut = (std::abs(dz) < dzErrCutBase && dzSig < dzSigCutBase);//3D loos selection
      bool passesLooseCut = (std::abs(dz) < dzCutLoose && dzSig < dzSigCutLoose);//3D loos selection
      bool passesTightCut = (std::abs(dz) < dzCutTight && dzSig < dzSigCutTight);//3D tight selection


//    if (std::abs(dz) < 0.05 && dzSig < 0.5) {//Old Loose Condition
    if (std::abs(dz) < 1) {//dZ Condition
 
 
      if (std::abs(dz)<=dzCutLoose) {//only dzcut 
        fjInputs_loose.push_back(pj);
        ++N_selected_loose;
      //  if (isHS) ++N_selected_HS_loose;
      }//End of if (passesLooseCut)

      if (std::abs(dz)<=dzCutTight) {
        fjInputs_tight.push_back(pj);
        ++N_selected_tight;
       // if (isHS) ++N_selected_HS_tight;
      }//End of if (passesTightCut)

    }//End of if (std::abs(dz) < 1) {}//dZ conditon dicupled with dt condition
    
      // compute only if timing is valid:
      if (pf.timeError() <= 0 || !std::isfinite(pf.time())) continue;

      float t = pf.time();//nanoseconds. pf.time=t_track_i -pvs_t
      float tErr = pf.timeError();//nanoseconds
      float tSig=pf.time()/pf.timeError();

      // === Robust validity check ===
      // Treat as "valid" only if error is positive and not too large
      hasValidTime = (
          isInMTD &&        //Ensures only trust time info(in MTD acceptance)
          tErr > 0 &&
          tErr < 0.1f    //old:tErr < 0.2f, Conservative threshold (30â50 ps typical)
//          std::abs(t) <1  //old:abs(t) <1, sanity check: time should be < 10 ns, change from 100 to 10=>Selection result not changed, 10= 10,000 ps.
      );

      pvHasValidTime =  (
    	std::isfinite(pvs_TimeErr_) &&
    	pvs_TimeErr_ < 1e6
	);   // choose a sensible threshold of pvs_t=real value. 

      pf_hasValidTime.push_back(hasValidTime ? 1 : 0);//
     
        float dt = 999;
        float dtErr = 999;
        float dtSig = 999;//
       
      if (hasValidTime && pvHasValidTime) {
      //Safe to use time:MTD time resolution ≈ 30–40 picoseconds = 0.03–0.04 nanoseconds.
      //!!!pf.time=t_track -pvs_t: It is the time residual relative to the primary vetex.
        dt = t - pv.t();//convert ns → ps=dt * 1000.0;=>dt=t_track-2(pvs_t);!No physics meaning
        dtErr =std::sqrt( tErr*tErr + pvs_TimeErr_*pvs_TimeErr_ );
        dtSig = dt/dtErr;

//        dtSig = std::abs(dt) / std::sqrt(tErr*tErr + pvs_TimeErr_*pvs_TimeErr_);
//           float dzCutLoose=2;//0.05
        float dtCutBase=1.62;//0.03
        float dtCutLoose=0.4;//0.03
        float dtCutTight=0.2;//0.03

        float dtErrCutBase=0.071;//0.03
        float dtErrCutLoose=0.044;//0.03
        float dtErrCutTight=0.039;//0.03

        float dtSigCutBase=3;//0.2/2
        float dtSigCutLoose=1.5;//0.2/1.5
        float dtSigCutTight=1;//0.5/1

        // Save commone PF-level "timed" PF
        pf_time.push_back(pf.time());
        pf_timeError.push_back(pf.timeError());
        pf_timeSig.push_back(tSig);
        pf_dt.push_back(dt);
        pf_dtErr.push_back(dtErr);
        pf_dtSig.push_back(dtSig);
        
        // Save PF index (ONCE)
        pf_indices_withTime_.push_back(iPF);//global PF indices.

      bool keepDisplaced = (isDisplaced && hasValidTime);//4D selection
      bool hasTimeCompatibleWithPV= (std::abs(dz)<=dzCutLoose)&&hasValidTime && pvHasValidTime &&(dt<dtCutLoose)&&(dtSig<dtSigCutBase);//track +time
      bool passes4D = (std::abs(dz)<=dzCutTight)&&hasValidTime && pvHasValidTime&&(dt<dtCutTight)&&(dtSig<dtSigCutBase);//tract_resolution+time_resolution      

      pf_keepDisplaced.push_back(keepDisplaced ? 1 : 0);
      pf_hasTimeCompatibleWithPV.push_back(hasTimeCompatibleWithPV ? 1 : 0);

      if (hasTimeCompatibleWithPV) {//track +time
        // Reuse the same PseudoJet (already has global user_index)
        fastjet::PseudoJet pj_time = pj; // copy keeps iPF index
        fjInputs_time.push_back(pj_time);

      }//End of if (hasTimeCompatibleWithPV)//time+loosecut

      if (passes4D) { //tract_resolution+time_resolution
        fastjet::PseudoJet pj_time4D = pj; // copy keeps iPF index
        fjInputs_time4D.push_back(pj_time4D);
        pf_for_time.push_back(&pf);
      }//End of if (passes4D),time+tightcut
  
      } else {
         dt =    999;
         dtErr = 999;
         dtSig = 999;      
        // Invalid or missing time
        pf_time.push_back(999);
        pf_timeError.push_back(999);
        pf_timeSig.push_back(999);
        pf_dt.push_back(999);
        pf_dtErr.push_back(999);
        pf_dtSig.push_back(999);

        }//end of if (hasValidTime&& pvHasValidTime) else {}     
//      }//End of if (std::abs(dz) < 1) {}//When dZ+dt condition
    } else {
      // Fill with defaults to avoid division by zero, preserve structure
      pf_dxy.push_back(999);
      pf_dz.push_back(999);
      pf_dzError.push_back(999);  // avoid division by zero
      pf_dzSig.push_back(999);
      pf_time.push_back(999);
      pf_timeError.push_back(999);
      pf_timeSig.push_back(999);
      pf_dt.push_back(999);
      pf_dtErr.push_back(999);
      pf_dtSig.push_back(999);
      pf_isInMTD.push_back(0);

       
      //Neutral candidate -no dz/dzSig
      //If you consider only chared particle,It does not filled with pj,comment out *.push_back(pj)
      //CMSSW_15_1_0_pre4/Optionally:include all neutrals in tight/loose
      ++N_neutral;
        //!!!Place to investigate!!
      fjInputs_fromPV3.push_back(pj);
      fjInputs_tight.push_back(pj);
      fjInputs_loose.push_back(pj);
      fjInputs_time.push_back(pj);
      fjInputs_time4D.push_back(pj);
//
    }//End of if (pf.hasTrackDetails() && isCharged) else{}
    //outside the MTD geometry &seem to have time info:Indicate a reconstruction or simulation artifact.
    ++pf_coll_index;

  }//End of for (const auto& pf : pf_coll), End of PF Loop:

if (best_dR_in_event == 999.f)
  best_dR_in_event = -1.f;

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
  auto pfrawJets = fastjet::sorted_by_pt(cs_raw.inclusive_jets(10.0));//return only jets with pt ≥ 10 GeV.
  
  auto cs_fromPV3= fastjet::ClusterSequence(fjInputs_fromPV3, jetDef);
  auto fromPV3Jets = fastjet::sorted_by_pt(cs_fromPV3.inclusive_jets(10.0));
  
  auto cs_tight = fastjet::ClusterSequence(fjInputs_tight, jetDef);
  auto tightJets = fastjet::sorted_by_pt(cs_tight.inclusive_jets(10.0));

  auto cs_loose = fastjet::ClusterSequence(fjInputs_loose, jetDef);
  auto looseJets = fastjet::sorted_by_pt(cs_loose.inclusive_jets(10.0));

  auto cs_time = fastjet::ClusterSequence(fjInputs_time, jetDef);
  auto timeJets = fastjet::sorted_by_pt(cs_time.inclusive_jets(10.0));

  auto cs_time4D = fastjet::ClusterSequence(fjInputs_time4D, jetDef);
  auto timeJets4D = fastjet::sorted_by_pt(cs_time4D.inclusive_jets(10.0));
 
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

  jetTime_time4D_all_.clear();
  jetTimeError_time4D_all_.clear();
  jetTimeSig_time4D_all_.clear();

  edm::LogPrint("TimeDebug") 
      << "fjInputs_time size = " << fjInputs_time.size();

//============= UPDATED JET-TIME ALGORITHM =============//
//for (const auto& jet : timeJets) {
for (const auto& jet : timeJets4D) {

    float wSum = 0.0f;         // Σ (1/σ_t^2)
    float wtSum = 0.0f;        // Σ (t/σ_t^2)
    float t_jet = -999.f;
    float tErr_jet = -999.f;
    float tSig_jet = -999.f;

    for (const auto& c : jet.constituents()) {//c is a copy of original PseudoJet, with its original user_index() preserved.

        int idx = c.user_index();
        if (idx < 0 || (size_t)idx >= pf_for_time.size()) continue;

        const auto* pf = pf_for_time[idx];
        if (!pf) continue;

        const float t    = pf->time();
        const float tErr = pf->timeError();

        // Valid MTD time
        if (tErr <= 0.f || tErr >= 0.2f) continue;

        // Weight = 1 / σ²
        const float w = 1.f / (tErr * tErr);

        wSum  += w;
        wtSum += w * t;
    }

    if (wSum > 0.f) {
        t_jet    = wtSum / wSum;
        tErr_jet = std::sqrt(1.f / wSum);
        tSig_jet = (tErr_jet > 0) ? std::abs(t_jet - pvs_t_) / tErr_jet : -999.f;
    }

    jetTime_time4D_all_.push_back(t_jet);
    jetTimeError_time4D_all_.push_back(tErr_jet);
    jetTimeSig_time4D_all_.push_back(tSig_jet);
}//for (const auto& jet : timeJets) 
//=========================== END UPDATED BLOCK ==============================//


//============ End of PF loop - diagnostic summary========================

float frac_matched_all = (nPF_total > 0) ? float(nPF_matched)/nPF_total : 0.f;
float frac_matched_ch  = (nPF_charged > 0) ? float(nPF_charged_matched)/nPF_charged : 0.f;
float frac_matched_neu = (nPF_neutral > 0) ? float(nPF_neutral_matched)/nPF_neutral : 0.f;


std::cout << std::fixed << std::setprecision(2);
std::cout << "[DiagPF] Event " << iEvent.id().event()
          << " | total=" << nPF_total
          << " matched=" << nPF_matched
          << " (" << 100*frac_matched_all << "%)"
          << " | charged=" << nPF_charged << " (" << 100*frac_matched_ch << "% matched)"
          << " | neutral=" << nPF_neutral << " (" << 100*frac_matched_neu << "% matched)"
          << std::endl;


//===== Generic std::vector<pat::Jet> Loop Template=====
auto processJetCollection = [&](const std::vector<pat::Jet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,
                                std::vector<int>& jetMatched_all_,     // RECO → GEN match flag
                                std::vector<int>& genJetMatched_all_,  // GEN → RECO match flag
                                std::vector<int>& genMatchedDen_all_,  // GEN → RECO match flag
                                std::vector<int>& jetIsHS_all_,
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
/*
                                std::vector<float>& jetPt_5leading_,
                                std::vector<float>& jetAbsEta_5leading_,
                                std::vector<float>& jetResponse_5leading_,
                                std::vector<float>& genPt_5leading_,
                                std::vector<float>& puFracPt_5leading_,
                                std::vector<float>& puFracCount_5leading_,
*/
				//per-collection summary variables
                                int& totalRecoJetsClean,
                                int& totalPUJets,
                                float& puJetFraction_all_,
				float& efficiency_out,
				float& effLossAmbig,
				float& effWithAmbig,
				float& mistag,
				float& purity_out) {
    int idx = 0;
    totalRecoJetsClean = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;
//    int nMatchedR = 0;
   
    std::vector<int> genJetAmbigMatched(genJets.size(), 0);
   // Compute totalGenJets with the same minGenPt(pt > 20)
    int totalGenJets = 0; // apply pt>10 cut here
    for (size_t i=0; i<genJets.size(); ++i) {
      const auto& g = genJets[i];
      if (g.pt() <= 20.0) continue;
      totalGenJets++;

    // denominator for efficiency profile
    genEtaDen_all_.push_back(std::abs(g.eta()));

    // placeholder, updated later after jet loop
    genMatchedDen_all_.push_back(0);
    }

    std::vector<int> genJetMatched(genJets.size(), 0);
    for (const auto& jet : jetsIn) {//jet = one RECO jet, jetsIn = all reconstructed jets in that collection

        // Get PF indices for this jet
        std::vector<unsigned int> pf_indices_this_jet =  getPFIndicesFromPatJet(jet);
         pf_indices_general_all_.push_back(pf_indices_this_jet);

        // Jet ↔ GenJet matching/ classificaiton
        auto  truthMatch = classifyJetHS(jet, genJets,packedGenParticles,
                                1.0,        // dRMax, 0.3
                                20.0,       // minGenPt 10
                                0.2,        // maxDz (loose PV cut)0.3
                                genvertex_z_);  // generator PV

        // --- NEW: store ΔR to matched gen jet
        jetDeltaR_all_.push_back(truthMatch.dR);  // use the correct vector per collection


         const reco::GenJet* bestMatchedGenJet = truthMatch.matchedGen;//gen jet matched to current reco jet    
        // ----------------------------
        // GEN → RECO matching (efficiency)
        // ----------------------------
        if (bestMatchedGenJet != nullptr) {//genJets = vector of all GEN jets,genJets[i] = ith GEN jet
            for (size_t i = 0; i < genJets.size(); ++i) {
                if (&genJets[i] == bestMatchedGenJet) {
                    if (!truthMatch.isAmbiguous)  // enforce your definition
                        genJetMatched[i] = 1;//clean matched,whether gen jet i was matched by any reco jet
                    else
                        genJetAmbigMatched[i] = 1;//ambiguous matched 
                    break;
                }
            }
         }

        // ----------------------------
        // Skip ambiguous for RECO
        // ----------------------------
        if (truthMatch.isAmbiguous) continue;
        totalRecoJetsClean++;

        // ----------------------------
        // RECO → GEN matching (mis-tag)
        // ----------------------------
        bool isMatched = false;
        if (bestMatchedGenJet != nullptr) {
          isMatched = true;
            nMatchedReco++;
        }
        // Store RECO-side matching flag (needed for mis-tag)
        jetMatched_all_.push_back(isMatched ? 1 : 0);     

         float minDR = truthMatch.dR; // example: distance saved in the struct
         bool isHSjet = truthMatch.isHS;
         bool isPUjet = !isHSjet;//jetIsHS_all_
     
        
        // HS/PU counting (strict)
        if (!truthMatch.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, bestMatchedGenJet);

	//=====================================================================
	//  Compute per-jet PU fractions (algorithmic and truth-based)
	//=====================================================================
	PUContentReco algoAcc;
	PFTruthContentReco truthAcc;

//	auto pfIndices = getPFIndicesFromPatJet(jet,pf_for_allCollections);
//	auto pfIndices = getPFIndicesFromPatJet(jet);//Redundent
	const auto& pfIndices = pf_indices_this_jet;
	for (unsigned int idx : pfIndices) {
  	if (idx >= pf_for_allCollections.size()) continue;
  	const pat::PackedCandidate* pf = pf_for_allCollections[idx];

  	// algorithmic classification
//  	float pvTime = pvs_t_.empty() ? 0.f : pvs_t_[0];
        float pvTime = pvs_t_;  	
  	updatePUContentReco(pf, algoAcc, pvTime, useTimingFallbackPuppi);

  	// truth classification (reuse cached pf_isPU_truth)
  	updatePFTruthContentReco(pf, truthAcc, idx, pf_isPU_truth);
	}//End of for (unsigned int idx : pfIndices)

	// Calculate fractions safely
	float puFracCount_algo  = (algoAcc.nTot     > 0) ? float(algoAcc.nPU) / algoAcc.nTot     : -1.f;
	float puFracPt_algo     = (algoAcc.sumPtTot > 0) ? algoAcc.sumPtPU   / algoAcc.sumPtTot  : -1.f;
	float puFracCount_truth = (truthAcc.nTot    > 0) ? float(truthAcc.nPU)/ truthAcc.nTot    : -1.f;
	float puFracPt_truth    = (truthAcc.sumPtTot> 0) ? truthAcc.sumPtPU  / truthAcc.sumPtTot : -1.f;


        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        jetPhi_all_.push_back(jet.phi()); 
        jet_pfIndices_all_.push_back(pfIndices);//=pf_indices_general_all_.push_back(pf_indices_this_jet);
        genPt_all_.push_back(bestMatchedGenJet ? bestMatchedGenJet->pt() : -1.0);
        genEta_all_.push_back(bestMatchedGenJet ? std::abs(bestMatchedGenJet->eta()) : -1.0);
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
            genPt_5leading_.push_back(bestMatchedGenJet ? bestMatchedGenJet->pt() : -1.0);
            jetResponse_5leading_.push_back(response);
            puFracCount_5leading_.push_back(pu.fracCount);
            puFracPt_5leading_.push_back(pu.fracPt);
        }//if (idx < 5)
*/
        idx++;
    }//End of for (const auto& jet : jetsIn)

    //=====Theo's method:This is GEN → RECO matching, required for efficiency=====
    int nGenMatched = 0;
    int nGenMatchedAmbig = 0;
    int igen_pass = 0;

    for (size_t i = 0; i < genJets.size(); ++i) {
        if (genJets[i].pt() <= 20.0) continue;
        // profile numerator partner
        genMatchedDen_all_[igen_pass] = genJetMatched[i];
        genJetMatched_all_.push_back(genJetMatched[i]);
        if (genJetMatched[i] == 1)
            nGenMatched++;
        else if (genJetAmbigMatched[i] == 1)
            nGenMatchedAmbig++;
        igen_pass++;
    }//End of for (size_t i = 0; i < genJets.size(); ++i)

    //PU jet fraction
    puJetFraction_all_ = (totalRecoJetsClean > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJetsClean
        : -1.0f;
   
    //Jet efficiency/purity: Count HS vs PU jets explicitly 
    //efficiency = fraction of gen jets reconstructed.
    //purity = fraction of  jetIsHS_all_.push_back(truthMatch.isHS ? 1 : 0);reco jets that are HS.    

//    int nGenMatched = std::count(genJetMatched.begin(), genJetMatched.end(), 1);
    const int nRecoJetsHS = totalRecoJetsClean - totalPUJets;
    efficiency_out = (totalGenJets > 0) ? float(nGenMatched) / totalGenJets : -1.0f;
    effLossAmbig =(totalGenJets > 0) ? float(nGenMatchedAmbig)/totalGenJets : -1.0f;
    effWithAmbig = (totalGenJets > 0) ? float(nGenMatched + nGenMatchedAmbig)/totalGenJets : -1.0f;
    mistag = (totalRecoJetsClean > 0) ? float(totalRecoJetsClean - nMatchedReco)/totalRecoJetsClean : -1.0f;    
    purity_out =(totalRecoJetsClean > 0) ? float(nMatchedReco)/totalRecoJetsClean : -1.0f;    

//    efficiency_out = (totalGenJets > 0)     ? float(nRecoJetsHS)/totalGenJets : -1.0f;
//    purity_out     = (totalRecoJetsClean > 0)   ? float(nRecoJetsHS)/totalRecoJetsClean : -1.0f;

std::cout
<< "totalGenJets      = " << totalGenJets << "\n"
<< "clean matched     = " << nGenMatched << "\n"
<< "ambiguous matched = " << nGenMatchedAmbig << "\n"
<< "strict eff        = " << efficiency_out << "\n"
<< "ambig loss        = " << effLossAmbig << "\n"
<< "eff incl ambig    = " << effWithAmbig << "\n";

};//end of auto processJetCollection = [&](){



//===== Generic std::vector<fastjet::PseudoJet> Loop Template=====
auto processFastJetCollection = [&](const std::vector<fastjet::PseudoJet>& jetsIn,
                                const std::vector<reco::GenJet>& genJets,
                                std::vector<int>& jetMatched_all_,     // RECO → GEN match flag
                                std::vector<int>& genJetMatched_all_,  // GEN → RECO match flag
                                std::vector<int>& genMatchedDen_all_,  // GEN → RECO match flag

                                std::vector<int>& jetIsHS_all_,
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
/*
                                std::vector<float>& jetPt_5leading_,
                                std::vector<float>& jetAbsEta_5leading_,
                                std::vector<float>& jetResponse_5leading_,
                                std::vector<float>& genPt_5leading_,
                                std::vector<float>& puFracPt_5leading_,
                                std::vector<float>& puFracCount_5leading_,
*/

                                int& totalRecoJetsClean,
                                int& totalPUJets,
                                float& puJetFraction_all_,
				float& efficiency_out,
				float& effLossAmbig,
				float& effWithAmbig,
				float& mistag,
				float& purity_out) {
    int idx = 0;
    totalRecoJetsClean = 0;
    totalPUJets   = 0;
    int nMatchedReco = 0;
    std::vector<int> genJetAmbigMatched(genJets.size(), 0); 

   // Compute totalGenJets with the same minGenPt(pt > 20)
    int totalGenJets = 0; // apply pt>10 cut here
    for (size_t i=0; i<genJets.size(); ++i) {
      const auto& g = genJets[i];
      if (g.pt() <= 20.0) continue;
      totalGenJets++;

    // denominator for efficiency profile
    genEtaDen_all_.push_back(std::abs(g.eta()));

    // placeholder, updated later after jet loop
    genMatchedDen_all_.push_back(0);
    }
//    int jetCounter = 0;
    std::vector<int> genJetMatched(genJets.size(), 0);
    for (const auto& jet : jetsIn) {
/*  
      //----------DEBUGGING------------
        std::cout << "\n[JET] #" << jetCounter
                  << " pt=" << jet.pt()
                  << " eta=" << jet.eta()
                  << std::endl;

        auto consts = jet.constituents();//c is a copy of original PseudoJet, with its original user_index() preserved.

        int nPrint = 0;

        for (const auto& c : consts) {

        int idx = c.user_index();

        std::cout << "  constituent user_index=" << idx;

        // Safety check
        if (idx < 0 || idx >= (int)pf_for_allCollections.size()) {
            std::cout << "  [INVALID INDEX!]" << std::endl;
            continue;
        }

        const auto& pf = pf_for_allCollections[idx];

        std::cout << " -> PF(pt=" << pf->pt()
                  << ", eta=" << pf->eta()
                  << ", phi=" << pf->phi()
                  << ", time=" << pf->time()
                  << ")"
                  << std::endl;

        if (++nPrint > 5) break; // limit per jet
    }//end of for (const auto& c : consts)
    jetCounter++;
    if (jetCounter > 3) break; // limit jets
//--------------EDN of DEBUGGING-------------
*/

        // Get PF indices for this jet
        auto pf_indices_this_jet = getPFIndicesFromPseudoJet(jet, pf_for_allCollections);
        pf_indices_general_all_.push_back(pf_indices_this_jet);//global PF indices.

        auto truthMatch = classifyJetHS(jet, genJets,packedGenParticles,
                                1.0,        // dRMax
                                20.0,       // minGenPt
                                0.2,        // maxDz (loose PV cut)
                                genvertex_z_);  // generator PV
    
        // --- NEW: store ΔR to matched gen jet
        jetDeltaR_all_.push_back(truthMatch.dR);  // use the correct vector per collection

         const reco::GenJet* bestMatchedGenJet = truthMatch.matchedGen;      
        // ----------------------------
        // GEN → RECO matching (efficiency)
        // ----------------------------
        if (bestMatchedGenJet != nullptr) {
            for (size_t i = 0; i < genJets.size(); ++i) {
                if (&genJets[i] == bestMatchedGenJet) {
                    if (!truthMatch.isAmbiguous)  // enforce your definition
                        genJetMatched[i] = 1;
                    else
                        genJetAmbigMatched[i] = 1;
                    break;
                }
            }
         }

        // ----------------------------
        // Skip ambiguous for RECO
        // ----------------------------
        if (truthMatch.isAmbiguous) continue;
        totalRecoJetsClean++;

        // ----------------------------
        // RECO → GEN matching (mis-tag)
        // ----------------------------
        bool isMatched = false;
        if (bestMatchedGenJet != nullptr) {
          isMatched = true;
            nMatchedReco++;
        }

        // Store RECO-side matching flag (needed for mis-tag)
        jetMatched_all_.push_back(isMatched ? 1 : 0);
     
         float minDR = truthMatch.dR; // example: distance saved in the struct
         bool isHSjet = truthMatch.isHS;
         bool isPUjet = !isHSjet;

     
        // HS/PU counting (strict)
        if (!truthMatch.isHS) totalPUJets++;

        // Response
        float response = computeResponse(jet, bestMatchedGenJet);

	//=====================================================================
	//  Compute per-fastjet PU fractions (algorithmic and truth-based)
	//=====================================================================
	PUContentReco algoAcc;
	PFTruthContentReco truthAcc;

//	auto pfIndices = getPFIndicesFromPseudoJet(jet, pf_for_allCollections);//Redundent

        //----- loop only for computing jet-level quantities---
	const auto& pfIndices = pf_indices_this_jet;
        for (unsigned int idx : pfIndices) {//loop over PFs to compute jet-level quantities
  	if (idx >= pf_for_allCollections.size()) continue;
 	 const pat::PackedCandidate* pf = pf_for_allCollections[idx];
  
/*
      // -------- DEBUG (exactly where mapping happens) --------
      if (idx >= pf_for_allCollections.size()) {
       edm::LogWarning("PFIndexDebug")
            << "[BAD INDEX] idx=" << idx
            << " pf_for_allCollections.size()=" << pf_for_allCollections.size();
      }//
      // --------END: DEBUG (exactly where mapping happens) --------
*/

         // algorithmic classification
//        float pvTime = pvs_t_.empty() ? 0.f : pvs_t_[0];
        float pvTime = pvs_t_;
  	updatePUContentReco(pf, algoAcc, pvs_t_, useTimingFallbackPuppi);
        
        // truth update using cached per-PF truth flags
  	updatePFTruthContentReco(pf, truthAcc, idx, pf_isPU_truth);
	}//End of for (unsigned int idx : pfIndices)

	float puFracCount_algo  = (algoAcc.nTot     > 0) ? float(algoAcc.nPU) / algoAcc.nTot     : -1.f;
	float puFracPt_algo     = (algoAcc.sumPtTot > 0) ? algoAcc.sumPtPU   / algoAcc.sumPtTot  : -1.f;
	float puFracCount_truth = (truthAcc.nTot    > 0) ? float(truthAcc.nPU)/ truthAcc.nTot    : -1.f;
	float puFracPt_truth    = (truthAcc.sumPtTot> 0) ? truthAcc.sumPtPU  / truthAcc.sumPtTot : -1.f;

        // Save ALL jets
        jetPt_all_.push_back(jet.pt());
        jetAbsEta_all_.push_back(std::abs(jet.eta()));
        jetPhi_all_.push_back(jet.phi()); 
        jet_pfIndices_all_.push_back(pfIndices);//=pf_indices_general_all_.push_back(pf_indices_this_jet);//global PF indices.

/*
        //-----DEBUGGING-------------
        std::cout << "[STORE] jet has " << pfIndices.size()
                  << " PF indices: ";

        for (size_t i = 0; i < std::min<size_t>(pfIndices.size(), 6); ++i) {
             std::cout << pfIndices[i] << " ";
        }
        std::cout << std::endl;
        //-----------------------------
*/
        genPt_all_.push_back(bestMatchedGenJet ? bestMatchedGenJet->pt() : -1.0);
        genEta_all_.push_back(bestMatchedGenJet ? std::abs(bestMatchedGenJet->eta()) : -1.0);
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
            genPt_5leading_.push_back(bestMatchedGenJet ? bestMatchedGenJet->pt() : -1.0);
            jetResponse_5leading_.push_back(response);
            puFracCount_5leading_.push_back(pu.fracCount);
            puFracPt_5leading_.push_back(pu.fracPt);
        }
*/
        idx++;
    }

    //=====Theo's method:This is GEN → RECO matching, required for efficiency=====
    int nGenMatched = 0;
    int nGenMatchedAmbig = 0;
    int igen_pass = 0;

    for (size_t i = 0; i < genJets.size(); ++i) {
        if (genJets[i].pt() <= 20.0) continue;
        // profile numerator partner
        genMatchedDen_all_[igen_pass] = genJetMatched[i];
        genJetMatched_all_.push_back(genJetMatched[i]);
        if (genJetMatched[i] == 1)
            nGenMatched++;
        else if (genJetAmbigMatched[i] == 1)
            nGenMatchedAmbig++;
        igen_pass++;
    }//End of for (size_t i = 0; i < genJets.size(); ++i)

    // PU jet fraction
    puJetFraction_all_ = (totalRecoJetsClean > 0)
        ? static_cast<float>(totalPUJets) / totalRecoJetsClean
        : -1.0f;
//    Jet efficiency/purity: Count HS vs PU jets explicitly 
//    int nGenMatched = std::count(genJetMatched.begin(), genJetMatched.end(), 1);
    const int nRecoJetsHS = totalRecoJetsClean - totalPUJets;

    efficiency_out = (totalGenJets > 0)     ? float(nGenMatched) / totalGenJets : -1.0f;
    effLossAmbig =(totalGenJets > 0) ? float(nGenMatchedAmbig)/totalGenJets : -1.0f;
    effWithAmbig = (totalGenJets > 0) ? float(nGenMatched + nGenMatchedAmbig)/totalGenJets : -1.0f;
    mistag = (totalRecoJetsClean > 0) ? float(totalRecoJetsClean - nMatchedReco)/totalRecoJetsClean : -1.0f;    
    purity_out =(totalRecoJetsClean > 0) ? float(nMatchedReco)/totalRecoJetsClean : -1.0f;    
//    efficiency_out = (totalGenJets > 0)     ? float(nRecoJetsHS)/totalGenJets : -1.0f;
//    purity_out     = (totalRecoJetsClean > 0)   ? float(nRecoJetsHS)/totalRecoJetsClean : -1.0f;

};//End of auto processFastJetCollection = [&]()

processJetCollection(*jets,  genJets, jetMatched_puppi_all_, genJetMatched_puppi_all_, genMatchedDen_puppi_all_, 
	jetIsHS_puppi_all_, jetPt_puppi_all_, jetAbsEta_puppi_all_, jetPhi_puppi_all_, jet_pfIndices_puppi_all_, jetResponse_PR_puppi_all_,genPt_puppi_all_, genEta_puppi_all_, genEtaDen_puppi_all_,puFracPt_algo_puppi_all_, puFracCount_algo_puppi_all_, puFracPt_truth_puppi_all_, puFracCount_truth_puppi_all_,jetDeltaR_puppi_all_,pf_indices_puppi_all_,
//	jetPt_puppi_5leading_, jetAbsEta_puppi_5leading_, jetResponse_PR_puppi_5leading_, genPt_puppi_5leading_, puFracPt_puppi_5leading_, puFracCount_puppi_5leading_,
	totalRecoJetsClean_puppi, totalPUJets_puppi, puJetFraction_puppi_all_,efficiency_puppi_, effLossAmbig_puppi_,effWithAmbig_puppi_,mistag_puppi_, purity_puppi_);


//const auto& pfrawJetsConst = pfrawJets;
processFastJetCollection(pfrawJets,  genJets, jetMatched_pfraw_all_, genJetMatched_pfraw_all_, genMatchedDen_pfraw_all_,
	jetIsHS_pfraw_all_, jetPt_pfraw_all_, jetAbsEta_pfraw_all_, jetPhi_pfraw_all_, jet_pfIndices_pfraw_all_, jetResponse_PR_pfraw_all_,genPt_pfraw_all_, genEta_pfraw_all_, genEtaDen_pfraw_all_, puFracPt_algo_pfraw_all_, puFracCount_algo_pfraw_all_,puFracPt_truth_pfraw_all_, puFracCount_truth_pfraw_all_,jetDeltaR_pfraw_all_,pf_indices_pfraw_all_,
//	jetPt_pfraw_5leading_, jetAbsEta_pfraw_5leading_, jetResponse_PR_pfraw_5leading_, genPt_pfraw_5leading_, puFracPt_pfraw_5leading_, puFracCount_pfraw_5leading_,
	totalRecoJetsClean_pfraw, totalPUJets_pfraw, puJetFraction_pfraw_all_, efficiency_pfraw_, effLossAmbig_pfraw_, effWithAmbig_pfraw_, mistag_pfraw_, purity_pfraw_);

processFastJetCollection(fromPV3Jets,  genJets, jetMatched_fromPV3_all_, genJetMatched_fromPV3_all_, genMatchedDen_fromPV3_all_,
	jetIsHS_fromPV3_all_, jetPt_fromPV3_all_, jetAbsEta_fromPV3_all_,jetPhi_fromPV3_all_, jet_pfIndices_fromPV3_all_, jetResponse_PR_fromPV3_all_,genPt_fromPV3_all_, genEta_fromPV3_all_, genEtaDen_fromPV3_all_, puFracPt_algo_fromPV3_all_, puFracCount_algo_fromPV3_all_,puFracPt_truth_fromPV3_all_, puFracCount_truth_fromPV3_all_,jetDeltaR_fromPV3_all_,pf_indices_fromPV3_all_,
//	jetPt_fromPV3_5leading_, jetAbsEta_fromPV3_5leading_, jetResponse_PR_fromPV3_5leading_, genPt_fromPV3_5leading_, puFracPt_fromPV3_5leading_, puFracCount_fromPV3_5leading_,
	totalRecoJetsClean_fromPV3, totalPUJets_fromPV3, puJetFraction_fromPV3_all_, efficiency_fromPV3_, effLossAmbig_fromPV3_, effWithAmbig_fromPV3_, mistag_fromPV3_, purity_fromPV3_);

processFastJetCollection(tightJets,  genJets, jetMatched_tight_all_, genJetMatched_tight_all_, genMatchedDen_tight_all_,
	jetIsHS_tight_all_,jetPt_tight_all_, jetAbsEta_tight_all_, jetPhi_tight_all_, jet_pfIndices_tight_all_, jetResponse_PR_tight_all_,genPt_tight_all_, genEta_tight_all_, genEtaDen_tight_all_, puFracPt_algo_tight_all_, puFracCount_algo_tight_all_,puFracPt_truth_tight_all_, puFracCount_truth_tight_all_,jetDeltaR_tight_all_,pf_indices_tight_all_,
//	jetPt_tight_5leading_, jetAbsEta_tight_5leading_, jetResponse_PR_tight_5leading_, genPt_tight_5leading_, puFracPt_tight_5leading_, puFracCount_tight_5leading_,
	totalRecoJetsClean_tight, totalPUJets_tight, puJetFraction_tight_all_, efficiency_tight_, effLossAmbig_tight_, effWithAmbig_tight_, mistag_tight_, purity_tight_);


processFastJetCollection(looseJets,  genJets, jetMatched_loose_all_, genJetMatched_loose_all_, genMatchedDen_loose_all_,
	jetIsHS_loose_all_, jetPt_loose_all_, jetAbsEta_loose_all_,jetPhi_loose_all_, jet_pfIndices_loose_all_, jetResponse_PR_loose_all_,genPt_loose_all_,genEta_loose_all_, genEtaDen_loose_all_, puFracPt_algo_loose_all_, puFracCount_algo_loose_all_,puFracPt_truth_loose_all_, puFracCount_truth_loose_all_,jetDeltaR_loose_all_,pf_indices_loose_all_,
//	jetPt_loose_5leading_, jetAbsEta_loose_5leading_, jetResponse_PR_loose_5leading_, genPt_loose_5leading_, puFracPt_loose_5leading_, puFracCount_loose_5leading_,
	totalRecoJetsClean_loose, totalPUJets_loose, puJetFraction_loose_all_, efficiency_loose_, effLossAmbig_loose_, effWithAmbig_loose_, mistag_loose_,purity_loose_);


processFastJetCollection(timeJets,  genJets, jetMatched_time_all_, genJetMatched_time_all_, genMatchedDen_time_all_,
	jetIsHS_time_all_, jetPt_time_all_, jetAbsEta_time_all_, jetPhi_time_all_, jet_pfIndices_time_all_, jetResponse_PR_time_all_,genPt_time_all_,genEta_time_all_, genEtaDen_time_all_, puFracPt_algo_time_all_, puFracCount_algo_time_all_,puFracPt_truth_time_all_, puFracCount_truth_time_all_, jetDeltaR_time_all_,pf_indices_time_all_,
//	jetPt_time_5leading_, jetAbsEta_time_5leading_, jetResponse_PR_time_5leading_, genPt_time_5leading_, puFracPt_time_5leading_, puFracCount_time_5leading_,
	totalRecoJetsClean_time, totalPUJets_time, puJetFraction_time_all_, efficiency_time_, effLossAmbig_time_, effWithAmbig_time_, mistag_time_, purity_time_);


processFastJetCollection(timeJets4D,  genJets, jetMatched_time4D_all_, genJetMatched_time4D_all_, genMatchedDen_time4D_all_,
	jetIsHS_time4D_all_, jetPt_time4D_all_, jetAbsEta_time4D_all_, jetPhi_time4D_all_, jet_pfIndices_time4D_all_, jetResponse_PR_time4D_all_,genPt_time4D_all_, genEta_time4D_all_, genEtaDen_time4D_all_, puFracPt_algo_time4D_all_, puFracCount_algo_time4D_all_,puFracPt_truth_time4D_all_, puFracCount_truth_time4D_all_, jetDeltaR_time4D_all_,pf_indices_time4D_all_,
//	jetPt_time4D_5leading_, jetAbsEta_time4D_5leading_, jetResponse_PR_time4D_5leading_, genPt_time4D_5leading_, puFracPt_time4D_5leading_, puFracCount_time4D_5leading_,
	totalRecoJetsClean_time4D, totalPUJets_time4D, puJetFraction_time4D_all_, efficiency_time4D_, effLossAmbig_time4D_, effWithAmbig_time4D_, mistag_time4D_,purity_time4D_);



//  jetResponse_PR_tight_ = -1;
//  jetResponse_PR_loose_ = -1;
//  jetAbsEta_ = -1; 


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


}//End of void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {


//-------------------Same PFset Test: sJet=fast jet constituents test 
void comparePFConstituents(const pat::Jet& patJet,
                          const fastjet::PseudoJet& fjJet,
                          const std::vector<const pat::PackedCandidate*>& pf)
{
    auto patIdx = getPFIndicesFromPatJet(patJet);
    auto fjIdx  = getPFIndicesFromPseudoJet(fjJet, pf);

    std::set<unsigned int> patSet(patIdx.begin(), patIdx.end());
    std::set<unsigned int> fjSet(fjIdx.begin(), fjIdx.end());

    size_t nCommon = 0;
    for (auto idx : patSet) {
        if (fjSet.count(idx)) nCommon++;
    }

    std::cout << "PAT size=" << patSet.size()
              << " FJ size=" << fjSet.size()
              << " common=" << nCommon
              << " purity(PAT)=" << float(nCommon)/patSet.size()
              << " purity(FJ)="  << float(nCommon)/fjSet.size()
              << std::endl;
}//End of void comparePFConstituents()
//-------------------End of Same PFset Test: sJet=fast jet constituents test 


// void JetTreeProducer::fillHSGenJets() 정의
void JetTreeProducer::fillHSGenJets(
    const reco::GenJetCollection& genJets
) {
    nHSGenJets_ = 0;
    hsGenJet_pt_.clear();
    hsGenJet_eta_.clear();

    for (const auto& jet : genJets) {
        if (jet.pt() < 10.0) continue;
        if (std::abs(jet.eta()) > 3) continue;

        nHSGenJets_++;
        hsGenJet_pt_.push_back(jet.pt());
        hsGenJet_eta_.push_back(jet.eta());
    }
}//End of void JetTreeProducer::fillHSGenJets()



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

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/PatCandidates/interface/Jet.h"

#include "TTree.h"
#include "TFile.h"

class JetTreeProducer : public edm::EDAnalyzer {
public:
    explicit JetTreeProducer(const edm::ParameterSet&);
    ~JetTreeProducer() override = default;

private:
    void beginJob() override;
    void analyze(const edm::Event&, const edm::EventSetup&) override;
    void endJob() override;

    edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;

    TFile* file_;
    TTree* tree_;

    float pt_, eta_, phi_, mass_;
};

JetTreeProducer::JetTreeProducer(const edm::ParameterSet& iConfig) {
    jetsToken_ = consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jets"));
}

void JetTreeProducer::beginJob() {
    file_ = new TFile("jetTree.root", "RECREATE");
    tree_ = new TTree("JetTree", "JetTree");

    tree_->Branch("pt", &pt_, "pt/F");
    tree_->Branch("eta", &eta_, "eta/F");
    tree_->Branch("phi", &phi_, "phi/F");
    tree_->Branch("mass", &mass_, "mass/F");
}

void JetTreeProducer::analyze(const edm::Event& iEvent, const edm::EventSetup&) {
    edm::Handle<std::vector<pat::Jet>> jets;
    iEvent.getByToken(jetsToken_, jets);

    for (const auto& jet : *jets) {
        pt_ = jet.pt();
        eta_ = jet.eta();
        phi_ = jet.phi();
        mass_ = jet.mass();
        tree_->Fill();
    }
}

void JetTreeProducer::endJob() {
    file_->cd();
    tree_->Write();
    file_->Close();
}

DEFINE_FWK_MODULE(JetTreeProducer);


#include "MyAnalysis/JetTreeProducer/interface/PFParticle.h"
#include "TClass.h"

// Required for ROOT to generate dictionary and vtable

PFParticle::PFParticle()
  : m_pt(0), m_eta(0), m_phi(0), m_energy(0), m_charge(0),
    m_particleID(eX),
    m_puppi_weight(1.0),
    m_puppi_weight_nolep(1.0),
    m_dzSig(-99),
    m_time(-99)
{}
PFParticle::~PFParticle() {}

void PFParticle::setKinematics(float pt, float eta, float phi, float energy, float charge) {
  m_pt = pt; m_eta = eta; m_phi = phi; m_energy = energy; m_charge = charge;
}

void PFParticle::setParticleID(EParticleID id) {
  m_particleID = id;
}

void PFParticle::setPuppiWeight(float w) {
  m_puppi_weight = w;
}

void PFParticle::setPuppiWeightNoLep(float w) {
  m_puppi_weight_nolep = w;
}

void PFParticle::setDzSig(float dzsig) {
  m_dzSig = dzsig;
}

void PFParticle::setTime(float time) {
  m_time = time;
}

ClassImp(PFParticle)

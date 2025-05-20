//#pragma once
#ifndef MYANALYSIS_JETTREEPRODUCER_PFPARTICLE_H
#define MYANALYSIS_JETTREEPRODUCER_PFPARTICLE_H

#include "Rtypes.h"
//#include "Particle.h"

//class PFParticle : public Particle{
class PFParticle {

public:
  PFParticle();
  virtual ~PFParticle();

  enum EParticleID {
    eX=0,          /**< undefined */
    eH,            /**< charged hadron */
    eE,            /**< electron  */
    eMu,           /**< muon  */
    eGamma,        /**< photon */
    eH0,           /**< neutral hadron */
    eH_HF,         /**< HF tower identified as a hadron */
    eEgamma_HF     /**< HF tower identified as an EM particle */
  };

//    PFParticle()
//      :  m_pt(0), m_eta(0), m_phi(0), m_energy(0), m_charge(0),
//      m_particleID(eX),
//      m_puppi_weight(1.0),
//      m_puppi_weight_nolep(1.0),
//      m_dzSig(-99),
//      m_time(-99)
//  {}


  // Accessors
  float getPt() const { return m_pt; }
  float getEta() const { return m_eta; }
  float getPhi() const { return m_phi; }
  float getEnergy() const { return m_energy; }
  float getCharge() const { return m_charge; }

  EParticleID particleID() const { return m_particleID; }
  float puppiWeight() const { return m_puppi_weight; }
  float puppiWeightNoLep() const { return m_puppi_weight_nolep; }
  float dzSig() const { return m_dzSig; }
  float time() const { return m_time; }

  // Setters
  void setKinematics(float pt, float eta, float phi, float energy, float charge);
  void setParticleID(EParticleID id);
  void setPuppiWeight(float w);
  void setPuppiWeightNoLep(float w);
  void setDzSig(float dzsig);
  void setTime(float time);
  
//  void setKinematics(float pt, float eta, float phi, float energy, float charge) {
//   m_pt = pt; m_eta = eta; m_phi = phi; m_energy = energy; m_charge = charge;
//  }
//  void setParticleID(EParticleID id) { m_particleID = id; }
//  void setPuppiWeight(float w) { m_puppi_weight = w; }
//  void setPuppiWeightNoLep(float w) { m_puppi_weight_nolep = w; }
//  void setDzSig(float dzsig) { m_dzSig = dzsig; }
//  void setTime(float time) { m_time = time; }

 private:
//  float x_, y_, z_;
  float m_pt, m_eta, m_phi, m_energy, m_charge;
  EParticleID m_particleID;
  float m_puppi_weight;
  float m_puppi_weight_nolep;
  float m_dzSig;
  float m_time;

  ClassDef(PFParticle, 1)
};
#endif // MyAnalysis_JetTreeProducer_PFParticle_h

//  
//       ,---.     ,--,    .---.     ,--,    ,---.    .-. .-. 
//       | .-'   .' .')   / .-. )  .' .'     | .-'    |  \| | 
//       | `-.   |  |(_)  | | |(_) |  |  __  | `-.    |   | | 
//       | .-'   \  \     | | | |  \  \ ( _) | .-'    | |\  | 
//       |  `--.  \  `-.  \ `-' /   \  `-) ) |  `--.  | | |)| 
//       /( __.'   \____\  )---'    )\____/  /( __.'  /(  (_) 
//      (__)              (_)      (__)     (__)     (__)     
//
//  This file is part of ECOGEN.
//
//  ECOGEN is the legal property of its developers, whose names 
//  are listed in the copyright file included with this source 
//  distribution.
//
//  ECOGEN is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published 
//  by the Free Software Foundation, either version 3 of the License, 
//  or (at your option) any later version.
//  
//  ECOGEN is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with ECOGEN (file LICENSE).  
//  If not, see <http://www.gnu.org/licenses/>.

//! \file      MixThermalEq.cpp
//! \author    F. Petitpas
//! \version   1.0
//! \date      May 04 2018

#include <cmath>
#include "MixThermalEq.h"

using namespace std;
using namespace tinyxml2;

//***************************************************************************

MixThermalEq::MixThermalEq() :m_density(0.), m_pressure(0.), m_velocity(0), m_energie(0.), m_totalEnergy(0.), m_thermalEqSoundSpeed(0.) {}

//***************************************************************************

MixThermalEq::MixThermalEq(XMLElement *state, string fileName) :
  m_density(0.), m_pressure(0.), m_energie(0.), m_totalEnergy(0.), m_thermalEqSoundSpeed(0.)
{
  XMLElement *sousElement(state->FirstChildElement("mixture"));
  if (sousElement == NULL) throw ErrorXMLElement("mixture", fileName, __FILE__, __LINE__);
  //Attributes reading
  //------------------
  XMLError error;
  XMLElement *dataMix(sousElement->FirstChildElement("dataMix"));
  if (dataMix == NULL) throw ErrorXMLElement("dataMix", fileName, __FILE__, __LINE__);
  //pressure
  error = dataMix->QueryDoubleAttribute("pressure", &m_pressure);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("pressure", fileName, __FILE__, __LINE__);
  //temperature
  error = dataMix->QueryDoubleAttribute("temperature", &m_temperature);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("temperature", fileName, __FILE__, __LINE__);

  //velocity
  XMLElement *velocity(sousElement->FirstChildElement("velocity"));
  if (velocity == NULL) throw ErrorXMLElement("velocity", fileName, __FILE__, __LINE__);
  double velocityX(0.), velocityY(0.), velocityZ(0.);
  error = velocity->QueryDoubleAttribute("x", &velocityX);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("x", fileName, __FILE__, __LINE__);
  error = velocity->QueryDoubleAttribute("y", &velocityY);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("y", fileName, __FILE__, __LINE__);
  error = velocity->QueryDoubleAttribute("z", &velocityZ);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("z", fileName, __FILE__, __LINE__);
  m_velocity.setXYZ(velocityX, velocityY, velocityZ);
}

//***************************************************************************

MixThermalEq::~MixThermalEq(){}

//***************************************************************************

void MixThermalEq::allocateAndCopyMixture(Mixture **mixture)
{
  *mixture = new MixThermalEq(*this);
}

//***************************************************************************

void MixThermalEq::copyMixture(Mixture &mixture)
{
  m_density = mixture.getDensity();
  m_pressure = mixture.getPressure();
  m_temperature = mixture.getTemperature();
  m_velocity = mixture.getVelocity();
  m_energie = mixture.getEnergy();
  m_totalEnergy = mixture.getTotalEnergy();
  m_thermalEqSoundSpeed = mixture.getMixSoundSpeed();
}

//***************************************************************************

double MixThermalEq::computeDensity(const double *alphak, const double *rhok, const int &numberPhases)
{
  double rho(0.);
  for(int k=0;k<numberPhases;k++)
  {
      rho += alphak[k]*rhok[k];
  }
  return rho;
}

//***************************************************************************

double MixThermalEq::computePressure(const double *alphak, const double *pk, const int &numberPhases)
{
  double p(0.);
  for(int k=0;k<numberPhases;k++)
  {
      p += alphak[k]*pk[k];
  }
  return p;
}

//***************************************************************************

double MixThermalEq::computePressure(double *masses, const double &mixInternalEnerg, Phase **phases, const int &numberPhases)
{
  //Restrictions
  if (numberPhases > 2) Errors::errorMessage("more than two phases not permitted in thermal equilibrium model : MixThermalEq::computePressure");
  for (int k = 0; k < numberPhases; k++) {
    if (phases[k]->getEos()->getType() != "IG" && phases[k]->getEos()->getType() != "SG") { Errors::errorMessage("Only IG and SG permitted in thermal equilibrium model : MixThermalEq::computePressure"+ phases[k]->getEos()->getType()); }
  }

  double rhoMel(0.);
  for (int k = 0; k < numberPhases; k++) {
    rhoMel += masses[k];
  }

  //Formulae of pressure for 2 phases goverened by SG EOS (Le Martelot, 2013, thesis)
  double gamma1 = phases[0]->getEos()->getGamma();
  double pInf1 = phases[0]->getEos()->getPInf();
  double cv1 = phases[0]->getEos()->getCv();
  double e01 = phases[0]->getEos()->getERef();
  double Y1 = masses[0] / rhoMel;

  double gamma2 = phases[1]->getEos()->getGamma();
  double pInf2 = phases[1]->getEos()->getPInf();
  double cv2 = phases[1]->getEos()->getCv();
  double e02 = phases[1]->getEos()->getERef();
  double Y2 = masses[1] / rhoMel;

  double q = Y1*e01 + Y2*e02;
  double cvMel = Y1*cv1 + Y2*cv2;
  
  double A1 = Y1*(gamma1 - 1.)*cv1 / cvMel*(rhoMel*(mixInternalEnerg - q) - pInf1);
  double A2 = Y2*(gamma2 - 1.)*cv2 / cvMel*(rhoMel*(mixInternalEnerg - q) - pInf2);

  m_pressure = 0.5*(A1 + A2 - (pInf1 + pInf2)) + sqrt(0.25*(A2 - A1 - (pInf2 - pInf1))*(A2 - A1 - (pInf2 - pInf1)) + A1*A2);
 
  return m_pressure;
}

//***************************************************************************

double MixThermalEq::computeTemperature(double *masses, const double &pressure, Phase **phases, const int &numberPhases)
{
  //Restrictions
  for (int k = 0; k < numberPhases; k++) {
    if (phases[k]->getEos()->getType() != "IG" && phases[k]->getEos()->getType() != "SG") { Errors::errorMessage("Only IG and SG permitted in thermal equilibrium model : MixThermalEq::computePressure"); }
  }

  double rhoMel(0.);
  for (int k = 0; k < numberPhases; k++) {
    rhoMel += masses[k];
  }

  //Formulae for phases goverened by SG EOS (Le Martelot, 2013, phd thesis)
  double gammak, pInfk, cvk, Yk;
  m_temperature = 0.;
  for (int k = 0; k < numberPhases; k++) {
    gammak = phases[k]->getEos()->getGamma();
    pInfk = phases[k]->getEos()->getPInf();
    cvk = phases[k]->getEos()->getCv();
    Yk = masses[k] / rhoMel;
    m_temperature += Yk*(gammak - 1.)*cvk / (pressure + pInfk);
  }
  m_temperature = 1. / (m_temperature*rhoMel);

  return m_temperature;
}

//***************************************************************************

double MixThermalEq::computeInternalEnergy(const double *Yk, const double *ek, const int &numberPhases)
{
  double e(0.);
  for(int k=0;k<numberPhases;k++)
  {
      e += Yk[k]*ek[k];
  }
  return e;
}

//***************************************************************************

double MixThermalEq::computeFrozenSoundSpeed(const double *Yk, const double *ck, const int &numberPhases)
{
  double cF(0.);
  for(int k=0;k<numberPhases;k++)
  {
      cF += Yk[k]*ck[k]*ck[k];
  }
  return sqrt(cF);
}

//***************************************************************************

double MixThermalEq::computeTemperatureIsentrope(const double *Yk, const double &p0, const double &T0, const double &p, const int &numberPhases, double *dTdp)
{
  //Restrictions
  if (numberPhases > 2) Errors::errorMessage("more than two phases not permitted in thermal equilibrium model : MixThermalEq::computeTemperatureIsentrope");
  for (int k = 0; k < numberPhases; k++) {
    if (TB->eos[k]->getType() != "IG" && TB->eos[k]->getType() != "SG") { Errors::errorMessage("Only IG and SG permitted in thermal equilibrium model : MixThermalEq::computeTemperatureIsentrope" + TB->eos[k]->getType()); }
  }

  //Formulae for phases goverened by SG EOS
  double T(T0), cM(0.); 
  if (dTdp != NULL) *dTdp = 0.;
  for (int k = 0; k < numberPhases; k++) {
    cM += Yk[k] * TB->eos[k]->getGamma()* TB->eos[k]->getCv();
  }
  double puissance(0.), fk;
  for (int k = 0; k < numberPhases; k++) {
    puissance = (TB->eos[k]->getGamma() - 1.)*Yk[k] * TB->eos[k]->getCv()/cM;
    fk = pow((p + TB->eos[k]->getPInf()) / ((p0 + TB->eos[k]->getPInf())), puissance);
    T *= fk;
    if (dTdp != NULL) *dTdp += puissance/ (p + TB->eos[k]->getPInf());
  }
  if (dTdp != NULL) *dTdp *= T;

  return T;
}

//***************************************************************************

double MixThermalEq::computeEnthalpyIsentrope(const double *Yk, const double &p0, const double &T0, const double &p, const int &numberPhases, double *dhdp)
{
  //Restrictions
  for (int k = 0; k < numberPhases; k++) {
    if (TB->eos[k]->getType() != "IG" && TB->eos[k]->getType() != "SG") { Errors::errorMessage("Only IG and SG permitted in thermal equilibrium model : MixThermalEq::computeEnthalpyIsentrope" + TB->eos[k]->getType()); }
  }

  double dTdp(0.);
  double T = this->computeTemperatureIsentrope(Yk, p0, T0, p, numberPhases, &dTdp);
  //Formulae for phases goverened by SG EOS
  double h(0.);
  if (dhdp != NULL) *dhdp = 0.;
  for (int k = 0; k < numberPhases; k++) {
    h += Yk[k] * (TB->eos[k]->getGamma()*TB->eos[k]->getCv()*T + TB->eos[k]->getERef());
    if (dhdp != NULL) *dhdp += Yk[k] * TB->eos[k]->getGamma()*TB->eos[k]->getCv()*dTdp;
  }
  
  return h;
}

//***************************************************************************

double MixThermalEq::computeVolumeIsentrope(const double *Yk, const double &p0, const double &T0, const double &p, const int &numberPhases, double *dvdp)
{
  //Restrictions
  for (int k = 0; k < numberPhases; k++) {
    if (TB->eos[k]->getType() != "IG" && TB->eos[k]->getType() != "SG") { Errors::errorMessage("Only IG and SG permitted in thermal equilibrium model : MixThermalEq::computeVolumeIsentrope" + TB->eos[k]->getType()); }
  }

  double dTdp(0.);
  double T = this->computeTemperatureIsentrope(Yk, p0, T0, p, numberPhases, &dTdp);
  //Formulae for phases goverened by SG EOS
  double v(0.), vk(0.), dvk(0.); 
  if (dvdp != NULL) *dvdp = 0.;
  for (int k = 0; k < numberPhases; k++) {
    vk = ((TB->eos[k]->getGamma() - 1.)*TB->eos[k]->getCv()*T)/ (p + TB->eos[k]->getPInf());
    dvk = ((TB->eos[k]->getGamma() - 1.)*TB->eos[k]->getCv()*dTdp - vk) / (p + TB->eos[k]->getPInf());
    v += Yk[k] * vk;
    if (dvdp != NULL) *dvdp += Yk[k] * dvk;
  }

  return v;
}

//***************************************************************************

void MixThermalEq::computeMixtureVariables(Phase **vecPhase, const int &numberPhases)
{
  //mixture density and pressure
  m_density = 0.;
  for (int k = 0; k < numberPhases; k++) {
    m_density += vecPhase[k]->getAlpha()*vecPhase[k]->getDensity();
  }
  //Mass fraction
  for (int k = 0; k < numberPhases; k++) {
    TB->Yk[k] = vecPhase[k]->getAlpha()*vecPhase[k]->getDensity() / m_density;
  }
  //Specific internal energy, speed of sound
  m_energie = 0.;
  m_thermalEqSoundSpeed = 0.;
  for (int k = 0; k < numberPhases; k++) {
    m_energie += TB->Yk[k] * vecPhase[k]->getEnergy();
    m_thermalEqSoundSpeed += TB->Yk[k] * vecPhase[k]->getSoundSpeed()*vecPhase[k]->getSoundSpeed();
  }
  m_thermalEqSoundSpeed = sqrt(m_thermalEqSoundSpeed);  
  //m_totalEnergy cannot be computed here because depending on extra additional energies
}

//***************************************************************************

void MixThermalEq::internalEnergyToTotalEnergy(vector<QuantitiesAddPhys*> &vecGPA)
{
  m_totalEnergy = m_energie + 0.5*m_velocity.squaredNorm();
  for (unsigned int pa = 0; pa < vecGPA.size(); pa++) {
    m_totalEnergy += vecGPA[pa]->computeEnergyAddPhys()/m_density; //Caution /m_density important
  }
}

//***************************************************************************

void MixThermalEq::totalEnergyToInternalEnergy(vector<QuantitiesAddPhys*> &vecGPA)
{
  m_energie = m_totalEnergy - 0.5*m_velocity.squaredNorm();
  for (unsigned int pa = 0; pa < vecGPA.size(); pa++) {
    m_energie -= vecGPA[pa]->computeEnergyAddPhys()/m_density; //Caution /m_density important
  }
}

//***************************************************************************

void MixThermalEq::localProjection(const Coord &normal, const Coord &tangent, const Coord &binormal)
{
  m_velocity.localProjection(normal, tangent, binormal);
}

//***************************************************************************

void MixThermalEq::reverseProjection(const Coord &normal, const Coord &tangent, const Coord &binormal)
{
  m_velocity.reverseProjection(normal, tangent, binormal);
}

//****************************************************************************
//**************************** DATA PRINTING *********************************
//****************************************************************************

double MixThermalEq::returnScalar(const int &numVar) const
{
  switch (numVar)
  {
  case 1:
    return m_density; break;
  case 2:
    return m_pressure; break;
  case 3:
    return m_temperature; break;
  default:
    return 0.; break;
  }
}

//***************************************************************************

Coord MixThermalEq::returnVector(const int &numVar) const
{
  switch (numVar)
  {
  case 1:
    return m_velocity; break;
  default:
    return 0; break;
  }
}

//***************************************************************************

string MixThermalEq::returnNameScalar(const int &numVar) const
{
  switch (numVar)
  {
  case 1:
    return "Density_Mixture"; break;
  case 2:
    return "Pressure_Mixture"; break;
  case 3:
    return "Temperature_Mixture"; break;
  default:
    return "NoName"; break;
  }
}

//***************************************************************************

string MixThermalEq::returnNameVector(const int &numVar) const
{
  switch (numVar)
  {
  case 1:
    return "Velocity_Mixture_norm"; break;
  default:
    return "NoName"; break;
  }
}

//****************************************************************************
//**************************** DATA READING **********************************
//****************************************************************************

void MixThermalEq::setScalar(const int &numVar, const double &value)
{
  switch (numVar)
  {
  case 1:
    m_density = value; break;
  case 2:
    m_pressure = value; break;
  case 3:
    m_temperature = value; break;
  default:
    Errors::errorMessage("numVar not found in MixThermalEq::setScalar"); break;
  }
}

//***************************************************************************

void MixThermalEq::setVector(const int &numVar, const Coord &value)
{
  switch (numVar)
  {
  case 1:
    m_velocity = value; break;
  default:
    Errors::errorMessage("numVar not found in MixThermalEq::setVector"); break;
  }
}

//****************************************************************************
//****************************** PARALLEL ************************************
//****************************************************************************

int MixThermalEq::numberOfTransmittedVariables() const
{
  //3 scalar + 1 vector : 6 variables
  return 6;
}

//***************************************************************************

void MixThermalEq::fillBuffer(double *buffer, int &counter) const
{
  buffer[++counter] = m_pressure;
  buffer[++counter] = m_temperature;
  buffer[++counter] = m_velocity.getX();
  buffer[++counter] = m_velocity.getY();
  buffer[++counter] = m_velocity.getZ();
  buffer[++counter] = m_totalEnergy;
}

//***************************************************************************

void MixThermalEq::getBuffer(double *buffer, int &counter)
{
  m_pressure = buffer[++counter];
  m_temperature = buffer[++counter];
  m_velocity.setX(buffer[++counter]);
  m_velocity.setY(buffer[++counter]);
  m_velocity.setZ(buffer[++counter]);
  m_totalEnergy = buffer[++counter];
}

//****************************************************************************
//******************************* ORDER 2 ************************************
//****************************************************************************

void MixThermalEq::computeSlopesMixture(const Mixture &sLeft, const Mixture &sRight, const double &distance)
{
  m_pressure = (sRight.getPressure() - sLeft.getPressure()) / distance;
  m_temperature = (sRight.getTemperature() - sLeft.getTemperature()) / distance;
  m_velocity.setX((sRight.getVelocity().getX() - sLeft.getVelocity().getX()) / distance);
  m_velocity.setY((sRight.getVelocity().getY() - sLeft.getVelocity().getY()) / distance);
  m_velocity.setZ((sRight.getVelocity().getZ() - sLeft.getVelocity().getZ()) / distance);
}

//***************************************************************************

void MixThermalEq::setToZero()
{
  m_pressure = 0.; m_temperature = 0.;
  m_velocity.setX(0.); m_velocity.setY(0.); m_velocity.setZ(0.);
}

//***************************************************************************

void MixThermalEq::extrapolate(const Mixture &slope, const double &distance)
{
  m_pressure += slope.getPressure()*distance;
  m_temperature += slope.getTemperature()*distance;
  m_velocity.setX(m_velocity.getX() + slope.getVelocity().getX() * distance);
  m_velocity.setY(m_velocity.getY() + slope.getVelocity().getY() * distance);
  m_velocity.setZ(m_velocity.getZ() + slope.getVelocity().getZ() * distance);
}

//***************************************************************************

void MixThermalEq::limitSlopes(const Mixture &slopeGauche, const Mixture &slopeDroite, Limiter &globalLimiter)
{
  m_pressure = globalLimiter.limiteSlope(slopeGauche.getPressure(), slopeDroite.getPressure());
  m_temperature = globalLimiter.limiteSlope(slopeGauche.getTemperature(), slopeDroite.getTemperature());
  m_velocity.setX(globalLimiter.limiteSlope(slopeGauche.getVelocity().getX(), slopeDroite.getVelocity().getX()));
  m_velocity.setY(globalLimiter.limiteSlope(slopeGauche.getVelocity().getY(), slopeDroite.getVelocity().getY()));
  m_velocity.setZ(globalLimiter.limiteSlope(slopeGauche.getVelocity().getZ(), slopeDroite.getVelocity().getZ()));
}

//****************************************************************************
//************************** ORDER 2 PARALLEL ********************************
//****************************************************************************

int MixThermalEq::numberOfTransmittedSlopes() const
{
	return 5;
}

//***************************************************************************

void MixThermalEq::fillBufferSlopes(double *buffer, int &counter) const
{
  buffer[++counter] = m_pressure;
  buffer[++counter] = m_temperature;
	buffer[++counter] = m_velocity.getX();
	buffer[++counter] = m_velocity.getY();
	buffer[++counter] = m_velocity.getZ();
}

//***************************************************************************

void MixThermalEq::getBufferSlopes(double *buffer, int &counter)
{
  m_pressure = buffer[++counter];
  m_temperature = buffer[++counter];
	m_velocity.setX(buffer[++counter]);
	m_velocity.setY(buffer[++counter]);
	m_velocity.setZ(buffer[++counter]);
}

//****************************************************************************
//******************************* ACCESSORS **********************************
//****************************************************************************

double MixThermalEq::getDensity() const
{
  return m_density;
}

//***************************************************************************

double MixThermalEq::getPressure() const
{
  return m_pressure;
}

//***************************************************************************

double MixThermalEq::getTemperature() const
{
  return m_temperature;
}


//***************************************************************************

double MixThermalEq::getU() const { return m_velocity.getX(); }
double MixThermalEq::getV() const { return m_velocity.getY(); }
double MixThermalEq::getW() const { return m_velocity.getZ(); }

//***************************************************************************

Coord MixThermalEq::getVelocity() const
{
  return m_velocity;
}

//***************************************************************************

double MixThermalEq::getEnergy() const
{
  return m_energie;
}

//***************************************************************************

double MixThermalEq::getTotalEnergy() const
{
  return m_totalEnergy;
}

//***************************************************************************

double MixThermalEq::getMixSoundSpeed() const
{
  return m_thermalEqSoundSpeed;
}

//***************************************************************************

void MixThermalEq::setPressure(const double &p) { m_pressure = p; }

//***************************************************************************

void MixThermalEq::setVelocity(const double &u, const double &v, const double &w) { m_velocity.setXYZ(u, v, w); }

//***************************************************************************

void MixThermalEq::setVelocity(const Coord &vit) { m_velocity = vit; }

//***************************************************************************

void MixThermalEq::setU(const double &u) { m_velocity.setX(u); }

//***************************************************************************

void MixThermalEq::setV(const double &v) { m_velocity.setY(v); }

//***************************************************************************

void MixThermalEq::setW(const double &w) { m_velocity.setZ(w); }

//***************************************************************************

void MixThermalEq::setTotalEnergy(double &totalEnergy)
{
  m_totalEnergy = totalEnergy;
}

//****************************************************************************
//***************************** OPERATORS ************************************
//****************************************************************************

void MixThermalEq::changeSign()
{
  m_pressure = -m_pressure;
  m_temperature = -m_temperature;
  m_velocity = m_velocity*-1.;
}

//***************************************************************************

void MixThermalEq::multiplyAndAdd(const Mixture &slopesMixtureTemp, const double &coeff)
{
  m_pressure += slopesMixtureTemp.getPressure()*coeff;
  m_temperature += slopesMixtureTemp.getTemperature()*coeff;
  m_velocity += slopesMixtureTemp.getVelocity()*coeff;
}

//***************************************************************************

void MixThermalEq::divide(const double &coeff)
{
  m_pressure /= coeff;
  m_temperature /= coeff;
  m_velocity /= coeff;
}

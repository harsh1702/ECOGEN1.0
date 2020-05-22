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

//! \file      ModThermalEq.cpp
//! \author    F. Petitpas
//! \version   1.0
//! \date      May 04 2018

#include <iostream>
#include <cmath>
#include <algorithm>
#include "ModThermalEq.h"
#include "PhaseThermalEq.h"

using namespace std;

const std::string ModThermalEq::NAME = "THERMALEQ";

//***********************************************************************

ModThermalEq::ModThermalEq(int &numberTransports, const int &numberPhases) :
  Model(NAME,numberTransports)
{
  fluxBufferThermalEq = new FluxThermalEq(numberPhases);
  sourceConsThermEq = new FluxThermalEq(numberPhases);
}

//***********************************************************************

ModThermalEq::~ModThermalEq()
{
  delete fluxBufferThermalEq;
  delete sourceConsThermEq;
}

//***********************************************************************

void ModThermalEq::allocateCons(Flux **cons, const int &numberPhases)
{
  *cons = new FluxThermalEq(numberPhases);
}

//***********************************************************************

void ModThermalEq::allocatePhase(Phase **phase)
{
  *phase = new PhaseThermalEq;
}

//***********************************************************************

void ModThermalEq::allocateMixture(Mixture **mixture)
{
  *mixture = new MixThermalEq;
}

//***********************************************************************

void ModThermalEq::fulfillState(Phase **phases, Mixture *mixture, const int &numberPhases, Prim type)
{
  //Complete phases and mixture states from : alphak, pressure and temperature
  for (int k = 0; k < numberPhases; k++) {
    phases[k]->setPressure(mixture->getPressure());
    phases[k]->setDensity(phases[k]->getEos()->computeDensity(mixture->getPressure(), mixture->getTemperature()));
    phases[k]->extendedCalculusPhase(mixture->getVelocity());
  }
  mixture->computeMixtureVariables(phases, numberPhases);
}

//****************************************************************************
//********************* Cell to cell Riemann solvers *************************
//****************************************************************************

void ModThermalEq::solveRiemannIntern(Cell &cellLeft, Cell &cellRight, const int &numberPhases, const double &dxLeft, const double &dxRight, double &dtMax) const
{
  Phase *vecPhase;
  double sL, sR;
  double pStar(0.), rhoStar(0.), uStar(0.), vStar(0.), wStar(0.), EStar(0.), eStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getMixSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();
  double uR = cellRight.getMixture()->getVelocity().getX(), cR = cellRight.getMixture()->getMixSoundSpeed(), pR = cellRight.getMixture()->getPressure(), rhoR = cellRight.getMixture()->getDensity();

  //Davies
  sL = min(uL - cL, uR - cR);
  sR = max(uR + cR, uL + cL);

  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  if (abs(sR)>1.e-3) dtMax = min(dtMax, dxRight / abs(sR));

  //compute left and right mass flow rates and sM
  double mL(rhoL*(sL - uL)), mR(rhoR*(sR - uR)), mkL, mkR;
  double sM((pR - pL + mL*uL - mR*uR) / (mL - mR));
  if (abs(sM)<1.e-8) sM = 0.;

  //Solution sampling
  if (sL >= 0.){
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellLeft.getPhase(k);
      double alpha = vecPhase->getAlpha();
      double density = vecPhase->getDensity();
      fluxBufferThermalEq->m_masse[k] = alpha*density*uL;
    }
    double vitY = cellLeft.getMixture()->getVelocity().getY(); double vitZ = cellLeft.getMixture()->getVelocity().getZ();
    double totalEnergy = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();
    fluxBufferThermalEq->m_qdm.setX(rhoL*uL*uL + pL);
    fluxBufferThermalEq->m_qdm.setY(rhoL*vitY*uL);
    fluxBufferThermalEq->m_qdm.setZ(rhoL*vitZ*uL);
    fluxBufferThermalEq->m_energMixture = (rhoL*totalEnergy + pL)*uL;

  }
  else if (sR <= 0.){
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellRight.getPhase(k);
      double alpha = vecPhase->getAlpha();
      double density = vecPhase->getDensity();
      fluxBufferThermalEq->m_masse[k] = alpha*density*uR;
    }
    double vitY = cellRight.getMixture()->getVelocity().getY(); double vitZ = cellRight.getMixture()->getVelocity().getZ();
    double totalEnergy = cellRight.getMixture()->getEnergy() + 0.5*cellRight.getMixture()->getVelocity().squaredNorm();
    fluxBufferThermalEq->m_qdm.setX(rhoR*uR*uR + pR);
    fluxBufferThermalEq->m_qdm.setY(rhoR*vitY*uR);
    fluxBufferThermalEq->m_qdm.setZ(rhoR*vitZ*uR);
    fluxBufferThermalEq->m_energMixture = (rhoR*totalEnergy + pR)*uR;

  }
  else if (sM >= 0.){
    //Compute left solution state
    double vitY = cellLeft.getMixture()->getVelocity().getY(); double vitZ = cellLeft.getMixture()->getVelocity().getZ();
    double totalEnergy = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();
    rhoStar = mL / (sL - sM);
    EStar = totalEnergy + (sM - uL)*(sM + pL / mL);
    pStar = mL*(sM - uL) + pL;
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellLeft.getPhase(k);
      double alpha = vecPhase->getAlpha();
      double density = vecPhase->getDensity();
      mkL = alpha*density*(sL - uL);
      fluxBufferThermalEq->m_masse[k] = mkL / (sL - sM) * sM;
    }
    fluxBufferThermalEq->m_qdm.setX(rhoStar*sM*sM + pStar);
    fluxBufferThermalEq->m_qdm.setY(rhoStar*vitY*sM);
    fluxBufferThermalEq->m_qdm.setZ(rhoStar*vitZ*sM);
    fluxBufferThermalEq->m_energMixture = (rhoStar*EStar + pStar)*sM;
  }
  else{
    //Compute right solution state
    double vitY = cellRight.getMixture()->getVelocity().getY(); double vitZ = cellRight.getMixture()->getVelocity().getZ();
    double totalEnergy = cellRight.getMixture()->getEnergy() + 0.5*cellRight.getMixture()->getVelocity().squaredNorm();
    rhoStar = mR / (sR - sM);
    EStar = totalEnergy + (sM - uR)*(sM + pR / mR);
    pStar = mR*(sM - uR) + pR;
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellRight.getPhase(k);
      double alpha = vecPhase->getAlpha();
      double density = vecPhase->getDensity();
      mkR = alpha*density*(sR - uR);
      fluxBufferThermalEq->m_masse[k] = mkR / (sR - sM) * sM;
    }
    fluxBufferThermalEq->m_qdm.setX(rhoStar*sM*sM + pStar);
    fluxBufferThermalEq->m_qdm.setY(rhoStar*vitY*sM);
    fluxBufferThermalEq->m_qdm.setZ(rhoStar*vitZ*sM);
    fluxBufferThermalEq->m_energMixture = (rhoStar*EStar + pStar)*sM;
  }

  //Contact discontinuity velocity
  fluxBufferThermalEq->m_sM = sM;
}

//****************************************************************************
//************** Half Riemann solvers for boundary conditions ****************
//****************************************************************************

void ModThermalEq::solveRiemannWall(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax) const
{
  double sL;
  double pStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getMixSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();

  sL = min(uL - cL, -uL - cL);
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));

  pStar = rhoL*(uL - sL)*uL + pL;

  for (int k = 0; k < numberPhases; k++)
  {
    fluxBufferThermalEq->m_masse[k] = 0.;
  }
  fluxBufferThermalEq->m_qdm.setX(pStar);
  fluxBufferThermalEq->m_qdm.setY(0.);
  fluxBufferThermalEq->m_qdm.setZ(0.);
  fluxBufferThermalEq->m_energMixture = 0.;

  //Contact discontinuity velocity
  fluxBufferThermalEq->m_sM = 0.;
}

//****************************************************************************

void ModThermalEq::solveRiemannTank(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double *ak0, double *rhok0, double &p0, double &T0)
{
  double sL, zL, sM, vmv0, mL;
  double pStar(0.), uStar(0.), rhoStar(0.), uyStar(0.), uzStar(0.), EStar(0.), vStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getMixSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity(), TL = cellLeft.getMixture()->getTemperature();
  double uyL = cellLeft.getMixture()->getVelocity().getY(), uzL = cellLeft.getMixture()->getVelocity().getZ(), EL = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();

  zL = rhoL*cL;
  for (int k = 0; k < numberPhases; k++) {
    TB->Yk[k] = cellLeft.getPhase(k)->getAlpha()*cellLeft.getPhase(k)->getDensity() / rhoL;
  }

  //1) Left wave velocity estimation using pStar = p0
  //-------------------------------------------------
  pStar = p0;
  vStar = cellLeft.getMixture()->computeVolumeIsentrope(TB->Yk, pL, TL, pStar, numberPhases);
  vmv0 = vStar - 1. / rhoL;
  if (abs(vmv0) > 1e-10) { mL = sqrt((pL - pStar) / vmv0); }
  else { mL = zL; }
  sL = uL - mL / rhoL;
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  sM = uL + mL * vmv0;

  //2) Check for pathologic cases
  //-----------------------------
  if (sL >= 0.) { //supersonic outflow => left state solution
    uStar = uL;
    pStar = pL;
    for (int k = 0; k < numberPhases; k++) {
      TB->YkStar[k] = cellLeft.getPhase(k)->getAlpha()*cellLeft.getPhase(k)->getDensity() / rhoL;
    }
    rhoStar = rhoL;
    uyStar = uyL;
    uzStar = uzL;
    EStar = EL;
  }
  else if (sM >= -1e-3) { //subsonic outflow => star left state solution
    uStar = sM;
    pStar = p0; //approximation
    for (int k = 0; k < numberPhases; k++) {
      TB->YkStar[k] = cellLeft.getPhase(k)->getAlpha()*cellLeft.getPhase(k)->getDensity() / rhoL;
    }
    rhoStar = 1. / vStar;
    uyStar = uyL;
    uzStar = uzL;
    EStar = EL + (uStar - uL)*(uStar + pL / mL);
  }

  //3) Tank
  //-------
  else { //tank inflow => star right state solution
    //Total enthalpy and entropy in tank state
    double H0(0.);
    double rho0 = cellLeft.getMixture()->computeDensity(ak0, rhok0, numberPhases);
    for (int k = 0;k < numberPhases;k++) {
      TB->Yk0[k] = ak0[k] * rhok0[k] / rho0;
      H0 += TB->Yk0[k] * TB->eos[k]->computeTotalEnthalpy(rhok0[k], p0, 0.);  //default zero velocity in tank
    }
    //ITERATIVE PROCESS FOR PRESSURE DETERMINATION
    //--------------------------------------------
    int iteration(0);
    double p(0.5*p0);
    double f(0.), df(1.);
    double dmL;
    double uStarR(0.), duStarR(0.), uStarL(0.), duStarL(0.);
    double TStarL(0.), TStarR(0.), dTStarL(0.), dTStarR(0.);
    double hStarR(0.), dhStarR(0.);
    double vStarL(0.), dvStarL(0.);
    do {
      p -= f / df; iteration++;
      if (iteration > 50) Errors::errorMessage("solveRiemannTank not converged in ModThermalEq");
      //Physical pressure ?
      for (int k = 0; k < numberPhases; k++) { TB->eos[k]->verifyAndModifyPressure(p); }
      if (p > p0) { p = p0 - 1e-6; }
      //R) Tank rekations in the right (H=cte et s=cste)
      //mixture entropy constant brings the relation between Tr* andd p*
      TStarR = cellLeft.getMixture()->computeTemperatureIsentrope(TB->Yk0, p0, T0, p, numberPhases, &TStarR);
      hStarR = cellLeft.getMixture()->computeEnthalpyIsentrope(TB->Yk0, p0, T0, p, numberPhases, &dhStarR);
      uStarR = -sqrt(2.*(H0 - hStarR));
      duStarR = -dhStarR / uStarR;
      //L) Left relations s=cste (could be R-H if necessary)
      TStarL = cellLeft.getMixture()->computeTemperatureIsentrope(TB->Yk, pL, TL, p, numberPhases, &dTStarL);
      vStarL = cellLeft.getMixture()->computeVolumeIsentrope(TB->Yk, pL, TL, p, numberPhases, &dvStarL);
      vmv0 = vStarL - 1. / rhoL;
      if (abs(vmv0) > 1e-10) {
        mL = sqrt((pL - p) / vmv0);
        dmL = 0.5*(-vmv0 + (p - pL)*dvStarL) / (vmv0*vmv0) / mL;
      }
      else {
        mL = zL;
        dmL = 0.;
      }
      sL = uL - mL / rhoL;
      if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
      uStarL = uL + mL*vmv0;
      duStarL = dmL*vmv0 + mL* dvStarL;
      //solved function
      f = uStarR - uStarL;
      df = duStarR - duStarL;
    } while (abs(f)>1e-3); //End iterative loop
    pStar = p;
    uStar = 0.5*(uStarL + uStarR);
    rhoStar = 0.;
    for (int k = 0; k < numberPhases; k++) {
      TB->YkStar[k] = TB->Yk0[k];
      rhoStar += TB->YkStar[k] / TB->eos[k]->computeDensity(pStar,TStarR);
    }
    rhoStar = 1. / rhoStar;
    uyStar = 0.;
    uzStar = 0.;
    EStar = H0 - pStar/rhoStar;
  } //End tank case

  //4) Flux completion
  //------------------
  for (int k = 0; k < numberPhases; k++) {
    fluxBufferThermalEq->m_masse[k] = rhoStar* TB->YkStar[k] * uStar;
  }
  fluxBufferThermalEq->m_qdm.setX(rhoStar*uStar*uStar + pStar);
  fluxBufferThermalEq->m_qdm.setY(rhoStar*uStar*uyStar);
  fluxBufferThermalEq->m_qdm.setZ(rhoStar*uStar*uzStar);
  fluxBufferThermalEq->m_energMixture = (rhoStar*EStar + pStar)*uStar;

  //Contact discontinuity velocity
  fluxBufferThermalEq->m_sM = sM;
}

//****************************************************************************

void ModThermalEq::solveRiemannOutflow(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double p0, double *debitSurf)
{
  double sL, zL;
  double pStar(p0);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getMixSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity(), TL = cellLeft.getMixture()->getTemperature();;
  double vL = cellLeft.getMixture()->getVelocity().getY(), wL = cellLeft.getMixture()->getVelocity().getZ();

  for (int k = 0; k < numberPhases; k++) {
    TB->Yk[k] = cellLeft.getPhase(k)->getAlpha()*cellLeft.getPhase(k)->getDensity() / rhoL;
  }

  //Left wave acoustic estimation
  //-----------------------------
  zL = rhoL*cL;
  double rhoStar(0.), vmv0, mL, uStar;
  rhoStar = 1./cellLeft.getMixture()->computeVolumeIsentrope(TB->Yk, pL, TL, p0, numberPhases);
  vmv0 = 1./ rhoStar - 1. / rhoL;
  if (abs(vmv0) > 1e-10) {
    mL = sqrt((pL - p0) / vmv0);
  }
  else {
    mL = zL;
  }
  sL = uL - mL / rhoL;
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  uStar = uL + mL*vmv0;

  //Pathologic case sL>0
  if (sL >= 0.) { //Supersonic outflow => Left state solution
    uStar = uL;
    pStar = pL;
    rhoStar = rhoL;
  }

  //Flux completion
  double totalEnergy = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();
  double EStar(totalEnergy + (uStar - uL)*(uStar - pL / mL));
  for (int k = 0; k < numberPhases; k++) {
    fluxBufferThermalEq->m_masse[k] = rhoStar * TB->Yk[k] * uStar ;
  }
  fluxBufferThermalEq->m_qdm.setX(uStar*uStar*rhoStar + pStar);
  fluxBufferThermalEq->m_qdm.setY(uStar*vL*rhoStar);
  fluxBufferThermalEq->m_qdm.setZ(uStar*wL*rhoStar);
  fluxBufferThermalEq->m_energMixture = (EStar*rhoStar + pStar)*uStar;

  //Contact discontinuity velocity
  fluxBufferThermalEq->m_sM = uStar;

  //Specific mass flow rate output (kg/s/m�)
  for (int k = 0; k < numberPhases; k++) {
    debitSurf[k] = fluxBufferThermalEq->m_masse[k];
  }
}

//****************************************************************************

double ModThermalEq::getSM()
{
  return fluxBufferThermalEq->m_sM;
}

//****************************************************************************

Coord ModThermalEq::getVelocity(Cell *cell) const
{
  return cell->getMixture()->getVelocity();
}

//****************************************************************************
//***************************** others methods *******************************
//****************************************************************************

void ModThermalEq::reverseProjection(const Coord normal, const Coord tangent, const Coord binormal) const
{
  Coord fluxProjete;
  fluxProjete.setX(normal.getX()*fluxBufferThermalEq->m_qdm.getX() + tangent.getX()*fluxBufferThermalEq->m_qdm.getY() + binormal.getX()*fluxBufferThermalEq->m_qdm.getZ());
  fluxProjete.setY(normal.getY()*fluxBufferThermalEq->m_qdm.getX() + tangent.getY()*fluxBufferThermalEq->m_qdm.getY() + binormal.getY()*fluxBufferThermalEq->m_qdm.getZ());
  fluxProjete.setZ(normal.getZ()*fluxBufferThermalEq->m_qdm.getX() + tangent.getZ()*fluxBufferThermalEq->m_qdm.getY() + binormal.getZ()*fluxBufferThermalEq->m_qdm.getZ());
  fluxBufferThermalEq->m_qdm.setXYZ(fluxProjete.getX(), fluxProjete.getY(), fluxProjete.getZ());
}

//****************************************************************************

string ModThermalEq::whoAmI() const
{
  return m_name;
}

//****************************************************************************

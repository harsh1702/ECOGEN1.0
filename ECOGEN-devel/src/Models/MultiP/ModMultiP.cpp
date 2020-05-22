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

//! \file      ModMultiP.cpp
//! \author    F. Petitpas
//! \version   1.0
//! \date      June 5 2017

#include <iostream>
#include <cmath>
#include <algorithm>
#include "ModMultiP.h"
#include "PhaseMultiP.h"

using namespace std;

const std::string ModMultiP::NAME = "MULTIP";

//***********************************************************************

ModMultiP::ModMultiP(int &numberTransports, const int &numberPhases) :
  Model(NAME,numberTransports)
{
  fluxBufferMultiP = new FluxMultiP(this,numberPhases);
  sourceConsMultiP = new FluxMultiP(this, numberPhases);
}

//***********************************************************************

ModMultiP::~ModMultiP()
{
  delete fluxBufferMultiP;
  delete sourceConsMultiP;
}

//***********************************************************************

void ModMultiP::allocateCons(Flux **cons, const int &numberPhases)
{
  *cons = new FluxMultiP(this,numberPhases);
}

//***********************************************************************

void ModMultiP::allocatePhase(Phase **phase)
{
  *phase = new PhaseMultiP;
}

//***********************************************************************

void ModMultiP::allocateMixture(Mixture **mixture)
{
  *mixture = new MixMultiP;
}

//***********************************************************************

void ModMultiP::fulfillState(Phase **phases, Mixture *mixture, const int &numberPhases, Prim type)
{
  //Complete phases state
  for (int k = 0; k < numberPhases; k++) {
    phases[k]->extendedCalculusPhase(mixture->getVelocity());
  }
  //Complete mixture variables using phases variable
  mixture->computeMixtureVariables(phases, numberPhases);
}

//***********************************************************************

//****************************************************************************
//********************* Cell to cell Riemann solvers *************************
//****************************************************************************

void ModMultiP::solveRiemannIntern(Cell &cellLeft, Cell &cellRight, const int &numberPhases, const double &dxLeft, const double &dxRight, double &dtMax) const
{
  Phase *vecPhase;
  double sL, sR;
  double pStar(0.), rhoStar(0.), uStar(0.), vStar(0.), wStar(0.), EStar(0.), eStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getFrozenSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();
  double uR = cellRight.getMixture()->getVelocity().getX(), cR = cellRight.getMixture()->getFrozenSoundSpeed(), pR = cellRight.getMixture()->getPressure(), rhoR = cellRight.getMixture()->getDensity();

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
      double energie = vecPhase->getEnergy();
      fluxBufferMultiP->m_alpha[k] = alpha*sM;
      fluxBufferMultiP->m_masse[k] = alpha*density*uL;
      fluxBufferMultiP->m_energ[k] = alpha*density*energie*uL;
    }
    double vitY = cellLeft.getMixture()->getVelocity().getY(); double vitZ = cellLeft.getMixture()->getVelocity().getZ();
    double totalEnergy = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();
    fluxBufferMultiP->m_qdm.setX(rhoL*uL*uL + pL);
    fluxBufferMultiP->m_qdm.setY(rhoL*vitY*uL);
    fluxBufferMultiP->m_qdm.setZ(rhoL*vitZ*uL);
    fluxBufferMultiP->m_energMixture = (rhoL*totalEnergy + pL)*uL;

  }
  else if (sR <= 0.){
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellRight.getPhase(k);
      double alpha = vecPhase->getAlpha();
      double density = vecPhase->getDensity();
      double energie = vecPhase->getEnergy();
      fluxBufferMultiP->m_alpha[k] = alpha*sM;
      fluxBufferMultiP->m_masse[k] = alpha*density*uR;
      fluxBufferMultiP->m_energ[k] = alpha*density*energie*uR;
    }
    double vitY = cellRight.getMixture()->getVelocity().getY(); double vitZ = cellRight.getMixture()->getVelocity().getZ();
    double totalEnergy = cellRight.getMixture()->getEnergy() + 0.5*cellRight.getMixture()->getVelocity().squaredNorm();
    fluxBufferMultiP->m_qdm.setX(rhoR*uR*uR + pR);
    fluxBufferMultiP->m_qdm.setY(rhoR*vitY*uR);
    fluxBufferMultiP->m_qdm.setZ(rhoR*vitZ*uR);
    fluxBufferMultiP->m_energMixture = (rhoR*totalEnergy + pR)*uR;

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
      double pressure = vecPhase->getPressure();
      totalEnergy = vecPhase->getTotalEnergy();
      mkL = density*(sL - uL);
      TB->rhokStar[k] = mkL / (sL - sM);
      TB->pkStar[k] = TB->eos[k]->computePressureIsentropic(pressure, density, TB->rhokStar[k]);
      TB->ekStar[k] = TB->eos[k]->computeEnergy(TB->rhokStar[k], TB->pkStar[k]);
      fluxBufferMultiP->m_alpha[k] = alpha*sM;
      fluxBufferMultiP->m_masse[k] = alpha* TB->rhokStar[k] * sM;
      fluxBufferMultiP->m_energ[k] = alpha* TB->rhokStar[k] * TB->ekStar[k] * sM;
    }
    fluxBufferMultiP->m_qdm.setX(rhoStar*sM*sM + pStar);
    fluxBufferMultiP->m_qdm.setY(rhoStar*vitY*sM);
    fluxBufferMultiP->m_qdm.setZ(rhoStar*vitZ*sM);
    fluxBufferMultiP->m_energMixture = (rhoStar*EStar + pStar)*sM;
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
      double pressure = vecPhase->getPressure();
      totalEnergy = vecPhase->getTotalEnergy();
      mkR = density*(sR - uR);
      TB->rhokStar[k] = mkR / (sR - sM);
      TB->pkStar[k] = TB->eos[k]->computePressureIsentropic(pressure, density, TB->rhokStar[k]);
      TB->ekStar[k] = TB->eos[k]->computeEnergy(TB->rhokStar[k], TB->pkStar[k]);
      fluxBufferMultiP->m_alpha[k] = alpha*sM;
      fluxBufferMultiP->m_masse[k] = alpha* TB->rhokStar[k] * sM;
      fluxBufferMultiP->m_energ[k] = alpha* TB->rhokStar[k] * TB->ekStar[k] * sM;
    }
    fluxBufferMultiP->m_qdm.setX(rhoStar*sM*sM + pStar);
    fluxBufferMultiP->m_qdm.setY(rhoStar*vitY*sM);
    fluxBufferMultiP->m_qdm.setZ(rhoStar*vitZ*sM);
    fluxBufferMultiP->m_energMixture = (rhoStar*EStar + pStar)*sM;
  }

  //Contact discontinuity velocity
  fluxBufferMultiP->m_sM = sM;
}

//****************************************************************************
//************** Half Riemann solvers for boundary conditions ****************
//****************************************************************************

void ModMultiP::solveRiemannWall(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax) const
{
  double sL;
  double pStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getFrozenSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();

  sL = min(uL - cL, -uL - cL);
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));

  pStar = rhoL*(uL - sL)*uL + pL;

  for (int k = 0; k < numberPhases; k++)
  {
    fluxBufferMultiP->m_alpha[k] = 0.;
    fluxBufferMultiP->m_masse[k] = 0.;
    fluxBufferMultiP->m_energ[k] = 0.;
  }
  fluxBufferMultiP->m_qdm.setX(pStar);
  fluxBufferMultiP->m_qdm.setY(0.);
  fluxBufferMultiP->m_qdm.setZ(0.);
  fluxBufferMultiP->m_energMixture = 0.;

  //Contact discontinuity velocity
  fluxBufferMultiP->m_sM = 0.;
}

//****************************************************************************

void ModMultiP::solveRiemannInflow(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, const double m0, const double *ak0, const double *rhok0, const double *pk0) const
{
  double sL, zL;
  double pStar(0.), uStar(0.), rhoStar(0.);

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getFrozenSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();
  double vL = cellLeft.getMixture()->getVelocity().getY(), wL = cellLeft.getMixture()->getVelocity().getZ();

  //Compute total enthalpy of injected fluid
  double rho0 = cellLeft.getMixture()->computeDensity(ak0, rhok0, numberPhases);
  double u0 = m0 / rho0;
  for (int k = 0;k < numberPhases;k++) {
    TB->Hk0[k] = TB->eos[k]->computeTotalEnthalpy(rhok0[k], pk0[k], u0);
    TB->Yk0[k] = ak0[k] * rhok0[k] / rho0;
  }

  //ITERATIVE PROCESS FOR PRESSURE SOLUTION DETERMINATION
  //-----------------------------------------------------
  //Estimates for acoustic wave sL
  sL = uL - cL;
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  zL = rhoL*cL;

  int iteration(0);
  pStar = pL;
  double f(0.), df(1.);
  double u, du, v, dv, hk;

  do {
    pStar -= f / df; iteration++;
    if (iteration > 50) Errors::errorMessage("solveRiemannInflow not converged in modMultiP");
    //Physical pressure ?
    for (int k = 0; k < numberPhases; k++) {
      TB->eos[k]->verifyAndModifyPressure(pStar);
    }
    //Left acoustic relations
    u = uL + (pL - pStar) / zL;
    if (u >= -1e-6) u = -1e-6;
    du = -1. / zL;
    //Compute from m0, Hk0, Yk0 on the right
    v = u / m0;
    dv = du / m0;
    f = v;
    df = dv;
    for (int k = 0; k < numberPhases; k++) {
      hk = TB->Hk0[k] - 0.5*u*u;
      TB->vkStar[k] = TB->eos[k]->vfpfh(pStar, hk);
      double dvk = TB->eos[k]->dvdpch(pStar, hk) - TB->eos[k]->dvdhcp(pStar, hk)*u*du;
      f -= TB->Yk0[k] * TB->vkStar[k];
      df -= TB->Yk0[k] * dvk;
    }
  } while (abs(f)>1e-10);

  //Flux completion
  double Estar(0.5*(u*u + vL*vL + wL*wL)), ek, rhok;
  for (int k = 0; k<numberPhases; k++) {
    rhok = 1. / TB->vkStar[k];
    ek = TB->eos[k]->computeEnergy(rhok, pStar); Estar += TB->Yk0[k] * ek;
    fluxBufferMultiP->m_alpha[k] = TB->Yk0[k] * TB->vkStar[k] / v*u;
    fluxBufferMultiP->m_masse[k] = fluxBufferMultiP->m_alpha[k] * rhok;
    fluxBufferMultiP->m_energ[k] = fluxBufferMultiP->m_alpha[k] * rhok*ek;
  }
  fluxBufferMultiP->m_qdm.setX(u*u / v + pStar);
  fluxBufferMultiP->m_qdm.setY(u*vL / v);
  fluxBufferMultiP->m_qdm.setZ(u*wL / v);
  fluxBufferMultiP->m_energMixture = (Estar / v + pStar)*u;

  //Contact discontinuity velocity
  fluxBufferMultiP->m_sM = u;
}

//****************************************************************************

void ModMultiP::solveRiemannTank(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double *ak0, double *rhok0, double &p0, double &T0)
{
  double sL, zL, sM, vmv0, mL;
  double pStar(0.), uStar(0.), rhoStar(0.), vStar(0.), uyStar(0.), uzStar(0.);
  Phase *vecPhase;

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getFrozenSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();
  double uyL = cellLeft.getMixture()->getVelocity().getY(), uzL = cellLeft.getMixture()->getVelocity().getZ();

  zL = rhoL*cL;

  //1) Left wave velocity estimation using pStar = p0
  //-------------------------------------------------
  pStar = p0; vStar = 0.;
  for (int k = 0; k < numberPhases; k++) {
    vecPhase = cellLeft.getPhase(k);
    //TB->rhokStar[k] = TB->eos[k]->computeDensityIsentropic(vecPhase->getPressure(), vecPhase->getDensity(), pStar); //other possiblity
    TB->rhokStar[k] = TB->eos[k]->computeDensityHugoniot(vecPhase->getPressure(), vecPhase->getDensity(), pStar);
    vStar += vecPhase->getAlpha()*vecPhase->getDensity() / rhoL / TB->rhokStar[k];
  }
  vmv0 = vStar - 1. / rhoL;
  if (abs(vmv0) > 1e-10) { mL = sqrt((pL - pStar) / vmv0); }
  else { mL = zL; }
  sL = uL - mL / rhoL;
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  sM = uL + mL*vmv0;

  //2) Check for pathologic cases
  //-----------------------------
  if (sL >= 0.) { //supersonic outflow => left state solution
    uStar = uL;
    pStar = pL;
    for (int k = 0; k < numberPhases; k++) {
      vecPhase = cellLeft.getPhase(k);
      TB->rhokStar[k] = vecPhase->getDensity();
      TB->YkStar[k] = vecPhase->getAlpha()*vecPhase->getDensity() / rhoL;
    }
    rhoStar = rhoL;
    uyStar = uyL;
    uzStar = uzL;
  }
  else if (sM >= -1e-3) { //subsonic outflow => star left state solution
    uStar = sM;
    pStar = p0;  //approximation
    for (int k = 0; k < numberPhases; k++) {
      // TB->rhokStar[k] unchanged : see 1)
      TB->YkStar[k] = cellLeft.getPhase(k)->getAlpha()*cellLeft.getPhase(k)->getDensity() / rhoL;
    }
    rhoStar = 1. / vStar;
    uyStar = uyL;
    uzStar = uzL;
  }

  //3) Tank
  //-------
  else { //tank inflow => star right state solution
    //Total enthalpy in tank state
    double H0(0.), v0(0.);
    double rho0 = cellLeft.getMixture()->computeDensity(ak0, rhok0, numberPhases);
    for (int k = 0;k < numberPhases;k++) {
      TB->Yk0[k] = ak0[k] * rhok0[k] / rho0;
      H0 += TB->Yk0[k] * TB->eos[k]->computeTotalEnthalpy(rhok0[k], p0, v0);  //default zero velocity in tank
    }
    //ITERATIVE PROCESS FOR PRESSURE DETERMINATION
    //--------------------------------------------
    int iteration(0);
    double p(0.5*p0);
    double f(0.), df(1.);
    double hk, dhk, rhok, drhok, dmL, YkL;
    double uStarR(0.), duStarR(0.), uStarL(0.), duStarL(0.);
    double vStarL(0.), dvStarL(0.);
    do {
      p -= f / df; iteration++;
      if (iteration > 50) Errors::errorMessage("solveRiemannTank not converged in modMultiP");
      //Physical pressure ?
      for (int k = 0; k < numberPhases; k++) { TB->eos[k]->verifyAndModifyPressure(p); }
      if (p > p0) { p = p0 - 1e-6; }
      //R) Tank rekations in the right (H=cte et sk=cste)
      uStarR = H0; duStarR = 0.;
      for (int k = 0; k < numberPhases; k++) {
        TB->rhokStar[k] = TB->eos[k]->computeDensityIsentropic(p0, rhok0[k], p);
        hk = TB->eos[k]->computeEnthalpyIsentropic(p0, rhok0[k], p, &dhk);
        uStarR -= TB->Yk0[k] * hk;
        duStarR -= TB->Yk0[k] * dhk;
      }
      uStarR = -sqrt(2.*uStarR);
      duStarR = duStarR / uStarR;
      //L) Left relations sk=cste (could be R-H if needed)
      vStarL = 0.; dvStarL = 0.;
      for (int k = 0; k < numberPhases; k++) {
        vecPhase = cellLeft.getPhase(k);
        //rhok = TB->eos[k]->computeDensityIsentropic(vecPhase->getPressure(), vecPhase->getDensity(), p, &drhok); //other possiblity
        rhok = TB->eos[k]->computeDensityHugoniot(vecPhase->getPressure(), vecPhase->getDensity(), p, &drhok);
        YkL = vecPhase->getAlpha()*vecPhase->getDensity() / rhoL;
        vStarL += YkL / rhok;
        dvStarL -= YkL / (rhok * rhok) * drhok;
      }
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
      duStarL = dmL*vmv0 + mL*dvStarL;
      //solved function
      f = uStarR - uStarL;
      df = duStarR - duStarL;
    } while (abs(f)>1e-3); //End iterative loop
    pStar = p;
    uStar = 0.5*(uStarL + uStarR);
    rhoStar = 0.;
    for (int k = 0; k < numberPhases; k++) {
      TB->YkStar[k] = TB->Yk0[k];
      rhoStar += TB->YkStar[k] / TB->rhokStar[k];
    }
    rhoStar = 1. / rhoStar;
    uyStar = 0.;
    uzStar = 0.;
  } //End tank case

  //4) Flux completion
  //------------------
  double EStar(0.5*(uStar*uStar + uyStar*uyStar + uzStar*uzStar)), ek;
  for (int k = 0; k < numberPhases; k++) {
    ek = TB->eos[k]->computeEnergy(TB->rhokStar[k], pStar); EStar += TB->YkStar[k] * ek;
    fluxBufferMultiP->m_alpha[k] = TB->YkStar[k] * rhoStar / TB->rhokStar[k] * uStar;
    fluxBufferMultiP->m_masse[k] = fluxBufferMultiP->m_alpha[k] * TB->rhokStar[k];
    fluxBufferMultiP->m_energ[k] = fluxBufferMultiP->m_masse[k] * ek;
  }
  fluxBufferMultiP->m_qdm.setX(rhoStar*uStar*uStar + pStar);
  fluxBufferMultiP->m_qdm.setY(rhoStar*uStar*uyStar);
  fluxBufferMultiP->m_qdm.setZ(rhoStar*uStar*uzStar);
  fluxBufferMultiP->m_energMixture = (rhoStar*EStar + pStar)*uStar;

  //Contact discontinuity velocity
  fluxBufferMultiP->m_sM = sM;
}

//****************************************************************************

void ModMultiP::solveRiemannOutflow(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double p0, double *debitSurf)
{
  double sL, sM, zL;
  double pStar(p0), EStar(0.), vStar(0.), uStar(0.);
  Phase *vecPhase;

  double uL = cellLeft.getMixture()->getVelocity().getX(), cL = cellLeft.getMixture()->getFrozenSoundSpeed(), pL = cellLeft.getMixture()->getPressure(), rhoL = cellLeft.getMixture()->getDensity();
  double uyL = cellLeft.getMixture()->getVelocity().getY(), uzL = cellLeft.getMixture()->getVelocity().getZ(), EL = cellLeft.getMixture()->getEnergy() + 0.5*cellLeft.getMixture()->getVelocity().squaredNorm();

  zL = rhoL*cL;

  //Left wave : isentropic wave assumption
  //--------------------------------------
  double vSmvL, mL;
  for (int k = 0; k < numberPhases; k++) {
    vecPhase = cellLeft.getPhase(k);
    //TB->rhokStar[k] = TB->eos[k]->computeDensityIsentropic(pL, vecPhase->getDensity(), pStar); //other possiblity
    TB->rhokStar[k] = TB->eos[k]->computeDensityHugoniot(pL, vecPhase->getDensity(), pStar);
    vStar += vecPhase->getAlpha()*vecPhase->getDensity() /rhoL / TB->rhokStar[k];
  }
  vSmvL = vStar - 1. / rhoL;
  if (abs(vSmvL) > 1e-10) { mL = sqrt((pL - pStar) / vSmvL); }
//  else { cout << "bug" << endl;  mL = zL; }
  sL = uL - mL / rhoL;
  if (abs(sL)>1.e-3) dtMax = min(dtMax, dxLeft / abs(sL));
  sM = uL + mL*vSmvL;

  //Pathologic case sL>0
  if (sL >= 0.) { //Supersonic outflow => Left state solution
    uStar = uL;
    pStar = pL;
    for (int k = 0; k < numberPhases; k++) { TB->rhokStar[k] = cellLeft.getPhase(k)->getDensity(); }
    vStar = 1. / rhoL;
    EStar = EL;
  }
  else if (sM < 0) { //Inflow conditions : the outflow assumption is not adapted
    uStar = uL;
    pStar = pL;
    for (int k = 0; k < numberPhases; k++) { TB->rhokStar[k] = cellLeft.getPhase(k)->getDensity(); }
    vStar = 1. / rhoL;
    EStar = EL;
  }
  else { //imposed pressure outflow OK
    uStar = sM;
    EStar = EL + (uStar - uL)*(uStar - pL / mL);
  }

  //Flux completion
  double ekStar;
  for (int k = 0; k < numberPhases; k++) {
    vecPhase = cellLeft.getPhase(k);
    double YkL = vecPhase->getAlpha()*vecPhase->getDensity() / rhoL;
    ekStar = TB->eos[k]->computeEnergy(TB->rhokStar[k], pStar);
    fluxBufferMultiP->m_alpha[k] = YkL / TB->rhokStar[k] / vStar * uStar;
    fluxBufferMultiP->m_masse[k] = fluxBufferMultiP->m_alpha[k] * TB->rhokStar[k];
    fluxBufferMultiP->m_energ[k] = fluxBufferMultiP->m_masse[k] * ekStar;
  }
  fluxBufferMultiP->m_qdm.setX(uStar*uStar / vStar + pStar);
  fluxBufferMultiP->m_qdm.setY(uStar*uyL / vStar);
  fluxBufferMultiP->m_qdm.setZ(uStar*uzL / vStar);
  fluxBufferMultiP->m_energMixture = (EStar / vStar + pStar)*uStar;

  //Contact discontinuity velocity
  fluxBufferMultiP->m_sM = uStar;

  //Specific mass flow rate output (kg/s/m�)
  for (int k = 0; k < numberPhases; k++) {
    debitSurf[k] = fluxBufferMultiP->m_masse[k];
  }
}

//****************************************************************************
//********************** Transport Riemann solvers ***************************
//****************************************************************************

void ModMultiP::solveRiemannTransportIntern(Cell &cellLeft, Cell &cellRight, const int &numberTransports)
{
	for (int k = 0; k < numberTransports; k++) {
		fluxBufferTransport[k].solveRiemann(cellLeft.getTransport(k).getValue(), cellRight.getTransport(k).getValue(), fluxBufferMultiP->m_sM);
	}
}

//****************************************************************************

void ModMultiP::solveRiemannTransportWall(const int &numberTransports)
{
	for (int k = 0; k < numberTransports; k++) {
    fluxBufferTransport[k].solveRiemannWall();
	}
}

//****************************************************************************

void ModMultiP::solveRiemannTransportInflow(Cell &cellLeft, const int &numberTransports, double *valueTransports)
{
	for (int k = 0; k < numberTransports; k++) {
    fluxBufferTransport[k].solveRiemannInflow(cellLeft.getTransport(k).getValue(), fluxBufferMultiP->m_sM, valueTransports[k]);
	}
}

//****************************************************************************

void ModMultiP::solveRiemannTransportTank(Cell &cellLeft, const int &numberTransports, double *valueTransports)
{
	for (int k = 0; k < numberTransports; k++) {
    fluxBufferTransport[k].solveRiemannTank(cellLeft.getTransport(k).getValue(), fluxBufferMultiP->m_sM, valueTransports[k]);
	}
}

//****************************************************************************

void ModMultiP::solveRiemannTransportOutflow(Cell &cellLeft, const int &numberTransports, double *valueTransports)
{
	for (int k = 0; k < numberTransports; k++) {
    fluxBufferTransport[k].solveRiemannOutflow(cellLeft.getTransport(k).getValue(), fluxBufferMultiP->m_sM, valueTransports[k]);
	}
}

//****************************************************************************

double ModMultiP::getSM()
{
  return fluxBufferMultiP->m_sM;
}

//****************************************************************************

Coord ModMultiP::getVelocity(Cell *cell) const
{
  return cell->getMixture()->getVelocity();
}

//****************************************************************************
//***************************** others methods *******************************
//****************************************************************************

void ModMultiP::reverseProjection(const Coord normal, const Coord tangent, const Coord binormal) const
{
  Coord fluxProjete;
  fluxProjete.setX(normal.getX()*fluxBufferMultiP->m_qdm.getX() + tangent.getX()*fluxBufferMultiP->m_qdm.getY() + binormal.getX()*fluxBufferMultiP->m_qdm.getZ());
  fluxProjete.setY(normal.getY()*fluxBufferMultiP->m_qdm.getX() + tangent.getY()*fluxBufferMultiP->m_qdm.getY() + binormal.getY()*fluxBufferMultiP->m_qdm.getZ());
  fluxProjete.setZ(normal.getZ()*fluxBufferMultiP->m_qdm.getX() + tangent.getZ()*fluxBufferMultiP->m_qdm.getY() + binormal.getZ()*fluxBufferMultiP->m_qdm.getZ());
  fluxBufferMultiP->m_qdm.setXYZ(fluxProjete.getX(), fluxProjete.getY(), fluxProjete.getZ());
}

//****************************************************************************

string ModMultiP::whoAmI() const
{
  return m_name;
}

//****************************************************************************

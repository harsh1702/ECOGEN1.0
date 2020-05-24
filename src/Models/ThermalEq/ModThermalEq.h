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

#ifndef MODTHERMALEQ_H
#define MODTHERMALEQ_H

//! \file      ModThermalEq.h
//! \author    F. Petitpas
//! \version   1.0
//! \date      May 04 2018

#include "../Model.h"
#include "../../Cell.h"
#include "FluxThermalEq.h"
#include "MixThermalEq.h"

//! \class     ModThermalEq
//! \brief     Model class for mechanical and thermal equilibrium multiphase flows
class ModThermalEq : public Model
{
  public:
    //! \brief     Thermal equilibrium model constructor
    //! \param     numberTransports    number of additional transport equations
    //! \param     numberPhases        number of phases
    ModThermalEq(int &numberTransports, const int &numberPhases);
    virtual ~ModThermalEq();

    virtual void allocateCons(Flux **cons, const int &numberPhases);
    virtual void allocatePhase(Phase **phase);
    virtual void allocateMixture(Mixture **mixture);

    virtual void fulfillState(Phase **phases, Mixture *mixture, const int &numberPhases, Prim type = vecPhases);

    //Hydrodynamic Riemann solvers
    //----------------------------
    virtual void solveRiemannIntern(Cell &cellLeft, Cell &cellRight, const int &numberPhases, const double &dxLeft, const double &dxRight, double &dtMax) const; // Riemann between two computed cells
    virtual void solveRiemannWall(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax) const; // Riemann between left cell and wall
    virtual void solveRiemannTank(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double *ak0, double *rhok0, double &p0, double &T0) ; // Riemann for tank
    virtual void solveRiemannOutflow(Cell &cellLeft, const int &numberPhases, const double &dxLeft, double &dtMax, double p0, double *debitSurf) ; // Riemann for outflow with imposed pressure

    virtual void reverseProjection(const Coord normal, const Coord tangent, const Coord binormal) const;

    //Accessors
    //---------
    virtual double getSM();
    virtual Coord getVelocity(Cell *cell) const;

    virtual std::string whoAmI() const;

  protected:

  private:
    static const std::string NAME;
};

#endif // MODTHERMALEQ_H

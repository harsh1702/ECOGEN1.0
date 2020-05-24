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

//! \file      SourceHeating.cpp
//! \author    F. Petitpas
//! \version   1.0
//! \date      January 10 2018

#include "SourceHeating.h"

using namespace std;
using namespace tinyxml2;

//***********************************************************************

SourceHeating::SourceHeating(){}

//***********************************************************************

SourceHeating::SourceHeating(XMLElement *element, string fileName)
{
  XMLElement *sousElement(element->FirstChildElement("dataHeating"));
  if (sousElement == NULL) throw ErrorXMLElement("dataHeating", fileName, __FILE__, __LINE__);
  //Collecting attributes
  //---------------------
  XMLError error;
  //Specific heat flux value
  error = sousElement->QueryDoubleAttribute("volumeHeatPower", &m_q);
  if (error != XML_NO_ERROR) throw ErrorXMLAttribut("volumeHeatPower", fileName, __FILE__, __LINE__);
}

//***********************************************************************

SourceHeating::~SourceHeating()
{}

//***********************************************************************

void SourceHeating::integrateSourceTerms(Cell *cell, const int &numberPhases, const double &dt)
{
  //Source terms integration on conservative quantities
  cell->buildCons(numberPhases);
  cell->getCons()->integrateSourceTermsHeating(cell, dt, numberPhases, m_q);
  cell->buildPrim(numberPhases);
}

//***********************************************************************
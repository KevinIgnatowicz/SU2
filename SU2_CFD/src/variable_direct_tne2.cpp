﻿/*!
 * \file variable_direct_tne2.cpp
 * \brief Definition of the solution fields.
 * \author F. Palacios, T. Economon
 * \version 6.1.0 "Falcon"
 *
 * The current SU2 release has been coordinated by the
 * SU2 International Developers Society <www.su2devsociety.org>
 * with selected contributions from the open-source community.
 *
 * The main research teams contributing to the current release are:
 *  - Prof. Juan J. Alonso's group at Stanford University.
 *  - Prof. Piero Colonna's group at Delft University of Technology.
 *  - Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *  - Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *  - Prof. Rafael Palacios' group at Imperial College London.
 *  - Prof. Vincent Terrapon's group at the University of Liege.
 *  - Prof. Edwin van der Weide's group at the University of Twente.
 *  - Lab. of New Concepts in Aeronautics at Tech. Institute of Aeronautics.
 *
 * Copyright 2012-2018, Francisco D. Palacios, Thomas D. Economon,
 *                      Tim Albring, and the SU2 contributors.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/variable_structure.hpp"
#include <math.h>

CTNE2EulerVariable::CTNE2EulerVariable(void) : CVariable() {

  /*--- Array initialization ---*/
  Primitive          = NULL;
  //Secondary          = NULL;

  Gradient_Primitive = NULL;
  //Gradient_Secondary = NULL;

  Limiter_Primitive  = NULL;
  Limiter            = NULL;
  //Limiter_Secondary  = NULL;

  nPrimVar           = 0;
  nPrimVarGrad       = 0;

  //nSecondaryVar      = 0;
  //nSecondaryVarGrad  = 0;

  //Undivided_Laplacian = NULL;
  //Solution_New        = NULL;

  dPdU   = NULL;   dTdU   = NULL;
  dTvedU = NULL;   eves   = NULL;
  Cvves  = NULL;

  /*--- Define structure of the primtive variable vector ---*/
  // Primitive: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T
  // GradPrim:  [rho1, ..., rhoNs, T, Tve, u, v, w, P]^T
  RHOS_INDEX    = 0;
  T_INDEX       = nSpecies;
  TVE_INDEX     = nSpecies+1;
  VEL_INDEX     = nSpecies+2;
  P_INDEX       = nSpecies+nDim+2;
  RHO_INDEX     = nSpecies+nDim+3;
  H_INDEX       = nSpecies+nDim+4;
  A_INDEX       = nSpecies+nDim+5;
  RHOCVTR_INDEX = nSpecies+nDim+6;
  RHOCVVE_INDEX = nSpecies+nDim+7;

}

CTNE2EulerVariable::CTNE2EulerVariable(unsigned short val_ndim,
                                       unsigned short val_nvar,
                                       unsigned short val_nprimvar,
                                       unsigned short val_nprimvargrad,
                                       CConfig *config) : CVariable(val_ndim,
                                                                    val_nvar,
                                                                    config) {

  nDim         = val_ndim;
  nVar         = val_nvar;

  nPrimVar     = val_nprimvar;
  nPrimVarGrad = val_nprimvargrad;

  nSpecies     = config->GetnSpecies();
  ionization   = config->GetIonization();

  /*--- Array initialization ---*/
  Primitive          = NULL;
  //Secondary          = NULL;

  Gradient_Primitive = NULL;
  //Gradient_Secondary = NULL;

  Limiter_Primitive  = NULL;
  Limiter            = NULL;
  //Limiter_Secondary  = NULL;

  //Solution_New       = NULL;

  dPdU   = NULL;   dTdU   = NULL;
  dTvedU = NULL;   eves   = NULL;
  Cvves  = NULL;

  /*--- Define structure of the primtive variable vector ---*/
  // Primitive: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T
  // GradPrim:  [rho1, ..., rhoNs, T, Tve, u, v, w, P]^T
  RHOS_INDEX    = 0;
  T_INDEX       = nSpecies;
  TVE_INDEX     = nSpecies+1;
  VEL_INDEX     = nSpecies+2;
  P_INDEX       = nSpecies+nDim+2;
  RHO_INDEX     = nSpecies+nDim+3;
  H_INDEX       = nSpecies+nDim+4;
  A_INDEX       = nSpecies+nDim+5;
  RHOCVTR_INDEX = nSpecies+nDim+6;
  RHOCVVE_INDEX = nSpecies+nDim+7;

}

CTNE2EulerVariable::CTNE2EulerVariable(su2double val_pressure,
                                       su2double *val_massfrac,
                                       su2double *val_mach,
                                       su2double val_temperature,
                                       su2double val_temperature_ve,
                                       unsigned short val_ndim,
                                       unsigned short val_nvar,
                                       unsigned short val_nvarprim,
                                       unsigned short val_nvarprimgrad,
                                       CConfig *config) : CVariable(val_ndim,
                                                                    val_nvar,
                                                                    config   ) {

  unsigned short iEl, iMesh, iDim, iSpecies, iVar, nDim, nEl, nHeavy, nMGSmooth;
  unsigned short *nElStates;
  su2double *xi, *Ms, *thetav, **thetae, **g, *hf, *Tref;
  su2double rhoE, rhoEve, Ev, Ee, Ef, T, Tve, rho, rhoCvtr, rhos;
  su2double RuSI, Ru, sqvel, num, denom, conc, soundspeed;

  /*--- Get Mutation++ mixture ---*/
  nSpecies     = config->GetnSpecies();
  nDim         = val_ndim;
  nPrimVar     = val_nvarprim;
  nPrimVarGrad = val_nvarprimgrad;
  nMGSmooth    = 0;

  /*--- Define structure of the primtive variable vector ---*/
  // Primitive: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T
  // GradPrim:  [rho1, ..., rhoNs, T, Tve, u, v, w, P]^T
  RHOS_INDEX    = 0;
  T_INDEX       = nSpecies;
  TVE_INDEX     = nSpecies+1;
  VEL_INDEX     = nSpecies+2;
  P_INDEX       = nSpecies+nDim+2;
  RHO_INDEX     = nSpecies+nDim+3;
  H_INDEX       = nSpecies+nDim+4;
  A_INDEX       = nSpecies+nDim+5;
  RHOCVTR_INDEX = nSpecies+nDim+6;
  RHOCVVE_INDEX = nSpecies+nDim+7;

  /*--- Array initialization ---*/
  Primitive          = NULL;
  //Secondary          = NULL;

  Gradient_Primitive = NULL;
  //Gradient_Secondary = NULL;

  Limiter_Primitive  = NULL;
  Limiter            = NULL;
  //Limiter_Secondary  = NULL;

  //Solution_New = NULL;

  /*--- Allocate & initialize residual vectors ---*/
  Res_TruncError = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++) {
    Res_TruncError[iVar] = 0.0;
  }

  /*--- If using multigrid, allocate residual-smoothing vectors ---*/
  for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++)
    nMGSmooth += config->GetMG_CorrecSmooth(iMesh);

  if (nMGSmooth > 0) {
    Residual_Sum = new su2double [nVar];
    Residual_Old = new su2double [nVar];
  }

  /*--- Allocate & initialize primitive variable & gradient arrays ---*/
  Limiter = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++) Limiter[iVar] = 0.0;

  Limiter_Primitive = new su2double [nPrimVarGrad];
  for (iVar = 0; iVar < nPrimVarGrad; iVar++) Limiter_Primitive[iVar] = 0.0;

  //Limiter_Secondary = new su2double [nSecondaryVarGrad];
  //for (iVar = 0; iVar < nSecondaryVarGrad; iVar++) Limiter_Secondary[iVar] = 0.0;

  Solution_Max      = new su2double [nPrimVarGrad];
  Solution_Min      = new su2double [nPrimVarGrad];
  for (iVar = 0; iVar < nVar; iVar++){
    Solution_Max[iVar] = 0.0;
    Solution_Min[iVar] = 0.0;
  }

  /*--- Incompressible flow, primitive variables ---*/
  Primitive = new su2double [nPrimVar];
  for (iVar = 0; iVar < nPrimVar; iVar++) Primitive[iVar] = 0.0;

  //Secondary = new su2double [nSecondaryVar];
  //for (iVar = 0; iVar < nSecondaryVar; iVar++) Secondary[iVar] = 0.0;

  /*--- Compressible flow, gradients primitive variables ---*/
  Gradient_Primitive = new su2double* [nPrimVarGrad];
  for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
    Gradient_Primitive[iVar] = new su2double [nDim];
    for (iDim = 0; iDim < nDim; iDim++)
      Gradient_Primitive[iVar][iDim] = 0.0;
  }

  /*--- Allocate partial derivative vectors ---*/
  dPdU      = new su2double [nVar];
  dTdU      = new su2double [nVar];
  dTvedU    = new su2double [nVar];

  /*--- Allocate primitive vibrational energy arrays ---*/
  eves      = new su2double [nSpecies];
  Cvves     = new su2double [nSpecies];

  /*--- Determine the number of heavy species ---*/
  ionization = config->GetIonization();
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Load variables from the config class --*/
  xi        = config->GetRotationModes();      // Rotational modes of energy storage
  Ms        = config->GetMolar_Mass();         // Species molar mass
  thetav    = config->GetCharVibTemp();        // Species characteristic vib. temperature [K]
  thetae    = config->GetCharElTemp();         // Characteristic electron temperature [K]
  g         = config->GetElDegeneracy();       // Degeneracy of electron states
  nElStates = config->GetnElStates();          // Number of electron states
  Tref      = config->GetRefTemperature();     // Thermodynamic reference temperature [K]
  hf        = config->GetEnthalpy_Formation(); // Formation enthalpy [J/kg]

  /*--- Rename & initialize for convenience ---*/
  RuSI      = UNIVERSAL_GAS_CONSTANT;           // Universal gas constant [J/(mol*K)]
  Ru        = 1000.0*RuSI;                       // Universal gas constant [J/(kmol*K)]
  Tve       = val_temperature_ve;                // Vibrational temperature [K]
  T         = val_temperature;                   // Translational-rotational temperature [K]
  sqvel     = 0.0;                               // Velocity^2 [m2/s2]
  rhoE      = 0.0;                               // Mixture total energy per mass [J/kg]
  rhoEve    = 0.0;                               // Mixture vib-el energy per mass [J/kg]
  denom     = 0.0;
  conc      = 0.0;
  rhoCvtr   = 0.0;

  /*--- Calculate mixture density from supplied primitive quantities ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++)
    denom += val_massfrac[iSpecies] * (Ru/Ms[iSpecies]) * T;

  for (iSpecies = 0; iSpecies < nEl; iSpecies++)
    denom += val_massfrac[nSpecies-1] * (Ru/Ms[nSpecies-1]) * Tve;

  rho = val_pressure / denom;

  /*--- Calculate sound speed and extract velocities ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    conc += val_massfrac[iSpecies]*rho/Ms[iSpecies];
    rhoCvtr += rho*val_massfrac[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies];
  }

  soundspeed = sqrt((1.0 + Ru/rhoCvtr*conc) * val_pressure/rho);

  for (iDim = 0; iDim < nDim; iDim++)
    sqvel += val_mach[iDim]*soundspeed * val_mach[iDim]*soundspeed;

  /*--- Calculate energy (RRHO) from supplied primitive quanitites ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    // Species density
    rhos = val_massfrac[iSpecies]*rho;

    // Species formation energy
    Ef = hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies];

    // Species vibrational energy
    if (thetav[iSpecies] != 0.0)
      Ev = Ru/Ms[iSpecies] * thetav[iSpecies] / (exp(thetav[iSpecies]/Tve)-1.0);
    else
      Ev = 0.0;

    // Species electronic energy
    num = 0.0;
    denom = g[iSpecies][0] * exp(thetae[iSpecies][0]/Tve);

    for (iEl = 1; iEl < nElStates[iSpecies]; iEl++) {
      num   += g[iSpecies][iEl] * thetae[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
      denom += g[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
    }

    Ee = Ru/Ms[iSpecies] * (num/denom);

    // Mixture total energy
    rhoE += rhos * ((3.0/2.0+xi[iSpecies]/2.0) * Ru/Ms[iSpecies] * (T-Tref[iSpecies])
                    + Ev + Ee + Ef + 0.5*sqvel);

    // Mixture vibrational-electronic energy
    rhoEve += rhos * (Ev + Ee);
  }

  for (iSpecies = 0; iSpecies < nEl; iSpecies++) {
    // Species formation energy
    Ef = hf[nSpecies-1] - Ru/Ms[nSpecies-1] * Tref[nSpecies-1];

    // Electron t-r mode contributes to mixture vib-el energy
    rhoEve += (3.0/2.0) * Ru/Ms[nSpecies-1] * (Tve - Tref[nSpecies-1]);
  }

  /*--- Initialize Solution & Solution_Old vectors ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    Solution[iSpecies]     = rho*val_massfrac[iSpecies];
    Solution_Old[iSpecies] = rho*val_massfrac[iSpecies];
  }
  for (iDim = 0; iDim < nDim; iDim++) {
    Solution[nSpecies+iDim]     = rho*val_mach[iDim]*soundspeed;
    Solution_Old[nSpecies+iDim] = rho*val_mach[iDim]*soundspeed;
  }
  Solution[nSpecies+nDim]       = rhoE;
  Solution_Old[nSpecies+nDim]   = rhoE;
  Solution[nSpecies+nDim+1]     = rhoEve;
  Solution_Old[nSpecies+nDim+1] = rhoEve;

  /*--- Assign primitive variables ---*/
  Primitive[T_INDEX]   = val_temperature;
  Primitive[TVE_INDEX] = val_temperature_ve;
  Primitive[P_INDEX]   = val_pressure;

}

CTNE2EulerVariable::CTNE2EulerVariable(su2double *val_solution,
                                       unsigned short val_ndim,
                                       unsigned short val_nvar,
                                       unsigned short val_nvarprim,
                                       unsigned short val_nvarprimgrad,
                                       CConfig *config) : CVariable(val_ndim,
                                                                    val_nvar,
                                                                    config) {

  unsigned short iVar, iDim, iMesh, nMGSmooth;

  nSpecies     = config->GetnSpecies();
  nDim         = val_ndim;
  nPrimVar     = val_nvarprim;
  nPrimVarGrad = val_nvarprimgrad;
  nMGSmooth    = 0;

  /*--- Define structure of the primtive variable vector ---*/
  // Primitive: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T
  // GradPrim:  [rho1, ..., rhoNs, T, Tve, u, v, w, P]^T
  RHOS_INDEX    = 0;
  T_INDEX       = nSpecies;
  TVE_INDEX     = nSpecies+1;
  VEL_INDEX     = nSpecies+2;
  P_INDEX       = nSpecies+nDim+2;
  RHO_INDEX     = nSpecies+nDim+3;
  H_INDEX       = nSpecies+nDim+4;
  A_INDEX       = nSpecies+nDim+5;
  RHOCVTR_INDEX = nSpecies+nDim+6;
  RHOCVVE_INDEX = nSpecies+nDim+7;

  /*--- Array initialization ---*/
  Primitive          = NULL;
  //Secondary          = NULL;

  Gradient_Primitive = NULL;
  //Gradient_Secondary = NULL;

  Limiter_Primitive  = NULL;
  Limiter            = NULL;
  //Limiter_Secondary  = NULL;

  //Solution_New        = NULL;

  /*--- Allocate & initialize residual vectors ---*/
  Res_TruncError = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++) {
    Res_TruncError[iVar] = 0.0;
  }

  /*--- If using multigrid, allocate residual-smoothing vectors ---*/
  for (iMesh = 0; iMesh <= config->GetnMGLevels(); iMesh++)
    nMGSmooth += config->GetMG_CorrecSmooth(iMesh);

  if (nMGSmooth > 0) {
    Residual_Sum = new su2double [nVar];
    Residual_Old = new su2double [nVar];
  }

  /*--- If using limiters, allocate the arrays ---*/
  Limiter_Primitive = new su2double [nPrimVarGrad];
  for (iVar = 0; iVar < nPrimVarGrad; iVar++)
    Limiter_Primitive[iVar] = 0.0;

  //Limiter_Secondary = new su2double [nSecondaryVarGrad];
  //for (iVar = 0; iVar < nSecondaryVarGrad; iVar++)
  //  Limiter_Secondary[iVar] = 0.0;

  Limiter = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++)
    Limiter[iVar] = 0.0;

  Solution_Max = new su2double [nPrimVarGrad];
  Solution_Min = new su2double [nPrimVarGrad];
  for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
    Solution_Max[iVar] = 0.0;
    Solution_Min[iVar] = 0.0;
  }

  /*--- Allocate & initialize primitive variable & gradient arrays ---*/
  Primitive = new su2double [nPrimVar];
  for (iVar = 0; iVar < nPrimVar; iVar++) Primitive[iVar] = 0.0;

  Gradient_Primitive = new su2double* [nPrimVarGrad];
  for (iVar = 0; iVar < nPrimVarGrad; iVar++) {
    Gradient_Primitive[iVar] = new su2double [nDim];
    for (iDim = 0; iDim < nDim; iDim++)
      Gradient_Primitive[iVar][iDim] = 0.0;
  }

  //Gradient_Secondary = new su2double* [nSecondaryVarGrad];
  //for (iVar = 0; iVar < nSecondaryVarGrad; iVar++) {
  //  Gradient_Secondary[iVar] = new su2double [nDim];
  //  for (iDim = 0; iDim < nDim; iDim++)
  //    Gradient_Secondary[iVar][iDim] = 0.0;
  //}

  /*--- Allocate partial derivative vectors ---*/
  dPdU   = new su2double [nVar];
  dTdU   = new su2double [nVar];
  dTvedU = new su2double [nVar];

  /*--- Allocate vibrational-electronic arrays ---*/
  eves  = new su2double[nSpecies];
  Cvves = new su2double[nSpecies];

  /*--- Determine the number of heavy species ---*/
  ionization = config->GetIonization();

  /*--- Initialize Solution & Solution_Old vectors ---*/
  for (iVar = 0; iVar < nVar; iVar++) {
    Solution[iVar]     = val_solution[iVar];
    Solution_Old[iVar] = val_solution[iVar];
  }

  /*--- Initialize Tve to the free stream for Newton-Raphson method ---*/
  Primitive[TVE_INDEX] = config->GetTemperature_FreeStream();
  Primitive[T_INDEX]   = config->GetTemperature_FreeStream();
  Primitive[P_INDEX]   = config->GetPressure_FreeStream();
}

CTNE2EulerVariable::~CTNE2EulerVariable(void) {

  unsigned short iVar;

  if (Primitive          != NULL) delete [] Primitive;
  //if (Secondary          != NULL) delete [] Secondary;
  if (Limiter_Primitive  != NULL) delete [] Limiter_Primitive;
  //if (Limiter            != NULL) delete [] Limiter;
  //if (Limiter_Secondary  != NULL) delete [] Limiter_Secondary;

  if (Gradient_Primitive != NULL) {
    for (iVar = 0; iVar < nPrimVarGrad; iVar++)
      if (Gradient_Primitive[iVar] != NULL) delete [] Gradient_Primitive[iVar];
    delete [] Gradient_Primitive;
  }

  //if (Gradient_Secondary != NULL) {
  //  for (iVar = 0; iVar < nSecondaryVarGrad; iVar++)
  //    if (Gradient_Secondary[iVar] != NULL) delete [] Gradient_Secondary[iVar];
  //  delete [] Gradient_Secondary;
  //}

  if (dPdU   != NULL) delete [] dPdU;
  if (dTdU   != NULL) delete [] dTdU;
  if (dTvedU != NULL) delete [] dTvedU;
  if (eves   != NULL) delete [] eves;
  if (Cvves  != NULL) delete [] Cvves;
}

void CTNE2EulerVariable::SetGradient_PrimitiveZero(unsigned short val_primvar) {

  unsigned short iVar, iDim;

  for (iVar = 0; iVar < val_primvar; iVar++)
    for (iDim = 0; iDim < nDim; iDim++)
      Gradient_Primitive[iVar][iDim] = 0.0;
}

//void CEulerVariable::SetGradient_SecondaryZero(unsigned short val_secondaryvar) {
//    unsigned short iVar, iDim;
//
//    for (iVar = 0; iVar < val_secondaryvar; iVar++)
//        for (iDim = 0; iDim < nDim; iDim++)
//            Gradient_Secondary[iVar][iDim] = 0.0;
//}

su2double CTNE2EulerVariable::GetProjVel(su2double *val_vector) {

  su2double ProjVel, density;
  unsigned short iDim, iSpecies;

  ProjVel = 0.0;
  density = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    density += Solution[iSpecies];
  for (iDim = 0; iDim < nDim; iDim++)
    ProjVel += Solution[nSpecies+iDim]*val_vector[iDim]/density;

  return ProjVel;
}

bool CTNE2EulerVariable::SetDensity(void) {

  unsigned short iSpecies;
  su2double Density;

  Density = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    Primitive[RHOS_INDEX+iSpecies] = Solution[iSpecies];
    Density += Solution[iSpecies];
  }
  Primitive[RHO_INDEX] = Density;

  return true;
}

void CTNE2EulerVariable::SetVelocity2(void) {

  unsigned short iDim;

  Velocity2 = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    Primitive[VEL_INDEX+iDim] = Solution[nSpecies+iDim] / Primitive[RHO_INDEX];
    Velocity2 +=  Solution[nSpecies+iDim]*Solution[nSpecies+iDim]
        / (Primitive[RHO_INDEX]*Primitive[RHO_INDEX]);
  }
}

bool CTNE2EulerVariable::SetTemperature(CConfig *config) {

  // Note: Requires previous call to SetDensity()
  unsigned short iEl, iSpecies, iDim, nHeavy, nEl, iIter, maxIter, *nElStates;
  su2double *xi, *Ms, *thetav, **thetae, **g, *hf, *Tref;
  su2double rho, rhoE, rhoEve, rhoEve_t, rhoE_ref, rhoE_f;
  su2double evs, eels;
  su2double RuSI, Ru, sqvel, rhoCvtr, rhoCvve;
  su2double Cvvs, Cves, Tve, Tve2, Tve_o;
  su2double f, df, tol;
  su2double exptv, thsqr, thoTve;
  su2double num, denom, num2, num3;
  su2double Tmin, Tmax, Tvemin, Tvemax;

  /*--- Set tolerance for Newton-Raphson method ---*/
  tol     = 1.0E-4;
  maxIter = 100;

  /*--- Set temperature clipping values ---*/
  Tmin   = 100.0;
  Tmax   = 6E4;
  Tvemin = 100.0;
  Tvemax = 4E4;

  /*--- Determine the number of heavy species ---*/
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Load variables from the config class --*/
  xi        = config->GetRotationModes();      // Rotational modes of energy storage
  Ms        = config->GetMolar_Mass();         // Species molar mass
  thetav    = config->GetCharVibTemp();        // Species characteristic vib. temperature [K]
  Tref      = config->GetRefTemperature();     // Thermodynamic reference temperature [K]
  hf        = config->GetEnthalpy_Formation(); // Formation enthalpy [J/kg]
  thetae    = config->GetCharElTemp();
  g         = config->GetElDegeneracy();
  nElStates = config->GetnElStates();

  /*--- Rename & initialize for convenience ---*/
  RuSI     = UNIVERSAL_GAS_CONSTANT;           // Universal gas constant [J/(mol*K)]
  Ru       = 1000.0*RuSI;                      // Universal gas constant [J/(kmol*k)]
  rho      = Primitive[RHO_INDEX];             // Mixture density [kg/m3]
  rhoE     = Solution[nSpecies+nDim];          // Density * energy [J/m3]
  rhoEve   = Solution[nSpecies+nDim+1];        // Density * energy_ve [J/m3]
  rhoE_f   = 0.0;                              // Density * formation energy [J/m3]
  rhoE_ref = 0.0;                              // Density * reference energy [J/m3]
  rhoCvtr  = 0.0;                              // Mix spec. heat @ const. volume [J/(kg*K)]
  sqvel    = 0.0;                              // Velocity^2 [m2/s2]

  /*--- Error checking ---*/
  if (rhoE < 0.0)
    rhoE = EPS;
  if (rhoEve < 0.0)
    rhoEve = EPS;

  /*--- Calculate mixture properties (heavy particles only) ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    rhoCvtr  += Solution[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies];
    rhoE_ref += Solution[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies] * Tref[iSpecies];
    rhoE_f   += Solution[iSpecies] * (hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies]);
  }
  for (iDim = 0; iDim < nDim; iDim++)
    sqvel    += (Solution[nSpecies+iDim]/rho) * (Solution[nSpecies+iDim]/rho);

  /*--- Calculate translational-rotational temperature ---*/
  Primitive[T_INDEX] = (rhoE - rhoEve - rhoE_f + rhoE_ref - 0.5*rho*sqvel) / rhoCvtr;

  /*--- Calculate vibrational-electronic temperature ---*/
  // NOTE: Cannot write an expression explicitly in terms of Tve
  // NOTE: We use Newton-Raphson to iteratively find the value of Tve
  // NOTE: Use T as an initial guess
  Tve   = Primitive[TVE_INDEX];
  Tve_o = Primitive[TVE_INDEX];

  for (iIter = 0; iIter < maxIter; iIter++) {
    rhoEve_t = 0.0;
    rhoCvve  = 0.0;
    for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {

      /*--- Vibrational energy ---*/
      if (thetav[iSpecies] != 0.0) {

        /*--- Rename for convenience ---*/
        thoTve = thetav[iSpecies]/Tve;
        exptv = exp(thetav[iSpecies]/Tve);
        thsqr = thetav[iSpecies]*thetav[iSpecies];

        /*--- Calculate vibrational energy ---*/
        evs  = Ru/Ms[iSpecies] * thetav[iSpecies] / (exptv - 1.0);

        /*--- Calculate species vibrational specific heats ---*/
        Cvvs  = Ru/Ms[iSpecies] * thoTve*thoTve * exptv / ((exptv-1.0)*(exptv-1.0));

        /*--- Add contribution ---*/
        rhoEve_t += Solution[iSpecies] * evs;
        rhoCvve  += Solution[iSpecies] * Cvvs;
      }
      /*--- Electronic energy ---*/
      if (nElStates[iSpecies] != 0) {
        num = 0.0; num2 = 0.0;
        denom = g[iSpecies][0] * exp(-thetae[iSpecies][0]/Tve);
        num3  = g[iSpecies][0] * (thetae[iSpecies][0]/(Tve*Tve))*exp(-thetae[iSpecies][0]/Tve);
        for (iEl = 1; iEl < nElStates[iSpecies]; iEl++) {
          thoTve = thetae[iSpecies][iEl]/Tve;
          exptv = exp(-thetae[iSpecies][iEl]/Tve);

          num   += g[iSpecies][iEl] * thetae[iSpecies][iEl] * exptv;
          denom += g[iSpecies][iEl] * exptv;
          num2  += g[iSpecies][iEl] * (thoTve*thoTve) * exptv;
          num3  += g[iSpecies][iEl] * thoTve/Tve * exptv;
        }
        eels = Ru/Ms[iSpecies] * (num/denom);
        Cves = Ru/Ms[iSpecies] * (num2/denom - num*num3/(denom*denom));

        rhoEve_t += Solution[iSpecies] * eels;
        rhoCvve  += Solution[iSpecies] * Cves;
      }
    }
    for (iSpecies = 0; iSpecies < nEl; iSpecies++) {
      Cves = 3.0/2.0 * Ru/Ms[nSpecies-1];
      rhoEve_t += Solution[nSpecies-1] * Cves * Tve;
      rhoCvve += Solution[nSpecies-1] * Cves;
    }

    /*--- Determine function f(Tve) and df/dTve ---*/
    f  = rhoEve - rhoEve_t;
    df = -rhoCvve;
    Tve2 = Tve - (f/df)*0.5;

    /*--- Check for non-physical conditions ---*/
    if ((Tve2 != Tve2) || (Tve2 < 0))
      Tve2 = 1.4*Primitive[T_INDEX];

    /*--- Check for convergence ---*/
    if (fabs(Tve2-Tve) < tol) break;
    if (iIter == maxIter-1) {
      cout << "WARNING!!! Tve convergence not reached!" << endl;
      cout << "rhoE: " << rhoE << endl;
      cout << "rhoEve: " << rhoEve << endl;
      cout << "T: " << Primitive[T_INDEX] << endl;
      cout << "Tve2: " << Tve2 << endl;
      cout << "Tve_o: " << Tve_o << endl;
      Tve2 = Tve_o;
      break;
    }
    Tve = Tve2;
  }

  Primitive[TVE_INDEX] = Tve2;

  /*--- Error checking ---*/
  if (Primitive[T_INDEX] <= Tmin) {
    cout << "WARNING: T = " << Primitive[T_INDEX] << "\t -- Clipping at: " << Tmin << endl;
    Primitive[T_INDEX] = Tmin;
  } else if (Primitive[T_INDEX] >= Tmax) {
    cout << "WARNING: T = " << Primitive[T_INDEX] << "\t -- Clipping at: " << Tmax << endl;
    Primitive[T_INDEX] = Tmax;
  }
  if (Primitive[TVE_INDEX] <= Tvemin) {
    cout << "WARNING: Tve = " << Primitive[TVE_INDEX] << "\t -- Clipping at: " << Tvemin << endl;
    Primitive[TVE_INDEX] = Tvemin;
  } else if (Primitive[TVE_INDEX] >= Tvemax) {
    cout << "WARNING: Tve = " << Primitive[TVE_INDEX] << "\t -- Clipping at: " << Tvemax << endl;
    Primitive[TVE_INDEX] = Tvemax;
  }

  /*--- Assign Gas Properties ---*/
  Primitive[RHOCVTR_INDEX] = rhoCvtr;
  Primitive[RHOCVVE_INDEX] = rhoCvve;

  /*--- Check that the solution is physical ---*/
  if ((Primitive[T_INDEX] > 0.0) && (Primitive[TVE_INDEX] > 0.0)) return false;
  else return true;
}

void CTNE2EulerVariable::SetGasProperties(CConfig *config) {

  // NOTE: Requires computation of vib-el temperature.
  // NOTE: For now, neglecting contribution from electronic excitation

  unsigned short iSpecies, nHeavy, nEl;
  su2double rhoCvtr, rhoCvve, RuSI, Ru, Tve;
  su2double *xi, *Ms, *thetav;

  /*--- Determine the number of heavy species ---*/
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Load variables from the config class --*/
  xi     = config->GetRotationModes(); // Rotational modes of energy storage
  Ms     = config->GetMolar_Mass();    // Species molar mass
  thetav = config->GetCharVibTemp();   // Species characteristic vib. temperature [K]
  RuSI   = UNIVERSAL_GAS_CONSTANT;     // Universal gas constant [J/(mol*K)]
  Ru     = 1000.0*RuSI;                // Universal gas constant [J/(kmol*K)]
  Tve    = Primitive[nSpecies+1];      // Vibrational-electronic temperature [K]

  rhoCvtr = 0.0;
  rhoCvve = 0.0;

  /*--- Heavy particle contribution ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    rhoCvtr += Solution[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies];

    if (thetav[iSpecies] != 0.0) {
      rhoCvve += Ru/Ms[iSpecies] * (thetav[iSpecies]/Tve)*(thetav[iSpecies]/Tve) * exp(thetav[iSpecies]/Tve)
          / ((exp(thetav[iSpecies]/Tve)-1.0)*(exp(thetav[iSpecies]/Tve)-1.0));
    }

    //--- [ Electronic energy goes here ] ---//

  }

  /*--- Free-electron contribution ---*/
  for (iSpecies = 0; iSpecies < nEl; iSpecies++) {
    rhoCvve += Solution[nSpecies-1] * 3.0/2.0 * Ru/Ms[nSpecies-1];
  }

  /*--- Store computed values ---*/
  Primitive[RHOCVTR_INDEX] = rhoCvtr;
  Primitive[RHOCVVE_INDEX] = rhoCvve;
}

bool CTNE2EulerVariable::SetPressure(CConfig *config) {

  // NOTE: Requires computation of trans-rot & vib-el temperatures.

  unsigned short iSpecies, nHeavy, nEl;
  su2double *Ms;
  su2double P, RuSI, Ru;

  /*--- Determine the number of heavy species ---*/
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Read gas mixture properties from config ---*/
  Ms = config->GetMolar_Mass();

  /*--- Rename for convenience ---*/
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;

  /*--- Solve for mixture pressure using ideal gas law & Dalton's law ---*/
  // Note: If free electrons are present, use Tve for their partial pressure
  P = 0.0;
  for(iSpecies = 0; iSpecies < nHeavy; iSpecies++)
    P += Solution[iSpecies] * Ru/Ms[iSpecies] * Primitive[T_INDEX];

  for (iSpecies = 0; iSpecies < nEl; iSpecies++)
    P += Solution[nSpecies-1] * Ru/Ms[nSpecies-1] * Primitive[TVE_INDEX];

  /*--- Store computed values and check for a physical solution ---*/
  Primitive[P_INDEX] = P;
  if (Primitive[P_INDEX] > 0.0) return false;
  else return true;
}

bool CTNE2EulerVariable::SetSoundSpeed(CConfig *config) {

  // NOTE: Requires SetDensity(), SetTemperature(), SetPressure(), & SetGasProperties().

  //  unsigned short iDim, iSpecies, nHeavy, nEl;
  //  double dPdrhoE, factor, Ru, radical, radical2;
  //  double *Ms, *xi;
  //
  //  /*--- Determine the number of heavy species ---*/
  //  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  //  else            { nHeavy = nSpecies;   nEl = 0; }
  //
  //  /*--- Read gas mixture properties from config ---*/
  //  Ms = config->GetMolar_Mass();
  //  xi = config->GetRotationModes();
  //
  //  /*--- Rename for convenience ---*/
  //  Ru = UNIVERSAL_GAS_CONSTANT;
  //
  //  /*--- Calculate partial derivative of pressure w.r.t. rhoE ---*/
  //  factor = 0.0;
  //  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++)
  //    factor += Solution[iSpecies] / Ms[iSpecies];
  //  dPdrhoE = Ru/Primitive[RHOCVTR_INDEX] * factor;
  //
  //  /*--- Calculate a^2 using Gnoffo definition (NASA TP 2867) ---*/
  //  radical = (1.0+dPdrhoE) * Primitive[P_INDEX]/Primitive[RHO_INDEX];

  /// alternative
  unsigned short iSpecies, iDim;
  su2double radical2;

  radical2 = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    radical2 += Primitive[RHOS_INDEX+iSpecies]/Primitive[RHO_INDEX] * dPdU[iSpecies];
  for (iDim = 0; iDim < nDim; iDim++)
    radical2 += Primitive[VEL_INDEX+iDim]*dPdU[nSpecies+iDim];
  radical2 += (Solution[nSpecies+nDim]+Primitive[P_INDEX])/Primitive[RHO_INDEX] * dPdU[nSpecies+nDim];
  radical2 += Solution[nSpecies+nDim+1]/Primitive[RHO_INDEX] * dPdU[nSpecies+nDim+1];

  if (radical2 < 0.0) return true;
  else {Primitive[A_INDEX] = sqrt(radical2); return false; }

  //  if (radical < 0.0) return true;
  //  else { Primitive[A_INDEX] = sqrt(radical); return false; }
}

void CTNE2EulerVariable::CalcdPdU(su2double *V, su2double *val_eves,
                                  CConfig *config, su2double *val_dPdU) {

  // Note: Requires SetDensity(), SetTemperature(), SetPressure(), & SetGasProperties()
  // Note: Electron energy not included properly.

  unsigned short iDim, iSpecies, iEl, nHeavy, nEl, *nElStates;
  su2double *Ms, *Tref, *hf, *xi, *thetav, **thetae, **g;
  su2double RuSI, Ru, RuBAR, CvtrBAR, rhoCvtr, rhoCvve, Cvtrs, rho_el, sqvel, conc;
  su2double rho, rhos, T, Tve, ef;
  su2double num, denom;

  if (val_dPdU == NULL) {
    cout << "ERROR: CalcdPdU - Array dPdU not allocated!" << endl;
    exit(1);
  }

  /*--- Determine the number of heavy species ---*/
  if (ionization) {
    nHeavy = nSpecies-1;
    nEl    = 1;
    rho_el = V[RHOS_INDEX+nSpecies-1];
  } else {
    nHeavy = nSpecies;
    nEl    = 0;
    rho_el = 0.0;
  }

  /*--- Read gas mixture properties from config ---*/
  Ms        = config->GetMolar_Mass();
  Tref      = config->GetRefTemperature();
  hf        = config->GetEnthalpy_Formation();
  xi        = config->GetRotationModes();
  thetav    = config->GetCharVibTemp();
  thetae    = config->GetCharElTemp();
  g         = config->GetElDegeneracy();
  nElStates = config->GetnElStates();

  /*--- Rename for convenience ---*/
  RuSI    = UNIVERSAL_GAS_CONSTANT;
  Ru      = 1000.0*RuSI;
  T       = V[T_INDEX];
  Tve     = V[TVE_INDEX];
  rho     = V[RHO_INDEX];
  rhoCvtr = V[RHOCVTR_INDEX];
  rhoCvve = V[RHOCVVE_INDEX];

  /*--- Pre-compute useful quantities ---*/
  RuBAR   = 0.0;
  CvtrBAR = 0.0;
  sqvel   = 0.0;
  conc    = 0.0;
  for (iDim = 0; iDim < nDim; iDim++)
    sqvel += V[VEL_INDEX+iDim] * V[VEL_INDEX+iDim];
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    CvtrBAR += V[RHOS_INDEX+iSpecies]*(3.0/2.0 + xi[iSpecies]/2.0)*Ru/Ms[iSpecies];
    conc    += V[RHOS_INDEX+iSpecies]/Ms[iSpecies];
  }

  // Species density
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    rhos  = V[RHOS_INDEX+iSpecies];
    ef    = hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies];
    Cvtrs = (3.0/2.0+xi[iSpecies]/2.0)*Ru/Ms[iSpecies];

    val_dPdU[iSpecies] =  T*Ru/Ms[iSpecies] + Ru*conc/rhoCvtr *
        (-Cvtrs*(T-Tref[iSpecies]) -
         ef + 0.5*sqvel);
  }
  if (ionization) {
    for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
      //      evibs = Ru/Ms[iSpecies] * thetav[iSpecies]/(exp(thetav[iSpecies]/Tve)-1.0);
      //      num = 0.0;
      //      denom = g[iSpecies][0] * exp(-thetae[iSpecies][0]/Tve);
      //      for (iEl = 1; iEl < nElStates[iSpecies]; iEl++) {
      //        num   += g[iSpecies][iEl] * thetae[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
      //        denom += g[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
      //      }
      //      eels = Ru/Ms[iSpecies] * (num/denom);

      val_dPdU[iSpecies] -= rho_el * Ru/Ms[nSpecies-1] * (val_eves[iSpecies])/rhoCvve;
    }
    ef = hf[nSpecies-1] - Ru/Ms[nSpecies-1]*Tref[nSpecies-1];
    val_dPdU[nSpecies-1] = Ru*conc/rhoCvtr * (-ef + 0.5*sqvel)
        + Ru/Ms[nSpecies-1]*Tve
        - rho_el*Ru/Ms[nSpecies-1] * (-3.0/2.0*Ru/Ms[nSpecies-1]*Tve)/rhoCvve;
  }
  // Momentum
  for (iDim = 0; iDim < nDim; iDim++)
    val_dPdU[nSpecies+iDim] = -conc*Ru*V[VEL_INDEX+iDim]/rhoCvtr;

  // Total energy
  val_dPdU[nSpecies+nDim]   = conc*Ru / rhoCvtr;

  // Vib.-el energy
  val_dPdU[nSpecies+nDim+1] = -val_dPdU[nSpecies+nDim]
      + rho_el*Ru/Ms[nSpecies-1]*1.0/rhoCvve;
}

su2double CTNE2EulerVariable::CalcEve(CConfig *config, su2double val_Tve,
                                      unsigned short val_Species) {

  unsigned short iEl, *nElStates;
  su2double *Ms, *thetav, **thetae, **g, *hf, *Tref, RuSI, Ru;
  su2double Tve, Ev, Eel, Ef;
  su2double num, denom;

  /*--- Read gas mixture properties from config ---*/
  Ms        = config->GetMolar_Mass();

  /*--- Rename for convenience ---*/
  RuSI  = UNIVERSAL_GAS_CONSTANT;
  Ru    = 1000.0*RuSI;
  Tve   = val_Tve;

  /*--- Electron species energy ---*/
  if ((ionization) && (val_Species == nSpecies-1)) {

    /*--- Get quantities from CConfig ---*/
    Tref = config->GetRefTemperature();
    hf   = config->GetEnthalpy_Formation();

    /*--- Calculate formation energy ---*/
    Ef = hf[val_Species] - Ru/Ms[val_Species] * Tref[val_Species];

    /*--- Electron t-r mode contributes to mixture vib-el energy ---*/
    Eel = (3.0/2.0) * Ru/Ms[val_Species] * (Tve - Tref[val_Species]) + Ef;
    Ev  = 0.0;

  }

  /*--- Heavy particle energy ---*/
  else {

    /*--- Read from CConfig ---*/
    thetav    = config->GetCharVibTemp();
    thetae    = config->GetCharElTemp();
    g         = config->GetElDegeneracy();
    nElStates = config->GetnElStates();

    /*--- Calculate vibrational energy (harmonic-oscillator model) ---*/
    if (thetav[val_Species] != 0.0)
      Ev = Ru/Ms[val_Species] * thetav[val_Species] / (exp(thetav[val_Species]/Tve)-1.0);
    else
      Ev = 0.0;

    /*--- Calculate electronic energy ---*/
    num = 0.0;
    denom = g[val_Species][0] * exp(-thetae[val_Species][0]/Tve);
    for (iEl = 1; iEl < nElStates[val_Species]; iEl++) {
      num   += g[val_Species][iEl] * thetae[val_Species][iEl] * exp(-thetae[val_Species][iEl]/Tve);
      denom += g[val_Species][iEl] * exp(-thetae[val_Species][iEl]/Tve);
    }
    Eel = Ru/Ms[val_Species] * (num/denom);
  }

  return Ev + Eel;
}

su2double CTNE2EulerVariable::CalcHs(CConfig *config, su2double val_T,
                                     su2double val_eves, unsigned short val_Species) {

  su2double RuSI, Ru, *xi, *Ms, *hf, *Tref, T, eve, ef, hs;

  /*--- Read from config ---*/
  xi   = config->GetRotationModes();
  Ms   = config->GetMolar_Mass();
  hf   = config->GetEnthalpy_Formation();
  Tref = config->GetRefTemperature();

  /*--- Rename for convenience ---*/
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;
  T    = val_T;

  /*--- Calculate vibrational-electronic energy per unit mass ---*/
  eve = val_eves;

  /*--- Calculate formation energy ---*/
  ef = hf[val_Species] - Ru/Ms[val_Species]*Tref[val_Species];

  hs = Ru/Ms[val_Species]*T
      + (3.0/2.0+xi[val_Species]/2.0)*Ru/Ms[val_Species]*T
      + hf[val_Species] + eve;

  //  hs = (3.0/2.0+xi[val_Species]/2.0) * Ru/Ms[val_Species] * (T-Tref[val_Species])
  //   + eve + ef;

  return hs;
}

su2double CTNE2EulerVariable::CalcCvve(su2double val_Tve, CConfig *config, unsigned short val_Species) {

  unsigned short iEl, *nElStates;
  su2double *Ms, *thetav, **thetae, **g, RuSI, Ru;
  su2double thoTve, exptv, thsqr, Cvvs, Cves;
  su2double Tve;
  su2double num, num2, num3, denom;

  /*--- Read from config ---*/
  thetav    = config->GetCharVibTemp();
  thetae    = config->GetCharElTemp();
  g         = config->GetElDegeneracy();
  Ms        = config->GetMolar_Mass();
  nElStates = config->GetnElStates();

  /*--- Rename for convenience ---*/
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;
  Tve  = val_Tve;

  /*--- If requesting electron specific heat ---*/
  if (ionization && val_Species == nSpecies-1) {
    Cvvs = 0.0;
    Cves = 3.0/2.0 * Ru/Ms[nSpecies-1];
  }

  /*--- Heavy particle specific heat ---*/
  else {

    /*--- Vibrational energy ---*/
    if (thetav[val_Species] != 0.0) {
      thoTve = thetav[val_Species]/Tve;
      exptv = exp(thetav[val_Species]/Tve);
      thsqr = thetav[val_Species]*thetav[val_Species];
      Cvvs  = Ru/Ms[val_Species] * thoTve*thoTve * exptv / ((exptv-1.0)*(exptv-1.0));
    } else {
      Cvvs = 0.0;
    }

    /*--- Electronic energy ---*/
    if (nElStates[val_Species] != 0) {
      num = 0.0; num2 = 0.0;
      denom = g[val_Species][0] * exp(-thetae[val_Species][0]/Tve);
      num3  = g[val_Species][0] * (thetae[val_Species][0]/(Tve*Tve))*exp(-thetae[val_Species][0]/Tve);
      for (iEl = 1; iEl < nElStates[val_Species]; iEl++) {
        thoTve = thetae[val_Species][iEl]/Tve;
        exptv = exp(-thetae[val_Species][iEl]/Tve);

        num   += g[val_Species][iEl] * thetae[val_Species][iEl] * exptv;
        denom += g[val_Species][iEl] * exptv;
        num2  += g[val_Species][iEl] * (thoTve*thoTve) * exptv;
        num3  += g[val_Species][iEl] * thoTve/Tve * exptv;
      }
      Cves = Ru/Ms[val_Species] * (num2/denom - num*num3/(denom*denom));
    } else {
      Cves = 0.0;
    }

  }
  return Cvvs + Cves;
}

void CTNE2EulerVariable::CalcdTdU(su2double *V, CConfig *config,
                                  su2double *val_dTdU) {

  unsigned short iDim, iSpecies, nHeavy, nEl;
  su2double *Ms, *xi, *Tref, *hf;
  su2double v2, ef, T, Cvtrs, rhoCvtr, RuSI, Ru;

  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Get gas properties from config settings ---*/
  Ms   = config->GetMolar_Mass();
  xi   = config->GetRotationModes();
  hf   = config->GetEnthalpy_Formation();
  Tref = config->GetRefTemperature();

  /*--- Rename for convenience ---*/
  rhoCvtr = V[RHOCVTR_INDEX];
  T       = V[T_INDEX];
  RuSI    = UNIVERSAL_GAS_CONSTANT;
  Ru      = 1000.0*RuSI;

  /*--- Calculate supporting quantities ---*/
  v2 = 0.0;
  for (iDim = 0; iDim < nDim; iDim++)
    v2 += V[VEL_INDEX+iDim]*V[VEL_INDEX+iDim];

  /*--- Species density derivatives ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    ef    = hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies];
    Cvtrs = (3.0/2.0 + xi[iSpecies]/2.0)*Ru/Ms[iSpecies];
    val_dTdU[iSpecies]   = (-ef + 0.5*v2 + Cvtrs*(Tref[iSpecies]-T)) / rhoCvtr;
  }
  if (ionization) {
    cout << "CTNE2Variable: NEED TO IMPLEMENT dTdU for IONIZED MIX" << endl;
    exit(1);
  }

  /*--- Momentum derivatives ---*/
  for (iDim = 0; iDim < nDim; iDim++)
    val_dTdU[nSpecies+iDim] = -V[VEL_INDEX+iDim] / V[RHOCVTR_INDEX];

  /*--- Energy derivatives ---*/
  val_dTdU[nSpecies+nDim]   =  1.0 / V[RHOCVTR_INDEX];
  val_dTdU[nSpecies+nDim+1] = -1.0 / V[RHOCVTR_INDEX];

}

void CTNE2EulerVariable::CalcdTvedU(su2double *V, su2double *val_eves, CConfig *config,
                                    su2double *val_dTvedU) {

  unsigned short iDim, iSpecies;
  su2double rhoCvve;

  /*--- Rename for convenience ---*/
  rhoCvve = V[RHOCVVE_INDEX];

  /*--- Species density derivatives ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    val_dTvedU[iSpecies] = -val_eves[iSpecies]/rhoCvve;
  }
  /*--- Momentum derivatives ---*/
  for (iDim = 0; iDim < nDim; iDim++)
    val_dTvedU[nSpecies+iDim] = 0.0;

  /*--- Energy derivatives ---*/
  val_dTvedU[nSpecies+nDim]   = 0.0;
  val_dTvedU[nSpecies+nDim+1] = 1.0 / rhoCvve;

}

bool CTNE2EulerVariable::SetPrimVar_Compressible(CConfig *config) {

  bool nonPhys, bkup;
  unsigned short iVar;

  /*--- Convert conserved to primitive variables ---*/
  nonPhys = Cons2PrimVar(config, Solution, Primitive, dPdU, dTdU, dTvedU, eves, Cvves);
  if (nonPhys) {
    for (iVar = 0; iVar < nVar; iVar++)
      Solution[iVar] = Solution_Old[iVar];
    bkup = Cons2PrimVar(config, Solution, Primitive, dPdU, dTdU, dTvedU, eves, Cvves);
  }

  SetVelocity2();

  return nonPhys;

  //	unsigned short iVar, iSpecies;
  //  bool check_dens, check_press, check_sos, check_temp, RightVol;
  //
  //  /*--- Initialize booleans that check for physical solutions ---*/
  //  check_dens  = false;
  //  check_press = false;
  //  check_sos   = false;
  //  check_temp  = false;
  //  RightVol    = true;
  //
  //  /*--- Calculate primitive variables ---*/
  //  // Solution:  [rho1, ..., rhoNs, rhou, rhov, rhow, rhoe, rhoeve]^T
  //  // Primitive: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T
  //  // GradPrim:  [rho1, ..., rhoNs, T, Tve, u, v, w, P]^T
  //  SetDensity();                             // Compute species & mixture density
  //	SetVelocity2();                           // Compute the square of the velocity (req. mixture density).
  //  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
  //    check_dens = ((Solution[iSpecies] < 0.0) || check_dens);  // Check the density
  //  check_temp  = SetTemperature(config);     // Compute temperatures (T & Tve) (req. mixture density).
  // 	check_press = SetPressure(config);        // Requires T & Tve computation.
  //  CalcdPdU(Primitive, config, dPdU);        // Requires density, pressure, rhoCvtr, & rhoCvve.
  //  CalcdTdU(Primitive, config, dTdU);
  //  CalcdTvedU(Primitive, config, dTvedU);
  //  check_sos   = SetSoundSpeed(config);      // Requires density, pressure, rhoCvtr, & rhoCvve.
  //
  //  /*--- Check that the solution has a physical meaning ---*/
  //  if (check_dens || check_press || check_sos || check_temp) {
  //
  //    /*--- Copy the old solution ---*/
  //    for (iVar = 0; iVar < nVar; iVar++)
  //      Solution[iVar] = Solution_Old[iVar];
  //
  //    /*--- Recompute the primitive variables ---*/
  //    SetDensity();                           // Compute mixture density
  //    SetVelocity2();                         // Compute square of the velocity (req. mixture density).
  //    check_temp  = SetTemperature(config);   // Compute temperatures (T & Tve)
  //    check_press = SetPressure(config);      // Requires T & Tve computation.
  //    CalcdPdU(Primitive, config, dPdU);                       // Requires density, pressure, rhoCvtr, & rhoCvve.
  //    CalcdTdU(Primitive, config, dTdU);
  //    CalcdTvedU(Primitive, config, dTvedU);
  //    check_sos   = SetSoundSpeed(config);    // Requires density & pressure computation.
  //
  //    RightVol = false;
  //  }
  //
  //  SetEnthalpy();                            // Requires density & pressure computation.
  //
  //
  //
  //  return RightVol;
}

bool CTNE2EulerVariable::Cons2PrimVar(CConfig *config, su2double *U, su2double *V,
                                      su2double *val_dPdU, su2double *val_dTdU,
                                      su2double *val_dTvedU, su2double *val_eves,
                                      su2double *val_Cvves) {

  bool ionization, errT, errTve, NRconvg, Bconvg, nonPhys;
  unsigned short iDim, iEl, iSpecies, nHeavy, nEl, iIter, maxBIter, maxNIter;
  su2double rho, rhoE, rhoEve, rhoE_f, rhoE_ref, rhoEve_min, rhoEve_max, rhoEve_t;
  su2double RuSI, Ru, sqvel, rhoCvtr, rhoCvve;
  su2double Tve, Tve2, Tve_o;
  su2double f, df, NRtol, Btol, scale;
  su2double Tmin, Tmax, Tvemin, Tvemax;
  su2double radical2;
  su2double *xi, *Ms, *hf, *Tref;

  /*--- Conserved & primitive vector layout ---*/
  // U:  [rho1, ..., rhoNs, rhou, rhov, rhow, rhoe, rhoeve]^T
  // V: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T

  /*--- Set booleans ---*/
  errT    = false;
  errTve  = false;
  nonPhys = false;

  /*--- Set temperature clipping values ---*/
  Tmin   = 50.0;
  Tvemin = 50.0;
  Tmax   = 8E4;
  Tvemax = 8E4;

  /*--- Set temperature algorithm paramters ---*/
  NRtol    = 1.0E-6;    // Tolerance for the Newton-Raphson method
  Btol     = 1.0E-4;    // Tolerance for the Bisection method
  maxNIter = 18;        // Maximum Newton-Raphson iterations
  maxBIter = 32;        // Maximum Bisection method iterations
  scale    = 0.5;       // Scaling factor for Newton-Raphson step

  /*--- Read parameters from config ---*/
  xi         = config->GetRotationModes();      // Rotational modes of energy storage
  Ms         = config->GetMolar_Mass();         // Species molar mass
  Tref       = config->GetRefTemperature();     // Thermodynamic reference temperature [K]
  hf         = config->GetEnthalpy_Formation(); // Formation enthalpy [J/kg]
  ionization = config->GetIonization();         // Molecule Ionization

  /*--- Rename variables for convenience ---*/
  RuSI   = UNIVERSAL_GAS_CONSTANT;    // Universal gas constant [J/(mol*K)]
  Ru     = 1000.0*RuSI;               // Universal gas constant [J/(kmol*K)]
  rhoE   = U[nSpecies+nDim];          // Density * energy [J/m3]
  rhoEve = U[nSpecies+nDim+1];        // Density * energy_ve [J/m3]

  /*--- Determine the number of heavy species ---*/
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Assign species & mixture density ---*/
  // Note: if any species densities are < 0, these values are re-assigned
  //       in the primitive AND conserved vectors to ensure positive density
  V[RHO_INDEX] = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    if (U[iSpecies] < 0.0) {
      V[RHOS_INDEX+iSpecies] = 1E-20;
      U[iSpecies]            = 1E-20;
      //nonPhys                = true;
    } else
      V[RHOS_INDEX+iSpecies] = U[iSpecies];
    V[RHO_INDEX]            += U[iSpecies];
  }

  /*--- Assign mixture velocity ---*/
  sqvel = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    V[VEL_INDEX+iDim] = U[nSpecies+iDim]/V[RHO_INDEX];
    sqvel            += V[VEL_INDEX+iDim]*V[VEL_INDEX+iDim];
  }

  /*--- Translational-Rotational Temperature ---*/

  // Rename for convenience
  rho = V[RHO_INDEX];

  // Determine properties of the mixture at the current state
  rhoE_f   = 0.0;
  rhoE_ref = 0.0;
  rhoCvtr  = 0.0;
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    rhoCvtr  += U[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies];
    rhoE_ref += U[iSpecies] * (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies] * Tref[iSpecies];
    rhoE_f   += U[iSpecies] * (hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies]);
  }

  // Calculate T-R temperature
  V[T_INDEX] = (rhoE - rhoEve - rhoE_f + rhoE_ref - 0.5*rho*sqvel) / rhoCvtr;
  V[RHOCVTR_INDEX] = rhoCvtr;

  // Determine if the temperature lies within the acceptable range
  if (V[T_INDEX] < Tmin) {
    V[T_INDEX] = Tmin;
    nonPhys = true;
    errT    = true;
  } else if (V[T_INDEX] > Tmax){
    V[T_INDEX] = Tmax;
    nonPhys = true;
    errT    = true;
  }


  /*--- Vibrational-Electronic Temperature ---*/

  // Check for non-physical solutions
  rhoEve_min = 0.0;
  rhoEve_max = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    rhoEve_min += U[iSpecies]*CalcEve(config, Tvemin, iSpecies);
    rhoEve_max += U[iSpecies]*CalcEve(config, Tvemax, iSpecies);
  }
  if (rhoEve < rhoEve_min) {
    errTve       = true;
    nonPhys      = true;
    V[TVE_INDEX] = Tvemin;
  } else if (rhoEve > rhoEve_max) {
    errTve       = true;
    nonPhys      = true;
    V[TVE_INDEX] = Tvemax;
  } else {

    /*--- Execute a Newton-Raphson root-finding method to find Tve ---*/
    // Initialize to the translational-rotational temperature
    Tve   = V[T_INDEX];

    // Execute the root-finding method
    NRconvg = false;
    //    for (iIter = 0; iIter < maxNIter; iIter++) {
    //      rhoEve_t = 0.0;
    //      rhoCvve  = 0.0;
    //      for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    //        val_eves[iSpecies]  = CalcEve(config, Tve, iSpecies);
    //        val_Cvves[iSpecies] = CalcCvve(Tve, config, iSpecies);
    //        rhoEve_t += U[iSpecies]*val_eves[iSpecies];
    //        rhoCvve  += U[iSpecies]*val_Cvves[iSpecies];
    //      }
    //
    //      // Find the root
    //      f  = U[nSpecies+nDim+1] - rhoEve_t;
    //      df = -rhoCvve;
    //      Tve2 = Tve - (f/df)*scale;
    //
    //      // Check for nonphysical steps
    //      if ((Tve2 < Tvemin) || (Tve2 > Tvemax))
    //        break;
    ////      if (Tve2 < Tvemin)
    ////        Tve2 = Tvemin;
    ////      else if (Tve2 > Tvemax)
    ////        Tve2 = Tvemax;
    //
    //      // Check for convergence
    //      if (fabs(Tve2 - Tve) < NRtol) {
    //        NRconvg = true;
    //        break;
    //      } else {
    //        Tve = Tve2;
    //      }
    //    }

    // If the Newton-Raphson method has converged, assign the value of Tve.
    // Otherwise, execute a bisection root-finding method
    if (NRconvg)
      V[TVE_INDEX] = Tve;
    else {

      // Assign the bounds
      Tve_o = Tvemin;
      Tve2  = Tvemax;

      // Execute the root-finding method
      Bconvg = false;
      for (iIter = 0; iIter < maxBIter; iIter++) {
        Tve      = (Tve_o+Tve2)/2.0;
        rhoEve_t = 0.0;
        for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
          val_eves[iSpecies] = CalcEve(config, Tve, iSpecies);
          rhoEve_t          += U[iSpecies] * val_eves[iSpecies];
        }

        if (fabs(rhoEve_t - U[nSpecies+nDim+1]) < Btol) {
          V[TVE_INDEX] = Tve;
          for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
            val_Cvves[iSpecies] = CalcCvve(Tve, config, iSpecies);
          Bconvg = true;
          break;
        } else {
          if (rhoEve_t > rhoEve) Tve2 = Tve;
          else                  Tve_o = Tve;
        }
      }

      // If absolutely no convergence, then assign to the TR temperature
      if (!Bconvg) {
        V[TVE_INDEX] = V[T_INDEX];
        for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
          val_eves[iSpecies]  = CalcEve(config, V[TVE_INDEX], iSpecies);
          val_Cvves[iSpecies] = CalcCvve(V[TVE_INDEX], config, iSpecies);
        }
      }
    }
  }

  /*--- Set mixture rhoCvve ---*/
  rhoCvve = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    rhoCvve += U[iSpecies]*val_Cvves[iSpecies];
  V[RHOCVVE_INDEX] = rhoCvve;


  /*--- If there are clipped temperatures, correct the energy terms ---*/
  //  if (errT) {
  //    U[nSpecies+nDim]   = rhoCvtr*V[T_INDEX] + rhoCvve*V[TVE_INDEX] + rhoE_f
  //                       - rhoE_ref + 0.5*rho*sqvel;
  //  }
  //  if (errTve) {
  //    U[nSpecies+nDim]   = rhoCvtr*V[T_INDEX] + rhoCvve*V[TVE_INDEX] + rhoE_f
  //                       - rhoE_ref + 0.5*rho*sqvel;
  //    U[nSpecies+nDim+1] = rhoCvve*V[TVE_INDEX];
  //  }


  /*--- Pressure ---*/
  V[P_INDEX] = 0.0;
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++)
    V[P_INDEX] += U[iSpecies] * Ru/Ms[iSpecies] * V[T_INDEX];
  for (iEl = 0; iEl < nEl; iEl++)
    V[P_INDEX] += U[nSpecies-1] * Ru/Ms[nSpecies-1] * V[TVE_INDEX];

  if (V[P_INDEX] < 0.0) {
    V[P_INDEX] = 1E-20;
    nonPhys = true;
  }

  /*--- Partial derivatives of pressure and temperature ---*/
  CalcdPdU(  V, val_eves, config, val_dPdU  );
  CalcdTdU(  V, config, val_dTdU  );
  CalcdTvedU(V, val_eves, config, val_dTvedU);


  /*--- Sound speed ---*/
  radical2 = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    radical2 += V[RHOS_INDEX+iSpecies]/V[RHO_INDEX] * val_dPdU[iSpecies];
  for (iDim = 0; iDim < nDim; iDim++)
    radical2 += V[VEL_INDEX+iDim]*val_dPdU[nSpecies+iDim];
  radical2 += (U[nSpecies+nDim]+V[P_INDEX])/V[RHO_INDEX] * val_dPdU[nSpecies+nDim];
  radical2 += U[nSpecies+nDim+1]/V[RHO_INDEX] * val_dPdU[nSpecies+nDim+1];
  V[A_INDEX] = sqrt(radical2);

  if (radical2 < 0.0) {
    nonPhys = true;
    V[A_INDEX] = EPS;
  }


  /*--- Enthalpy ---*/
  V[H_INDEX] = (U[nSpecies+nDim] + V[P_INDEX])/V[RHO_INDEX];

  return nonPhys;
}

void CTNE2EulerVariable::Prim2ConsVar(CConfig *config, su2double *V, su2double *U) {
  unsigned short iDim, iEl, iSpecies, nEl, nHeavy;
  unsigned short *nElStates;
  su2double Ru, RuSI, Tve, T, sqvel, rhoE, rhoEve, Ef, Ev, Ee, rhos, rhoCvtr, num, denom;
  su2double *thetav, *Ms, *xi, *hf, *Tref;
  su2double **thetae, **g;

  /*--- Determine the number of heavy species ---*/
  ionization = config->GetIonization();
  if (ionization) { nHeavy = nSpecies-1; nEl = 1; }
  else            { nHeavy = nSpecies;   nEl = 0; }

  /*--- Load variables from the config class --*/
  xi        = config->GetRotationModes();      // Rotational modes of energy storage
  Ms        = config->GetMolar_Mass();         // Species molar mass
  thetav    = config->GetCharVibTemp();        // Species characteristic vib. temperature [K]
  thetae    = config->GetCharElTemp();         // Characteristic electron temperature [K]
  g         = config->GetElDegeneracy();       // Degeneracy of electron states
  nElStates = config->GetnElStates();          // Number of electron states
  Tref      = config->GetRefTemperature();     // Thermodynamic reference temperature [K]
  hf        = config->GetEnthalpy_Formation(); // Formation enthalpy [J/kg]

  /*--- Rename & initialize for convenience ---*/
  RuSI    = UNIVERSAL_GAS_CONSTANT;         // Universal gas constant [J/(mol*K)] (SI units)
  Ru      = 1000.0*RuSI;                    // Universal gas constant [J/(kmol*K)]
  Tve     = V[TVE_INDEX];                   // Vibrational temperature [K]
  T       = V[T_INDEX];                     // Translational-rotational temperature [K]
  sqvel   = 0.0;                            // Velocity^2 [m2/s2]
  rhoE    = 0.0;                            // Mixture total energy per mass [J/kg]
  rhoEve  = 0.0;                            // Mixture vib-el energy per mass [J/kg]
  denom   = 0.0;
  rhoCvtr = 0.0;

  for (iDim = 0; iDim < nDim; iDim++)
    sqvel += V[VEL_INDEX+iDim]*V[VEL_INDEX+iDim];

  /*--- Set species density ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    U[iSpecies] = V[RHOS_INDEX+iSpecies];

  /*--- Set momentum ---*/
  for (iDim = 0; iDim < nDim; iDim++)
    U[nSpecies+iDim] = V[RHO_INDEX]*V[VEL_INDEX+iDim];

  /*--- Set the total energy ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    rhos = U[iSpecies];

    // Species formation energy
    Ef = hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies];

    // Species vibrational energy
    if (thetav[iSpecies] != 0.0)
      Ev = Ru/Ms[iSpecies] * thetav[iSpecies] / (exp(thetav[iSpecies]/Tve)-1.0);
    else
      Ev = 0.0;

    // Species electronic energy
    num = 0.0;
    denom = g[iSpecies][0] * exp(thetae[iSpecies][0]/Tve);
    for (iEl = 1; iEl < nElStates[iSpecies]; iEl++) {
      num   += g[iSpecies][iEl] * thetae[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
      denom += g[iSpecies][iEl] * exp(-thetae[iSpecies][iEl]/Tve);
    }
    Ee = Ru/Ms[iSpecies] * (num/denom);

    // Mixture total energy
    rhoE += rhos * ((3.0/2.0+xi[iSpecies]/2.0) * Ru/Ms[iSpecies] * (T-Tref[iSpecies])
                    + Ev + Ee + Ef + 0.5*sqvel);

    // Mixture vibrational-electronic energy
    rhoEve += rhos * (Ev + Ee);
  }
  for (iSpecies = 0; iSpecies < nEl; iSpecies++) {
    // Species formation energy
    Ef = hf[nSpecies-1] - Ru/Ms[nSpecies-1] * Tref[nSpecies-1];

    // Electron t-r mode contributes to mixture vib-el energy
    rhoEve += (3.0/2.0) * Ru/Ms[nSpecies-1] * (Tve - Tref[nSpecies-1]);
  }

  /*--- Set energies ---*/
  U[nSpecies+nDim]   = rhoE;
  U[nSpecies+nDim+1] = rhoEve;

  return;
}

/*!
 * \brief Set Gradient of the primitive variables from conserved
 */
bool CTNE2EulerVariable::GradCons2GradPrimVar(CConfig *config, su2double *U,
                                              su2double *V, su2double **GradU,
                                              su2double **GradV) {

  unsigned short iSpecies, iEl, iDim, jDim, iVar, nHeavy, *nElStates;
  su2double rho, rhoCvtr, rhoCvve, T, Tve, eve, ef, eref, RuSI, Ru;
  su2double Cvvs, Cves, dCvvs, dCves;
  su2double thoTve, exptv;
  su2double An1, Bd1, Bn1, Bn2, Bn3, Bn4, A, B;
  su2double *rhou, *Grhou2;
  su2double *xi, *Ms, *Tref, *hf, *thetav;
  su2double **thetae, **g;

  /*--- Conserved & primitive vector layout ---*/
  // U:  [rho1, ..., rhoNs, rhou, rhov, rhow, rhoe, rhoeve]^T
  // V: [rho1, ..., rhoNs, T, Tve, u, v, w, P, rho, h, a, rhoCvtr, rhoCvve]^T

  /*--- Allocate arrays ---*/
  Grhou2 = new su2double[nDim];
  rhou   = new su2double[nDim];

  /*--- Determine number of heavy-particle species ---*/
  if (config->GetIonization()) nHeavy = nSpecies-1;
  else                         nHeavy = nSpecies;

  /*--- Rename for convenience ---*/
  rho     = V[RHO_INDEX];
  rhoCvtr = V[RHOCVTR_INDEX];
  rhoCvve = V[RHOCVVE_INDEX];
  T       = V[T_INDEX];
  Tve     = V[TVE_INDEX];
  xi      = config->GetRotationModes();
  Ms      = config->GetMolar_Mass();
  Tref    = config->GetRefTemperature();
  hf      = config->GetEnthalpy_Formation();
  RuSI    = UNIVERSAL_GAS_CONSTANT;
  Ru      = 1000.0*RuSI;
  thetav  = config->GetCharVibTemp();
  g       = config->GetElDegeneracy();
  thetae  = config->GetCharElTemp();
  nElStates = config->GetnElStates();

  for (iDim = 0; iDim < nDim; iDim++)
    for (iVar = 0; iVar < nPrimVarGrad; iVar++)
      GradV[iVar][iDim] = 0.0;

  for (iDim = 0; iDim < nDim; iDim++) {

    Grhou2[iDim] = 0.0;

    /*--- Species density ---*/
    for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
      GradV[RHOS_INDEX+iSpecies][iDim] = GradU[iSpecies][iDim];
      GradV[RHO_INDEX][iDim]          += GradU[iSpecies][iDim];
    }

    /*--- Velocity ---*/
    for (jDim = 0; jDim < nDim; jDim++) {
      GradV[VEL_INDEX+jDim][iDim] = (rho*GradU[nSpecies+jDim][iDim] -
          rhou[jDim]*GradV[RHO_INDEX][iDim]) / (rho*rho);
    }

    /*--- Specific Heat (T-R) ---*/
    GradV[RHOCVTR_INDEX][iDim] = 0.0;
    for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
      GradV[RHOCVTR_INDEX][iDim] += GradU[iSpecies][iDim]*(3.0+xi[iSpecies])/2.0 *Ru/Ms[iSpecies];

    /*--- Temperature ---*/
    // Calculate the gradient of rho*u^2
    for (jDim = 0; jDim < nDim; jDim++) {
      Grhou2[iDim] += 2.0/rho*(rhou[jDim]*GradU[nSpecies+jDim][iDim]) -
          GradV[RHO_INDEX][iDim]/(rho*rho)*(GradU[nSpecies+jDim][iDim]*
          GradU[nSpecies+jDim][iDim]);
    }
    // Calculate baseline GradT
    GradV[T_INDEX][iDim] = 1.0/rhoCvtr*(GradU[nSpecies+nDim][iDim] -
        GradU[nSpecies+nDim+1][iDim] -
        0.5*Grhou2[iDim] -
        T*GradV[RHOCVTR_INDEX][iDim]);
    // Subtract formation/reference energies
    for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
      eref = (3.0/2.0 + xi[iSpecies]/2.0) * Ru/Ms[iSpecies] * Tref[iSpecies];
      ef   = (hf[iSpecies] - Ru/Ms[iSpecies]*Tref[iSpecies]);
      GradV[T_INDEX][iDim] -= 1.0/rhoCvtr*(GradU[iSpecies][iDim]*(ef+eref));
    }

    /*--- Vibrational-electronic temperature ---*/
    GradV[TVE_INDEX][iDim] = GradU[nSpecies+nDim+1][iDim]/rhoCvve;
    for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
      eve = CalcEve(config, V[TVE_INDEX], iSpecies);
      GradV[TVE_INDEX][iDim] -= GradU[iSpecies][iDim]*eve;
    }

    /*--- Pressure ---*/
    for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
      GradV[P_INDEX][iDim] += GradU[iSpecies][iDim]*Ru/Ms[iSpecies]*T +
          U[iSpecies]*Ru/Ms[iSpecies]*GradV[T_INDEX][iDim];
    }

    /*--- Enthalpy ---*/
    GradV[H_INDEX][iDim] = rho*(GradU[nSpecies+nDim][iDim] + GradV[P_INDEX][iDim]) -
        (U[nSpecies+nDim]+V[P_INDEX])*GradV[RHO_INDEX][iDim] / (rho*rho);

    /*--- Specific Heat (V-E) ---*/
    for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
      // Vibrational energy specific heat
      if (thetav[iSpecies] != 0) {
        Cvvs = Ru/Ms[iSpecies]*(thetav[iSpecies]*thetav[iSpecies]/(Tve*Tve))*
            exp(thetav[iSpecies]/Tve)/((exp(thetav[iSpecies]/Tve)-1)*
                                       (exp(thetav[iSpecies]/Tve)-1));
        dCvvs = (-2/Tve - thetav[iSpecies]/(Tve*Tve) +
                 2*thetav[iSpecies]*exp(thetav[iSpecies]/Tve)/
                 (Tve*Tve*(exp(thetav[iSpecies]/Tve)-1)))*Cvvs;
      } else {
        Cvvs = 0.0;
        dCvvs = 0.0;
      }


      // Electronic energy specific heat
      An1 = 0.0;
      Bn1 = 0.0;
      Bn2 = g[iSpecies][0]*thetae[iSpecies][0]/(Tve*Tve)*exp(-thetae[iSpecies][0]/Tve);
      Bn3 = g[iSpecies][0]*(thetae[iSpecies][0]*thetae[iSpecies][0]/
          (Tve*Tve*Tve*Tve))*exp(-thetae[iSpecies][0]/Tve);
      Bn4 = 0.0;
      Bd1 = g[iSpecies][0]*exp(-thetae[iSpecies][0]/Tve);
      for (iEl = 1; iEl < nElStates[iSpecies]; iEl++) {
        thoTve = thetae[iSpecies][iEl]/Tve;
        exptv = exp(-thetae[iSpecies][iEl]/Tve);

        An1 += g[iSpecies][iEl]*thoTve*thoTve*exptv;
        Bn1 += g[iSpecies][iEl]*thetae[iSpecies][iEl]*exptv;
        Bn2 += g[iSpecies][iEl]*thoTve/Tve*exptv;
        Bn3 += g[iSpecies][iEl]*thoTve*thoTve/(Tve*Tve)*exptv;
        Bn4 += g[iSpecies][iEl]*thoTve*thoTve*thoTve/Tve*exptv;
        Bd1 += g[iSpecies][iEl]*exptv;
      }
      A = An1/Bd1;
      B = Bn1*Bn2/(Bd1*Bd1);
      Cves = Ru/Ms[iSpecies]*(A-B);

      dCves = Ru/Ms[iSpecies]*(-2.0/Tve*(A-B) - 2*Bn2/Bd1*(A-B) -
                               Bn1*Bn3/(Bd1*Bd1) + Bn4/Bd1);

      GradV[RHOCVVE_INDEX][iDim] += V[RHOS_INDEX+iSpecies]*(dCvvs+dCves)*GradV[TVE_INDEX][iDim] +
          GradV[RHOS_INDEX+iSpecies][iDim]*(Cvvs+Cves);

    }
  }

  delete [] Grhou2;
  delete [] rhou;
  return false;
}


void CTNE2EulerVariable::SetPrimVar_Gradient(CConfig *config) {

  /*--- Use built-in method on TNE2 variable global types ---*/
  GradCons2GradPrimVar(config, Solution, Primitive,
                       Gradient, Gradient_Primitive);

}

CTNE2NSVariable::CTNE2NSVariable(void) : CTNE2EulerVariable() { }

CTNE2NSVariable::CTNE2NSVariable(unsigned short val_ndim,
                                 unsigned short val_nvar,
                                 unsigned short val_nprimvar,
                                 unsigned short val_nprimvargrad,
                                 CConfig *config) : CTNE2EulerVariable(val_ndim,
                                                                       val_nvar,
                                                                       val_nprimvar,
                                                                       val_nprimvargrad,
                                                                       config) {

  Temperature_Ref = config->GetTemperature_Ref();
  Viscosity_Ref   = config->GetViscosity_Ref();
  Viscosity_Inf   = config->GetViscosity_FreeStreamND();
  Prandtl_Lam     = config->GetPrandtl_Lam();
  DiffusionCoeff  = new su2double[nSpecies];
  Dij = new su2double*[nSpecies];
  for (unsigned short iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    Dij[iSpecies] = new su2double[nSpecies];
}

CTNE2NSVariable::CTNE2NSVariable(su2double val_pressure, su2double *val_massfrac,
                                 su2double *val_mach, su2double val_temperature,
                                 su2double val_temperature_ve,
                                 unsigned short val_ndim,
                                 unsigned short val_nvar,
                                 unsigned short val_nvarprim,
                                 unsigned short val_nvarprimgrad,
                                 CConfig *config) : CTNE2EulerVariable(val_pressure,
                                                                       val_massfrac,
                                                                       val_mach,
                                                                       val_temperature,
                                                                       val_temperature_ve,
                                                                       val_ndim,
                                                                       val_nvar,
                                                                       val_nvarprim,
                                                                       val_nvarprimgrad,
                                                                       config) {

  Temperature_Ref = config->GetTemperature_Ref();
  Viscosity_Ref   = config->GetViscosity_Ref();
  Viscosity_Inf   = config->GetViscosity_FreeStreamND();
  Prandtl_Lam     = config->GetPrandtl_Lam();
  DiffusionCoeff  = new su2double[nSpecies];
  Dij = new su2double*[nSpecies];
  for (unsigned short iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    Dij[iSpecies] = new su2double[nSpecies];
}

CTNE2NSVariable::CTNE2NSVariable(su2double *val_solution, unsigned short val_ndim,
                                 unsigned short val_nvar,
                                 unsigned short val_nprimvar,
                                 unsigned short val_nprimvargrad,
                                 CConfig *config) : CTNE2EulerVariable(val_solution,
                                                                       val_ndim,
                                                                       val_nvar,
                                                                       val_nprimvar,
                                                                       val_nprimvargrad,
                                                                       config) {
  Temperature_Ref = config->GetTemperature_Ref();
  Viscosity_Ref   = config->GetViscosity_Ref();
  Viscosity_Inf   = config->GetViscosity_FreeStreamND();
  Prandtl_Lam     = config->GetPrandtl_Lam();
  DiffusionCoeff  = new su2double[nSpecies];
  Dij = new su2double*[nSpecies];
  for (unsigned short iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    Dij[iSpecies] = new su2double[nSpecies];
}

CTNE2NSVariable::~CTNE2NSVariable(void) {
  delete [] DiffusionCoeff;
  for (unsigned short iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    delete [] Dij[iSpecies];
  delete [] Dij;
}

void CTNE2NSVariable::SetDiffusionCoeff_GuptaYos(CConfig *config) {

  unsigned short iSpecies, jSpecies, nHeavy, nEl;
  su2double rho, T, Tve, P;
  su2double *Ms, Mi, Mj, pi, RuSI, Ru, kb, gam_i, gam_j, gam_t, Theta_v;
  su2double denom, d1_ij, D_ij;
  su2double ***Omega00, Omega_ij;

  /*--- Acquire gas parameters from CConfig ---*/
  Omega00 = config->GetCollisionIntegral00();
  Ms      = config->GetMolar_Mass();
  if (ionization) {nHeavy = nSpecies-1;  nEl = 1;}
  else            {nHeavy = nSpecies;    nEl = 0;}

  /*--- Rename for convenience ---*/
  rho  = Primitive[RHO_INDEX];
  T    = Primitive[T_INDEX];
  Tve  = Primitive[TVE_INDEX];
  P    = Primitive[P_INDEX];
  pi   = PI_NUMBER;
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;
  kb   = BOLTZMANN_CONSTANT;

  /*--- Calculate mixture gas constant ---*/
  gam_t = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    gam_t += Primitive[RHOS_INDEX+iSpecies] / (rho*Ms[iSpecies]);
  }

  /*--- Mixture thermal conductivity via Gupta-Yos approximation ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {

    /*--- Initialize the species diffusion coefficient ---*/
    DiffusionCoeff[iSpecies] = 0.0;

    /*--- Calculate molar concentration ---*/
    Mi      = Ms[iSpecies];
    gam_i   = Primitive[RHOS_INDEX+iSpecies] / (rho*Mi);
    Theta_v = config->GetCharVibTemp(iSpecies);

    denom = 0.0;
    for (jSpecies = 0; jSpecies < nHeavy; jSpecies++) {
      if (jSpecies != iSpecies) {
        Mj    = Ms[jSpecies];
        gam_j = Primitive[RHOS_INDEX+iSpecies] / (rho*Mj);

        /*--- Calculate the Omega^(0,0)_ij collision cross section ---*/
        Omega_ij = 1E-20 * Omega00[iSpecies][jSpecies][3]
            * pow(T, Omega00[iSpecies][jSpecies][0]*log(T)*log(T)
            + Omega00[iSpecies][jSpecies][1]*log(T)
            + Omega00[iSpecies][jSpecies][2]);

        /*--- Calculate "delta1_ij" ---*/
        d1_ij = 8.0/3.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*T*(Mi+Mj))) * Omega_ij;

        /*--- Calculate heavy-particle binary diffusion coefficient ---*/
        D_ij = kb*T/(P*d1_ij);
        denom += gam_j/D_ij;
      }
    }

    if (ionization) {
      jSpecies = nSpecies-1;
      Mj       = config->GetMolar_Mass(jSpecies);
      gam_j    = Primitive[RHOS_INDEX+iSpecies] / (rho*Mj);

      /*--- Calculate the Omega^(0,0)_ij collision cross section ---*/
      Omega_ij = 1E-20 * Omega00[iSpecies][jSpecies][3]
          * pow(Tve, Omega00[iSpecies][jSpecies][0]*log(Tve)*log(Tve)
          + Omega00[iSpecies][jSpecies][1]*log(Tve)
          + Omega00[iSpecies][jSpecies][2]);

      /*--- Calculate "delta1_ij" ---*/
      d1_ij = 8.0/3.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*Tve*(Mi+Mj))) * Omega_ij;
    }

    /*--- Assign species diffusion coefficient ---*/
    DiffusionCoeff[iSpecies] = gam_t*gam_t*Mi*(1-Mi*gam_i) / denom;
  }
  if (ionization) {
    iSpecies = nSpecies-1;
    /*--- Initialize the species diffusion coefficient ---*/
    DiffusionCoeff[iSpecies] = 0.0;

    /*--- Calculate molar concentration ---*/
    Mi      = Ms[iSpecies];
    gam_i   = Primitive[RHOS_INDEX+iSpecies] / (rho*Mi);

    denom = 0.0;
    for (jSpecies = 0; jSpecies < nHeavy; jSpecies++) {
      if (iSpecies != jSpecies) {
        Mj    = config->GetMolar_Mass(jSpecies);
        gam_j = Primitive[RHOS_INDEX+iSpecies] / (rho*Mj);

        /*--- Calculate the Omega^(0,0)_ij collision cross section ---*/
        Omega_ij = 1E-20 * Omega00[iSpecies][jSpecies][3]
            * pow(Tve, Omega00[iSpecies][jSpecies][0]*log(Tve)*log(Tve)
            + Omega00[iSpecies][jSpecies][1]*log(Tve)
            + Omega00[iSpecies][jSpecies][2]);

        /*--- Calculate "delta1_ij" ---*/
        d1_ij = 8.0/3.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*Tve*(Mi+Mj))) * Omega_ij;

        /*--- Calculate heavy-particle binary diffusion coefficient ---*/
        D_ij = kb*Tve/(P*d1_ij);
        denom += gam_j/D_ij;
      }
    }
    DiffusionCoeff[iSpecies] = gam_t*gam_t*Ms[iSpecies]*(1-Ms[iSpecies]*gam_i)
        / denom;
  }
  //  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
  //    DiffusionCoeff[iSpecies] = 0.0;
}

void CTNE2NSVariable::SetLaminarViscosity_GuptaYos(CConfig *config) {

  unsigned short iSpecies, jSpecies, nHeavy, nEl;
  su2double rho, T, Tve;
  su2double *Ms, Mi, Mj, pi, Ru, RuSI, Na, gam_i, gam_j, denom;
  su2double ***Omega11, Omega_ij, d2_ij;

  /*--- Acquire gas parameters from CConfig ---*/
  Omega11 = config->GetCollisionIntegral11();
  Ms      = config->GetMolar_Mass();
  if (ionization) {nHeavy = nSpecies-1;  nEl = 1;}
  else            {nHeavy = nSpecies;    nEl = 0;}

  /*--- Rename for convenience ---*/
  rho  = Primitive[RHO_INDEX];
  T    = Primitive[T_INDEX];
  Tve  = Primitive[TVE_INDEX];
  pi   = PI_NUMBER;
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;
  Na   = AVOGAD_CONSTANT;

  LaminarViscosity = 0.0;

  /*--- Mixture viscosity via Gupta-Yos approximation ---*/
  for (iSpecies = 0; iSpecies < nHeavy; iSpecies++) {
    denom = 0.0;

    /*--- Calculate molar concentration ---*/
    Mi    = Ms[iSpecies];
    gam_i = Primitive[RHOS_INDEX+iSpecies] / (rho*Mi);
    for (jSpecies = 0; jSpecies < nHeavy; jSpecies++) {
      Mj    = Ms[jSpecies];
      gam_j = Primitive[RHOS_INDEX+jSpecies] / (rho*Mj);

      /*--- Calculate "delta" quantities ---*/
      Omega_ij = 1E-20 * Omega11[iSpecies][jSpecies][3]
          * pow(T, Omega11[iSpecies][jSpecies][0]*log(T)*log(T)
          + Omega11[iSpecies][jSpecies][1]*log(T)
          + Omega11[iSpecies][jSpecies][2]);
      d2_ij = 16.0/5.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*T*(Mi+Mj))) * Omega_ij;

      /*--- Add to denominator of viscosity ---*/
      denom += gam_j*d2_ij;
    }
    if (ionization) {
      jSpecies = nSpecies-1;
      Mj    = Ms[jSpecies];
      gam_j = Primitive[RHOS_INDEX+jSpecies] / (rho*Mj);

      /*--- Calculate "delta" quantities ---*/
      Omega_ij = 1E-20 * Omega11[iSpecies][jSpecies][3]
          * pow(Tve, Omega11[iSpecies][jSpecies][0]*log(Tve)*log(Tve)
          + Omega11[iSpecies][jSpecies][1]*log(Tve)
          + Omega11[iSpecies][jSpecies][2]);
      d2_ij = 16.0/5.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*Tve*(Mi+Mj))) * Omega_ij;

      denom += gam_j*d2_ij;
    }

    /*--- Calculate species laminar viscosity ---*/
    LaminarViscosity += (Mi/Na * gam_i) / denom;
  }
  if (ionization) {
    iSpecies = nSpecies-1;
    denom = 0.0;

    /*--- Calculate molar concentration ---*/
    Mi    = Ms[iSpecies];
    gam_i = Primitive[RHOS_INDEX+iSpecies] / (rho*Mi);
    for (jSpecies = 0; jSpecies < nSpecies; jSpecies++) {
      Mj    = Ms[jSpecies];
      gam_j = Primitive[RHOS_INDEX+jSpecies] / (rho*Mj);

      /*--- Calculate "delta" quantities ---*/
      Omega_ij = 1E-20 * Omega11[iSpecies][jSpecies][3]
          * pow(Tve, Omega11[iSpecies][jSpecies][0]*log(Tve)*log(Tve)
          + Omega11[iSpecies][jSpecies][1]*log(Tve)
          + Omega11[iSpecies][jSpecies][2]);
      d2_ij = 16.0/5.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*Tve*(Mi+Mj))) * Omega_ij;

      /*--- Add to denominator of viscosity ---*/
      denom += gam_j*d2_ij;
    }
    LaminarViscosity += (Mi/Na * gam_i) / denom;
  }
}

void CTNE2NSVariable ::SetThermalConductivity_GuptaYos(CConfig *config) {
  unsigned short iSpecies, jSpecies, nHeavy, nEl;
  su2double rho, T, Tve, Cvve;
  su2double *xi, *Ms, Mi, Mj, mi, mj, pi, R, RuSI, Ru, Na, kb, gam_i, gam_j, Theta_v;
  su2double denom_t, denom_r, d1_ij, d2_ij, a_ij;
  su2double ***Omega00, ***Omega11, Omega_ij;

  if (ionization) {
    cout << "SetThermalConductivity: NEEDS REVISION w/ IONIZATION" << endl;
    exit(1);
  }

  /*--- Acquire gas parameters from CConfig ---*/
  Omega00 = config->GetCollisionIntegral00();
  Omega11 = config->GetCollisionIntegral11();
  Ms      = config->GetMolar_Mass();
  xi      = config->GetRotationModes();
  if (ionization) {nHeavy = nSpecies-1;  nEl = 1;}
  else            {nHeavy = nSpecies;    nEl = 0;}

  /*--- Rename for convenience ---*/
  rho  = Primitive[RHO_INDEX];
  T    = Primitive[T_INDEX];
  Tve  = Primitive[TVE_INDEX];
  Cvve = Primitive[RHOCVVE_INDEX]/rho;
  pi   = PI_NUMBER;
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;
  Na   = AVOGAD_CONSTANT;
  kb   = BOLTZMANN_CONSTANT;

  /*--- Calculate mixture gas constant ---*/
  R = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    R += Ru * Primitive[RHOS_INDEX+iSpecies]/rho;
  }

  /*--- Mixture thermal conductivity via Gupta-Yos approximation ---*/
  ThermalCond    = 0.0;
  ThermalCond_ve = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {

    /*--- Calculate molar concentration ---*/
    Mi      = Ms[iSpecies];
    mi      = Mi/Na;
    gam_i   = Primitive[RHOS_INDEX+iSpecies] / (rho*Mi);
    Theta_v = config->GetCharVibTemp(iSpecies);

    denom_t = 0.0;
    denom_r = 0.0;
    for (jSpecies = 0; jSpecies < nSpecies; jSpecies++) {
      Mj    = config->GetMolar_Mass(jSpecies);
      mj    = Mj/Na;
      gam_j = Primitive[RHOS_INDEX+iSpecies] / (rho*Mj);

      a_ij = 1.0 + (1.0 - mi/mj)*(0.45 - 2.54*mi/mj) / ((1.0 + mi/mj)*(1.0 + mi/mj));

      /*--- Calculate the Omega^(0,0)_ij collision cross section ---*/
      Omega_ij = 1E-20 * Omega00[iSpecies][jSpecies][3]
          * pow(T, Omega00[iSpecies][jSpecies][0]*log(T)*log(T)
          + Omega00[iSpecies][jSpecies][1]*log(T)
          + Omega00[iSpecies][jSpecies][2]);

      /*--- Calculate "delta1_ij" ---*/
      d1_ij = 8.0/3.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*T*(Mi+Mj))) * Omega_ij;

      /*--- Calculate the Omega^(1,1)_ij collision cross section ---*/
      Omega_ij = 1E-20 * Omega11[iSpecies][jSpecies][3]
          * pow(T, Omega11[iSpecies][jSpecies][0]*log(T)*log(T)
          + Omega11[iSpecies][jSpecies][1]*log(T)
          + Omega11[iSpecies][jSpecies][2]);

      /*--- Calculate "delta2_ij" ---*/
      d2_ij = 16.0/5.0 * sqrt((2.0*Mi*Mj) / (pi*Ru*T*(Mi+Mj))) * Omega_ij;

      denom_t += a_ij*gam_j*d2_ij;
      denom_r += gam_j*d1_ij;
    }

    /*--- Translational contribution to thermal conductivity ---*/
    ThermalCond    += (15.0/4.0)*kb*gam_i/denom_t;

    /*--- Translational contribution to thermal conductivity ---*/
    if (xi[iSpecies] != 0.0)
      ThermalCond  += kb*gam_i/denom_r;

    /*--- Vibrational-electronic contribution to thermal conductivity ---*/
    ThermalCond_ve += kb*Cvve/R*gam_i / denom_r;
  }
}

void CTNE2NSVariable::SetTransportCoefficients_WBE(CConfig *config) {

  unsigned short iSpecies, jSpecies;
  su2double *Ms, Mi, Mj, M;
  su2double rho, T, Tve, RuSI, Ru, *xi;
  su2double Xs[nSpecies], conc;
  su2double Cves;
  su2double phis[nSpecies], mus[nSpecies], ks[nSpecies], kves[nSpecies];
  su2double denom, tmp1, tmp2;
  su2double **Blottner;
  su2double ***Omega00, Omega_ij;

  /*--- Rename for convenience ---*/
  rho  = Primitive[RHO_INDEX];
  T    = Primitive[T_INDEX];
  Tve  = Primitive[TVE_INDEX];
  Ms   = config->GetMolar_Mass();
  xi   = config->GetRotationModes();
  RuSI = UNIVERSAL_GAS_CONSTANT;
  Ru   = 1000.0*RuSI;

  /*--- Acquire collision integral information ---*/
  Omega00 = config->GetCollisionIntegral00();

  /*--- Calculate species mole fraction ---*/
  conc = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    Xs[iSpecies] = Primitive[RHOS_INDEX+iSpecies]/Ms[iSpecies];
    conc        += Xs[iSpecies];
  }
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    Xs[iSpecies] = Xs[iSpecies]/conc;

  /*--- Calculate mixture molar mass (kg/mol) ---*/
  // Note: Species molar masses stored as kg/kmol, need 1E-3 conversion
  M = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    M += Ms[iSpecies]*Xs[iSpecies];
  M = M*1E-3;


  /*---+++                  +++---*/
  /*--- Diffusion coefficients ---*/
  /*---+++                  +++---*/

  /*--- Solve for binary diffusion coefficients ---*/
  // Note: Dij = Dji, so only loop through req'd indices
  // Note: Correlation requires kg/mol, hence 1E-3 conversion from kg/kmol
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    Mi = Ms[iSpecies]*1E-3;
    for (jSpecies = iSpecies; jSpecies < nSpecies; jSpecies++) {
      Mj = Ms[jSpecies]*1E-3;

      /*--- Calculate the Omega^(0,0)_ij collision cross section ---*/
      Omega_ij = 1E-20/PI_NUMBER * Omega00[iSpecies][jSpecies][3]
          * pow(T, Omega00[iSpecies][jSpecies][0]*log(T)*log(T)
          +  Omega00[iSpecies][jSpecies][1]*log(T)
          +  Omega00[iSpecies][jSpecies][2]);

      Dij[iSpecies][jSpecies] = 7.1613E-25*M*sqrt(T*(1/Mi+1/Mj))/(rho*Omega_ij);
      Dij[jSpecies][iSpecies] = 7.1613E-25*M*sqrt(T*(1/Mi+1/Mj))/(rho*Omega_ij);
    }
  }

  /*--- Calculate species-mixture diffusion coefficient --*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    denom = 0.0;
    for (jSpecies = 0; jSpecies < nSpecies; jSpecies++) {
      if (jSpecies != iSpecies) {
        denom += Xs[jSpecies]/Dij[iSpecies][jSpecies];
      }
    }
    DiffusionCoeff[iSpecies] = (1-Xs[iSpecies])/denom;
    //    DiffusionCoeff[iSpecies] = 0.0;
  }


  /*---+++             +++---*/
  /*--- Laminar viscosity ---*/
  /*---+++             +++---*/

  /*--- Get Blottner coefficients ---*/
  Blottner = config->GetBlottnerCoeff();

  /*--- Use Blottner's curve fits for species viscosity ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    mus[iSpecies] = 0.1*exp((Blottner[iSpecies][0]*log(T)  +
                            Blottner[iSpecies][1])*log(T) +
        Blottner[iSpecies][2]);

  /*--- Determine species 'phi' value for Blottner model ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    phis[iSpecies] = 0.0;
    for (jSpecies = 0; jSpecies < nSpecies; jSpecies++) {
      tmp1 = 1.0 + sqrt(mus[iSpecies]/mus[jSpecies])*pow(Ms[jSpecies]/Ms[iSpecies], 0.25);
      tmp2 = sqrt(8.0*(1.0+Ms[iSpecies]/Ms[jSpecies]));
      phis[iSpecies] += Xs[jSpecies]*tmp1*tmp1/tmp2;
    }
  }

  /*--- Calculate mixture laminar viscosity ---*/
  LaminarViscosity = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
    LaminarViscosity += Xs[iSpecies]*mus[iSpecies]/phis[iSpecies];


  /*---+++                +++---*/
  /*--- Thermal conductivity ---*/
  /*---+++                +++---*/

  /*--- Determine species tr & ve conductivities ---*/
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    Cves = CalcCvve(Tve, config, iSpecies);
    ks[iSpecies] = mus[iSpecies]*(15.0/4.0 + xi[iSpecies]/2.0)*Ru/Ms[iSpecies];
    kves[iSpecies] = mus[iSpecies]*Cves;
  }

  /*--- Calculate mixture tr & ve conductivities ---*/
  ThermalCond    = 0.0;
  ThermalCond_ve = 0.0;
  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++) {
    ThermalCond    += Xs[iSpecies]*ks[iSpecies]/phis[iSpecies];
    ThermalCond_ve += Xs[iSpecies]*kves[iSpecies]/phis[iSpecies];
  }
}

bool CTNE2NSVariable::SetVorticity(void) {
  su2double u_y = Gradient_Primitive[VEL_INDEX][1];
  su2double v_x = Gradient_Primitive[VEL_INDEX+1][0];
  su2double u_z = 0.0;
  su2double v_z = 0.0;
  su2double w_x = 0.0;
  su2double w_y = 0.0;

  if (nDim == 3) {
    u_z = Gradient_Primitive[VEL_INDEX][2];
    v_z = Gradient_Primitive[VEL_INDEX+1][2];
    w_x = Gradient_Primitive[VEL_INDEX+2][0];
    w_y = Gradient_Primitive[VEL_INDEX+2][1];
  }

  Vorticity[0] = w_y-v_z;
  Vorticity[1] = -(w_x-u_z);
  Vorticity[2] = v_x-u_y;

  return false;
}

bool CTNE2NSVariable::SetPrimVar_Compressible(CConfig *config) {

  bool nonPhys, bkup;
  unsigned short iVar;

  nonPhys = Cons2PrimVar(config, Solution, Primitive, dPdU, dTdU, dTvedU, eves, Cvves);
  if (nonPhys) {
    for (iVar = 0; iVar < nVar; iVar++)
      Solution[iVar] = Solution_Old[iVar];
    bkup = Cons2PrimVar(config, Solution, Primitive, dPdU, dTdU, dTvedU, eves, Cvves);
  }

  SetVelocity2();

  switch (config->GetKind_TransCoeffModel()) {
  case WBE:
    SetTransportCoefficients_WBE(config);
    break;
  case GUPTAYOS:
    SetDiffusionCoeff_GuptaYos(config);
    SetLaminarViscosity_GuptaYos(config);              // Requires temperature computation.
    SetThermalConductivity_GuptaYos(config);
    break;
  }

  //  unsigned short iSpecies;
  //  cout << "T: " << Primitive[T_INDEX] << endl;
  //  cout << "Tve: " << Primitive[TVE_INDEX] << endl;
  //
  //  SetTransportCoefficients_WBE(config);
  //  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
  //    cout << "Ds[" << iSpecies << "]: " << DiffusionCoeff[iSpecies] << endl;
  //  cout << "mu: " << LaminarViscosity << endl;
  //  cout << "ktr: " << ThermalCond << endl;
  //  cout << "kve: " << ThermalCond_ve << endl;
  //
  //  cout << endl << endl;
  //
  //  SetDiffusionCoeff_GuptaYos(config);
  //  SetLaminarViscosity_GuptaYos(config);              // Requires temperature computation.
  //  SetThermalConductivity_GuptaYos(config);
  //  for (iSpecies = 0; iSpecies < nSpecies; iSpecies++)
  //    cout << "Ds[" << iSpecies << "]: " << DiffusionCoeff[iSpecies] << endl;
  //  cout << "mu: " << LaminarViscosity << endl;
  //  cout << "ktr: " << ThermalCond << endl;
  //  cout << "kve: " << ThermalCond_ve << endl;
  //  cin.get();

  return nonPhys;
}

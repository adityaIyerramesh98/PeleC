#ifndef PROB_H_
#define PROB_H_

#include <AMReX_Print.H>
#include <AMReX_Geometry.H>
#include <AMReX_FArrayBox.H>
#include <AMReX_REAL.H>
#include <AMReX_GpuMemory.H>
#include <AMReX_ParmParse.H>

#ifdef PELEC_USE_MASA
#include <masa.h>
using namespace MASA;
#endif

#include "mechanism.H"

#include "PeleC.H"
#include "IndexDefines.H"
#include "PelePhysics.H"

#include "Constants.H"
#include "Tagging.H"
#include "ProblemDerive.H"
#include "prob_parm.H"

AMREX_GPU_DEVICE
AMREX_FORCE_INLINE
void
pc_initdata(
  int i,
  int j,
  int k,
  amrex::Array4<amrex::Real> const& state,
  amrex::GeometryData const& geomdata,
  ProbParmDevice const& /*prob_parm*/)
{
  // Geometry
  const amrex::Real* prob_lo = geomdata.ProbLo();
  // const amrex::Real* prob_hi = geomdata.ProbHi();
  const amrex::Real* dx = geomdata.CellSize();
  masa_set_param("deltabar", dx[0]);
  const amrex::Real x = prob_lo[0] + (i + 0.5) * dx[0];
  const amrex::Real y = prob_lo[1] + (j + 0.5) * dx[1];
  const amrex::Real z = prob_lo[2] + (k + 0.5) * dx[2];

  // Get state from MASA
  const amrex::Real rho = masa_eval_3d_exact_rho(x, y, z);
  const amrex::Real u[3] = {
    masa_eval_3d_exact_u(x, y, z), masa_eval_3d_exact_v(x, y, z),
    masa_eval_3d_exact_w(x, y, z)};
  const amrex::Real p = masa_eval_3d_exact_p(x, y, z);
  amrex::Real massfrac[NUM_SPECIES] = {1.0};

  amrex::Real eint, T;
  auto eos = pele::physics::PhysicsType::eos();
  eos.RYP2E(rho, massfrac, p, eint);
  eos.EY2T(eint, massfrac, T);

  // Set the state
  state(i, j, k, URHO) = rho;
  state(i, j, k, UMX) = rho * u[0];
  state(i, j, k, UMY) = rho * u[1];
  state(i, j, k, UMZ) = rho * u[2];
  state(i, j, k, UEINT) = rho * eint;
  state(i, j, k, UEDEN) =
    rho * (eint + 0.5 * (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
  state(i, j, k, UTEMP) = T;
  for (int n = 0; n < NUM_SPECIES; n++)
    state(i, j, k, UFS + n) = rho * massfrac[n];
}

AMREX_GPU_DEVICE
AMREX_FORCE_INLINE
void
bcnormal(
  const amrex::Real x[AMREX_SPACEDIM],
  const amrex::Real* /*s_int[NVAR]*/,
  amrex::Real s_ext[NVAR],
  const int /*idir*/,
  const int /*sgn*/,
  const amrex::Real /*time*/,
  amrex::GeometryData const& /*geomdata*/,
  ProbParmDevice const& /*prob_parm*/)
{
  const amrex::Real rho = masa_eval_3d_exact_rho(x[0], x[1], x[2]);
  const amrex::Real u[3] = {
    masa_eval_3d_exact_u(x[0], x[1], x[2]),
    masa_eval_3d_exact_v(x[0], x[1], x[2]),
    masa_eval_3d_exact_w(x[0], x[1], x[2])};
  const amrex::Real p = masa_eval_3d_exact_p(x[0], x[1], x[2]);
  amrex::Real massfrac[NUM_SPECIES] = {1.0};

  amrex::Real eint, T;
  auto eos = pele::physics::PhysicsType::eos();
  eos.RYP2E(rho, massfrac, p, eint);
  eos.EY2T(eint, massfrac, T);

  // Set the external state
  s_ext[URHO] = rho;
  s_ext[UMX] = rho * u[0];
  s_ext[UMY] = rho * u[1];
  s_ext[UMZ] = rho * u[2];
  s_ext[UEINT] = rho * eint;
  s_ext[UEDEN] = rho * (eint + 0.5 * (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]));
  s_ext[UTEMP] = T;
  for (int n = 0; n < NUM_SPECIES; n++)
    s_ext[UFS + n] = rho * massfrac[n];
}

void pc_prob_close();

using ProblemTags = EmptyProbTagStruct;
using ProblemDerives = EmptyProbDeriveStruct;

#endif

#include "../include/source.h"
#include "../include/config.h"
#include "../include/jacobian.h"
#include "../include/kokkos_abstractions.h"
#include "../include/lagrange_poly.h"
#include "../include/specfem_mpi.h"
#include "../include/utils.h"

void specfem::sources::source::check_locations(const type_real xmin,
                                               const type_real xmax,
                                               const type_real zmin,
                                               const type_real zmax,
                                               const specfem::MPI::MPI *mpi) {
  specfem::utilities::check_locations(this->get_x(), this->get_z(), xmin, xmax,
                                      zmin, zmax, mpi);
}

void specfem::sources::force::locate(
    const specfem::HostView3d<int> ibool,
    const specfem::HostView2d<type_real> coord,
    const specfem::HostMirror1d<type_real> xigll,
    const specfem::HostMirror1d<type_real> zigll, const int nproc,
    const specfem::HostView2d<type_real> coorg,
    const specfem::HostView2d<int> knods, const int npgeo,
    const specfem::HostView1d<element_type> ispec_type,
    const specfem::MPI::MPI *mpi) {
  std::tie(this->xi, this->gamma, this->ispec, this->islice) =
      specfem::utilities::locate(ibool, coord, xigll, zigll, nproc,
                                 this->get_x(), this->get_z(), coorg, knods,
                                 npgeo, mpi);
  if (this->islice == mpi->get_rank())
    this->el_type = ispec_type(ispec);
}

void specfem::sources::moment_tensor::locate(
    const specfem::HostView3d<int> ibool,
    const specfem::HostView2d<type_real> coord,
    const specfem::HostMirror1d<type_real> xigll,
    const specfem::HostMirror1d<type_real> zigll, const int nproc,
    const specfem::HostView2d<type_real> coorg,
    const specfem::HostView2d<int> knods, const int npgeo,
    const specfem::HostView1d<element_type> ispec_type,
    const specfem::MPI::MPI *mpi) {
  std::tie(this->xi, this->gamma, this->ispec, this->islice) =
      specfem::utilities::locate(ibool, coord, xigll, zigll, nproc,
                                 this->get_x(), this->get_z(), coorg, knods,
                                 npgeo, mpi);

  if (this->islice == mpi->get_rank()) {
    if (ispec_type(ispec) != elastic)
      throw std::runtime_error(
          "Found a Moment-tensor source in acoustic/poroelastic element");
  }
  int ngnod = coorg.extent(1);
  this->s_coorg = specfem::HostView2d<type_real>(
      "specfem::sources::moment_tensor::s_coorg", ndim, ngnod);

  // Store s_coorg for better caching
  for (int in = 0; in < ngnod; in++) {
    this->s_coorg(0, in) = coorg(0, knods(in, ispec));
    this->s_coorg(1, in) = coorg(1, knods(in, ispec));
  }

  return;
}

void specfem::sources::force::compute_source_array(
    quadrature::quadrature &quadx, quadrature::quadrature &quadz,
    specfem::HostView3d<type_real> source_array) {

  type_real xi = this->xi;
  type_real gamma = this->gamma;
  type_real angle = this->angle;
  element_type el_type = this->el_type;
  wave_type wave = this->wave;

  auto [hxis, hpxis] = Lagrange::compute_lagrange_interpolants(
      xi, quadx.get_N(), quadx.get_hxi());
  auto [hgammas, hpgammas] = Lagrange::compute_lagrange_interpolants(
      gamma, quadz.get_N(), quadz.get_hxi());

  int nquadx = quadx.get_N();
  int nquadz = quadz.get_N();

  type_real hlagrange;

  for (int i = 0; i < nquadx; i++) {
    for (int j = 0; j < nquadz; j++) {
      hlagrange = hxis(i) * hgammas(j);

      if (el_type == acoustic || (el_type == elastic && wave == sh)) {
        source_array(j, i, 0) = hlagrange;
        source_array(j, i, 1) = hlagrange;
      } else if ((el_type == elastic && wave == p_sv) ||
                 el_type == poroelastic) {
        source_array(j, i, 0) = sin(angle) * hlagrange;
        source_array(j, i, 1) = -1.0 * cos(angle) * hlagrange;
      }
    }
  }
};

void specfem::sources::moment_tensor::compute_source_array(
    quadrature::quadrature &quadx, quadrature::quadrature &quadz,
    specfem::HostView3d<type_real> source_array) {

  type_real xi = this->xi;
  type_real gamma = this->gamma;
  type_real Mxx = this->Mxx;
  type_real Mxz = this->Mxz;
  type_real Mzz = this->Mzz;
  auto s_coorg = this->s_coorg;
  int ngnod = s_coorg.extent(1);

  auto [hxis, hpxis] = Lagrange::compute_lagrange_interpolants(
      xi, quadx.get_N(), quadx.get_hxi());
  auto [hgammas, hpgammas] = Lagrange::compute_lagrange_interpolants(
      gamma, quadz.get_N(), quadz.get_hxi());

  int nquadx = quadx.get_N();
  int nquadz = quadz.get_N();

  type_real hlagrange;
  type_real dxis_dx = 0;
  type_real dxis_dz = 0;
  type_real dgammas_dx = 0;
  type_real dgammas_dz = 0;

  for (int i = 0; i < nquadx; i++) {
    for (int j = 0; j < nquadz; j++) {
      type_real xil = quadx.get_hxi()(i);
      type_real gammal = quadz.get_hxi()(j);
      auto [xix, xiz, gammax, gammaz] =
          jacobian::compute_inverted_derivatives(s_coorg, ngnod, xil, gammal);
      hlagrange = hxis(i) * hgammas(j);
      dxis_dx += hlagrange * xix;
      dxis_dz += hlagrange * xiz;
      dgammas_dx += hlagrange * gammax;
      dgammas_dz += hlagrange * gammaz;
    }
  }

  for (int i = 0; i < nquadx; i++) {
    for (int j = 0; j < nquadz; j++) {
      type_real dsrc_dx = (hpxis(i) * dxis_dx) * hgammas(j) +
                          hxis(i) * (hpgammas(j) * dgammas_dx);
      type_real dsrc_dz = (hpxis(i) * dxis_dz) * hgammas(j) +
                          hxis(i) * (hpgammas(j) * dgammas_dz);

      source_array(j, i, 0) += Mxx * dsrc_dx + Mxz * dsrc_dz;
      source_array(j, i, 1) += Mxz * dsrc_dx + Mzz * dsrc_dz;
    }
  }
};

void specfem::sources::force::check_locations(const type_real xmin,
                                              const type_real xmax,
                                              const type_real zmin,
                                              const type_real zmax,
                                              const specfem::MPI::MPI *mpi) {

  specfem::utilities::check_locations(this->get_x(), this->get_z(), xmin, xmax,
                                      zmin, zmax, mpi);
  mpi->cout(
      "ToDo:: Need to implement a check to see if acoustic source lies on an "
      "acoustic surface");
}

void specfem::sources::force::compute_stf(){};

void specfem::sources::moment_tensor::compute_stf(){};

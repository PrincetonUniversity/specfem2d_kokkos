#pragma once

#include "enumerations/medium.hpp"
#include "kokkos_abstractions.h"
#include "point/coordinates.hpp"
#include "point/kernels.hpp"
#include <Kokkos_Core.hpp>

namespace specfem {
namespace compute {
namespace impl {
namespace kernels {

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag>
class kernels_container;

template <>
class kernels_container<specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::isotropic> {
public:
  constexpr static auto value_type = specfem::element::medium_tag::elastic;
  constexpr static auto property_type =
      specfem::element::property_tag::isotropic;
  int nspec;
  int ngllz;
  int ngllx;

  using ViewType = Kokkos::View<type_real ***, Kokkos::LayoutLeft,
                                Kokkos::DefaultExecutionSpace>;

  ViewType rho;
  ViewType::HostMirror h_rho;
  ViewType mu;
  ViewType::HostMirror h_mu;
  ViewType kappa;
  ViewType::HostMirror h_kappa;
  ViewType rhop;
  ViewType::HostMirror h_rhop;
  ViewType alpha;
  ViewType::HostMirror h_alpha;
  ViewType beta;
  ViewType::HostMirror h_beta;

  kernels_container() = default;

  kernels_container(const int nspec, const int ngllz, const int ngllx)
      : nspec(nspec), ngllz(ngllz), ngllx(ngllx),
        rho("specfem::compute::impl::kernels::elastic::rho", nspec, ngllz,
            ngllx),
        mu("specfem::compute::impl::kernels::elastic::mu", nspec, ngllz, ngllx),
        kappa("specfem::compute::impl::kernels::elastic::kappa", nspec, ngllz,
              ngllx),
        rhop("specfem::compute::impl::kernels::elastic::rhop", nspec, ngllz,
             ngllx),
        alpha("specfem::compute::impl::kernels::elastic::alpha", nspec, ngllz,
              ngllx),
        beta("specfem::compute::impl::kernels::elastic::beta", nspec, ngllz,
             ngllx),
        h_rho(Kokkos::create_mirror_view(rho)),
        h_mu(Kokkos::create_mirror_view(mu)),
        h_kappa(Kokkos::create_mirror_view(kappa)),
        h_rhop(Kokkos::create_mirror_view(rhop)),
        h_alpha(Kokkos::create_mirror_view(alpha)),
        h_beta(Kokkos::create_mirror_view(beta)) {

    initialize();
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = rho(ispec, iz, ix);
    kernels.mu = mu(ispec, iz, ix);
    kernels.kappa = kappa(ispec, iz, ix);
    kernels.rhop = rhop(ispec, iz, ix);
    kernels.alpha = alpha(ispec, iz, ix);
    kernels.beta = beta(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.mu)
        .copy_from(&mu(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_from(&kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_from(&rhop(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_from(&alpha(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.beta)
        .copy_from(&beta(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void
  load_host_kernels(specfem::point::index<PointKernelType::dimension> &index,
                    PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = h_rho(ispec, iz, ix);
    kernels.mu = h_mu(ispec, iz, ix);
    kernels.kappa = h_kappa(ispec, iz, ix);
    kernels.rhop = h_rhop(ispec, iz, ix);
    kernels.alpha = h_alpha(ispec, iz, ix);
    kernels.beta = h_beta(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void load_host_kernels(
      specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.mu)
        .copy_from(&h_mu(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_from(&h_kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_from(&h_rhop(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_from(&h_alpha(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.beta)
        .copy_from(&h_beta(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) = kernels.rho;
    mu(ispec, iz, ix) = kernels.mu;
    kappa(ispec, iz, ix) = kernels.kappa;
    rhop(ispec, iz, ix) = kernels.rhop;
    alpha(ispec, iz, ix) = kernels.alpha;
    beta(ispec, iz, ix) = kernels.beta;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.mu)
        .copy_to(&mu(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_to(&kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_to(&rhop(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_to(&alpha(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.beta)
        .copy_to(&beta(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) = kernels.rho;
    h_mu(ispec, iz, ix) = kernels.mu;
    h_kappa(ispec, iz, ix) = kernels.kappa;
    h_rhop(ispec, iz, ix) = kernels.rhop;
    h_alpha(ispec, iz, ix) = kernels.alpha;
    h_beta(ispec, iz, ix) = kernels.beta;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.mu)
        .copy_to(&h_mu(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_to(&h_kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_to(&h_rhop(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_to(&h_alpha(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.beta)
        .copy_to(&h_beta(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) += kernels.rho;
    mu(ispec, iz, ix) += kernels.mu;
    kappa(ispec, iz, ix) += kernels.kappa;
    rhop(ispec, iz, ix) += kernels.rhop;
    alpha(ispec, iz, ix) += kernels.alpha;
    beta(ispec, iz, ix) += kernels.beta;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&mu(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.mu;
    Kokkos::Experimental::where(mask, lhs).copy_to(&mu(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&kappa(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.kappa;
    Kokkos::Experimental::where(mask, lhs).copy_to(&kappa(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&rhop(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rhop;
    Kokkos::Experimental::where(mask, lhs).copy_to(&rhop(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&alpha(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.alpha;
    Kokkos::Experimental::where(mask, lhs).copy_to(&alpha(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&beta(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.beta;
    Kokkos::Experimental::where(mask, lhs).copy_to(&beta(ispec, iz, ix),
                                                   tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) += kernels.rho;
    h_mu(ispec, iz, ix) += kernels.mu;
    h_kappa(ispec, iz, ix) += kernels.kappa;
    h_rhop(ispec, iz, ix) += kernels.rhop;
    h_alpha(ispec, iz, ix) += kernels.alpha;
    h_beta(ispec, iz, ix) += kernels.beta;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_mu(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.mu;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_mu(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_kappa(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.kappa;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_kappa(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_rhop(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rhop;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_rhop(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_alpha(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.alpha;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_alpha(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_beta(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.beta;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_beta(ispec, iz, ix),
                                                   tag_type());
  }

  void copy_to_host() {
    Kokkos::deep_copy(h_rho, rho);
    Kokkos::deep_copy(h_mu, mu);
    Kokkos::deep_copy(h_kappa, kappa);
    Kokkos::deep_copy(h_rhop, rhop);
    Kokkos::deep_copy(h_alpha, alpha);
    Kokkos::deep_copy(h_beta, beta);
  }

  void copy_to_device() {
    Kokkos::deep_copy(rho, h_rho);
    Kokkos::deep_copy(mu, h_mu);
    Kokkos::deep_copy(kappa, h_kappa);
    Kokkos::deep_copy(rhop, h_rhop);
    Kokkos::deep_copy(alpha, h_alpha);
    Kokkos::deep_copy(beta, h_beta);
  }

  void initialize() {
    Kokkos::parallel_for(
        "specfem::compute::impl::kernels::elastic::initialize",
        Kokkos::MDRangePolicy<Kokkos::Rank<3> >({ 0, 0, 0 },
                                                { nspec, ngllz, ngllx }),
        KOKKOS_CLASS_LAMBDA(const int ispec, const int iz, const int ix) {
          this->rho(ispec, iz, ix) = 0.0;
          this->mu(ispec, iz, ix) = 0.0;
          this->kappa(ispec, iz, ix) = 0.0;
          this->rhop(ispec, iz, ix) = 0.0;
          this->alpha(ispec, iz, ix) = 0.0;
          this->beta(ispec, iz, ix) = 0.0;
        });
  }
};

template <>
class kernels_container<specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::anisotropic> {
public:
  constexpr static auto value_type = specfem::element::medium_tag::elastic;
  constexpr static auto property_type =
      specfem::element::property_tag::anisotropic;
  int nspec;
  int ngllz;
  int ngllx;

  using ViewType = Kokkos::View<type_real ***, Kokkos::LayoutLeft,
                                Kokkos::DefaultExecutionSpace>;

  ViewType rho;
  ViewType::HostMirror h_rho;
  ViewType c11;
  ViewType::HostMirror h_c11;
  ViewType c13;
  ViewType::HostMirror h_c13;
  ViewType c15;
  ViewType::HostMirror h_c15;
  ViewType c33;
  ViewType::HostMirror h_c33;
  ViewType c35;
  ViewType::HostMirror h_c35;
  ViewType c55;
  ViewType::HostMirror h_c55;
  ViewType c12;
  ViewType::HostMirror h_c12;
  ViewType c23;
  ViewType::HostMirror h_c23;
  ViewType c25;
  ViewType::HostMirror h_c25;

  kernels_container() = default;

  kernels_container(const int nspec, const int ngllz, const int ngllx)
      : nspec(nspec), ngllz(ngllz), ngllx(ngllx),
        rho("specfem::compute::properties::rho", nspec, ngllz, ngllx),
        h_rho(Kokkos::create_mirror_view(rho)),
        c11("specfem::compute::properties::c11", nspec, ngllz, ngllx),
        h_c11(Kokkos::create_mirror_view(c11)),
        c12("specfem::compute::properties::c12", nspec, ngllz, ngllx),
        h_c12(Kokkos::create_mirror_view(c12)),
        c13("specfem::compute::properties::c13", nspec, ngllz, ngllx),
        h_c13(Kokkos::create_mirror_view(c13)),
        c15("specfem::compute::properties::c15", nspec, ngllz, ngllx),
        h_c15(Kokkos::create_mirror_view(c15)),
        c33("specfem::compute::properties::c33", nspec, ngllz, ngllx),
        h_c33(Kokkos::create_mirror_view(c33)),
        c35("specfem::compute::properties::c35", nspec, ngllz, ngllx),
        h_c35(Kokkos::create_mirror_view(c35)),
        c55("specfem::compute::properties::c55", nspec, ngllz, ngllx),
        h_c55(Kokkos::create_mirror_view(c55)),
        c23("specfem::compute::properties::c23", nspec, ngllz, ngllx),
        h_c23(Kokkos::create_mirror_view(c23)),
        c25("specfem::compute::properties::c25", nspec, ngllz, ngllx),
        h_c25(Kokkos::create_mirror_view(c25)) {

    initialize();
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = rho(ispec, iz, ix);
    kernels.c11 = c11(ispec, iz, ix);
    kernels.c12 = c12(ispec, iz, ix);
    kernels.c13 = c13(ispec, iz, ix);
    kernels.c15 = c15(ispec, iz, ix);
    kernels.c33 = c33(ispec, iz, ix);
    kernels.c35 = c35(ispec, iz, ix);
    kernels.c55 = c55(ispec, iz, ix);
    kernels.c23 = c23(ispec, iz, ix);
    kernels.c25 = c25(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c11)
        .copy_from(&c11(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c12)
        .copy_from(&c12(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c13)
        .copy_from(&c13(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c15)
        .copy_from(&c15(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c33)
        .copy_from(&c33(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c35)
        .copy_from(&c35(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c55)
        .copy_from(&c55(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c23)
        .copy_from(&c23(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c25)
        .copy_from(&c25(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void
  load_host_kernels(specfem::point::index<PointKernelType::dimension> &index,
                    PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = h_rho(ispec, iz, ix);
    kernels.c11 = h_c11(ispec, iz, ix);
    kernels.c12 = h_c12(ispec, iz, ix);
    kernels.c13 = h_c13(ispec, iz, ix);
    kernels.c15 = h_c15(ispec, iz, ix);
    kernels.c33 = h_c33(ispec, iz, ix);
    kernels.c35 = h_c35(ispec, iz, ix);
    kernels.c55 = h_c55(ispec, iz, ix);
    kernels.c23 = h_c23(ispec, iz, ix);
    kernels.c25 = h_c25(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void load_host_kernels(
      specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c11)
        .copy_from(&h_c11(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c13)
        .copy_from(&h_c13(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c15)
        .copy_from(&h_c15(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c33)
        .copy_from(&h_c33(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c35)
        .copy_from(&h_c35(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c55)
        .copy_from(&h_c55(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c23)
        .copy_from(&h_c23(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c25)
        .copy_from(&h_c25(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) = kernels.rho;
    c11(ispec, iz, ix) = kernels.c11;
    c13(ispec, iz, ix) = kernels.c13;
    c15(ispec, iz, ix) = kernels.c15;
    c33(ispec, iz, ix) = kernels.c33;
    c35(ispec, iz, ix) = kernels.c35;
    c55(ispec, iz, ix) = kernels.c55;
    c23(ispec, iz, ix) = kernels.c23;
    c25(ispec, iz, ix) = kernels.c25;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c11)
        .copy_to(&c11(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c13)
        .copy_to(&c13(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c15)
        .copy_to(&c15(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c33)
        .copy_to(&c33(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c35)
        .copy_to(&c35(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c55)
        .copy_to(&c55(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c23)
        .copy_to(&c23(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c25)
        .copy_to(&c25(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) = kernels.rho;
    h_c11(ispec, iz, ix) = kernels.c11;
    h_c13(ispec, iz, ix) = kernels.c13;
    h_c15(ispec, iz, ix) = kernels.c15;
    h_c33(ispec, iz, ix) = kernels.c33;
    h_c35(ispec, iz, ix) = kernels.c35;
    h_c55(ispec, iz, ix) = kernels.c55;
    h_c23(ispec, iz, ix) = kernels.c23;
    h_c25(ispec, iz, ix) = kernels.c25;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;
    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c11)
        .copy_to(&h_c11(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c13)
        .copy_to(&h_c13(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c15)
        .copy_to(&h_c15(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c33)
        .copy_to(&h_c33(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c35)
        .copy_to(&h_c35(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c55)
        .copy_to(&h_c55(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c23)
        .copy_to(&h_c23(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.c25)
        .copy_to(&h_c25(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) += kernels.rho;
    c11(ispec, iz, ix) += kernels.c11;
    c12(ispec, iz, ix) += kernels.c12;
    c13(ispec, iz, ix) += kernels.c13;
    c15(ispec, iz, ix) += kernels.c15;
    c33(ispec, iz, ix) += kernels.c33;
    c35(ispec, iz, ix) += kernels.c35;
    c55(ispec, iz, ix) += kernels.c55;
    c23(ispec, iz, ix) += kernels.c23;
    c25(ispec, iz, ix) += kernels.c25;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c11(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c11;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c11(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c12(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c12;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c12(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c13(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c13;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c13(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c15(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c15;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c15(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c33(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c33;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c33(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c35(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c35;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c35(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c55(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c55;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c55(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c23(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c23;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c23(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&c25(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c25;
    Kokkos::Experimental::where(mask, lhs).copy_to(&c25(ispec, iz, ix),
                                                   tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) += kernels.rho;
    h_c11(ispec, iz, ix) += kernels.c11;
    h_c12(ispec, iz, ix) += kernels.c12;
    h_c13(ispec, iz, ix) += kernels.c13;
    h_c15(ispec, iz, ix) += kernels.c15;
    h_c33(ispec, iz, ix) += kernels.c33;
    h_c35(ispec, iz, ix) += kernels.c35;
    h_c55(ispec, iz, ix) += kernels.c55;
    h_c23(ispec, iz, ix) += kernels.c23;
    h_c25(ispec, iz, ix) += kernels.c25;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c11(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c11;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c11(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c12(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c12;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c12(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c13(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c13;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c13(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c15(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c15;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c15(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c33(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c33;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c33(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c35(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c35;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c35(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c55(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c55;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c55(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c23(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c23;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c23(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_c25(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.c25;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_c25(ispec, iz, ix),
                                                   tag_type());
  }

  void copy_to_host() {
    Kokkos::deep_copy(h_rho, rho);
    Kokkos::deep_copy(h_c11, c11);
    Kokkos::deep_copy(h_c13, c13);
    Kokkos::deep_copy(h_c15, c15);
    Kokkos::deep_copy(h_c33, c33);
    Kokkos::deep_copy(h_c35, c35);
    Kokkos::deep_copy(h_c55, c55);
    Kokkos::deep_copy(h_c23, c23);
    Kokkos::deep_copy(h_c25, c25);
  }

  void copy_to_device() {
    Kokkos::deep_copy(rho, h_rho);
    Kokkos::deep_copy(c11, h_c11);
    Kokkos::deep_copy(c13, h_c13);
    Kokkos::deep_copy(c15, h_c15);
    Kokkos::deep_copy(c33, h_c33);
    Kokkos::deep_copy(c35, h_c35);
    Kokkos::deep_copy(c55, h_c55);
    Kokkos::deep_copy(c23, h_c23);
    Kokkos::deep_copy(c25, h_c25);
  }

  void initialize() {
    Kokkos::parallel_for(
        "specfem::compute::impl::kernels::elastic::initialize",
        Kokkos::MDRangePolicy<Kokkos::Rank<3> >({ 0, 0, 0 },
                                                { nspec, ngllz, ngllx }),
        KOKKOS_CLASS_LAMBDA(const int ispec, const int iz, const int ix) {
          this->rho(ispec, iz, ix) = 0.0;
          this->c11(ispec, iz, ix) = 0.0;
          this->c12(ispec, iz, ix) = 0.0;
          this->c13(ispec, iz, ix) = 0.0;
          this->c15(ispec, iz, ix) = 0.0;
          this->c33(ispec, iz, ix) = 0.0;
          this->c35(ispec, iz, ix) = 0.0;
          this->c55(ispec, iz, ix) = 0.0;
          this->c23(ispec, iz, ix) = 0.0;
          this->c25(ispec, iz, ix) = 0.0;
        });
  }
};

template <>
class kernels_container<specfem::element::medium_tag::acoustic,
                        specfem::element::property_tag::isotropic> {
public:
  constexpr static auto value_type = specfem::element::medium_tag::acoustic;
  constexpr static auto property_type =
      specfem::element::property_tag::isotropic;
  int nspec;
  int ngllz;
  int ngllx;

  using ViewType = Kokkos::View<type_real ***, Kokkos::LayoutLeft,
                                Kokkos::DefaultExecutionSpace>;
  ViewType rho;
  ViewType::HostMirror h_rho;
  ViewType kappa;
  ViewType::HostMirror h_kappa;
  ViewType rho_prime;
  ViewType::HostMirror h_rho_prime;
  ViewType alpha;
  ViewType::HostMirror h_alpha;

  kernels_container() = default;

  kernels_container(const int nspec, const int ngllz, const int ngllx)
      : nspec(nspec), ngllz(ngllz), ngllx(ngllx),
        rho("specfem::compute::impl::kernels::acoustic::rho", nspec, ngllz,
            ngllx),
        kappa("specfem::compute::impl::kernels::acoustic::kappa", nspec, ngllz,
              ngllx),
        rho_prime("specfem::compute::impl::kernels::acoustic::rho_prime", nspec,
                  ngllz, ngllx),
        alpha("specfem::compute::impl::kernels::acoustic::alpha", nspec, ngllz,
              ngllx),
        h_rho(Kokkos::create_mirror_view(rho)),
        h_kappa(Kokkos::create_mirror_view(kappa)),
        h_rho_prime(Kokkos::create_mirror_view(rho_prime)),
        h_alpha(Kokkos::create_mirror_view(alpha)) {

    initialize();
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = rho(ispec, iz, ix);
    kernels.kappa = kappa(ispec, iz, ix);
    kernels.rhop = rho_prime(ispec, iz, ix);
    kernels.alpha = alpha(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void load_device_kernels(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_from(&kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_from(&rho_prime(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_from(&alpha(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void load_host_kernels(
      const specfem::point::index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    kernels.rho = h_rho(ispec, iz, ix);
    kernels.kappa = h_kappa(ispec, iz, ix);
    kernels.rhop = h_rho_prime(ispec, iz, ix);
    kernels.alpha = h_alpha(ispec, iz, ix);
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void load_host_kernels(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_from(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_from(&h_kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_from(&h_rho_prime(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_from(&h_alpha(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) = kernels.rho;
    kappa(ispec, iz, ix) = kernels.kappa;
    rho_prime(ispec, iz, ix) = kernels.rhop;
    alpha(ispec, iz, ix) = kernels.alpha;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void update_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_to(&kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_to(&rho_prime(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_to(&alpha(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) = kernels.rho;
    h_kappa(ispec, iz, ix) = kernels.kappa;
    h_rho_prime(ispec, iz, ix) = kernels.rhop;
    h_alpha(ispec, iz, ix) = kernels.alpha;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void update_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    Kokkos::Experimental::where(mask, kernels.rho)
        .copy_to(&h_rho(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.kappa)
        .copy_to(&h_kappa(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.rhop)
        .copy_to(&h_rho_prime(ispec, iz, ix), tag_type());
    Kokkos::Experimental::where(mask, kernels.alpha)
        .copy_to(&h_alpha(ispec, iz, ix), tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    rho(ispec, iz, ix) += kernels.rho;
    kappa(ispec, iz, ix) += kernels.kappa;
    rho_prime(ispec, iz, ix) += kernels.rhop;
    alpha(ispec, iz, ix) += kernels.alpha;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  KOKKOS_INLINE_FUNCTION void add_kernels_on_device(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&kappa(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.kappa;
    Kokkos::Experimental::where(mask, lhs).copy_to(&kappa(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&rho_prime(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rhop;
    Kokkos::Experimental::where(mask, lhs).copy_to(&rho_prime(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&alpha(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.alpha;
    Kokkos::Experimental::where(mask, lhs).copy_to(&alpha(ispec, iz, ix),
                                                   tag_type());
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<!PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    h_rho(ispec, iz, ix) += kernels.rho;
    h_kappa(ispec, iz, ix) += kernels.kappa;
    h_rho_prime(ispec, iz, ix) += kernels.rhop;
    h_alpha(ispec, iz, ix) += kernels.alpha;
  }

  template <
      typename PointKernelType,
      typename std::enable_if_t<PointKernelType::simd::using_simd, int> = 0>
  void add_kernels_on_host(
      const specfem::point::simd_index<PointKernelType::dimension> &index,
      const PointKernelType &kernels) const {

    static_assert(PointKernelType::medium_tag == value_type);
    static_assert(PointKernelType::property_tag == property_type);

    using simd_type = typename PointKernelType::simd::datatype;
    using mask_type = typename PointKernelType::simd::mask_type;
    using tag_type = typename PointKernelType::simd::tag_type;

    mask_type mask([&](std::size_t lane) { return index.mask(lane); });

    const int ispec = index.ispec;
    const int iz = index.iz;
    const int ix = index.ix;

    simd_type lhs;

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_rho(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.rho;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_rho(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_kappa(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.kappa;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_kappa(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(
        &h_rho_prime(ispec, iz, ix), tag_type());
    lhs += kernels.rhop;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_rho_prime(ispec, iz, ix),
                                                   tag_type());

    Kokkos::Experimental::where(mask, lhs).copy_from(&h_alpha(ispec, iz, ix),
                                                     tag_type());
    lhs += kernels.alpha;
    Kokkos::Experimental::where(mask, lhs).copy_to(&h_alpha(ispec, iz, ix),
                                                   tag_type());
  }

  void copy_to_host() {
    Kokkos::deep_copy(h_rho, rho);
    Kokkos::deep_copy(h_kappa, kappa);
    Kokkos::deep_copy(h_rho_prime, rho_prime);
    Kokkos::deep_copy(h_alpha, alpha);
  }

  void copy_to_device() {
    Kokkos::deep_copy(rho, h_rho);
    Kokkos::deep_copy(kappa, h_kappa);
    Kokkos::deep_copy(rho_prime, h_rho_prime);
    Kokkos::deep_copy(alpha, h_alpha);
  }

  void initialize() {
    Kokkos::parallel_for(
        "specfem::compute::impl::kernels::acoustic::initialize",
        Kokkos::MDRangePolicy<Kokkos::Rank<3> >({ 0, 0, 0 },
                                                { nspec, ngllz, ngllx }),
        KOKKOS_CLASS_LAMBDA(const int ispec, const int iz, const int ix) {
          this->rho(ispec, iz, ix) = 0.0;
          this->kappa(ispec, iz, ix) = 0.0;
          this->rho_prime(ispec, iz, ix) = 0.0;
          this->alpha(ispec, iz, ix) = 0.0;
        });
  }
};

} // namespace kernels
} // namespace impl
} // namespace compute
} // namespace specfem

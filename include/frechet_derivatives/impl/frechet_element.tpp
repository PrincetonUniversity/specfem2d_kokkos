#ifndef _FRECHET_DERIVATIVES_IMPL_FRECHLET_ELEMENT_TPP
#define _FRECHET_DERIVATIVES_IMPL_FRECHLET_ELEMENT_TPP

#include "algorithms/gradient.hpp"
#include "element_kernel/acoustic_isotropic.tpp"
#include "element_kernel/elastic_isotropic.tpp"
#include "element_kernel/element_kernel.hpp"
#include "point/field.hpp"
#include <Kokkos_Core.hpp>

template <int NGLL, specfem::dimension::type DimensionType,
          specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag>
specfem::frechet_derivatives::impl::frechet_elements<
    NGLL, DimensionType, MediumTag,
    PropertyTag>::frechet_elements(const specfem::compute::assembly &assembly)
    : adjoint_field(assembly.fields.adjoint),
      backward_field(assembly.fields.backward), kernels(assembly.kernels),
      properties(assembly.properties), quadrature(assembly.mesh.quadratures),
      partial_derivatives(assembly.partial_derivatives) {

  const int nspec = assembly.properties.nspec;

  // Count the number of elements that belong to this FE
  int nelements = 0;
  for (int ispec = 0; ispec < nspec; ++ispec) {
    if ((assembly.properties.h_element_types(ispec) == MediumTag) &&
        (assembly.properties.h_element_property(ispec) == PropertyTag)) {
      nelements++;
    }
  }

  // Allocate memory for the element index
  element_index = specfem::kokkos::DeviceView1d<int>(
      "specfem::frechet_derivatives::frechet_elements::element_index",
      nelements);
  h_element_index = Kokkos::create_mirror_view(element_index);

  // Fill the element index
  int ielement = 0;
  for (int ispec = 0; ispec < nspec; ++ispec) {
    if ((assembly.properties.h_element_types(ispec) == MediumTag) &&
        (assembly.properties.h_element_property(ispec) == PropertyTag)) {
      h_element_index(ielement) = ispec;
      ielement++;
    }
  }

  Kokkos::deep_copy(element_index, h_element_index);

  return;
}

template <int NGLL, specfem::dimension::type DimensionType,
          specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag>
void specfem::frechet_derivatives::impl::frechet_elements<
    NGLL, DimensionType, MediumTag, PropertyTag>::compute(const type_real &dt) {

  const int nelements = element_index.extent(0);

  if (nelements == 0) {
    return;
  }

  constexpr int components =
      specfem::medium::medium<DimensionType, MediumTag>::components;

  using ElementFieldType = specfem::element::field<
      NGLL, DimensionType, MediumTag, specfem::kokkos::DevScratchSpace,
      Kokkos::MemoryTraits<Kokkos::Unmanaged>, true, false, false, false>;

  using ElementQuadratureType = specfem::element::quadrature<
      NGLL, DimensionType, specfem::kokkos::DevScratchSpace,
      Kokkos::MemoryTraits<Kokkos::Unmanaged>, true, false>;

  using AdjointPointFieldType =
      specfem::point::field<DimensionType, MediumTag, false, false, true,
                            false>;

  using BackwardPointFieldType =
      specfem::point::field<DimensionType, MediumTag, true, false, false,
                            false>;

  using PointFieldDerivativesType =
      specfem::point::field_derivatives<DimensionType, MediumTag>;

  int scratch_size =
      2 * ElementFieldType::shmem_size() + ElementQuadratureType::shmem_size();

  Kokkos::parallel_for(
      "specfem::frechet_derivatives::frechet_derivatives::compute",
      Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>(nelements, Kokkos::AUTO,
                                                        Kokkos::AUTO)
          .set_scratch_size(0, Kokkos::PerTeam(scratch_size)),
      KOKKOS_LAMBDA(
          const Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type
              &team) {
        const int ielement = team.league_rank();
        const int ispec = element_index(ielement);

        // Allocate scratch memory
        ElementFieldType adjoint_element_field(team);
        ElementFieldType backward_element_field(team);
        ElementQuadratureType quadrature_element(team);

        // Populate Scratch Views
        specfem::compute::load_on_device(team, ispec, adjoint_field,
                                         adjoint_element_field);
        specfem::compute::load_on_device(team, ispec, backward_field,
                                         backward_element_field);
        specfem::compute::load_on_device(team, quadrature, quadrature_element);

        // Compute gradients
        Kokkos::parallel_for(
            Kokkos::TeamThreadRange(team, NGLL * NGLL), [=](const int &xz) {
              int ix, iz;
              sub2ind(xz, NGLL, iz, ix);
              const specfem::point::index index(ispec, iz, ix);
              const AdjointPointFieldType adjoint_point_field = [&]() {
                AdjointPointFieldType adjoint_point_field;
                specfem::compute::load_on_device(index, adjoint_field,
                                                 adjoint_point_field);
                return adjoint_point_field;
              }();

              const BackwardPointFieldType backward_point_field = [&]() {
                BackwardPointFieldType backward_point_field;
                specfem::compute::load_on_device(index, backward_field,
                                                 backward_point_field);
                return backward_point_field;
              }();

              const auto point_partial_derivatives =
                  [&]() -> specfem::point::partial_derivatives2<false> {
                specfem::point::partial_derivatives2<false>
                    point_partial_derivatives;
                specfem::compute::load_on_device(index, partial_derivatives,
                                                 point_partial_derivatives);
                return point_partial_derivatives;
              }();

              const auto adjoint_point_derivatives = [&]() {
                specfem::kokkos::array_type<type_real, components> dfield_dx;
                specfem::kokkos::array_type<type_real, components> dfield_dz;
                specfem::algorithms::gradient(
                    ix, iz, quadrature_element.hprime_gll,
                    adjoint_element_field.displacement,
                    point_partial_derivatives, dfield_dx, dfield_dz);
                return PointFieldDerivativesType(dfield_dx, dfield_dz);
              }();

              const auto backward_point_derivatives = [&]() {
                specfem::kokkos::array_type<type_real, components> dfield_dx;
                specfem::kokkos::array_type<type_real, components> dfield_dz;
                specfem::algorithms::gradient(
                    ix, iz, quadrature_element.hprime_gll,
                    backward_element_field.displacement,
                    point_partial_derivatives, dfield_dx, dfield_dz);
                return PointFieldDerivativesType(dfield_dx, dfield_dz);
              }();

              const auto point_properties =
                  [&]() -> specfem::point::properties<MediumTag, PropertyTag> {
                specfem::point::properties<MediumTag, PropertyTag>
                    point_properties;
                specfem::compute::load_on_device(index, properties,
                                                 point_properties);
                return point_properties;
              }();

              const auto point_kernel =
                  specfem::frechet_derivatives::impl::element_kernel(
                      point_properties, adjoint_point_field,
                      backward_point_field, adjoint_point_derivatives,
                      backward_point_derivatives, dt);

              const auto point_kernel_debug = [&]() {
                specfem::point::kernels<MediumTag, PropertyTag> point_kernel_debug;
                specfem::compute::load_on_device(index, kernels, point_kernel_debug);
                return point_kernel_debug;
              }();

              specfem::compute::add_on_device(index, point_kernel, kernels);
            });
      });

  return;
}

#endif /* _FRECHET_DERIVATIVES_IMPL_FRECHLET_ELEMENT_TPP */

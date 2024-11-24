#pragma once

#include "enumerations/dimension.hpp"
#include "enumerations/medium.hpp"
#include "enumerations/wavefield.hpp"
#include "point/field_derivatives.hpp"
#include "point/properties.hpp"
#include "point/stress.hpp"
#include <Kokkos_Core.hpp>

namespace specfem {
namespace medium {

template <bool UseSIMD>
KOKKOS_INLINE_FUNCTION specfem::point::stress<
    specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
    UseSIMD>
impl_compute_stress(
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        specfem::element::property_tag::isotropic, UseSIMD> &properties,
    const specfem::point::field_derivatives<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        UseSIMD> &field_derivatives) {

  using datatype =
      typename specfem::datatype::simd<type_real, UseSIMD>::datatype;
  const auto &du = field_derivatives.du;

  datatype sigma_xx, sigma_zz, sigma_xz;

  // P_SV case
  // sigma_xx
  sigma_xx = properties.lambdaplus2mu * du(0, 0) + properties.lambda * du(1, 1);

  // sigma_zz
  sigma_zz = properties.lambdaplus2mu * du(1, 1) + properties.lambda * du(0, 0);

  // sigma_xz
  sigma_xz = properties.mu * (du(0, 1) + du(1, 0));

  specfem::datatype::VectorPointViewType<type_real, 2, 2, UseSIMD> T;

  T(0, 0) = sigma_xx;
  T(0, 1) = sigma_xz;
  T(1, 0) = sigma_xz;
  T(1, 1) = sigma_zz;

  return { T };
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, typename MemberType,
          typename IteratorType, typename ChunkFieldType,
          typename QuadratureType, typename WavefieldViewType,
          std::enable_if_t<
              ((IteratorType::dimension == specfem::dimension::type::dim2) &&
               (MediumTag == specfem::element::medium_tag::elastic) &&
               (PropertyTag == specfem::element::property_tag::isotropic)),
              int> = 0>
KOKKOS_FUNCTION void
impl_compute_wavefield(const MemberType &team, const IteratorType &iterator,
                       const specfem::compute::assembly &assembly,
                       const QuadratureType &quadrature,
                       const ChunkFieldType &field,
                       const specfem::wavefield::component wavefield_component,
                       WavefieldViewType wavefield) {

  using FieldDerivativesType =
      specfem::point::field_derivatives<specfem::dimension::type::dim2,
                                        specfem::element::medium_tag::elastic,
                                        false>;

  using PointPropertyType = specfem::point::properties<
      specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
      specfem::element::property_tag::isotropic, false>;

  const auto &properties = assembly.properties;

  const auto &active_field = [&]() {
    if (wavefield_component == specfem::wavefield::component::displacement) {
      return field.displacement;
    } else if (wavefield_component == specfem::wavefield::component::velocity) {
      return field.velocity;
    } else if (wavefield_component ==
               specfem::wavefield::component::acceleration) {
      return field.acceleration;
    } else if (wavefield_component == specfem::wavefield::component::pressure) {
      return field.acceleration;
    } else {
      Kokkos::abort("component not supported");
    }
  }();

  if (wavefield_component == specfem::wavefield::component::pressure) {

    specfem::algorithms::gradient(
        team, iterator, assembly.partial_derivatives, quadrature.hprime_gll,
        active_field,
        [&](const typename IteratorType::index_type &iterator_index,
            const FieldDerivativesType::ViewType &du) {
          const auto &index = iterator_index.index;
          PointPropertyType point_property;

          specfem::compute::load_on_device(index, properties, point_property);

          // P_SV case
          // sigma_xx
          const auto sigma_xx = point_property.lambdaplus2mu * du(0, 0) +
                                point_property.lambda * du(1, 1);

          // sigma_zz
          const auto sigma_zz = point_property.lambdaplus2mu * du(1, 1) +
                                point_property.lambda * du(0, 0);

          // sigma_yy
          const auto sigma_yy =
              point_property.lambdaplus2mu * (du(0, 0) + du(1, 1));

          wavefield(index.ispec, index.iz, index.ix, 0) =
              -1.0 * (sigma_xx + sigma_zz + sigma_yy) / 3.0;
        });

    return;
  }

  Kokkos::parallel_for(
      Kokkos::TeamThreadRange(team, iterator.chunk_size()), [&](const int &i) {
        const auto iterator_index = iterator(i);
        const auto &index = iterator_index.index;
        wavefield(index.ispec, index.iz, index.ix, 0) =
            active_field(iterator_index.ielement, index.iz, index.ix, 0);
        wavefield(index.ispec, index.iz, index.ix, 1) =
            active_field(iterator_index.ielement, index.iz, index.ix, 1);
      });

  return;
}

} // namespace medium
} // namespace specfem

#ifndef _COMPUTE_FIELDS_SIMULATION_FIELD_HPP_
#define _COMPUTE_FIELDS_SIMULATION_FIELD_HPP_

#include "compute/fields/impl/field_impl.hpp"
#include "enumerations/medium.hpp"
#include "enumerations/simulation.hpp"
#include "enumerations/specfem_enums.hpp"
#include "kokkos_abstractions.h"
#include "specfem_setup.hpp"
#include <Kokkos_Core.hpp>

namespace specfem {
namespace compute {

template <typename simulation> struct simulation_field {

  using simulation_type = simulation;
  using elastic_type = specfem::enums::element::medium::elastic;
  using acoustic_type = specfem::enums::element::medium::acoustic;

  simulation_field(const specfem::compute::mesh &mesh,
                   const specfem::compute::properties &properties);

  template <typename medium>
  KOKKOS_INLINE_FUNCTION type_real &field(const int &iglob, const int &icomp);

  template <typename medium>
  inline type_real &h_field(const int &iglob, const int &icomp);

  template <typename medium>
  KOKKOS_INLINE_FUNCTION type_real &field_dot(const int &iglob,
                                              const int &icomp);

  template <typename medium>
  inline type_real &h_field_dot(const int &iglob, const int &icomp);

  template <typename medium>
  KOKKOS_INLINE_FUNCTION type_real &field_dot_dot(const int &iglob,
                                                  const int &icomp);

  template <typename medium>
  inline type_real &h_field_dot_dot(const int &iglob, const int &icomp);

  template <typename medium>
  KOKKOS_INLINE_FUNCTION type_real &mass_inverse(const int &iglob,
                                                 const int &icomp);

  template <typename medium>
  inline type_real &h_mass_inverse(const int &iglob, const int &icomp);

  int nglob = 0;
  Kokkos::View<int * [specfem::enums::element::ntypes], Kokkos::LayoutLeft,
               specfem::kokkos::DevMemSpace>
      assembly_index_mapping;
  Kokkos::View<int * [specfem::enums::element::ntypes], Kokkos::LayoutLeft,
               specfem::kokkos::HostMemSpace>
      h_assembly_index_mapping;
  specfem::compute::impl::field_impl<elastic_type> elastic;
  specfem::compute::impl::field_impl<acoustic_type> acoustic;
};

} // namespace compute
} // namespace specfem

#endif /* _COMPUTE_FIELDS_SIMULATION_FIELD_HPP_ */

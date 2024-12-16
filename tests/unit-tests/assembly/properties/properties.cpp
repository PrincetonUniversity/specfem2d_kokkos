#include "../test_fixture/test_fixture.hpp"
#include "datatypes/simd.hpp"
#include "enumerations/dimension.hpp"
#include "enumerations/medium.hpp"
#include "parallel_configuration/chunk_config.hpp"
#include "policies/chunk.hpp"
#include "specfem_setup.hpp"
#include <gtest/gtest.h>

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, bool using_simd = false>
std::string get_error_message(
    const specfem::point::properties<specfem::dimension::type::dim2, MediumTag,
                                  PropertyTag, false> &point_property,
    const type_real value);

template <>
std::string get_error_message(
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        specfem::element::property_tag::isotropic, false> &point_property,
    const type_real value) {
  std::ostringstream message;

  message << "\n\t Expected: " << value;
  message << "\n\t Got: \n";
  message << "\t\trho = " << point_property.rho << "\n";
  message << "\t\tmu = " << point_property.mu << "\n";
  message << "\t\tkappa = " << point_property.kappa << "\n";
  message << "\t\trhop = " << point_property.rhop << "\n";
  message << "\t\talpha = " << point_property.alpha << "\n";
  message << "\t\tbeta = " << point_property.beta << "\n";

  return message.str();
}

template <>
std::string get_error_message(
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        specfem::element::property_tag::anisotropic, false> &point_property,
    const type_real value) {
  std::ostringstream message;

  message << "\n\t Expected: " << value;
  message << "\n\t Got: \n";
  message << "\t\trho = " << point_property.rho << "\n";
  message << "\t\tc11 = " << point_property.c11 << "\n";
  message << "\t\tc13 = " << point_property.c13 << "\n";
  message << "\t\tc15 = " << point_property.c15 << "\n";
  message << "\t\tc33 = " << point_property.c33 << "\n";
  message << "\t\tc35 = " << point_property.c35 << "\n";
  message << "\t\tc55 = " << point_property.c55 << "\n";
  message << "\t\tc12 = " << point_property.c33 << "\n";
  message << "\t\tc23 = " << point_property.c35 << "\n";
  message << "\t\tc25 = " << point_property.c55 << "\n";

  return message.str();
}

template <>
std::string get_error_message(
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::acoustic,
        specfem::element::property_tag::isotropic, false> &point_property,
    const type_real value) {
  std::ostringstream message;

  message << "\n\t Expected: " << value;
  message << "\n\t Got: \n";
  message << "\t\trho = " << point_property.rho << "\n";
  message << "\t\tkappa = " << point_property.kappa << "\n";
  message << "\t\trhop = " << point_property.rhop << "\n";
  message << "\t\talpha = " << point_property.alpha << "\n";

  return message.str();
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag>
specfem::point::properties<specfem::dimension::type::dim2, MediumTag, PropertyTag,
                        false>
get_point_property(const int ispec, const int iz, const int ix,
                 const specfem::compute::properties &properties);

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag>
specfem::point::properties<specfem::dimension::type::dim2, MediumTag, PropertyTag,
                        false>
get_point_property(
    const int lane,
    const specfem::point::properties<specfem::dimension::type::dim2, MediumTag,
                                  PropertyTag, true> &point_property);

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::isotropic, false>
get_point_property(const int ispec, const int iz, const int ix,
                 const specfem::compute::properties &properties) {

  const auto elastic_isotropic = properties.elastic_isotropic;

  const int ispec_l = properties.h_property_index_mapping(ispec);

  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::elastic,
                          specfem::element::property_tag::isotropic, false>
      point_property;

  point_property.rho = elastic_isotropic.h_rho(ispec_l, iz, ix);
  point_property.mu = elastic_isotropic.h_mu(ispec_l, iz, ix);
  point_property.kappa = elastic_isotropic.h_kappa(ispec_l, iz, ix);
  point_property.rhop = elastic_isotropic.h_rhop(ispec_l, iz, ix);
  point_property.alpha = elastic_isotropic.h_alpha(ispec_l, iz, ix);
  point_property.beta = elastic_isotropic.h_beta(ispec_l, iz, ix);

  return point_property;
}

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::isotropic, false>
get_point_property(
    const int lane,
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        specfem::element::property_tag::isotropic, true> &point_property) {
  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::elastic,
                          specfem::element::property_tag::isotropic, false>
      point_property_l;

  point_property_l.rho = point_property.rho[lane];
  point_property_l.mu = point_property.mu[lane];
  point_property_l.kappa = point_property.kappa[lane];
  point_property_l.rhop = point_property.rhop[lane];
  point_property_l.alpha = point_property.alpha[lane];
  point_property_l.beta = point_property.beta[lane];

  return point_property_l;
}

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::anisotropic, false>
get_point_property(const int ispec, const int iz, const int ix,
                 const specfem::compute::properties &properties) {

  const auto elastic_anisotropic = properties.elastic_anisotropic;

  const int ispec_l = properties.h_property_index_mapping(ispec);

  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::elastic,
                          specfem::element::property_tag::anisotropic, false>
      point_property;

  point_property.rho = elastic_anisotropic.h_rho(ispec_l, iz, ix);
  point_property.c11 = elastic_anisotropic.h_c11(ispec_l, iz, ix);
  point_property.c13 = elastic_anisotropic.h_c13(ispec_l, iz, ix);
  point_property.c15 = elastic_anisotropic.h_c15(ispec_l, iz, ix);
  point_property.c33 = elastic_anisotropic.h_c33(ispec_l, iz, ix);
  point_property.c35 = elastic_anisotropic.h_c35(ispec_l, iz, ix);
  point_property.c55 = elastic_anisotropic.h_c55(ispec_l, iz, ix);
  point_property.c12 = elastic_anisotropic.h_c12(ispec_l, iz, ix);
  point_property.c23 = elastic_anisotropic.h_c23(ispec_l, iz, ix);
  point_property.c25 = elastic_anisotropic.h_c25(ispec_l, iz, ix);

  return point_property;
}

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::elastic,
                        specfem::element::property_tag::anisotropic, false>
get_point_property(
    const int lane,
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::elastic,
        specfem::element::property_tag::anisotropic, true> &point_property) {
  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::elastic,
                          specfem::element::property_tag::anisotropic, false>
      point_property_l;

  point_property_l.rho = point_property.rho[lane];
  point_property_l.c11 = point_property.c11[lane];
  point_property_l.c13 = point_property.c13[lane];
  point_property_l.c15 = point_property.c15[lane];
  point_property_l.c33 = point_property.c33[lane];
  point_property_l.c35 = point_property.c35[lane];
  point_property_l.c55 = point_property.c55[lane];
  point_property_l.c12 = point_property.c12[lane];
  point_property_l.c23 = point_property.c23[lane];
  point_property_l.c25 = point_property.c25[lane];

  return point_property_l;
}

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::acoustic,
                        specfem::element::property_tag::isotropic, false>
get_point_property(const int ispec, const int iz, const int ix,
                 const specfem::compute::properties &properties) {

  const auto acoustic_isotropic = properties.acoustic_isotropic;

  const int ispec_l = properties.h_property_index_mapping(ispec);

  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::acoustic,
                          specfem::element::property_tag::isotropic, false>
      point_property;

  point_property.rho = acoustic_isotropic.h_rho(ispec_l, iz, ix);
  point_property.kappa = acoustic_isotropic.h_kappa(ispec_l, iz, ix);
  point_property.alpha = acoustic_isotropic.h_alpha(ispec_l, iz, ix);
  point_property.rhop = acoustic_isotropic.h_rho_prime(ispec_l, iz, ix);

  return point_property;
}

template <>
specfem::point::properties<specfem::dimension::type::dim2,
                        specfem::element::medium_tag::acoustic,
                        specfem::element::property_tag::isotropic, false>
get_point_property(
    const int lane,
    const specfem::point::properties<
        specfem::dimension::type::dim2, specfem::element::medium_tag::acoustic,
        specfem::element::property_tag::isotropic, true> &point_property) {
  specfem::point::properties<specfem::dimension::type::dim2,
                          specfem::element::medium_tag::acoustic,
                          specfem::element::property_tag::isotropic, false>
      point_property_l;

  point_property_l.rho = point_property.rho[lane];
  point_property_l.kappa = point_property.kappa[lane];
  point_property_l.alpha = point_property.alpha[lane];
  point_property_l.rhop = point_property.rhop[lane];

  return point_property_l;
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, bool using_simd,
          typename IndexViewType, typename ValueViewType>
void check_to_value(const specfem::compute::properties properties,
                    const IndexViewType &ispecs,
                    const ValueViewType &values_to_store) {
  const int nspec = properties.nspec;
  const int ngllx = properties.ngllx;
  const int ngllz = properties.ngllz;

  std::vector<int> elements;

  const auto element_types = properties.h_element_types;
  const auto element_properties = properties.h_element_property;

  for (int ispec = 0; ispec < nspec; ispec++) {
    if ((element_types(ispec) == MediumTag) &&
        (element_properties(ispec) == PropertyTag)) {
      elements.push_back(ispec);
    }
  }

  constexpr int simd_size =
      specfem::datatype::simd<type_real, using_simd>::size();

  for (int i = 0; i < ispecs.extent(0); ++i) {
    for (int iz = 0; iz < ngllz; iz++) {
      for (int ix = 0; ix < ngllx; ix++) {
        const int ielement = ispecs(i);
        const int n_simd_elements = (simd_size + ielement > elements.size())
                                        ? elements.size() - ielement
                                        : simd_size;
        for (int j = 0; j < n_simd_elements; j++) {
          const auto point_property = get_point_property<MediumTag, PropertyTag>(
              ielement + j, iz, ix, properties);
          const type_real value = values_to_store(i);
          if (point_property != value) {
            std::ostringstream message;

            message << "\n \t Error at ispec = " << ielement + j
                    << ", iz = " << iz << ", ix = " << ix;
            message << get_error_message(point_property, value);

            throw std::runtime_error(message.str());
          }
        }
      }
    }
  }

  return;
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, bool Store, bool Add,
          bool using_simd, typename IndexViewType, typename ValueViewType>
void execute_store_or_add(specfem::compute::properties &properties,
                          const int element_size, const IndexViewType &ispecs,
                          const ValueViewType &values_to_store) {

  const int nspec = properties.nspec;
  const int ngllx = properties.ngllx;
  const int ngllz = properties.ngllz;

  const int N = ispecs.extent(0);

  using PointType = specfem::point::properties<specfem::dimension::type::dim2,
                                            MediumTag, PropertyTag, using_simd>;

  Kokkos::parallel_for(
      "check_store_on_device",
      Kokkos::MDRangePolicy<Kokkos::DefaultExecutionSpace, Kokkos::Rank<3> >(
          { 0, 0, 0 }, { N, ngllz, ngllx }),
      KOKKOS_LAMBDA(const int &i, const int &iz, const int &ix) {
        const int ielement = ispecs(i);
        constexpr int simd_size = PointType::simd::size();
        auto &properties_l = properties;
        const int n_simd_elements = (simd_size + ielement > element_size)
                                        ? element_size - ielement
                                        : simd_size;

        const auto index =
            get_index<using_simd>(ielement, n_simd_elements, iz, ix);
        const type_real value = values_to_store(i);
        PointType point(value);
        if constexpr (Store) {
          specfem::compute::store_on_device(index, point, properties_l);
        } else if constexpr (Add) {
          specfem::compute::add_on_device(index, point, properties_l);
        }
      });

  Kokkos::fence();
  properties.copy_to_host();
  return;
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, bool using_simd>
void check_store_and_add(specfem::compute::properties &properties) {

  const int nspec = properties.nspec;
  const int ngllx = properties.ngllx;
  const int ngllz = properties.ngllz;
  std::vector<int> elements;

  const auto element_types = properties.h_element_types;
  const auto element_properties = properties.h_element_property;

  for (int ispec = 0; ispec < nspec; ispec++) {
    if ((element_types(ispec) == MediumTag) &&
        (element_properties(ispec) == PropertyTag)) {
      elements.push_back(ispec);
    }
  }

  // Evaluate at N evenly spaced points
  constexpr int N = 20;

  if (elements.size() < N) {
    return;
  }

  Kokkos::View<int[N], Kokkos::DefaultExecutionSpace> ispecs("ispecs");
  Kokkos::View<type_real[N], Kokkos::DefaultExecutionSpace> values_to_store(
      "values_to_store");
  auto ispecs_h = Kokkos::create_mirror_view(ispecs);
  auto values_to_store_h = Kokkos::create_mirror_view(values_to_store);

  const int element_size = elements.size();
  const int step = element_size / N;

  for (int i = 0; i < N; i++) {
    ispecs_h(i) = elements[i * step];
    values_to_store_h(i) = 10.5 + i;
  }

  ispecs_h(N - 1) = elements[element_size - 5]; // check when simd is not full

  Kokkos::deep_copy(ispecs, ispecs_h);
  Kokkos::deep_copy(values_to_store, values_to_store_h);

  execute_store_or_add<MediumTag, PropertyTag, true, false, using_simd>(
      properties, element_size, ispecs, values_to_store);

  check_to_value<MediumTag, PropertyTag, using_simd>(properties, ispecs_h,
                                                     values_to_store_h);

  execute_store_or_add<MediumTag, PropertyTag, false, true, using_simd>(
      properties, element_size, ispecs, values_to_store);

  for (int i = 0; i < N; i++) {
    values_to_store_h(i) *= 2;
  }

  check_to_value<MediumTag, PropertyTag, using_simd>(properties, ispecs_h,
                                                     values_to_store_h);
}

template <specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag, bool using_simd>
void check_load_on_device(specfem::compute::properties &properties) {
  const int nspec = properties.nspec;
  const int ngllx = properties.ngllx;
  const int ngllz = properties.ngllz;
  std::vector<int> elements;

  const auto element_types = properties.h_element_types;
  const auto element_properties = properties.h_element_property;

  for (int ispec = 0; ispec < nspec; ispec++) {
    if ((element_types(ispec) == MediumTag) &&
        (element_properties(ispec) == PropertyTag)) {
      elements.push_back(ispec);
    }
  }

  // Evaluate at N evenly spaced points
  constexpr int N = 20;

  if (elements.size() < N) {
    return;
  }

  using PointType = specfem::point::properties<specfem::dimension::type::dim2,
                                            MediumTag, PropertyTag, using_simd>;

  Kokkos::View<int[N], Kokkos::DefaultExecutionSpace> ispecs("ispecs");
  Kokkos::View<type_real[N], Kokkos::DefaultExecutionSpace> values_to_store(
      "values_to_store");
  auto ispecs_h = Kokkos::create_mirror_view(ispecs);
  auto values_to_store_h = Kokkos::create_mirror_view(values_to_store);

  const int element_size = elements.size();
  const int step = element_size / N;

  for (int i = 0; i < N; i++) {
    ispecs_h(i) = elements[i * step];
    values_to_store_h(i) = 2 * (10.5 + i);
  }

  ispecs_h(N - 1) = elements[element_size - 5]; // check when simd is not full

  Kokkos::deep_copy(ispecs, ispecs_h);

  Kokkos::View<PointType **[N], Kokkos::DefaultExecutionSpace> point_propertys(
      "point_propertys", ngllz, ngllx);
  auto h_point_propertys = Kokkos::create_mirror_view(point_propertys);

  Kokkos::parallel_for(
      "check_load_on_device",
      Kokkos::MDRangePolicy<Kokkos::Rank<3> >({ 0, 0, 0 }, { N, ngllz, ngllx }),
      KOKKOS_LAMBDA(const int &i, const int &iz, const int &ix) {
        const int ielement = ispecs(i);
        constexpr int simd_size = PointType::simd::size();
        const int n_simd_elements = (simd_size + ielement > element_size)
                                        ? element_size - ielement
                                        : simd_size;

        const auto index =
            get_index<using_simd>(ielement, n_simd_elements, iz, ix);
        PointType point;
        specfem::compute::load_on_device(index, properties, point);
        point_propertys(iz, ix, i) = point;
      });

  Kokkos::fence();
  Kokkos::deep_copy(h_point_propertys, point_propertys);

  for (int i = 0; i < N; i++) {
    for (int iz = 0; iz < ngllz; iz++) {
      for (int ix = 0; ix < ngllx; ix++) {
        using simd = specfem::datatype::simd<type_real, using_simd>;
        const auto &point_property = h_point_propertys(iz, ix, i);
        const int ielement = ispecs_h(i);
        constexpr int simd_size = PointType::simd::size();
        const int n_simd_elements = (simd_size + ielement > element_size)
                                        ? element_size - ielement
                                        : simd_size;
        const type_real value_l = values_to_store_h(i);
        if constexpr (using_simd) {
          for (int lane = 0; lane < n_simd_elements; lane++) {
            const auto point_property_l = get_point_property(lane, point_property);
            if (point_property_l != value_l) {
              std::ostringstream message;

              message << "\n \t Error in function load_on_device";

              message << "\n \t Error at ispec = " << ielement << ", iz = " << 0
                      << ", ix = " << 0;
              message << get_error_message(point_property_l, value_l);

              throw std::runtime_error(message.str());
            }
          }
        } else if constexpr (!using_simd) {
          if (point_property != value_l) {
            std::ostringstream message;
            message << "\n \t Error in function load_on_device";

            message << "\n \t Error at ispec = " << ielement << ", iz = " << 0
                    << ", ix = " << 0;
            message << get_error_message(point_property, value_l);

            throw std::runtime_error(message.str());
          }
        }
      }
    }
  }

  return;
}

void test_properties(specfem::compute::assembly &assembly) {

  auto &properties = assembly.properties;

  check_store_and_add<specfem::element::medium_tag::elastic,
                      specfem::element::property_tag::isotropic, false>(
      properties);

  check_load_on_device<specfem::element::medium_tag::elastic,
                       specfem::element::property_tag::isotropic, false>(
      properties);

  check_store_and_add<specfem::element::medium_tag::elastic,
                      specfem::element::property_tag::isotropic, true>(properties);

  check_load_on_device<specfem::element::medium_tag::elastic,
                       specfem::element::property_tag::isotropic, true>(
      properties);

  check_store_and_add<specfem::element::medium_tag::elastic,
                      specfem::element::property_tag::anisotropic, false>(
      properties);

  check_load_on_device<specfem::element::medium_tag::elastic,
                       specfem::element::property_tag::anisotropic, false>(
      properties);

  check_store_and_add<specfem::element::medium_tag::elastic,
                      specfem::element::property_tag::anisotropic, true>(
      properties);

  check_load_on_device<specfem::element::medium_tag::elastic,
                       specfem::element::property_tag::anisotropic, true>(
      properties);

  check_store_and_add<specfem::element::medium_tag::acoustic,
                      specfem::element::property_tag::isotropic, false>(
      properties);

  check_load_on_device<specfem::element::medium_tag::acoustic,
                       specfem::element::property_tag::isotropic, false>(
      properties);

  check_store_and_add<specfem::element::medium_tag::acoustic,
                      specfem::element::property_tag::isotropic, true>(properties);

  check_load_on_device<specfem::element::medium_tag::acoustic,
                       specfem::element::property_tag::isotropic, true>(
      properties);
}

TEST_F(ASSEMBLY, properties_device_functions) {
  for (auto parameters : *this) {
    const auto Test = std::get<0>(parameters);
    auto assembly = std::get<1>(parameters);

    try {
      test_properties(assembly);

      std::cout << "-------------------------------------------------------\n"
                << "\033[0;32m[PASSED]\033[0m " << Test.name << "\n"
                << "-------------------------------------------------------\n\n"
                << std::endl;
    } catch (std::exception &e) {
      std::cout << "-------------------------------------------------------\n"
                << "\033[0;31m[FAILED]\033[0m \n"
                << "-------------------------------------------------------\n"
                << "- Test: " << Test.name << "\n"
                << "- Error: " << e.what() << "\n"
                << "-------------------------------------------------------\n\n"
                << std::endl;
      ADD_FAILURE();
    }
  }
}

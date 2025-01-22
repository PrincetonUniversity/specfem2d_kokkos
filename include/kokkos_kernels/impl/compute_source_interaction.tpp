#pragma once

#include "chunk_element/field.hpp"
#include "compute/assembly/assembly.hpp"
#include "datatypes/simd.hpp"
#include "boundary_conditions/boundary_conditions.hpp"
#include "enumerations/dimension.hpp"
#include "enumerations/medium.hpp"
#include "enumerations/wavefield.hpp"
#include "medium/compute_source.hpp"
#include "parallel_configuration/chunk_config.hpp"
#include "point/boundary.hpp"
#include "point/field.hpp"
#include "point/properties.hpp"
#include "point/sources.hpp"
#include "policies/chunk.hpp"
#include <Kokkos_Core.hpp>

template <specfem::dimension::type DimensionType,
          specfem::wavefield::simulation_field WavefieldType, int NGLL,
          specfem::element::medium_tag MediumTag,
          specfem::element::property_tag PropertyTag,
          specfem::element::boundary_tag BoundaryTag>
void specfem::kokkos_kernels::impl::compute_source_interaction(
    specfem::compute::assembly &assembly, const int &timestep) {

constexpr auto medium_tag = MediumTag;
constexpr auto property_tag = PropertyTag;
constexpr auto boundary_tag = BoundaryTag;
constexpr auto dimension = DimensionType;
constexpr int ngll = NGLL;
constexpr auto wavefield = WavefieldType;

const auto elements = assembly.sources.get_elements_on_device(
    MediumTag, PropertyTag, BoundaryTag, WavefieldType);

const int nelements = elements.extent(0);

if (nelements == 0)
  return;

auto &sources = assembly.sources;
const auto &properties = assembly.properties;
const auto &boundaries = assembly.boundaries;
const auto field = assembly.fields.get_simulation_field<wavefield>();

sources.update_timestep(timestep);

using PointSourcesType =
    specfem::point::source<dimension, medium_tag, wavefield>;
using PointPropertiesType =
    specfem::point::properties<dimension, medium_tag, property_tag, false>;
using PointBoundaryType =
    specfem::point::boundary<boundary_tag, dimension, false>;
using PointVelocityType = specfem::point::field<dimension, medium_tag, false,
                                                true, false, false, false>;

using simd = specfem::datatype::simd<type_real, false>;
constexpr int simd_size = simd::size();

#ifdef KOKKOS_ENABLE_CUDA
constexpr int nthreads = 32;
constexpr int lane_size = 1;
#else
constexpr int nthreads = 1;
constexpr int lane_size = 1;
#endif

using ParallelConfig =
    specfem::parallel_config::chunk_config<DimensionType, 1, 1, nthreads,
                                           lane_size, simd,
                                           Kokkos::DefaultExecutionSpace>;

using ChunkPolicy = specfem::policy::element_chunk<ParallelConfig>;

ChunkPolicy chunk_policy(elements, NGLL, NGLL);

Kokkos::parallel_for(
    "specfem::kernels::impl::domain_kernels::compute_source_interaction",
    static_cast<const typename ChunkPolicy::policy_type &>(chunk_policy),
    KOKKOS_LAMBDA(const typename ChunkPolicy::member_type &team) {
      for (int tile = 0; tile < ChunkPolicy::tile_size * simd_size;
           tile += ChunkPolicy::chunk_size * simd_size) {
        const int starting_element_index =
            team.league_rank() * ChunkPolicy::tile_size * simd_size + tile;

        if (starting_element_index >= nelements) {
          break;
        }

        const auto iterator =
            chunk_policy.league_iterator(starting_element_index);

        Kokkos::parallel_for(
            Kokkos::TeamThreadRange(team, iterator.chunk_size()),
            [&](const int i) {
              const auto iterator_index = iterator(i);
              const auto index = iterator_index.index;

              PointSourcesType point_source;
              specfem::compute::load_on_device(index, sources, point_source);

              PointPropertiesType point_property;
              specfem::compute::load_on_device(index, properties,
                                               point_property);

              auto acceleration =
                  specfem::medium::compute_source_contribution(point_source,
                                                               point_property);

              PointBoundaryType point_boundary;
              specfem::compute::load_on_device(index, boundaries,
                                               point_boundary);

              PointVelocityType velocity;
              specfem::compute::load_on_device(index, field, velocity);

              specfem::boundary_conditions::
                  apply_boundary_conditions(point_boundary, point_property,
                                            velocity, acceleration);

              specfem::compute::atomic_add_on_device(index, acceleration,
                                                     field);
            });
      }
    });

Kokkos::fence();
}

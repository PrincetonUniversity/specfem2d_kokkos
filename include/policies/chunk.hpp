#pragma once

#include "enumerations/dimension.hpp"
#include "point/coordinates.hpp"
#include <Kokkos_Core.hpp>
#include <string>
#include <type_traits>

namespace specfem {
namespace iterator {

namespace impl {
/**
 * @brief Struct to store the index of a quadrature point generated by chunk
 * policy.
 *
 * @tparam UseSIMD Indicates whether SIMD is used or not.
 * @tparam DimensionType Dimension type of the elements within this iterator.
 */
template <bool UseSIMD, specfem::dimension::type DimensionType>
struct chunk_index_type;

/**
 * @brief Template specialization when using SIMD.
 *
 */
template <specfem::dimension::type DimensionType>
struct chunk_index_type<true, DimensionType> {
  constexpr static auto dimension = DimensionType; ///< Dimension type
  int ielement; ///< Element index within the iterator range
  specfem::point::simd_index<dimension> index; ///< SIMD index of the quadrature
                                               ///< point(s)

  KOKKOS_INLINE_FUNCTION
  chunk_index_type(const int ielement,
                   const specfem::point::simd_index<dimension> index)
      : ielement(ielement), index(index) {}
};

/**
 * @brief Template specialization when not using SIMD.
 *
 */
template <specfem::dimension::type DimensionType>
struct chunk_index_type<false, DimensionType> {
  constexpr static auto dimension = DimensionType; ///< Dimension type
  int ielement; ///< Element index within the iterator range
  specfem::point::index<dimension> index; ///< Index of the quadrature point

  KOKKOS_INLINE_FUNCTION
  chunk_index_type(const int ielement,
                   const specfem::point::index<dimension> index)
      : ielement(ielement), index(index){};
};
} // namespace impl

/**
 * @brief Iterator to generate indices for quadrature points defined within this
 * iterator.
 *
 * @tparam ViewType View type for the indices of elements within this iterator.
 * @tparam DimensionType Dimension type of the elements within this iterator.
 * @tparam SIMD SIMD type to use simd operations @ref specfem::datatypes::simd
 */
template <typename ViewType, specfem::dimension::type DimensionType,
          typename SIMD>
class chunk;

/**
 * @brief Template specialization for 2D elements.
 *
 */
template <typename ViewType, typename SIMD>
class chunk<ViewType, specfem::dimension::type::dim2, SIMD> {
public:
  /**
   * @name Compile-time constants
   *
   */
  ///@{
  constexpr static auto dimension =
      specfem::dimension::type::dim2; ///< Dimension type
  ///@}

  /**
   * @name Type definitions
   *
   */
  ///@{
  using simd = SIMD; ///< SIMD type
  using index_type =
      typename impl::chunk_index_type<simd::using_simd, dimension>; ///< Index
                                                                    ///< type
  ///@}

private:
  constexpr static bool using_simd = simd::using_simd;
  constexpr static int simd_size = simd::size();

  ViewType indices; ///< View of indices of elements within this iterator
  int num_elements; ///< Number of elements within this iterator
  int ngllz;        ///< Number of GLL points in the z-direction
  int ngllx;        ///< Number of GLL points in the x-direction

  KOKKOS_INLINE_FUNCTION
  chunk(const ViewType &indices, const int ngllz, const int ngllx,
        std::true_type)
      : indices(indices), num_elements(indices.extent(0) / simd_size +
                                       (indices.extent(0) % simd_size != 0)),
        ngllz(ngllz), ngllx(ngllx) {}

  KOKKOS_INLINE_FUNCTION
  chunk(const ViewType &indices, const int ngllz, const int ngllx,
        std::false_type)
      : indices(indices), num_elements(indices.extent(0)), ngllz(ngllz),
        ngllx(ngllx) {}

  KOKKOS_INLINE_FUNCTION
  impl::chunk_index_type<false, dimension> operator()(const int i,
                                                      std::false_type) const {
#ifdef KOKKOS_ENABLE_CUDA
    int ielement = i % num_elements;
    int ispec = indices(ielement);
    int xz = i / num_elements;
    const int iz = xz / ngllz;
    const int ix = xz % ngllz;
#else
    const int ix = i % ngllx;
    const int iz = (i / ngllx) % ngllz;
    const int ielement = i / (ngllz * ngllx);
    int ispec = indices(ielement);
#endif
    return impl::chunk_index_type<false, dimension>(
        ielement, specfem::point::index<dimension>(ispec, iz, ix));
  }

  KOKKOS_INLINE_FUNCTION
  impl::chunk_index_type<true, dimension> operator()(const int i,
                                                     std::true_type) const {
#ifdef KOKKOS_ENABLE_CUDA
    int ielement = i % num_elements;
    int simd_elements = (simd_size + ielement > indices.extent(0))
                            ? indices.extent(0) - ielement
                            : simd_size;
    int ispec = indices(ielement);
    int xz = i / num_elements;
    const int iz = xz / ngllz;
    const int ix = xz % ngllz;
#else
    const int ix = i % ngllx;
    const int iz = (i / ngllx) % ngllz;
    const int ielement = i / (ngllz * ngllx);
    int simd_elements = (simd_size + ielement > indices.extent(0))
                            ? indices.extent(0) - ielement
                            : simd_size;
    int ispec = indices(ielement);
#endif
    return impl::chunk_index_type<true, dimension>(
        ielement,
        specfem::point::simd_index<dimension>(ispec, simd_elements, iz, ix));
  }

public:
  /**
   * @name Constructors
   *
   */
  ///@{
  /**
   * @brief Construct a new chunk iterator with a given view of indices.
   *
   * @param indices View of indices of elements within this iterator
   * @param ngllz Number of GLL points in the z-direction
   * @param ngllx Number of GLL points in the x-direction
   */
  KOKKOS_INLINE_FUNCTION
  chunk(const ViewType &indices, int ngllz, int ngllx)
      : chunk(indices, ngllz, ngllx,
              std::integral_constant<bool, using_simd>()) {
#if KOKKOS_VERSION < 40100
    static_assert(ViewType::Rank == 1, "View must be rank 1");
#else
    static_assert(ViewType::rank() == 1, "View must be rank 1");
#endif
  }
  ///@}

  /**
   * @brief Return the number of quadrature points within this chunk.
   *
   * @return int Number of quadrature points within this chunk
   */
  KOKKOS_FORCEINLINE_FUNCTION
  int chunk_size() const { return num_elements * ngllz * ngllx; }

  /**
   * @brief Returns the index within this iterator at the i-th quadrature point.
   *
   * @param i Index of the quadrature point within this iterator.
   * @return index_type Index of the quadrature point.
   */
  KOKKOS_INLINE_FUNCTION
  index_type operator()(const int i) const {
    return operator()(i, std::integral_constant<bool, using_simd>());
  }

  /**
   * @brief Get the range of spectral element indices within this iterator.
   *
   * @return Kokkos::pair<int, int> Range of spectral element indices within
   * this iterator.
   */
  KOKKOS_INLINE_FUNCTION
  Kokkos::pair<int, int> get_range() const {
    return Kokkos::make_pair(indices(0), indices(num_elements - 1));
  }
};
} // namespace iterator

namespace policy {

/**
 * @brief Element chunk policy to chunk a group of elements into Kokkos teams
 * and iterate over all the quadrature points within those chunks.
 *
 * @tparam ParallelConfig Parallel configuration for element chunk policy.
 */
template <typename ParallelConfig>
struct element_chunk
    : public Kokkos::TeamPolicy<typename ParallelConfig::execution_space> {

private:
  using IndexViewType = Kokkos::View<
      int *,
      typename ParallelConfig::execution_space::memory_space>; ///< View
                                                               ///< type for
                                                               ///< indices

public:
  /**
   * @name Type definitions
   *
   */
  ///@{
  using simd = typename ParallelConfig::simd; ///< SIMD configuration
  using execution_space =
      typename ParallelConfig::execution_space;            ///< Execution space
  using policy_type = Kokkos::TeamPolicy<execution_space>; ///< Policy type
  using member_type = typename policy_type::member_type;   ///< Member type
  using iterator_type =
      specfem::iterator::chunk<IndexViewType, ParallelConfig::dimension,
                               simd>; ///< Iterator
                                      ///< type
  ///@}

  /**
   * @name Compile-time constants
   *
   */
  ///@{
  constexpr static int chunk_size = ParallelConfig::chunk_size; ///< Chunk size
  constexpr static int num_threads =
      ParallelConfig::num_threads; ///< Chunk size
  constexpr static int vector_lanes =
      ParallelConfig::vector_lanes;                           ///< Vector lanes
  constexpr static int tile_size = ParallelConfig::tile_size; ///< Tile size
  constexpr static auto dimension =
      ParallelConfig::dimension;               ///< Dimension type
  constexpr static bool isPointPolicy = false; ///< Indicates whether this is a
                                               ///< point policy or not
  constexpr static bool isEdgePolicy = false;  ///< Indicates whether this is an
                                               ///< edge policy or not
  constexpr static bool isFacePolicy = false;  ///< Indicates whether this is a
                                               ///< face policy or not
  constexpr static bool isElementPolicy =
      true; ///< Indicates whether this is an
  ///< element policy or not
  constexpr static bool isKokkosRangePolicy =
      false; ///< Indicates that this is a Kokkos range policy
  constexpr static bool isKokkosTeamPolicy =
      true; ///< Indicates that this is a Kokkos team policy
  ///@}

private:
  constexpr static int simd_size = simd::size();
  constexpr static bool using_simd = simd::using_simd;

public:
  /**
   * @name Constructors
   *
   */
  ///@{

  /**
   * @brief Construct a new element chunk policy
   *
   * @param view View of elements to chunk
   * @param ngllz Number of GLL points in the z-direction
   * @param ngllx Number of GLL points in the x-direction
   */
  element_chunk(const IndexViewType &view, int ngllz, int ngllx)
      : policy_type(view.extent(0) / (tile_size * simd_size) +
                        (view.extent(0) % (tile_size * simd_size) != 0),
                    num_threads, vector_lanes),
        elements(view), ngllz(ngllz), ngllx(ngllx) {
#if KOKKOS_VERSION < 40100
    static_assert(IndexViewType::Rank == 1, "View must be rank 1");
#else
    static_assert(IndexViewType::rank() == 1, "View must be rank 1");
#endif
  }
  ///@}

  /**
   * @brief Implicit conversion to the underlying Kokkos policy type
   *
   * @return const policy_type & Underlying Kokkos policy type
   */
  operator const policy_type &() const { return *this; }

  /**
   * @brief Get iterator to iterator over chunk of elements associated with
   * Kokkos team
   *
   * @param start_index Starting index for the element within the team
   * @return iterator_type Iterator for the team
   */
  KOKKOS_INLINE_FUNCTION
  iterator_type league_iterator(const int start_index) const {
    const int start = start_index;
    const int end = (start + chunk_size * simd_size > elements.extent(0))
                        ? elements.extent(0)
                        : start + chunk_size * simd_size;
    const auto my_indices =
        Kokkos::subview(elements, Kokkos::make_pair(start, end));
    return iterator_type(my_indices, ngllz, ngllx);
  }

private:
  IndexViewType elements; ///< View of elements
  int ngllz;              ///< Number of GLL points in the z-direction
  int ngllx;              ///< Number of GLL points in the x-direction
};
} // namespace policy
} // namespace specfem

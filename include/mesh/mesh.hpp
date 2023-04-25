#ifndef _MESH_HPP
#define _MESH_HPP

#include "boundaries/boundaries.hpp"
#include "compute/interface.hpp"
#include "elements/elements.hpp"
#include "kokkos_abstractions.h"
#include "material.h"
#include "mpi_interfaces.h"
#include "quadrature.h"
#include "read_mesh_database.h"
#include "specfem_mpi.h"
#include "specfem_setup.hpp"
#include "surfaces.h"
#include <Kokkos_Core.hpp>

namespace specfem {

namespace mesh {
/**
 * @brief Mesh Interface
 *
 * The mesh is implemented as a C++ struct. The mesh struct defines all the
 * variables nacessary to populate structs within specfem::compute namespace
 *
 */
struct mesh {

  int npgeo; ///< Total number of spectral element control nodes
  int nspec; ///< Total number of spectral elements
  int nproc; ///< Total number of processors
  specfem::kokkos::HostView2d<type_real> coorg; ///< (x_a,z_a) for every
                                                ///< spectral element control
                                                ///< node

  specfem::materials::material_ind material_ind; ///< Struct used to store
                                                 ///< material information for
                                                 ///< every spectral element

  specfem::interfaces::interface interface; ///< Struct used to store data
                                            ///< required to implement MPI
                                            ///< interfaces

  specfem::mesh::boundaries::absorbing_boundary abs_boundary; ///< Struct used
                                                              ///< to store data
                                                              ///< required to
                                                              ///< implement
                                                              ///< absorbing
                                                              ///< boundary

  specfem::properties parameters; ///< Struct to store simulation launch
                                  ///< parameters

  specfem::surfaces::acoustic_free_surface
      acfree_surface; ///< Struct used to store data required to implement
                      ///< acoustic free surface

  specfem::mesh::boundaries::forcing_boundary
      acforcing_boundary; ///< Struct used to store data required to implement
                          ///< acoustic forcing boundary

  specfem::mesh::elements::tangential_elements tangential_nodes; ///< Defines
                                                                 ///< tangential
                                                                 ///< nodes

  specfem::mesh::elements::axial_elements axial_nodes; ///< Defines axial nodes

  /**
   * @brief Default mesh constructor
   *
   */
  mesh(){};

  /**
   * @brief Construct mesh from a fortran binary database file
   *
   * @param filename Fortran binary database filename
   * @param mpi pointer to MPI object to manage communication
   */
  mesh(const std::string filename, std::vector<specfem::material *> &materials,
       const specfem::MPI::MPI *mpi);

  /**
   * @brief User output
   *
   */
  std::string print(std::vector<specfem::material *> materials) const;
};
} // namespace mesh
} // namespace specfem

#endif

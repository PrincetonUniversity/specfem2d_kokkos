#pragma once

#include "mesh/coupled_interfaces/coupled_interfaces.hpp"
#include "mesh/coupled_interfaces/interface_container.hpp"
#include "specfem_mpi/interface.hpp"

namespace specfem {
namespace IO {
namespace mesh {
namespace impl {
namespace fortran {

template <specfem::element::medium_tag medium1,
          specfem::element::medium_tag medium2>
specfem::mesh::interface_container<medium1, medium2>
read_interfaces(const int num_interfaces, std::ifstream &stream,
                const specfem::MPI::MPI *mpi);

/* @brief Read the coupled interfaces from the database file
 *
 * @param stream input file stream
 * @param num_interfaces_elastic_acoustic
 * @param num_interfaces_acoustic_poroelastic
 * @param num_interfaces_elastic_poroelastic
 * @param mpi
 * @return specfem::mesh::coupled_interfaces
 */
specfem::mesh::coupled_interfaces read_coupled_interfaces(
    std::ifstream &stream, const int num_interfaces_elastic_acoustic,
    const int num_interfaces_acoustic_poroelastic,
    const int num_interfaces_elastic_poroelastic, const specfem::MPI::MPI *mpi);

} // namespace fortran
} // namespace impl
} // namespace mesh
} // namespace IO
} // namespace specfem

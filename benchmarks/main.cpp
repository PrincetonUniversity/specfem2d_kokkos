#include "execute.hpp"

void benchmark(specfem::compute::assembly &assembly,
               std::shared_ptr<specfem::time_scheme::time_scheme> time_scheme) {
  auto kernels = specfem::kokkos_kernels::domain_kernels<
      specfem::wavefield::simulation_field::forward,
      specfem::dimension::type::dim2, 5>(assembly);
  constexpr auto elastic = specfem::element::medium_tag::elastic;

  // kernels.initialize(time_scheme->get_timestep());

  const int nstep = time_scheme->get_max_timestep();

  for (const auto [istep, dt] : time_scheme->iterate_forward()) {
    time_scheme->apply_predictor_phase_forward(elastic);

    kernels.template update_wavefields<elastic>(istep);
    time_scheme->apply_corrector_phase_forward(elastic);

    if ((istep + 1) % 200 == 0) {
      std::cout << "Progress : executed " << istep + 1 << " steps of " << nstep
                << " steps" << std::endl;
    }
  }

  std::cout << std::endl;
}

void run_benchmark(const YAML::Node &parameter_dict,
                   const YAML::Node &default_dict, specfem::MPI::MPI *mpi) {

  // --------------------------------------------------------------
  //                    Read parameter file
  // --------------------------------------------------------------
  auto start_time = std::chrono::system_clock::now();
  specfem::runtime_configuration::setup setup(parameter_dict, default_dict);
  const auto database_filename = setup.get_databases();
  mpi->cout(setup.print_header(start_time));

  // --------------------------------------------------------------

  // --------------------------------------------------------------
  //                   Read mesh and materials
  // --------------------------------------------------------------
  const auto quadrature = setup.instantiate_quadrature();
  const auto mesh = specfem::IO::read_mesh(database_filename, mpi);

  // --------------------------------------------------------------

  // --------------------------------------------------------------
  //                   Instantiate Timescheme
  // --------------------------------------------------------------
  const auto time_scheme = setup.instantiate_timescheme();

  const int max_seismogram_time_step = time_scheme->get_max_seismogram_step();

  const int nstep_between_samples = time_scheme->get_nstep_between_samples();
  const type_real dt = setup.get_dt();
  // --------------------------------------------------------------

  // --------------------------------------------------------------
  //                   Generate Assembly
  // --------------------------------------------------------------
  mpi->cout("Generating assembly:");
  mpi->cout("-------------------------------");
  const std::vector<std::shared_ptr<specfem::sources::source> > sources;
  const std::vector<std::shared_ptr<specfem::receivers::receiver> > receivers;
  const int nsteps = setup.get_nsteps();
  specfem::compute::assembly assembly(
      mesh, quadrature, sources, receivers, setup.get_seismogram_types(),
      setup.get_t0(), dt, nsteps, max_seismogram_time_step,
      nstep_between_samples, setup.get_simulation_type(),
      setup.instantiate_property_reader());
  time_scheme->link_assembly(assembly);

  const auto solver_start_time = std::chrono::system_clock::now();
  benchmark(assembly, time_scheme);
  const auto solver_end_time = std::chrono::system_clock::now();
  std::chrono::duration<double> solver_time =
      solver_end_time - solver_start_time;
  std::cout << "Solver time: " << solver_time.count() << "s" << std::endl;
}

int main(int argc, char **argv) {
  // Initialize MPI
  specfem::MPI::MPI *mpi = new specfem::MPI::MPI(&argc, &argv);
  // Initialize Kokkos
  Kokkos::initialize(argc, argv);
  {
    const std::string default_file = __default_file__;
    const YAML::Node default_dict = YAML::LoadFile(default_file);
    run_benchmark(YAML::LoadFile(__benchmark_iso__), default_dict, mpi);
    run_benchmark(YAML::LoadFile(__benchmark_aniso__), default_dict, mpi);
  }
  // Finalize Kokkos
  Kokkos::finalize();
  // Finalize MPI
  delete mpi;
  return 0;
}

#ifndef __UTIL_EDGE_STORAGES_HPP_
#define __UTIL_EDGE_STORAGES_HPP_

#include "enumerations/specfem_enums.hpp"
#include "kokkos_abstractions.h"
#include <functional>
#include <vector>

namespace _util {
namespace edge_manager {

// different quadrature rules? consider GLL -> GL for intersections, since
// endpoints aren't needed
struct quadrature_rule {
  int nquad;
  std::vector<type_real> t; // knots
  std::vector<type_real> w; // weights
  std::vector<type_real> L; // Lagrange polynomial coeffs

  quadrature_rule(int nquad)
      : nquad(nquad), t(std::vector<type_real>(nquad)),
        w(std::vector<type_real>(nquad)),
        L(std::vector<type_real>(nquad * nquad)) {}
  quadrature_rule(quadrature_rule &other)
      : nquad(other.nquad), t(std::vector<type_real>(other.t)),
        w(std::vector<type_real>(other.w)), L(std::vector<type_real>(other.L)) {
  }

  type_real integrate(type_real *f);
  type_real deriv(type_real *f, type_real t);
  type_real interpolate(type_real *f, type_real t);

  template <int ngllcapacity>
  void sample_L(type_real buf[][ngllcapacity], type_real *t_vals, int t_size);
};

quadrature_rule gen_GLL(int ngll);

struct edge {
  int id;
  specfem::enums::edge::type bdry;
  edge() : id(-1), bdry(specfem::enums::edge::type::NONE) {}
  // edge(const edge& other) : id(other.id), bdry(other.bdry) {}
};

template <int ngllcapacity> struct edge_intersection {
  int a_ref_ind;
  int b_ref_ind;
  type_real a_mortar_trans[ngllcapacity][ngllcapacity];
  type_real b_mortar_trans[ngllcapacity][ngllcapacity];
  type_real a_param_start, a_param_end;
  type_real b_param_start, b_param_end;
  int a_ngll, b_ngll, ngll;
  type_real relax_param;
  edge_intersection()
      : ngll(0), a_ref_ind(-1), b_ref_ind(-1), a_ngll(0), b_ngll(0) {}

  type_real a_to_mortar(int node_index, type_real *quantity);
  type_real b_to_mortar(int node_index, type_real *quantity);
};

template <int ngllcapacity, int datacapacity> struct edge_data {
  edge parent;
  int ngll;
  type_real x[ngllcapacity];
  type_real z[ngllcapacity];
  type_real data[datacapacity][ngllcapacity];
  edge_data() : ngll(0) {}
  // edge_data(const edge_data<ngllcapacity, datacapacity>& other) :
  // parent(other.parent), ngll(other.ngll) {}
};

template <int ngll, int datacapacity> struct edge_storage {
public:
  edge_storage(const std::vector<edge> edges);

  void foreach_edge_on_host(
      const std::function<void(edge_data<ngll, datacapacity> &)> &func);
  void foreach_intersection_on_host(
      const std::function<void(edge_intersection<ngll> &,
                               edge_data<ngll, datacapacity> &,
                               edge_data<ngll, datacapacity> &)> &func);
  void build_intersections_on_host();

  int num_edges() { return n_edges; }
  int num_intersections() { return n_intersections; }

private:
  int n_edges;
  std::vector<edge> edges;

  specfem::kokkos::DeviceView1d<edge_data<ngll, datacapacity> >
      edge_data_container;
  specfem::kokkos::HostView1d<edge_data<ngll, datacapacity> >
      h_edge_data_container;

  int n_intersections;
  bool intersections_built;
  specfem::kokkos::DeviceView1d<edge_intersection<ngll> >
      intersection_container;
  specfem::kokkos::HostView1d<edge_intersection<ngll> >
      h_intersection_container;
};

} // namespace edge_manager
} // namespace _util
#endif

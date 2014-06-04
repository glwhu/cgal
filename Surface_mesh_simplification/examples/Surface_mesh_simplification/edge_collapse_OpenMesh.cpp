#define CGAL_BGL_TESTSUITE
#include <iostream>
#include <fstream>

#include <CGAL/Simple_cartesian.h>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

#include <CGAL/boost/graph/graph_traits_PolyMesh_ArrayKernelT.h>
#include <CGAL/boost/graph/properties_PolyMesh_ArrayKernelT.h>

// Simplification function
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_length_cost.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_placement.h>

typedef OpenMesh::PolyMesh_ArrayKernelT</* MyTraits*/> Surface_mesh;

typedef boost::graph_traits<Surface_mesh>::edge_descriptor edge_descriptor;
typedef boost::graph_traits<Surface_mesh>::edge_iterator edge_iterator;

class Constrained_edge_map
{
public:
  typedef boost::read_write_property_map_tag    category;
  typedef bool                                  value_type;
  typedef bool                                  reference;
  typedef edge_descriptor                       key_type;

  Constrained_edge_map(Surface_mesh& sm)
    : sm_(sm)
  {
    sm_.add_property(constraint);
  }

  inline friend reference get(const Constrained_edge_map& em, key_type e)
  {
    return em.sm_.property(em.constraint,em.sm_.edge_handle(e.idx())); 
  }
  
  inline friend void put(const Constrained_edge_map& em, key_type e, value_type b)
  {
    em.sm_.property(em.constraint,em.sm_.edge_handle(e.idx())) = b;
  }

private:
  Surface_mesh& sm_;
  OpenMesh::EPropHandleT<bool> constraint;
};


class OM_vertex_CGAL_point_pmap
{
public:
  typedef boost::read_write_property_map_tag category;

  typedef CGAL::Simple_cartesian<double>::Point_3               value_type;
  typedef CGAL::Simple_cartesian<double>::Point_3               reference;

  typedef boost::graph_traits<Surface_mesh>::vertex_descriptor key_type;

  OM_vertex_CGAL_point_pmap(const Surface_mesh& sm)
    : sm_(sm)
    {}
    
  OM_vertex_CGAL_point_pmap(const OM_vertex_CGAL_point_pmap& pm)
    : sm_(pm.sm_)
    {}

  inline friend reference get(const OM_vertex_CGAL_point_pmap& pm, key_type v)
  {
    Surface_mesh::Point const& omp = pm.sm_.point(v);
    return value_type(omp[0], omp[1], omp[2]);
  }

  inline friend void put(const OM_vertex_CGAL_point_pmap& pm, key_type v, const value_type& p)
  {
    const_cast<Surface_mesh&>(pm.sm_).set_point
      (v, Surface_mesh::Point(p[0], p[1], p[2]));
  }

  private:
  const Surface_mesh& sm_;
};



namespace SMS = CGAL::Surface_mesh_simplification ;

int main( int argc, char** argv ) 
{
  Surface_mesh surface_mesh;
  Constrained_edge_map constraints_map(surface_mesh);
  OpenMesh::IO::read_mesh(surface_mesh, argv[1]);
  
  // For the pupose of the example we mark 10 edges as constrained edges
  edge_iterator b,e;
  int count=0;
  for(boost::tie(b,e); b!= e; ++b){
      put(constraints_map,*b,(count++ <10));
  }
  // This is a stop predicate (defines when the algorithm terminates).
  // In this example, the simplification stops when the number of undirected edges
  // left in the surface mesh drops below the specified number (1000)
  SMS::Count_stop_predicate<Surface_mesh> stop(1000);
     
  // This the actual call to the simplification algorithm.
  // The surface mesh and stop conditions are mandatory arguments.

  int r = SMS::edge_collapse
            (surface_mesh
            ,stop
             ,CGAL::halfedge_index_map  (get(CGAL::halfedge_index  ,surface_mesh)) 
             .vertex_point_map(OM_vertex_CGAL_point_pmap(surface_mesh))
             .edge_is_constrained_map(constraints_map) 
             );
  
  surface_mesh.garbage_collection();
  std::cout << "\nFinished...\n" << r << " edges removed.\n" 
            << num_edges(surface_mesh) << " final edges.\n" ;
        
  //  std::ofstream os( argc > 2 ? argv[2] : "out.off" ) ; os << surface_mesh ;
  
  return 0 ;      
}

// EOF //

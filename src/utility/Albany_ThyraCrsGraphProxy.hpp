#ifndef ALBANY_THYRA_CRS_GRAPH_PROXY_HPP
#define ALBANY_THYRA_CRS_GRAPH_PROXY_HPP

#include "Teuchos_RCP.hpp"
#include "Albany_ThyraTypes.hpp"

namespace Albany {

/*
 * A class to provide proxy access to the functionality of a CrsGraph object
 * 
 * Thyra does not have the concept of 'Graph', since it is designed to abstract
 * at a mathematical level, in a world of vector spaces, operators, and vectors.
 * The concept of graph has to do with the particular implementation of an operator,
 * namely its storage. From the computational point of view it is an important object,
 * hence we need to have access to its functionalities. Since we can't get it
 * in Thyra, and we don't want to expose particular implementations (e.g., Epetra
 * vs Tpetra), we implementa a very light-weight structure, that can store a graph
 * and, upon request, create a linear operator associated with that graph.
 * The implementation details of the graph are hidden, as is the concrete linear
 * algebra package underneath. The global function 'Albany::build_type' is used
 * to determine in which format the graph has to be stored.
 */

struct ThyraCrsGraphProxy {

  // Create an empty graph, that needs to be filled later
  ThyraCrsGraphProxy (const Teuchos::RCP<const Thyra_VectorSpace> domain_vs,
                      const Teuchos::RCP<const Thyra_VectorSpace> range_vs,
                      const int nonzeros_per_row,
                      const bool static_profile = false);

  // Create a graph from an overlapped one
  ThyraCrsGraphProxy (const Teuchos::RCP<const Thyra_VectorSpace> domain_vs,
                      const Teuchos::RCP<const Thyra_VectorSpace> range_vs,
                      const Teuchos::RCP<const ThyraCrsGraphProxy> overlap_src);

  void insertGlobalIndices (const GO row, const Teuchos::ArrayView<const GO>& indices);
  void fillComplete ();

  Teuchos::RCP<const Thyra_VectorSpace> getDomainVectorSpace () const { return m_domain_vs; }
  Teuchos::RCP<const Thyra_VectorSpace> getRangeVectorSpace  () const { return m_range_vs; }

  bool is_filled () const { return m_filled; }

  Teuchos::RCP<Thyra_LinearOp>  createOp () const;

private:

  // Struct hiding the concrete implementation. This is an implementation
  // detail of this class, so it's private and its implementation is not in the header.
  struct ProxyImpl;
  Teuchos::RCP<ProxyImpl> m_graph;

  Teuchos::RCP<const Thyra_VectorSpace> m_domain_vs;
  Teuchos::RCP<const Thyra_VectorSpace> m_range_vs;

  bool m_filled;
};

} // namespace Albany

#endif // ALBANY_THYRA_CRS_GRAPH_PROXY_HPP

// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.

#ifndef PHAL_DOF_SIDE_TO_CELL_HPP
#define PHAL_DOF_SIDE_TO_CELL_HPP

#include "Albany_Layouts.hpp"
#include "Phalanx_Evaluator_Derived.hpp"
#include "Phalanx_Evaluator_WithBaseImpl.hpp"
#include "Phalanx_MDField.hpp"
#include "Phalanx_config.hpp"

namespace PHAL {
/** \brief Finite Element SideToCell Evaluator

    This evaluator creates a field defined cell-side wise from a cell wise field

*/

template <typename EvalT, typename Traits, typename ScalarT>
class DOFSideToCellBase : public PHX::EvaluatorWithBaseImpl<Traits>, public PHX::EvaluatorDerived<EvalT, Traits>
{
 public:
  DOFSideToCellBase(Teuchos::ParameterList const& p, const Teuchos::RCP<Albany::Layouts>& dl);

  void
  postRegistrationSetup(typename Traits::SetupData d, PHX::FieldManager<Traits>& vm);

  void
  evaluateFields(typename Traits::EvalData d);

 private:
  std::string                   sideSetName;
  std::vector<std::vector<int>> sideNodes;
  std::vector<int>              dims;

  // Input:
  //! Values on side
  PHX::MDField<ScalarT const> val_side;

  // Output:
  //! Values at nodes
  PHX::MDField<ScalarT> val_cell;

  enum LayoutType
  {
    CELL_SCALAR = 1,
    CELL_VECTOR,
    CELL_TENSOR,
    NODE_SCALAR,
    NODE_VECTOR,
    NODE_TENSOR,
    VERTEX_VECTOR
  };

  LayoutType layout;
};

// Some shortcut names
template <typename EvalT, typename Traits>
using DOFSideToCell = DOFSideToCellBase<EvalT, Traits, typename EvalT::ScalarT>;

template <typename EvalT, typename Traits>
using DOFSideToCellMesh = DOFSideToCellBase<EvalT, Traits, typename EvalT::MeshScalarT>;

template <typename EvalT, typename Traits>
using DOFSideToCellParam = DOFSideToCellBase<EvalT, Traits, typename EvalT::ParamScalarT>;

}  // Namespace PHAL

#endif  // DOF_SIDE_TO_CELL_HPP

//
// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.
//

#include "Phalanx_DataLayout.hpp"
#include "Phalanx_Print.hpp"

// uncomment the following line if you want debug output to be printed to screen

namespace PHAL {

//*****
template <typename EvalT, typename Traits, typename ScalarT>
FieldFrobeniusNormBase<EvalT, Traits, ScalarT>::FieldFrobeniusNormBase(
    const Teuchos::ParameterList&        p,
    const Teuchos::RCP<Albany::Layouts>& dl)
{
  std::string fieldName     = p.get<std::string>("Field Name");
  std::string fieldNormName = p.get<std::string>("Field Norm Name");

  std::string layout = p.get<std::string>("Field Layout");
  if (layout == "Cell Vector") {
    field      = decltype(field)(fieldName, dl->cell_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->cell_scalar2);

    dl->cell_vector->dimensions(dims);
  } else if (layout == "Cell Gradient") {
    field      = decltype(field)(fieldName, dl->cell_gradient);
    field_norm = decltype(field_norm)(fieldNormName, dl->cell_scalar2);

    dl->cell_gradient->dimensions(dims);
  } else if (layout == "Cell Node Vector") {
    field      = decltype(field)(fieldName, dl->node_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->node_scalar);

    dl->node_vector->dimensions(dims);
  } else if (layout == "Cell QuadPoint Vector") {
    field      = decltype(field)(fieldName, dl->qp_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->qp_scalar);

    dl->qp_vector->dimensions(dims);
  } else if (layout == "Cell QuadPoint Gradient") {
    field      = decltype(field)(fieldName, dl->qp_gradient);
    field_norm = decltype(field_norm)(fieldNormName, dl->qp_scalar);

    dl->qp_gradient->dimensions(dims);
  } else if (layout == "Cell Side Vector") {
    sideSetName = p.get<std::string>("Side Set Name");

    ALBANY_PANIC(
        !dl->isSideLayouts,
        "Error! The layouts structure does not appear to be that of a side "
        "set.\n");

    field      = decltype(field)(fieldName, dl->cell_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->cell_scalar2);

    dl->cell_vector->dimensions(dims);
  } else if (layout == "Cell Side Gradient") {
    sideSetName = p.get<std::string>("Side Set Name");

    ALBANY_PANIC(
        !dl->isSideLayouts,
        "Error! The layouts structure does not appear to be that of a side "
        "set.\n");

    field      = decltype(field)(fieldName, dl->cell_gradient);
    field_norm = decltype(field_norm)(fieldNormName, dl->cell_scalar2);

    dl->cell_gradient->dimensions(dims);
  } else if (layout == "Cell Side Node Vector") {
    sideSetName = p.get<std::string>("Side Set Name");

    ALBANY_PANIC(
        !dl->isSideLayouts,
        "Error! The layouts structure does not appear to be that of a side "
        "set.\n");

    field      = decltype(field)(fieldName, dl->node_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->node_scalar);

    dl->node_vector->dimensions(dims);
  } else if (layout == "Cell Side QuadPoint Vector") {
    sideSetName = p.get<std::string>("Side Set Name");

    ALBANY_PANIC(
        !dl->isSideLayouts,
        "Error! The layouts structure does not appear to be that of a side "
        "set.\n");

    field      = decltype(field)(fieldName, dl->qp_vector);
    field_norm = decltype(field_norm)(fieldNormName, dl->qp_scalar);

    dl->qp_vector->dimensions(dims);
  } else if (layout == "Cell Side QuadPoint Gradient") {
    sideSetName = p.get<std::string>("Side Set Name");

    ALBANY_PANIC(
        !dl->isSideLayouts,
        "Error! The layouts structure does not appear to be that of a side "
        "set.\n");

    field      = decltype(field)(fieldName, dl->qp_gradient);
    field_norm = decltype(field_norm)(fieldNormName, dl->qp_scalar);

    dl->qp_gradient->dimensions(dims);
  } else {
    ALBANY_PANIC(
        true,
        "Error! Invalid field layout.\n");
  }

  this->addDependentField(field);
  this->addEvaluatedField(field_norm);

  Teuchos::ParameterList& options =
      p.get<Teuchos::ParameterList*>("Parameter List")->sublist(fieldNormName);
  std::string type = options.get<std::string>("Regularization Type", "None");
  if (type == "None") {
    regularization_type = NONE;
    regularization      = 0.0;
  } else if (type == "Given Value") {
    regularization_type = GIVEN_VALUE;
    regularization      = options.get<double>("Regularization Value");
    printedReg          = -1.0;
  } else if (type == "Given Parameter") {
    regularization_type = GIVEN_PARAMETER;
    regularizationParam = decltype(regularizationParam)(
        options.get<std::string>("Regularization Parameter Name"),
        dl->shared_param);
    this->addDependentField(regularizationParam);
    printedReg = -1.0;
  } else if (type == "Parameter Exponential") {
    regularization_type = PARAMETER_EXPONENTIAL;
    regularizationParam = decltype(regularizationParam)(
        options.get<std::string>("Regularization Parameter Name"),
        dl->shared_param);
    this->addDependentField(regularizationParam);
    printedReg = -1.0;
  } else {
    ALBANY_PANIC(
        true,
        "Error! Invalid regularization type");
  }

  numDims = dims.size();

  this->setName(
      "FieldFrobeniusNormBase(" + fieldNormName + ")" + PHX::print<EvalT>());
}

//*****
template <typename EvalT, typename Traits, typename ScalarT>
void
FieldFrobeniusNormBase<EvalT, Traits, ScalarT>::postRegistrationSetup(
    typename Traits::SetupData d,
    PHX::FieldManager<Traits>& fm)
{
  this->utils.setFieldData(field, fm);
  this->utils.setFieldData(field_norm, fm);

  if (regularization_type == GIVEN_PARAMETER ||
      regularization_type == PARAMETER_EXPONENTIAL)
    this->utils.setFieldData(regularizationParam, fm);
  d.fill_field_dependencies(this->dependentFields(), this->evaluatedFields());
}

//*****
template <typename EvalT, typename Traits, typename ScalarT>
void
FieldFrobeniusNormBase<EvalT, Traits, ScalarT>::evaluateFields(
    typename Traits::EvalData workset)
{
  if (regularization_type == GIVEN_PARAMETER)
    regularization =
        Albany::convertScalar<const ScalarT>(regularizationParam(0));
  else if (regularization_type == PARAMETER_EXPONENTIAL)
    regularization = pow(
        10.0,
        -10.0 * Albany::convertScalar<const ScalarT>(regularizationParam(0)));

  ScalarT norm;
  switch (numDims) {
    case 2:
      // <Cell,Vector/Gradient>
      for (int cell(0); cell < workset.numCells; ++cell) {
        norm = 0;
        for (int dim(0); dim < dims[1]; ++dim) {
          norm += std::pow(field(cell, dim), 2);
        }
        field_norm(cell) = std::sqrt(norm + regularization);
      }
      break;
    case 3:
      // <Cell,Node/QuadPoint,Vector/Gradient> or <Cell,Side,Vector/Gradient>
      for (int cell(0); cell < workset.numCells; ++cell) {
        for (int i(0); i < dims[1]; ++i) {
          norm = 0;
          for (int dim(0); dim < dims[2]; ++dim) {
            norm += std::pow(field(cell, i, dim), 2);
          }
          field_norm(cell, i) = std::sqrt(norm + regularization);
        }
      }
      break;
    case 4:
      // <Cell,Side,Node/QuadPoint,Vector/Gradient>
      {
        const Albany::SideSetList&          ssList = *(workset.sideSets);
        Albany::SideSetList::const_iterator it_ss  = ssList.find(sideSetName);

        if (it_ss == ssList.end()) return;

        const std::vector<Albany::SideStruct>&          sideSet = it_ss->second;
        std::vector<Albany::SideStruct>::const_iterator iter_s;
        for (iter_s = sideSet.begin(); iter_s != sideSet.end(); ++iter_s) {
          // Get the local data of side and cell
          const int cell = iter_s->elem_LID;
          const int side = iter_s->side_local_id;

          for (int i(0); i < dims[2]; ++i) {
            norm = 0;
            for (int dim(0); dim < dims[3]; ++dim) {
              norm += std::pow(field(cell, side, i, dim), 2);
            }
            field_norm(cell, side, i) = std::sqrt(norm + regularization);
          }
        }
      }
      break;
    default:
      ALBANY_ASSERT(false, "Error! Invalid field layout.\n");
  }
}

}  // namespace PHAL

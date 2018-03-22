//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include <fstream>
#include "Albany_Utils.hpp"
#include "Phalanx_DataLayout.hpp"
#include "Teuchos_TestForException.hpp"

namespace LCM {

template <typename EvalT, typename Traits>
ACEdensity<EvalT, Traits>::ACEdensity(
    Teuchos::ParameterList&              p,
    const Teuchos::RCP<Albany::Layouts>& dl)
    : density_(
          p.get<std::string>("QP Variable Name"),
          p.get<Teuchos::RCP<PHX::DataLayout>>("QP Scalar Data Layout")),
      porosity_(
          p.get<std::string>("ACE Porosity"),
          dl->qp_scalar)
{
  Teuchos::ParameterList* density_list =
    p.get<Teuchos::ParameterList*>("Parameter List");

  Teuchos::RCP<PHX::DataLayout> vector_dl =
    p.get< Teuchos::RCP<PHX::DataLayout>>("QP Vector Data Layout");
  std::vector<PHX::DataLayout::size_type> dims;
  vector_dl->dimensions(dims);
  num_qps_  = dims[1];
  num_dims_ = dims[2];

  Teuchos::RCP<ParamLib> paramLib =
    p.get< Teuchos::RCP<ParamLib>>("Parameter Library", Teuchos::null);

  // Read density values
  rho_ice_ = density_list->get<double>("Ice Value");
  rho_wat_ = density_list->get<double>("Water Value");
  rho_sed_ = density_list->get<double>("Sediment Value");

  // Add density as a Sacado-ized parameter
  this->registerSacadoParameter("ACE Density", paramLib);

  // List evaluated fields
  this->addEvaluatedField(density_);
  
  // List dependent fields
  this->addDependentField(porosity_);
  
  this->setName("ACE Density" + PHX::typeAsString<EvalT>());
}

//
template <typename EvalT, typename Traits>
void
ACEdensity<EvalT, Traits>::postRegistrationSetup(
    typename Traits::SetupData d,
    PHX::FieldManager<Traits>& fm)
{
  // List all fields
  this->utils.setFieldData(density_, fm);
  this->utils.setFieldData(porosity_, fm);
  return;
}

//
// This function needs to know the water, ice, and sediment intrinsic densities
// plus the current QP ice/water saturations and QP porosity which come from 
// the material model.
// The density calculation is based on a volume average mixture model.
template <typename EvalT, typename Traits>
void
ACEdensity<EvalT, Traits>::evaluateFields(typename Traits::EvalData workset)
{
  int num_cells = workset.numCells;
  double w = 0.50;  // this is an evolving QP parameter, here temporary
  double f = 0.50;  // this is an evolving QP parameter, here temporary

  for (int cell = 0; cell < num_cells; ++cell) {
    for (int qp = 0; qp < num_qps_; ++qp) {
      density_(cell, qp) = porosity_(cell, qp)*(rho_ice_*f + rho_wat_*w) + 
                           ((1.0-porosity_(cell, qp))*rho_sed_);
    }
  }

  return;
}

//
template <typename EvalT, typename Traits>
typename ACEdensity<EvalT, Traits>::ScalarT&
ACEdensity<EvalT, Traits>::getValue(const std::string& n)
{
  if (n == "ACE Ice Density") {
    return rho_ice_;
  }
  if (n == "ACE Water Density") {
    return rho_wat_;
  }
  if (n == "ACE Sediment Density") {
    return rho_sed_;
  }

  ALBANY_ASSERT(false, "Invalid request for value of ACE Component Density");

  return rho_wat_; // does it matter what we return here?
}

}  // namespace LCM

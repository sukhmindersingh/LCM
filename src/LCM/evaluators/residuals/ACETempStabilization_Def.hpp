// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.

#include "Albany_Macros.hpp"
#include "Intrepid2_FunctionSpaceTools.hpp"
#include "PHAL_Utilities.hpp"
#include "Phalanx_DataLayout.hpp"

namespace LCM {

//*****
template <typename EvalT, typename Traits>
ACETempStabilization<EvalT, Traits>::ACETempStabilization(Teuchos::ParameterList const& p)
    : 
      tdot_(
          p.get<std::string>("QP Time Derivative Variable Name"),
          p.get<Teuchos::RCP<PHX::DataLayout>>("QP Scalar Data Layout")),
      wgradbf_(
          p.get<std::string>("Weighted Gradient BF Name"),
          p.get<Teuchos::RCP<PHX::DataLayout>>("Node QP Vector Data Layout")),
      tgrad_(
          p.get<std::string>("Gradient QP Variable Name"),
          p.get<Teuchos::RCP<PHX::DataLayout>>("QP Vector Data Layout")),
      stab_(p.get<std::string>("Stabilization Name"), 
            p.get<Teuchos::RCP<PHX::DataLayout>>("Node Scalar Data Layout")),
      thermal_cond_grad_at_qps_(p.get<std::string>("ACE Thermal Conductivity Gradient QP Variable Name"), 
			        p.get<Teuchos::RCP<PHX::DataLayout>>("QP Vector Data Layout")), 
      thermal_inertia_(
          p.get<std::string>("ACE Thermal Inertia QP Variable Name"),
          p.get<Teuchos::RCP<PHX::DataLayout>>("QP Scalar Data Layout")),
      tau_(p.get<std::string>("Tau Name"), 
           p.get<Teuchos::RCP<PHX::DataLayout>>("QP Scalar Data Layout"))
{
  this->addDependentField(tdot_);
  this->addDependentField(tgrad_);
  this->addDependentField(wgradbf_);
  this->addDependentField(thermal_cond_grad_at_qps_);
  this->addDependentField(thermal_inertia_);
  this->addEvaluatedField(stab_);
  this->addEvaluatedField(tau_);

  use_stab_ = p.get<bool>("Use Stabilization"); 

  Teuchos::RCP<PHX::DataLayout> vector_dl = p.get<Teuchos::RCP<PHX::DataLayout>>("Node QP Vector Data Layout");
  std::vector<PHX::DataLayout::size_type> dims;
  vector_dl->dimensions(dims);
  workset_size_ = dims[0];
  num_nodes_    = dims[1];
  num_qps_      = dims[2];
  num_dims_     = dims[3];
  this->setName("ACETempStabilization");
}

//*****
template <typename EvalT, typename Traits>
void
ACETempStabilization<EvalT, Traits>::postRegistrationSetup(
    typename Traits::SetupData d,
    PHX::FieldManager<Traits>& fm)
{
  this->utils.setFieldData(tgrad_, fm);
  this->utils.setFieldData(wgradbf_, fm);
  this->utils.setFieldData(tdot_, fm);
  this->utils.setFieldData(stab_, fm);
  this->utils.setFieldData(thermal_inertia_, fm);
  this->utils.setFieldData(thermal_cond_grad_at_qps_, fm);
  this->utils.setFieldData(tau_, fm);
}

//*****
template <typename EvalT, typename Traits>
void
ACETempStabilization<EvalT, Traits>::evaluateFields(typename Traits::EvalData workset)
{
  //Initialize stab_ to all zeros.  This will be the value 
  //returned in the case stabilization is turned off.
  for (std::size_t cell = 0; cell < workset_size_; ++cell) {
    for (std::size_t node = 0; node < num_nodes_; ++node) {
      stab_(cell, node) = 0.0;
    }
  }
  if (use_stab_ == true) {
    //Here we use a GLS-type stabilization which takes the form:
    //stab = -grad(kappa)*grad(w)*tau*(c*dT/dt - (grad(kappa)*grad(T))) 
    //for this problem, where tau is the stabilization parameter.
    //Here, we use tau = h^num_dims_/2.0/|grad(kappa)| 
    //as the stabilization parameter, following a common choice
    //in the literature.
    for (std::size_t cell = 0; cell < workset_size_; ++cell) {
      for (std::size_t qp = 0; qp < num_qps_; ++qp) {
        tau_(cell, qp) = 0.0; 
	ScalarT normGradKappa = 0.0; 
        for (std::size_t ndim = 0; ndim < num_dims_; ++ndim) {
	  normGradKappa += thermal_cond_grad_at_qps_(cell, qp, ndim); 
	}
	normGradKappa = std::sqrt(normGradKappa); 
      }
    }
    for (std::size_t cell = 0; cell < workset_size_; ++cell) {
      for (std::size_t node = 0; node < num_nodes_; ++node) {
        for (std::size_t qp = 0; qp < num_qps_; ++qp) {
          for (std::size_t ndim = 0; ndim < num_dims_; ++ndim) {
            stab_(cell, node) -= thermal_cond_grad_at_qps_(cell, qp, ndim) * wgradbf_(cell, node, qp, ndim)
	                         * tau_(cell, qp) * (thermal_inertia_(cell, qp) * tdot_(cell, qp)  - 
				 thermal_cond_grad_at_qps_(cell, qp, ndim) * tgrad_(cell, qp, ndim)); 
	  }
	}
      }
    }
  }
}

//*****
}  // namespace LCM

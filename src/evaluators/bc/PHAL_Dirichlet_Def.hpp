//
// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.
//

#include "Albany_Macros.hpp"
#include "Albany_ThyraUtils.hpp"
#include "PHAL_Dirichlet.hpp"
#include "Phalanx_DataLayout.hpp"
#include "Sacado_ParameterRegistration.hpp"

// **********************************************************************
// Genereric Template Code for Constructor and PostRegistrationSetup
// **********************************************************************

namespace PHAL {

template <typename EvalT, typename Traits>
DirichletBase<EvalT, Traits>::DirichletBase(Teuchos::ParameterList& p)
    : offset(p.get<int>("Equation Offset")),
      nodeSetID(p.get<std::string>("Node Set ID"))
{
  value = p.get<RealType>("Dirichlet Value");

  auto const name  = p.get<std::string>("Dirichlet Name");
  auto const dummy = p.get<Teuchos::RCP<PHX::DataLayout>>("Data Layout");
  PHX::Tag<ScalarT> const fieldTag(name, dummy);

  this->addEvaluatedField(fieldTag);
  this->setName(name + PHX::print<EvalT>());

  // Set up values as parameters for parameter library
  auto paramLib =
      p.get<Teuchos::RCP<ParamLib>>("Parameter Library", Teuchos::null);
  this->registerSacadoParameter(name, paramLib);

  {
    // Impose an ordering on DBC evaluators. This code works with the function
    // imposeOrder defined elsewhere. It happens that "Data Layout" is Dummy, so
    // we can use it.
    if (p.isType<std::string>("BCOrder Dependency")) {
      PHX::Tag<ScalarT> order_depends_on(
          p.get<std::string>("BCOrder Dependency"), dummy);
      this->addDependentField(order_depends_on);
    }
    if (p.isType<std::string>("BCOrder Evaluates")) {
      PHX::Tag<ScalarT> order_evaluates(
          p.get<std::string>("BCOrder Evaluates"), dummy);
      this->addEvaluatedField(order_evaluates);
    }
  }
}

template <typename EvalT, typename Traits>
void
DirichletBase<EvalT, Traits>::postRegistrationSetup(
    typename Traits::SetupData d,
    PHX::FieldManager<Traits>& /* fm */)
{
  d.fill_field_dependencies(this->dependentFields(), this->evaluatedFields());
}

// **********************************************************************
// Specialization: Residual
// **********************************************************************
template <typename Traits>
Dirichlet<PHAL::AlbanyTraits::Residual, Traits>::Dirichlet(
    Teuchos::ParameterList& p)
    : DirichletBase<PHAL::AlbanyTraits::Residual, Traits>(p)
{
}

// **********************************************************************
template <typename Traits>
void
Dirichlet<PHAL::AlbanyTraits::Residual, Traits>::evaluateFields(
    typename Traits::EvalData dirichletWorkset)
{
  auto const x      = dirichletWorkset.x;
  auto       f      = dirichletWorkset.f;
  auto const x_view = Albany::getLocalData(x);
  auto       f_view = Albany::getNonconstLocalData(f);

  // Grab the vector off node GIDs for this Node Set ID from the std::map
  std::vector<std::vector<int>> const& ns_nodes =
      dirichletWorkset.nodeSets->find(this->nodeSetID)->second;

  for (auto ns_node = 0; ns_node < ns_nodes.size(); ++ns_node) {
    auto const dof = ns_nodes[ns_node][this->offset];
    f_view[dof]    = x_view[dof] - this->value;
    // Record DOFs to avoid setting Schwarz BCs on them.
    dirichletWorkset.fixed_dofs_.insert(dof);
  }
}

// **********************************************************************
// Specialization: Jacobian
// **********************************************************************
template <typename Traits>
Dirichlet<PHAL::AlbanyTraits::Jacobian, Traits>::Dirichlet(
    Teuchos::ParameterList& p)
    : DirichletBase<PHAL::AlbanyTraits::Jacobian, Traits>(p)
{
}

// **********************************************************************
template <typename Traits>
void
Dirichlet<PHAL::AlbanyTraits::Jacobian, Traits>::evaluateFields(
    typename Traits::EvalData dirichletWorkset)
{
  auto        x       = dirichletWorkset.x;
  auto        f       = dirichletWorkset.f;
  auto        J       = dirichletWorkset.Jac;
  auto const  fill    = f != Teuchos::null;
  auto        f_view  = fill ? Albany::getNonconstLocalData(f) : Teuchos::null;
  auto        x_view  = fill ? Albany::getLocalData(x) : Teuchos::null;
  auto const  j_coeff = dirichletWorkset.j_coeff;
  auto const& ns_nodes =
      dirichletWorkset.nodeSets->find(this->nodeSetID)->second;

  Teuchos::Array<LO> index(1);
  Teuchos::Array<ST> value(1);
  Teuchos::Array<ST> entries;
  Teuchos::Array<LO> indices;
  value[0] = j_coeff;

  for (auto ns_node = 0; ns_node < ns_nodes.size(); ++ns_node) {
    auto const dof = ns_nodes[ns_node][this->offset];
    index[0]       = dof;

    // Extract the row, zero it out, then put j_coeff on diagonal
    Albany::getLocalRowValues(J, dof, indices, entries);
    for (auto& val : entries) { val = 0.0; }
    Albany::setLocalRowValues(J, dof, indices(), entries());
    Albany::setLocalRowValues(J, dof, index(), value());

    if (fill == true) { f_view[dof] = x_view[dof] - this->value.val(); }
  }
}

// **********************************************************************
// Simple evaluator to aggregate all Dirichlet BCs into one "field"
// **********************************************************************

template <typename EvalT, typename Traits>
DirichletAggregator<EvalT, Traits>::DirichletAggregator(
    Teuchos::ParameterList& p)
{
  auto        dl = p.get<Teuchos::RCP<PHX::DataLayout>>("Data Layout");
  auto const& dbcs =
      *p.get<Teuchos::RCP<std::vector<std::string>>>("DBC Names");

  for (auto i = 0; i < dbcs.size(); ++i) {
    PHX::Tag<ScalarT> fieldTag(dbcs[i], dl);
    this->addDependentField(fieldTag);
  }

  PHX::Tag<ScalarT> fieldTag(p.get<std::string>("DBC Aggregator Name"), dl);
  this->addEvaluatedField(fieldTag);
  this->setName("Dirichlet Aggregator" + PHX::print<EvalT>());
}

// **********************************************************************
template <typename EvalT, typename Traits>
void
DirichletAggregator<EvalT, Traits>::postRegistrationSetup(
    typename Traits::SetupData d,
    PHX::FieldManager<Traits>& vm)
{
  d.fill_field_dependencies(this->dependentFields(), this->evaluatedFields());
}

// **********************************************************************
}  // namespace PHAL

// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.

#ifndef ACETHERMALPROBLEM_HPP
#define ACETHERMALPROBLEM_HPP

#include "Albany_AbstractProblem.hpp"
#include "Albany_MaterialDatabase.hpp"
#include "Albany_ProblemUtils.hpp"
#include "PHAL_ConvertFieldType.hpp"
#include "PHAL_Dimension.hpp"
#include "PHAL_Workset.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"

namespace Albany {

/*!
 * \brief Abstract interface for representing finite element
 * problem.
 */
class ACEThermalProblem : public AbstractProblem
{
 public:
  //! Default constructor
  ACEThermalProblem(
      const Teuchos::RCP<Teuchos::ParameterList>& params,
      const Teuchos::RCP<ParamLib>&               paramLib,
      // const Teuchos::RCP<DistributedParameterLibrary>& distParamLib,
      int const                               numDim_,
      Teuchos::RCP<Teuchos::Comm<int> const>& commT_);

  //! Destructor
  ~ACEThermalProblem();

  //! Return number of spatial dimensions
  virtual int
  spatialDimension() const
  {
    return numDim;
  }

  //! Get boolean telling code if SDBCs are utilized
  virtual bool
  useSDBCs() const
  {
    return use_sdbcs_;
  }

  //! Build the PDE instantiations, boundary conditions, and initial solution
  virtual void
  buildProblem(
      Teuchos::ArrayRCP<Teuchos::RCP<Albany::MeshSpecsStruct>> meshSpecs,
      StateManager&                                            stateMgr);

  // Build evaluators
  virtual Teuchos::Array<Teuchos::RCP<const PHX::FieldTag>>
  buildEvaluators(
      PHX::FieldManager<PHAL::AlbanyTraits>&      fm0,
      Albany::MeshSpecsStruct const&              meshSpecs,
      Albany::StateManager&                       stateMgr,
      Albany::FieldManagerChoice                  fmchoice,
      const Teuchos::RCP<Teuchos::ParameterList>& responseList);

  //! Each problem must generate it's list of valide parameters
  Teuchos::RCP<Teuchos::ParameterList const>
  getValidProblemParameters() const;

 private:
  //! Private to prohibit copying
  ACEThermalProblem(const ACEThermalProblem&);

  //! Private to prohibit copying
  ACEThermalProblem&
  operator=(const ACEThermalProblem&);

 public:
  //! Main problem setup routine. Not directly called, but indirectly by
  //! following functions
  template <typename EvalT>
  Teuchos::RCP<const PHX::FieldTag>
  constructEvaluators(
      PHX::FieldManager<PHAL::AlbanyTraits>&      fm0,
      Albany::MeshSpecsStruct const&              meshSpecs,
      Albany::StateManager&                       stateMgr,
      Albany::FieldManagerChoice                  fmchoice,
      const Teuchos::RCP<Teuchos::ParameterList>& responseList);

  void
  constructDirichletEvaluators(std::vector<std::string> const& nodeSetIDs);
  void
  constructNeumannEvaluators(
      const Teuchos::RCP<Albany::MeshSpecsStruct>& meshSpecs);

 protected:
  int                    numDim;  // number spatial dimensions
  double                 C;       // heat capacity
  double                 rho;     // density
  Teuchos::RCP<Albany::MaterialDatabase> materialDB;

  const Teuchos::RCP<Teuchos::ParameterList> params;

  Teuchos::RCP<Teuchos::Comm<int> const> commT;

  Teuchos::RCP<Albany::Layouts> dl;

  /// Boolean marking whether SDBCs are used
  bool use_sdbcs_;
};

}  // namespace Albany

#include "Albany_EvaluatorUtils.hpp"
#include "Albany_ProblemUtils.hpp"
#include "Albany_ResponseUtilities.hpp"
#include "Albany_Utils.hpp"
#include "Intrepid2_DefaultCubatureFactory.hpp"
#include "ACEThermalConductivity.hpp"
#include "ACEThermalInertia.hpp"
#include "ACETempStandAloneResid.hpp"
#include "Shards_CellTopology.hpp"

template <typename EvalT>
Teuchos::RCP<const PHX::FieldTag>
Albany::ACEThermalProblem::constructEvaluators(
    PHX::FieldManager<PHAL::AlbanyTraits>&      fm0,
    Albany::MeshSpecsStruct const&              meshSpecs,
    Albany::StateManager&                       stateMgr,
    Albany::FieldManagerChoice                  fieldManagerChoice,
    const Teuchos::RCP<Teuchos::ParameterList>& responseList)
{
  using PHAL::AlbanyTraits;
  using PHX::DataLayout;
  using PHX::MDALayout;
  using std::string;
  using std::vector;
  using Teuchos::ParameterList;
  using Teuchos::RCP;
  using Teuchos::rcp;

  const CellTopologyData* const elem_top = &meshSpecs.ctd;

  RCP<Intrepid2::Basis<PHX::Device, RealType, RealType>> intrepidBasis =
      Albany::getIntrepid2Basis(*elem_top);
  RCP<shards::CellTopology> cellType = rcp(new shards::CellTopology(elem_top));

  int const numNodes    = intrepidBasis->getCardinality();
  int const worksetSize = meshSpecs.worksetSize;

  Intrepid2::DefaultCubatureFactory     cubFactory;
  RCP<Intrepid2::Cubature<PHX::Device>> cellCubature =
      cubFactory.create<PHX::Device, RealType, RealType>(
          *cellType, meshSpecs.cubatureDegree);

  int const numQPtsCell = cellCubature->getNumPoints();
  int const numVertices = cellType->getNodeCount();

  // Problem is steady or transient
  ALBANY_PANIC(
      number_of_time_deriv != 1,
      "ACETempStandAloneProblem must be defined as transient "
      "calculation.");

  *out << "Field Dimensions: Workset=" << worksetSize
       << ", Vertices= " << numVertices << ", Nodes= " << numNodes
       << ", QuadPts= " << numQPtsCell << ", Dim= " << numDim << std::endl;

  dl = rcp(new Albany::Layouts(
      worksetSize, numVertices, numNodes, numQPtsCell, numDim));
  Albany::EvaluatorUtils<EvalT, PHAL::AlbanyTraits> evalUtils(dl);

  // Temporary variable used numerous times below
  Teuchos::RCP<PHX::Evaluator<AlbanyTraits>> ev;

  Teuchos::ArrayRCP<string> dof_names(neq);
  dof_names[0] = "Temperature";
  Teuchos::ArrayRCP<string> dof_names_dot(neq);
  dof_names_dot[0] = "Temperature_dot";
  Teuchos::ArrayRCP<string> resid_names(neq);
  resid_names[0] = "Temperature Residual";

  fm0.template registerEvaluator<EvalT>(
      evalUtils.constructGatherSolutionEvaluator(
          false, dof_names, dof_names_dot));

  fm0.template registerEvaluator<EvalT>(
      evalUtils.constructScatterResidualEvaluator(false, resid_names));

  fm0.template registerEvaluator<EvalT>(
      evalUtils.constructGatherCoordinateVectorEvaluator());

  fm0.template registerEvaluator<EvalT>(
      evalUtils.constructMapToPhysicalFrameEvaluator(cellType, cellCubature));

  fm0.template registerEvaluator<EvalT>(
      evalUtils.constructComputeBasisFunctionsEvaluator(
          cellType, intrepidBasis, cellCubature));

  for (unsigned int i = 0; i < neq; i++) {
    fm0.template registerEvaluator<EvalT>(
        evalUtils.constructDOFInterpolationEvaluator(dof_names[i]));

    fm0.template registerEvaluator<EvalT>(
        evalUtils.constructDOFInterpolationEvaluator(dof_names_dot[i]));

    fm0.template registerEvaluator<EvalT>(
        evalUtils.constructDOFGradInterpolationEvaluator(dof_names[i]));
  }

  // ACE thermal conductivity
  {
    RCP<ParameterList> p = rcp(new ParameterList);

    p->set<string>("QP Variable Name", "ACE ThermalConductivity");
    p->set<string>("QP Coordinate Vector Name", "Coord Vec");
    p->set< RCP<DataLayout> >("Node Data Layout", dl->node_scalar);
    p->set< RCP<DataLayout> >("QP Scalar Data Layout", dl->qp_scalar);
    p->set< RCP<DataLayout> >("QP Vector Data Layout", dl->qp_vector);

    p->set<RCP<ParamLib> >("Parameter Library", paramLib);
    Teuchos::ParameterList& paramList = params->sublist("ACEThermalConductivity");
    p->set<Teuchos::ParameterList*>("Parameter List", &paramList);

    // Here we assume that the instance of this problem applies on a single element block
    p->set<string>("Element Block Name", meshSpecs.ebName);

    if(materialDB != Teuchos::null)
      p->set< RCP<Albany::MaterialDatabase> >("MaterialDB", materialDB);

    ev = rcp(new LCM::ACEThermalConductivity<EvalT,AlbanyTraits>(*p));
    fm0.template registerEvaluator<EvalT>(ev);
  }
              
  // ACE thermal inertia
  {
    RCP<ParameterList> p = rcp(new ParameterList);

    p->set<string>("QP Variable Name", "ACE ThermalInertia");
    p->set<string>("QP Coordinate Vector Name", "Coord Vec");
    p->set< RCP<DataLayout> >("Node Data Layout", dl->node_scalar);
    p->set< RCP<DataLayout> >("QP Scalar Data Layout", dl->qp_scalar);
    p->set< RCP<DataLayout> >("QP Vector Data Layout", dl->qp_vector);

    p->set<RCP<ParamLib> >("Parameter Library", paramLib);
    Teuchos::ParameterList& paramList = params->sublist("ACEThermalInertia");
    p->set<Teuchos::ParameterList*>("Parameter List", &paramList);

    // Here we assume that the instance of this problem applies on a single element block
    p->set<string>("Element Block Name", meshSpecs.ebName);

    if(materialDB != Teuchos::null)
      p->set< RCP<Albany::MaterialDatabase> >("MaterialDB", materialDB);

    ev = rcp(new LCM::ACEThermalInertia<EvalT,AlbanyTraits>(*p));
    fm0.template registerEvaluator<EvalT>(ev);
  }
              
  {  // Temperature Resid
    RCP<ParameterList> p = rcp(new ParameterList("Temperature Resid"));

    // Input
    p->set<string>("Weighted BF Name", "wBF");
    p->set<string>("QP Time Derivative Variable Name", "Temperature_dot");
    p->set<string>("Gradient QP Variable Name", "Temperature Gradient");
    p->set<string>("Weighted Gradient BF Name", "wGrad BF");

    p->set<RCP<DataLayout>>("Node QP Scalar Data Layout", dl->node_qp_scalar);
    p->set<RCP<DataLayout>>("QP Scalar Data Layout", dl->qp_scalar);
    p->set<RCP<DataLayout>>("QP Vector Data Layout", dl->qp_vector);
    p->set<RCP<DataLayout>>("Node QP Vector Data Layout", dl->node_qp_vector);

    p->set<double>("Heat Capacity", C);
    p->set<double>("Density", rho);

    p->set<string>("ACE ThermalConductivity Name", "ACE ThermalConductivity");
    p->set<string>("ACE ThermalInertia Name", "ACE ThermalInertia");
    p->set< RCP<DataLayout> >("QP Scalar Data Layout", dl->qp_scalar);

    // Output
    p->set<string>("Residual Name", "Temperature Residual");
    p->set<RCP<DataLayout>>("Node Scalar Data Layout", dl->node_scalar);

    ev = rcp(new LCM::ACETempStandAloneResid<EvalT, AlbanyTraits>(*p));
    fm0.template registerEvaluator<EvalT>(ev);
  }

  if (fieldManagerChoice == Albany::BUILD_RESID_FM) {
    PHX::Tag<typename EvalT::ScalarT> res_tag("Scatter", dl->dummy);
    fm0.requireField<EvalT>(res_tag);
    return res_tag.clone();
  }

  else if (fieldManagerChoice == Albany::BUILD_RESPONSE_FM) {
    Albany::ResponseUtilities<EvalT, PHAL::AlbanyTraits> respUtils(dl);
    return respUtils.constructResponses(
        fm0, *responseList, Teuchos::null, stateMgr);
  }

  return Teuchos::null;
}

#endif  // ALBANY_THERMALNONLINEARSOURCEPROBLEM_HPP

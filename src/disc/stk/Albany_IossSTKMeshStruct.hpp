//
// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.
//

#ifndef ALBANY_IOSS_STKMESHSTRUCT_HPP
#define ALBANY_IOSS_STKMESHSTRUCT_HPP

#include <Ionit_Initializer.h>

#include <stk_io/IossBridge.hpp>
#include <stk_io/StkMeshIoBroker.hpp>

#include "Albany_GenericSTKMeshStruct.hpp"
#include "Albany_config.h"

namespace Albany {

class IossSTKMeshStruct : public GenericSTKMeshStruct
{
 public:
  IossSTKMeshStruct(
      const Teuchos::RCP<Teuchos::ParameterList>& params,
      const Teuchos::RCP<Teuchos::ParameterList>& adaptParams,
      const Teuchos::RCP<Teuchos_Comm const>&     commT);

  ~IossSTKMeshStruct();

  void
  setFieldAndBulkData(
      const Teuchos::RCP<Teuchos_Comm const>&                   commT,
      const Teuchos::RCP<Teuchos::ParameterList>&               params,
      const unsigned int                                        neq_,
      const AbstractFieldContainer::FieldContainerRequirements& req,
      const Teuchos::RCP<Albany::StateInfoStruct>&              sis,
      const unsigned int                                        worksetSize,
      std::map<std::string, Teuchos::RCP<Albany::StateInfoStruct>> const&
          side_set_sis = {},
      const std::
          map<std::string, AbstractFieldContainer::FieldContainerRequirements>&
              side_set_req = {});

  int
  getSolutionFieldHistoryDepth() const
  {
    return m_solutionFieldHistoryDepth;
  }
  double
  getSolutionFieldHistoryStamp(int step) const;
  void
  loadSolutionFieldHistory(int step);

  //! Flag if solution has a restart values -- used in Init Cond
  bool
  hasRestartSolution() const
  {
    return m_hasRestartSolution;
  }

  //! If restarting, convenience function to return restart data time
  double
  restartDataTime() const
  {
    return m_restartDataTime;
  }

 private:
  Ioss::Init::Initializer ioInit;

  void
  loadOrSetCoordinates3d(int index);

  Teuchos::RCP<Teuchos::ParameterList const>
  getValidDiscretizationParameters() const;

  Teuchos::RCP<Teuchos::FancyOStream>    out;
  bool                                   usePamgen;
  bool                                   useSerialMesh;
  bool                                   periodic;
  Teuchos::RCP<stk::io::StkMeshIoBroker> mesh_data;

  bool   m_hasRestartSolution;
  double m_restartDataTime;
  int    m_solutionFieldHistoryDepth;
};

}  // Namespace Albany

#endif  // ALBANY_IOSS_STKMESHSTRUCT_HPP

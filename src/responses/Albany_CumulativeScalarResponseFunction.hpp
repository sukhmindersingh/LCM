//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#ifndef ALBANY_CUMULATIVE_SCALAR_RESPONSE_FUNCTION_HPP
#define ALBANY_CUMULATIVE_SCALAR_RESPONSE_FUNCTION_HPP

#include "Albany_SamplingBasedScalarResponseFunction.hpp"
#include "Teuchos_Array.hpp"

namespace Albany {

  /*!
   * \brief A response function that aggregates together multiple response
   * functions into one.
   */
  class CumulativeScalarResponseFunction :
    public SamplingBasedScalarResponseFunction {
  public:
  
    //! Default constructor
    CumulativeScalarResponseFunction(
      const Teuchos::RCP<const Teuchos_Comm>& commT,
      const Teuchos::Array< Teuchos::RCP<ScalarResponseFunction> >& responses);

    //! Setup response function
    virtual void setupT();

    //!Perform post registration setup
    virtual void postRegSetup();

    //! Destructor
    virtual ~CumulativeScalarResponseFunction();

    //! Get the number of responses
    virtual unsigned int numResponses() const;

    //! Evaluate response
    virtual void 
    evaluateResponseT(const double current_time,
		     const Tpetra_Vector* xdotT,
		     const Tpetra_Vector* xdotdotT,
		     const Tpetra_Vector& xT,
		     const Teuchos::Array<ParamVec>& p,
		     Tpetra_Vector& gT); 

    //! Evaluate tangent = dg/dx*dx/dp + dg/dxdot*dxdot/dp + dg/dp
    virtual void 
    evaluateTangentT(const double alpha, 
		    const double beta,
		    const double omega,
		    const double current_time,
		    bool sum_derivs,
		    const Tpetra_Vector* xdot,
		    const Tpetra_Vector* xdotdot,
		    const Tpetra_Vector& x,
		    const Teuchos::Array<ParamVec>& p,
		    ParamVec* deriv_p,
		    const Tpetra_MultiVector* Vxdot,
		    const Tpetra_MultiVector* Vxdotdot,
		    const Tpetra_MultiVector* Vx,
		    const Tpetra_MultiVector* Vp,
		    Tpetra_Vector* g,
		    Tpetra_MultiVector* gx,
		    Tpetra_MultiVector* gp);
    
    virtual void 
    evaluateGradientT(const double current_time,
		     const Tpetra_Vector* xdotT,
		     const Tpetra_Vector* xdotdotT,
		     const Tpetra_Vector& xT,
		     const Teuchos::Array<ParamVec>& p,
		     ParamVec* deriv_p,
		     Tpetra_Vector* gT,
		     Tpetra_MultiVector* dg_dxT,
		     Tpetra_MultiVector* dg_dxdotT,
		     Tpetra_MultiVector* dg_dxdotdotT,
		     Tpetra_MultiVector* dg_dpT);

  private:

    //! Evaluate Multi Vector distributed derivative dg_dp
    virtual void
    evaluateDistParamDerivT(
         const double current_time,
         const Tpetra_Vector* xdotT,
         const Tpetra_Vector* xdotdotT,
         const Tpetra_Vector& xT,
         const Teuchos::Array<ParamVec>& param_array,
         const std::string& dist_param_name,
         Tpetra_MultiVector* dg_dpT);

  private:

    //! Private to prohibit copying
    CumulativeScalarResponseFunction(const CumulativeScalarResponseFunction&);
    
    //! Private to prohibit copying
    CumulativeScalarResponseFunction& operator=(const CumulativeScalarResponseFunction&);

  protected:

    //! Response functions to add
    Teuchos::Array< Teuchos::RCP<ScalarResponseFunction> > responses;
    unsigned int num_responses;

  };

}

#endif // ALBANY_CUMULATIVE_SCALAR_RESPONSE_FUNCTION_HPP

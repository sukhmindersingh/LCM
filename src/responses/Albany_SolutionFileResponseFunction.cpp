//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include "Albany_SolutionFileResponseFunction.hpp"

#include "Albany_SolutionFileResponseFunction_Def.hpp"

namespace Albany {

template class SolutionFileResponseFunction<Albany::NormTwo>;
template class SolutionFileResponseFunction<Albany::NormInf>;

}  // namespace Albany

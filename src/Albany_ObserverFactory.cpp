//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//
#include <string>

#include "Albany_ObserverFactory.hpp"
#include "Teuchos_ParameterList.hpp"

namespace Albany {

NOXObserverFactory::NOXObserverFactory(const Teuchos::RCP<Application>& app)
    : app_(app)
{
}

}  // namespace Albany

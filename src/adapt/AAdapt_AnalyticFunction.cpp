//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include "AAdapt_AnalyticFunction.hpp"

#include <cmath>
#include <cstdlib>
#include <ctime>

#include "Teuchos_Exceptions.hpp"
#include "Teuchos_TestForException.hpp"

const double pi = 3.141592653589793;

// Factory method to build functions based on a string
Teuchos::RCP<AAdapt::AnalyticFunction>
AAdapt::createAnalyticFunction(
    std::string            name,
    int                    neq,
    int                    numDim,
    Teuchos::Array<double> data)
{
  Teuchos::RCP<AAdapt::AnalyticFunction> F;

  if (name == "Constant")
    F = Teuchos::rcp(new AAdapt::ConstantFunction(neq, numDim, data));

  else if (name == "Step X")
    F = Teuchos::rcp(new AAdapt::StepX(neq, numDim, data));

  else if (name == "TemperatureStep")
    F = Teuchos::rcp(new AAdapt::TemperatureStep(neq, numDim, data));

  else if (name == "Displacement Constant TemperatureStep")
    F = Teuchos::rcp(new AAdapt::DispConstTemperatureStep(neq, numDim, data));

  else if (name == "Displacement Constant TemperatureLinear")
    F = Teuchos::rcp(new AAdapt::DispConstTemperatureLinear(neq, numDim, data));

  else if (name == "TemperatureLinear")
    F = Teuchos::rcp(new AAdapt::TemperatureLinear(neq, numDim, data));

  else if (name == "1D Gauss-Sin")
    F = Teuchos::rcp(new AAdapt::GaussSin(neq, numDim, data));

  else if (name == "1D Gauss-Cos")
    F = Teuchos::rcp(new AAdapt::GaussCos(neq, numDim, data));

  else if (name == "Linear Y")
    F = Teuchos::rcp(new AAdapt::LinearY(neq, numDim, data));

  else if (name == "Linear")
    F = Teuchos::rcp(new AAdapt::Linear(neq, numDim, data));

  else if (name == "Constant Box")
    F = Teuchos::rcp(new AAdapt::ConstantBox(neq, numDim, data));

  else if (name == "About Z")
    F = Teuchos::rcp(new AAdapt::AboutZ(neq, numDim, data));

  else if (name == "Radial Z")
    F = Teuchos::rcp(new AAdapt::RadialZ(neq, numDim, data));

  else if (name == "About Linear Z")
    F = Teuchos::rcp(new AAdapt::AboutLinearZ(neq, numDim, data));

  else if (name == "Gaussian Z")
    F = Teuchos::rcp(new AAdapt::GaussianZ(neq, numDim, data));

  else if (name == "Circle")
    F = Teuchos::rcp(new AAdapt::Circle(neq, numDim, data));

  else if (name == "Gaussian Pressure")
    F = Teuchos::rcp(new AAdapt::GaussianPress(neq, numDim, data));

  else if (name == "Sin-Cos")
    F = Teuchos::rcp(new AAdapt::SinCos(neq, numDim, data));

  else if (name == "Sin Scalar")
    F = Teuchos::rcp(new AAdapt::SinScalar(neq, numDim, data));

  else if (name == "Taylor-Green Vortex")
    F = Teuchos::rcp(new AAdapt::TaylorGreenVortex(neq, numDim, data));

  else if (name == "1D Acoustic Wave")
    F = Teuchos::rcp(new AAdapt::AcousticWave(neq, numDim, data));

  else
    TEUCHOS_TEST_FOR_EXCEPTION(
        name != "Valid Initial Condition Function",
        std::logic_error,
        "Unrecognized initial condition function name: " << name);

  return F;
}

//*****************************************************************************
AAdapt::ConstantFunction::ConstantFunction(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != neq),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for Constant Function; neq = "
          << neq << ", data.size() = " << data.size() << std::endl);
}
void
AAdapt::ConstantFunction::compute(double* x, const double* X)
{
  if (data.size() > 0)
    for (int i = 0; i < neq; i++) x[i] = data[i];
}

//*****************************************************************************
AAdapt::StepX::StepX(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 5),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for Step X; Length = "
          << 5 << ", data.size() = " << data.size() << std::endl);
}

void
AAdapt::StepX::compute(double* x, const double* X)
{
  // Temperature bottom
  double T0 = data[0];
  // Temperature top
  double T1 = data[1];
  // constant temperature
  double T = data[2];
  // bottom x-coordinate
  double X0 = data[3];
  // top x-coordinate
  double X1 = data[4];

  const double TOL = 1.0e-12;

  // bottom
  if (X[0] < (X0 + TOL)) {
    x[0] = T0;
  } else if (X[0] > (X1 - TOL)) {
    x[0] = T1;
  } else {
    x[0] = T;
  }
}
//*****************************************************************************
#ifdef ALBANY_TSUNAMI
AAdapt::TsunamiBoussinesq1DSolitaryWave::TsunamiBoussinesq1DSolitaryWave(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq != 3 || numDim != 1 || data.size() != 3),  // ZW changed from 5 to 3
      std::logic_error,
      "Error! Invalid call of TsunamiBoussinesq1DSolitaryWave with "
          << neq << " " << numDim << " " << data.size() << "!\n");
}
// ZW: only need A1 and c
void
AAdapt::TsunamiBoussinesq1DSolitaryWave::compute(double* x, const double* X)
{
  double A1  = data[0];
  double c   = data[1];
  double X_0 = data[2];

  // Coordinate x is given by X[0]
  // FIXME, FILL IN for Zhiheng and Xiaoshu!
  x[0] = A1 * exp(-0.5 * (X[0] - X_0) * (X[0] - X_0));   // initial eta
  x[1] = A1 * exp(-0.25 * (X[0] - X_0) * (X[0] - X_0));  // initial ualpha
  x[2] = 0.25 * A1 * exp(-0.25 * (X[0] - X_0) * (X[0] - X_0)) *
         ((X[0] - X_0) * (X[0] - X_0) - 2);  // initial E1
}
#endif

//*****************************************************************************
AAdapt::TemperatureStep::TemperatureStep(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 6),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for TemperatureStep; Length = "
          << 6 << ", data.size() = " << data.size() << std::endl);
}

void
AAdapt::TemperatureStep::compute(double* x, const double* X)
{
  // Temperature bottom
  double T0 = data[0];
  // Temperature top
  double T1 = data[1];
  // constant temperature
  double T = data[2];
  // bottom coordinate
  double Z0 = data[3];
  // top coordinate
  double Z1 = data[4];
  // flag to specify which coordinate we want.
  // 0 == x-coordinate
  // 1 == y-coordinate
  // 2 == z-cordinate
  int coord = static_cast<int>(data[5]);

  // check that coordinate is valid
  if ((coord > 2) || (coord < 0)) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Coordinate not valid!" << std::endl);
  }

  const double TOL = 1.0e-12;

  // bottom
  if (X[coord] < (Z0 + TOL)) {
    x[0] = T0;
  } else if (X[coord] > (Z1 - TOL)) {
    x[0] = T1;
  } else {
    x[0] = T;
  }
}

//*****************************************************************************
AAdapt::DispConstTemperatureStep::DispConstTemperatureStep(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 9),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for Displacement Constant TemperatureStep; Length = "
          << 9 << ", data.size() = " << data.size() << std::endl);
}

void
AAdapt::DispConstTemperatureStep::compute(double* x, const double* X)
{
  // Get displacement
  for (int i = 0; i < 3; i++) x[i] = data[i];
  // Temperature bottom
  double T0 = data[3];
  // Temperature top
  double T1 = data[4];
  // constant temperature
  double T = data[5];
  // bottom coordinate
  double Z0 = data[6];
  // top coordinate
  double Z1 = data[7];
  // flag to specify which coordinate we want.
  // 0 == x-coordinate
  // 1 == y-coordinate
  // 2 == z-cordinate
  int coord = static_cast<int>(data[8]);

  // check that coordinate is valid
  if ((coord > 2) || (coord < 0)) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Coordinate not valid!" << std::endl);
  }

  const double TOL = 1.0e-12;

  // bottom
  if (X[coord] < (Z0 + TOL)) {
    x[3] = T0;
  } else if (X[coord] > (Z1 - TOL)) {
    x[3] = T1;
  } else {
    x[3] = T;
  }
}

//*****************************************************************************
AAdapt::DispConstTemperatureLinear::DispConstTemperatureLinear(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 8),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for Displacement Constant TemperatureLinear; Length = "
          << 8 << ", data.size() = " << data.size() << std::endl);
}

void
AAdapt::DispConstTemperatureLinear::compute(double* x, const double* X)
{
  // Get displacement
  for (int i = 0; i < 3; i++) x[i] = data[i];
  // Temperature bottom
  double T0 = data[3];
  // Temperature top
  double T1 = data[4];
  // bottom coordinate
  double Z0 = data[5];
  // top coordinate
  double Z1 = data[6];
  // flag to specify which coordinate we want.
  // 0 == x-coordinate
  // 1 == y-coordinate
  // 2 == z-cordinate
  int coord = static_cast<int>(data[7]);

  // check that coordinate is valid
  if ((coord > 2) || (coord < 0)) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Coordinate not valid!" << std::endl);
  }

  const double TOL = 1.0e-12;

  // check that temperatures are not equal
  if (std::abs(T0 - T1) <= TOL) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Temperature are equals!" << std::endl);
  }
  // check coordinates are not equal
  if (std::abs(Z0 - Z1) <= TOL) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true,
        std::logic_error,
        "Error! Z-coordinates are the same!" << std::endl);
  }

  // We interpolate Temperature as a linear function of z-ccordinate: T = b +
  // m*z
  double b = (T1 * Z0 - T0 * Z1) / (Z0 - Z1);
  //
  double m = (T0 - T1) / (Z0 - Z1);

  // assign temperature
  x[3] = b + m * X[coord];
}

//*****************************************************************************
AAdapt::TemperatureLinear::TemperatureLinear(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 5),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
      "Function Data for TemperatureLinear; Length = "
          << 5 << ", data.size() = " << data.size() << std::endl);
}

void
AAdapt::TemperatureLinear::compute(double* x, const double* X)
{
  // Temperature bottom
  double T0 = data[0];
  // Temperature top
  double T1 = data[1];
  // bottom coordinate
  double Z0 = data[2];
  // top coordinate
  double Z1 = data[3];
  // flag to specify which coordinate we want.
  // 0 == x-coordinate
  // 1 == y-coordinate
  // 2 == z-cordinate
  int coord = static_cast<int>(data[4]);

  // check that coordinate is valid
  if ((coord > 2) || (coord < 0)) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Coordinate not valid!" << std::endl);
  }

  const double TOL = 1.0e-12;

  // check that temperatures are not equal
  if (std::abs(T0 - T1) <= TOL) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Error! Temperature are equals!" << std::endl);
  }
  // check coordinates are not equal
  if (std::abs(Z0 - Z1) <= TOL) {
    TEUCHOS_TEST_FOR_EXCEPTION(
        true,
        std::logic_error,
        "Error! Z-coordinates are the same!" << std::endl);
  }

  // We interpolate Temperature as a linear function of z-ccordinate: T = b +
  // m*z
  double b = (T1 * Z0 - T0 * Z1) / (Z0 - Z1);
  //
  double m = (T0 - T1) / (Z0 - Z1);

  // assign temperature
  x[0] = b + m * X[coord];
}

//*****************************************************************************
// Private convenience function
long
AAdapt::seedgen(int worksetID)
{
  long seconds, s, seed, pid;

  pid = getpid();
  s   = time(&seconds); /* get CPU seconds since 01/01/1970 */

  // Use worksetID to give more randomness between calls

  seed = std::abs(((s * 181) * ((pid - 83) * 359) * worksetID) % 104729);
  return seed;
}

AAdapt::ConstantFunctionPerturbed::ConstantFunctionPerturbed(
    int                    neq_,
    int                    numDim_,
    int                    worksetID,
    Teuchos::Array<double> data_,
    Teuchos::Array<double> pert_mag_)
    : numDim(numDim_), neq(neq_), data(data_), pert_mag(pert_mag_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != neq || pert_mag.size() != neq),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
          << "Function Data for Constant Function Perturbed; neq = " << neq
          << ", data.size() = " << data.size()
          << ", pert_mag.size() = " << pert_mag.size() << std::endl);

  //  srand( time(NULL) ); // seed the random number gen
  srand(seedgen(worksetID));  // seed the random number gen
}

void
AAdapt::ConstantFunctionPerturbed::compute(double* x, const double* X)
{
  for (int i = 0; i < neq; i++)
    x[i] = data[i] + udrand(-pert_mag[i], pert_mag[i]);
}

// Private convenience function
double
AAdapt::ConstantFunctionPerturbed::udrand(double lo, double hi)
{
  static const double base    = 1.0 / (RAND_MAX + 1.0);
  double              deviate = std::rand() * base;
  return lo + deviate * (hi - lo);
}
//*****************************************************************************
AAdapt::ConstantFunctionGaussianPerturbed::ConstantFunctionGaussianPerturbed(
    int                    neq_,
    int                    numDim_,
    int                    worksetID,
    Teuchos::Array<double> data_,
    Teuchos::Array<double> pert_mag_)
    : numDim(numDim_),
      neq(neq_),
      data(data_),
      pert_mag(pert_mag_),
      //      rng(boost::random::random_device()()), // seed the rng
      rng(seedgen(worksetID)),  // seed the rng
      nd(neq_),
      var_nor(neq_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != neq || pert_mag.size() != neq),
      std::logic_error,
      "Error! Invalid specification of initial condition: incorrect length of "
          << "Function Data for Constant Function Gaussian Perturbed; neq = "
          << neq << ", data.size() = " << data.size()
          << ", pert_mag.size() = " << pert_mag.size() << std::endl);

  if (data.size() > 0 && pert_mag.size() > 0)
    for (int i = 0; i < neq; i++)
      if (pert_mag[i] > std::numeric_limits<double>::epsilon()) {
        nd[i] = Teuchos::rcp(
            new boost::normal_distribution<double>(data[i], pert_mag[i]));
        var_nor[i] =
            Teuchos::rcp(new boost::variate_generator<
                         boost::mt19937&,
                         boost::normal_distribution<double>>(rng, *nd[i]));
      }
}

void
AAdapt::ConstantFunctionGaussianPerturbed::compute(double* x, const double* X)
{
  for (int i = 0; i < neq; i++)
    if (var_nor[i] != Teuchos::null)
      x[i] = (*var_nor[i])();

    else
      x[i] = data[i];
}

//*****************************************************************************
AAdapt::GaussSin::GaussSin(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq != 1) || (numDim != 1) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of GaussSin with " << neq << " " << numDim << "  "
                                              << data.size() << std::endl);
}
void
AAdapt::GaussSin::compute(double* x, const double* X)
{
  x[0] = sin(pi * X[0]) + 0.5 * data[0] * X[0] * (1.0 - X[0]);
}

//*****************************************************************************
AAdapt::GaussCos::GaussCos(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq != 1) || (numDim != 1) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of GaussCos with " << neq << " " << numDim << "  "
                                              << data.size() << std::endl);
}
void
AAdapt::GaussCos::compute(double* x, const double* X)
{
  x[0] = 1 + cos(2 * pi * X[0]) + 0.5 * data[0] * X[0] * (1.0 - X[0]);
}
//*****************************************************************************
AAdapt::LinearY::LinearY(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 2) || (numDim < 2) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of LinearY with " << neq << " " << numDim << "  "
                                             << data.size() << std::endl);
}
void
AAdapt::LinearY::compute(double* x, const double* X)
{
  x[0] = 0.0;
  x[1] = data[0] * X[0];

  if (numDim > 2) x[2] = 0.0;
}
//*****************************************************************************
AAdapt::Linear::Linear(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != neq * numDim),
      std::logic_error,
      "Error! Invalid call of Linear with " << neq << " " << numDim << "  "
                                            << data.size() << std::endl);
}
void
AAdapt::Linear::compute(double* x, const double* X)
{
  for (auto eq = 0; eq < neq; ++eq) {
    double s{0.0};
    for (auto dim = 0; dim < numDim; ++dim) {
      s += data[eq * numDim + dim] * X[dim];
    }
    x[eq] = s;
  }
}
//*****************************************************************************
AAdapt::ConstantBox::ConstantBox(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (data.size() != 2 * numDim + neq),
      std::logic_error,
      "Error! Invalid call of Linear with " << neq << " " << numDim << "  "
                                            << data.size() << std::endl);
}
void
AAdapt::ConstantBox::compute(double* x, const double* X)
{
  bool in_box{true};
  for (auto dim = 0; dim < numDim; ++dim) {
    double const& lo = data[dim];
    double const& hi = data[dim + numDim];
    in_box           = in_box && lo <= X[dim] && X[dim] <= hi;
  }

  if (in_box == true) {
    for (auto eq = 0; eq < neq; ++eq) { x[eq] = data[2 * numDim + eq]; }
  }
}
//*****************************************************************************
AAdapt::AboutZ::AboutZ(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 2) || (numDim < 2) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of AboutZ with " << neq << " " << numDim << "  "
                                            << data.size() << std::endl);
}
void
AAdapt::AboutZ::compute(double* x, const double* X)
{
  x[0] = -data[0] * X[1];
  x[1] = data[0] * X[0];

  if (neq > 2) x[2] = 0.0;
}
//*****************************************************************************
AAdapt::RadialZ::RadialZ(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 2) || (numDim < 2) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of RadialZ with " << neq << " " << numDim << "  "
                                             << data.size() << std::endl);
}
void
AAdapt::RadialZ::compute(double* x, const double* X)
{
  x[0] = data[0] * X[0];
  x[1] = data[0] * X[1];

  if (neq > 2) x[2] = 0.0;
}
//*****************************************************************************
AAdapt::AboutLinearZ::AboutLinearZ(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 3) || (numDim < 3) || (data.size() != 1),
      std::logic_error,
      "Error! Invalid call of AboutLinearZ with "
          << neq << " " << numDim << "  " << data.size() << std::endl);
}
void
AAdapt::AboutLinearZ::compute(double* x, const double* X)
{
  x[0] = -data[0] * X[1] * X[2];
  x[1] = data[0] * X[0] * X[2];
  x[2] = 0.0;
}
//*****************************************************************************
AAdapt::GaussianZ::GaussianZ(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 2) || (numDim < 2) || (data.size() != 3),
      std::logic_error,
      "Error! Invalid call of GaussianZ with " << neq << " " << numDim << "  "
                                               << data.size() << std::endl);
}
void
AAdapt::GaussianZ::compute(double* x, const double* X)
{
  double const a = data[0];
  double const b = data[1];
  double const c = data[2];
  double const d = X[2] - b;

  x[0] = 0.0;
  x[1] = 0.0;
  x[2] = a * std::exp(-d * d / c / c / 2.0);
}
//*****************************************************************************
AAdapt::Circle::Circle(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  bool error = true;
  if (neq == 1 || neq == 3) error = false;
  TEUCHOS_TEST_FOR_EXCEPTION(
      error || (numDim != 2),
      std::logic_error,
      "Error! Invalid call of Circle with " << neq << " " << numDim << "  "
                                            << data.size() << std::endl);
}
void
AAdapt::Circle::compute(double* x, const double* X)
{
  if (((X[0] - .5) * (X[0] - .5) + (X[1] - .5) * (X[1] - .5)) < 1.0 / 16.0)
    x[0] = 1.0;
  else
    x[0] = 0.0;

  // This would be the initial condition for the auxiliary variables, but it
  // should not be needed.
  /*if (neq == 3) {
    x[1] = 0.0;
    x[2] = 0.0;
  }*/
}
//*****************************************************************************
AAdapt::GaussianPress::GaussianPress(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 3) || (numDim < 2) || (data.size() != 4),
      std::logic_error,
      "Error! Invalid call of GaussianPress with "
          << neq << " " << numDim << "  " << data.size() << std::endl);
}
void
AAdapt::GaussianPress::compute(double* x, const double* X)
{
  for (int i = 0; i < neq - 1; i++) { x[i] = 0.0; }

  x[neq - 1] = data[0] * exp(-data[1] * ((X[0] - data[2]) * (X[0] - data[2]) +
                                         (X[1] - data[3]) * (X[1] - data[3])));
}
//*****************************************************************************
AAdapt::SinCos::SinCos(int neq_, int numDim_, Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 3) || (numDim < 2),
      std::logic_error,
      "Error! Invalid call of SinCos with " << neq << " " << numDim << "  "
                                            << data.size() << std::endl);
}
void
AAdapt::SinCos::compute(double* x, const double* X)
{
  x[0] = sin(2.0 * pi * X[0]) * cos(2.0 * pi * X[1]);
  x[1] = cos(2.0 * pi * X[0]) * sin(2.0 * pi * X[1]);
  x[2] = sin(2.0 * pi * X[0]) * sin(2.0 * pi * X[1]);
}
//*****************************************************************************
AAdapt::SinScalar::SinScalar(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      neq != 1 || numDim < 2 || data.size() != numDim,
      std::logic_error,
      "Error! Invalid call of SinScalar with " << neq << " " << numDim << "  "
                                               << data.size() << std::endl);
}
void
AAdapt::SinScalar::compute(double* x, const double* X)
{
  x[0] = 1.0;
  for (int dim{0}; dim < numDim; ++dim) {
    x[0] *= sin(pi / data[dim] * X[dim]);
  }
}
//*****************************************************************************
AAdapt::TaylorGreenVortex::TaylorGreenVortex(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq < 3) || (numDim != 2),
      std::logic_error,
      "Error! Invalid call of TaylorGreenVortex with "
          << neq << " " << numDim << "  " << data.size() << std::endl);
}
void
AAdapt::TaylorGreenVortex::compute(double* x, const double* X)
{
  x[0] = 1.0;                                           // initial density
  x[1] = -cos(2.0 * pi * X[0]) * sin(2.0 * pi * X[1]);  // initial u-velocity
  x[2] = sin(2.0 * pi * X[0]) * cos(2.0 * pi * X[1]);   // initial v-velocity
  x[3] = cos(2.0 * pi * X[0]) + cos(2.0 * pi * X[1]);   // initial temperature
}
//*****************************************************************************
AAdapt::AcousticWave::AcousticWave(
    int                    neq_,
    int                    numDim_,
    Teuchos::Array<double> data_)
    : numDim(numDim_), neq(neq_), data(data_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      (neq > 3) || (numDim > 2) || (data.size() != 3),
      std::logic_error,
      "Error! Invalid call of AcousticWave with "
          << neq << " " << numDim << "  " << data.size() << std::endl);
}
void
AAdapt::AcousticWave::compute(double* x, const double* X)
{
  const double U0 = data[0];
  const double n  = data[1];
  const double L  = data[2];
  x[0]            = U0 * cos(n * X[0] / L);

  for (int i = 1; i < numDim; i++) x[i] = 0.0;
}

//*****************************************************************************
// ExpressionParser

AAdapt::ExpressionParser::ExpressionParser(
    int         neq_,
    int         spatialDim_,
    std::string expressionX_,
    std::string expressionY_,
    std::string expressionZ_)
    : spatialDim(spatialDim_),
      neq(neq_),
      expressionX(expressionX_),
      expressionY(expressionY_),
      expressionZ(expressionZ_)
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      neq < 1 || neq > spatialDim || spatialDim != 3,
      std::logic_error,
      "Error! Invalid call AAdapt::ExpressionParser::ExpressionParser(), neq = "
          << neq << ", spatialDim = " << spatialDim << ".");

  bool success;

#ifdef ALBANY_PAMGEN
  // set up RTCompiler
  rtcFunctionX.addVar("double", "x");
  rtcFunctionX.addVar("double", "y");
  rtcFunctionX.addVar("double", "z");
  rtcFunctionX.addVar("double", "value");
  success = rtcFunctionX.addBody(expressionX);
  if (!success) {
    std::string msg =
        "\n**** Error in AAdapt::ExpressionParser::ExpressionParser().\n";
    msg += "**** " + rtcFunctionX.getErrors() + "\n";
    TEUCHOS_TEST_FOR_EXCEPT_MSG(!success, msg);
  }

  if (neq > 1) {
    rtcFunctionY.addVar("double", "x");
    rtcFunctionY.addVar("double", "y");
    rtcFunctionY.addVar("double", "z");
    rtcFunctionY.addVar("double", "value");
    success = rtcFunctionY.addBody(expressionY);
    if (!success) {
      std::string msg =
          "\n**** Error in AAdapt::ExpressionParser::ExpressionParser().\n";
      msg += "**** " + rtcFunctionY.getErrors() + "\n";
      TEUCHOS_TEST_FOR_EXCEPT_MSG(!success, msg);
    }
  }

  if (neq > 2) {
    rtcFunctionZ.addVar("double", "x");
    rtcFunctionZ.addVar("double", "y");
    rtcFunctionZ.addVar("double", "z");
    rtcFunctionZ.addVar("double", "value");
    success = rtcFunctionZ.addBody(expressionZ);
    if (!success) {
      std::string msg =
          "\n**** Error in AAdapt::ExpressionParser::ExpressionParser().\n";
      msg += "**** " + rtcFunctionZ.getErrors() + "\n";
      TEUCHOS_TEST_FOR_EXCEPT_MSG(!success, msg);
    }
  }
#endif
}

void
AAdapt::ExpressionParser::compute(double* solution, const double* X)
{
  bool   success;
  double value;

#ifdef ALBANY_PAMGEN
  for (int i = 0; i < spatialDim; i++) {
    success = rtcFunctionX.varValueFill(i, X[i]);
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
        !success,
        "Error inAAdapt::ExpressionParser::compute(), "
        "rtcFunctionX.varValueFill(), " +
            rtcFunctionX.getErrors());
  }
  success = rtcFunctionX.varValueFill(spatialDim, 0.0);
  TEUCHOS_TEST_FOR_EXCEPT_MSG(
      !success,
      "Error inAAdapt::ExpressionParser::compute(), "
      "rtcFunctionX.varValueFill(), " +
          rtcFunctionX.getErrors());
  success = rtcFunctionX.execute();
  TEUCHOS_TEST_FOR_EXCEPT_MSG(
      !success,
      "Error inAAdapt::ExpressionParser::compute(), rtcFunctionX.execute(), " +
          rtcFunctionX.getErrors());
  solution[0] = rtcFunctionX.getValueOfVar("value");

  if (neq > 1) {
    for (int i = 0; i < spatialDim; i++) {
      success = rtcFunctionY.varValueFill(i, X[i]);
      TEUCHOS_TEST_FOR_EXCEPT_MSG(
          !success,
          "Error inAAdapt::ExpressionParser::compute(), "
          "rtcFunctionY.varValueFill(), " +
              rtcFunctionY.getErrors());
    }
    success = rtcFunctionY.varValueFill(spatialDim, 0.0);
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
        !success,
        "Error inAAdapt::ExpressionParser::compute(), "
        "rtcFunctionY.varValueFill(), " +
            rtcFunctionY.getErrors());
    success = rtcFunctionY.execute();
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
        !success,
        "Error inAAdapt::ExpressionParser::compute(), "
        "rtcFunctionY.execute(), " +
            rtcFunctionY.getErrors());
    solution[1] = rtcFunctionY.getValueOfVar("value");
  }

  if (neq > 2) {
    for (int i = 0; i < spatialDim; i++) {
      success = rtcFunctionZ.varValueFill(i, X[i]);
      TEUCHOS_TEST_FOR_EXCEPT_MSG(
          !success,
          "Error inAAdapt::ExpressionParser::compute(), "
          "rtcFunctionZ.varValueFill(), " +
              rtcFunctionZ.getErrors());
    }
    success = rtcFunctionZ.varValueFill(spatialDim, 0.0);
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
        !success,
        "Error inAAdapt::ExpressionParser::compute(), "
        "rtcFunctionZ.varValueFill(), " +
            rtcFunctionZ.getErrors());
    success = rtcFunctionZ.execute();
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
        !success,
        "Error inAAdapt::ExpressionParser::compute(), "
        "rtcFunctionZ.execute(), " +
            rtcFunctionZ.getErrors());
    solution[2] = rtcFunctionZ.getValueOfVar("value");
  }
#endif

  //   std::cout << "DEBUG CHECK ExpressionParser " << expressionX << "
  //   evaluated at " << X[0] << ", " << X[1] << ", " << X[2] << " yields " <<
  //   solution[0] << std::endl; std::cout << "DEBUG CHECK ExpressionParser " <<
  //   expressionY << " evaluated at " << X[0] << ", " << X[1] << ", " << X[2]
  //   << " yields " << solution[1] << std::endl; std::cout << "DEBUG CHECK
  //   ExpressionParser " << expressionZ << " evaluated at " << X[0] << ", " <<
  //   X[1] << ", " << X[2] << " yields " << solution[2] << std::endl;

  return;
}

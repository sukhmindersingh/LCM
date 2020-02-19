//
// Albany 3.0: Copyright 2016 National Technology & Engineering Solutions of
// Sandia, LLC (NTESS). This Software is released under the BSD license detailed
// in the file license.txt in the top-level Albany directory.
//

#include "ACEcommon.hpp"
#include "ACEice.hpp"
#include "Albany_STKDiscretization.hpp"

namespace LCM {

template <typename EvalT, typename Traits>
ACEiceMiniKernel<EvalT, Traits>::ACEiceMiniKernel(
    ConstitutiveModel<EvalT, Traits>&    model,
    Teuchos::ParameterList*              p,
    Teuchos::RCP<Albany::Layouts> const& dl)
    : BaseKernel(model)
{
  this->setIntegrationPointLocationFlag(true);

  // Baseline constants
  sat_mod_              = p->get<RealType>("Saturation Modulus", 0.0);
  sat_exp_              = p->get<RealType>("Saturation Exponent", 0.0);
  ice_density_          = p->get<RealType>("ACE Ice Density", 0.0);
  water_density_        = p->get<RealType>("ACE Water Density", 0.0);
  ice_thermal_cond_     = p->get<RealType>("ACE Ice Thermal Conductivity", 0.0);
  water_thermal_cond_   = p->get<RealType>("ACE Water Thermal Conductivity", 0.0);
  ice_heat_capacity_    = p->get<RealType>("ACE Ice Heat Capacity", 0.0);
  water_heat_capacity_  = p->get<RealType>("ACE Water Heat Capacity", 0.0);
  ice_saturation_init_  = p->get<RealType>("ACE Ice Initial Saturation", 0.0);
  ice_saturation_max_   = p->get<RealType>("ACE Ice Maximum Saturation", 0.0);
  water_saturation_min_ = p->get<RealType>("ACE Water Minimum Saturation", 0.0);
  salinity_base_        = p->get<RealType>("ACE Base Salinity", 0.0);
  salt_enhanced_D_      = p->get<RealType>("ACE Salt Enhanced D", 0.0);
  freeze_curve_width_   = p->get<RealType>("ACE Freezing Curve Width", 1.0);
  f_shift_              = p->get<RealType>("ACE Freezing Curve Shift", 0.25);
  latent_heat_          = p->get<RealType>("ACE Latent Heat", 0.0);
  porosity0_            = p->get<RealType>("ACE Surface Porosity", 0.0);
  erosion_rate_         = p->get<RealType>("ACE Erosion Rate", 0.0);
  element_size_         = p->get<RealType>("ACE Element Size", 0.0);
  critical_stress_      = p->get<RealType>("ACE Critical Stress", 0.0);
  critical_angle_       = p->get<RealType>("ACE Critical Angle", 0.0);

  if (p->isParameter("ACE Time File") == true) {
    auto const filename = p->get<std::string>("ACE Time File");
    time_               = vectorFromFile(filename);
  }
  if (p->isParameter("ACE Sea Level File") == true) {
    auto const filename = p->get<std::string>("ACE Sea Level File");
    sea_level_          = vectorFromFile(filename);
  }
  if (p->isParameter("ACE Z Depth File") == true) {
    auto const filename     = p->get<std::string>("ACE Z Depth File");
    z_above_mean_sea_level_ = vectorFromFile(filename);
  }
  if (p->isParameter("ACE Salinity File") == true) {
    auto const filename = p->get<std::string>("ACE Salinity File");
    salinity_           = vectorFromFile(filename);
    ALBANY_ASSERT(
      z_above_mean_sea_level_.size() == salinity_.size(),
      "*** ERROR: Number of z values and number of salinity values in ACE "
      "Salinity File must match.");
  }
  if (p->isParameter("ACE Ocean Salinity File") == true) {
    auto const filename = p->get<std::string>("ACE Ocean Salinity File");
    ocean_salinity_     = vectorFromFile(filename);
    ALBANY_ASSERT(
      time_.size() == ocean_salinity_.size(),
      "*** ERROR: Number of time values and number of ocean salinity values in "
      "ACE Ocean Salinity File must match.");
  }
  if (p->isParameter("ACE Porosity File") == true) {
    auto const filename = p->get<std::string>("ACE Porosity File");
    porosity_from_file_ = vectorFromFile(filename);
    ALBANY_ASSERT(
      z_above_mean_sea_level_.size() == porosity_from_file_.size(),
      "*** ERROR: Number of z values and number of porosity values in "
      "ACE Porosity File must match.");
  }
  if (p->isParameter("ACE Freezing Curve Width File") == true) {
    auto const filename = p->get<std::string>("ACE Freezing Curve Width File");
    freezing_curve_width_ = vectorFromFile(filename);
    ALBANY_ASSERT(
      z_above_mean_sea_level_.size() == freezing_curve_width_.size(),
      "*** ERROR: Number of z values and number of freezing curve width values "
      "in "
      "ACE Freezing Curve Width File must match.");
  }
  
  ALBANY_ASSERT(
      time_.size() == sea_level_.size(),
      "*** ERROR: Number of times and number of sea level values must match");

  // retrieve appropriate field name strings
  auto const cauchy_string       = field_name_map_["Cauchy_Stress"];
  auto const Fp_string           = field_name_map_["Fp"];
  auto const eqps_string         = field_name_map_["eqps"];
  auto const yieldSurface_string = field_name_map_["Yield_Surface"];
  auto const F_string            = field_name_map_["F"];
  auto const J_string            = field_name_map_["J"];

  // define the dependent fields
  setDependentField(F_string, dl->qp_tensor);
  setDependentField(J_string, dl->qp_scalar);
  setDependentField("Elastic Modulus", dl->qp_scalar);
  setDependentField("Hardening Modulus", dl->qp_scalar);
  setDependentField("Poissons Ratio", dl->qp_scalar);
  setDependentField("Yield Strength", dl->qp_scalar);
  setDependentField("Delta Time", dl->workset_scalar);
  setDependentField("ACE Temperature", dl->qp_scalar);

  // define the evaluated fields
  setEvaluatedField("ACE Bluff Salinity", dl->qp_scalar);
  setEvaluatedField("ACE Ice Saturation", dl->qp_scalar);
  setEvaluatedField("ACE Density", dl->qp_scalar);
  setEvaluatedField("ACE Heat Capacity", dl->qp_scalar);
  setEvaluatedField("ACE Thermal Conductivity", dl->qp_scalar);
  setEvaluatedField("ACE Thermal Inertia", dl->qp_scalar);
  setEvaluatedField("ACE Water Saturation", dl->qp_scalar);
  setEvaluatedField("ACE Porosity", dl->qp_scalar);
  setEvaluatedField("ACE Temperature Dot", dl->qp_scalar);
  setEvaluatedField("Failure Indicator", dl->cell_scalar);
  setEvaluatedField("ACE Exposure Time", dl->qp_scalar);
  setEvaluatedField(cauchy_string, dl->qp_tensor);
  setEvaluatedField(Fp_string, dl->qp_tensor);
  setEvaluatedField(eqps_string, dl->qp_scalar);
  setEvaluatedField(yieldSurface_string, dl->qp_scalar);

  // define the state variables

  // stress
  addStateVariable(
      cauchy_string,
      dl->qp_tensor,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output Cauchy Stress", false));

  // Fp
  addStateVariable(
      Fp_string,
      dl->qp_tensor,
      "identity",
      0.0,
      true,
      p->get<bool>("Output Fp", false));

  // eqps
  addStateVariable(
      eqps_string,
      dl->qp_scalar,
      "scalar",
      0.0,
      true,
      p->get<bool>("Output eqps", false));

  // yield surface
  addStateVariable(
      yieldSurface_string,
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output Yield Surface", false));
  
  // ACE Bluff salinity
  addStateVariable(
      "ACE Bluff Salinity",
      dl->qp_scalar,
      "scalar",
      salinity_base_,
      true,
      p->get<bool>("Output ACE Bluff Salinity", false));

  // ACE Ice saturation
  addStateVariable(
      "ACE Ice Saturation",
      dl->qp_scalar,
      "scalar",
      ice_saturation_init_,
      true,
      p->get<bool>("Output ACE Ice Saturation", false));

  // ACE Density
  addStateVariable(
      "ACE Density",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output ACE Density", false));

  // ACE Heat Capacity
  addStateVariable(
      "ACE Heat Capacity",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output ACE Heat Capacity", false));

  // ACE Thermal Conductivity
  addStateVariable(
      "ACE Thermal Conductivity",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output ACE Thermal Conductivity", false));

  // ACE Thermal Inertia
  addStateVariable(
      "ACE Thermal Inertia",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output ACE Thermal Inertia", false));

  // ACE Water Saturation
  addStateVariable(
      "ACE Water Saturation",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("ACE Water Saturation", false));

  // ACE Porosity
  addStateVariable(
      "ACE Porosity",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("ACE Porosity", false));

  // ACE Temperature is already registered in Mechanics Problem.

  // ACE Temperature Dot
  addStateVariable(
      "ACE Temperature Dot",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("ACE Temperature Dot", false));

  // failed state
  addStateVariable(
      "Failure Indicator",
      dl->cell_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output Failure Indicator", true));

  // exposure time
  addStateVariable(
      "ACE Exposure Time",
      dl->qp_scalar,
      "scalar",
      0.0,
      false,
      p->get<bool>("Output ACE Exposure Time", true));
}

template <typename EvalT, typename Traits>
void
ACEiceMiniKernel<EvalT, Traits>::init(
    Workset&                 workset,
    FieldMap<const ScalarT>& input_fields,
    FieldMap<ScalarT>&       output_fields)
{
  auto const cauchy_string       = field_name_map_["Cauchy_Stress"];
  auto const Fp_string           = field_name_map_["Fp"];
  auto const eqps_string         = field_name_map_["eqps"];
  auto const yieldSurface_string = field_name_map_["Yield_Surface"];
  auto const F_string            = field_name_map_["F"];
  auto const J_string            = field_name_map_["J"];

  def_grad_          = *input_fields[F_string];
  J_                 = *input_fields[J_string];
  elastic_modulus_   = *input_fields["Elastic Modulus"];
  hardening_modulus_ = *input_fields["Hardening Modulus"];
  poissons_ratio_    = *input_fields["Poissons Ratio"];
  yield_strength_    = *input_fields["Yield Strength"];
  delta_time_        = *input_fields["Delta Time"];
  temperature_       = *input_fields["ACE Temperature"];

  stress_           = *output_fields[cauchy_string];
  Fp_               = *output_fields[Fp_string];
  eqps_             = *output_fields[eqps_string];
  yield_surf_       = *output_fields[yieldSurface_string];
  bluff_salinity_   = *output_fields["ACE Bluff Salinity"];
  ice_saturation_   = *output_fields["ACE Ice Saturation"];
  density_          = *output_fields["ACE Density"];
  heat_capacity_    = *output_fields["ACE Heat Capacity"];
  thermal_cond_     = *output_fields["ACE Thermal Conductivity"];
  thermal_inertia_  = *output_fields["ACE Thermal Inertia"];
  water_saturation_ = *output_fields["ACE Water Saturation"];
  porosity_         = *output_fields["ACE Porosity"];
  tdot_             = *output_fields["ACE Temperature Dot"];
  failed_           = *output_fields["Failure Indicator"];
  exposure_time_    = *output_fields["ACE Exposure Time"];

  // get State Variables
  Fp_old_             = (*workset.stateArrayPtr)[Fp_string + "_old"];
  eqps_old_           = (*workset.stateArrayPtr)[eqps_string + "_old"];
  T_old_              = (*workset.stateArrayPtr)["ACE Temperature_old"];
  ice_saturation_old_ = (*workset.stateArrayPtr)["ACE Ice Saturation_old"];

  auto& disc        = *workset.disc;
  auto& stk_disc    = dynamic_cast<Albany::STKDiscretization&>(disc);
  auto& mesh_struct = *(stk_disc.getSTKMeshStruct());
  auto& field_cont  = *(mesh_struct.getFieldContainer());
  have_cell_boundary_indicator_ = field_cont.hasCellBoundaryIndicatorField();

  if (have_cell_boundary_indicator_ == true) {
    cell_boundary_indicator_ = workset.cell_boundary_indicator;
    ALBANY_ASSERT(cell_boundary_indicator_.is_null() == false);
  }

  current_time_ = workset.current_time;
  block_name_   = workset.EBName;

  auto const num_cells = workset.numCells;
  for (auto cell = 0; cell < num_cells; ++cell) { failed_(cell, 0) = 0.0; }
}

template <typename EvalT, typename Traits>
KOKKOS_INLINE_FUNCTION void
ACEiceMiniKernel<EvalT, Traits>::operator()(int cell, int pt) const
{
  constexpr minitensor::Index MAX_DIM{3};
  using Tensor = minitensor::Tensor<ScalarT, MAX_DIM>;
  Tensor const I(minitensor::eye<ScalarT, MAX_DIM>(num_dims_));
  Tensor       F(num_dims_);
  Tensor       sigma(num_dims_);

  ScalarT const& E     = elastic_modulus_(cell, pt);
  ScalarT const& nu    = poissons_ratio_(cell, pt);
  ScalarT const& K     = hardening_modulus_(cell, pt);
  ScalarT const& J1    = J_(cell, pt);
  ScalarT const& Tcurr = temperature_(cell, pt);
  ScalarT const  Jm23  = 1.0 / std::cbrt(J1 * J1);
  ScalarT const  kappa = E / (3.0 * (1.0 - 2.0 * nu));
  ScalarT const  mu    = E / (2.0 * (1.0 + nu));
  ScalarT        Y     = yield_strength_(cell, pt);

  auto const  coords        = this->model_.getCoordVecField();
  auto const  height        = Sacado::Value<ScalarT>::eval(coords(cell, pt, 2));
  auto const  current_time  = current_time_;
  auto const& Told          = T_old_(cell, pt);
  auto const& iold          = ice_saturation_old_(cell, pt);
  auto&&      delta_time    = delta_time_(0);
  auto&&      failed        = failed_(cell, 0);
  auto&&      exposure_time = exposure_time_(cell, pt);

  // Determine if erosion has occurred.
  auto const erosion_rate = erosion_rate_;
  auto const element_size = element_size_;
  bool const is_erodible  = erosion_rate > 0.0;
  auto const critical_exposure_time =
      is_erodible == true ? element_size / erosion_rate : 0.0;

  auto const sea_level =
      sea_level_.size() > 0 ?
          interpolateVectors(time_, sea_level_, current_time) :
          0.0;
  bool const is_exposed_to_water = (height <= sea_level);
  bool const is_at_boundary =
      have_cell_boundary_indicator_ == true ?
          static_cast<bool>(*(cell_boundary_indicator_[cell])) :
          false;

  bool const is_erodible_at_boundary = is_erodible && is_at_boundary;
  if (is_erodible_at_boundary == true) {
    if (is_exposed_to_water == true) { exposure_time += delta_time; }
    if (exposure_time >= critical_exposure_time) {
      // Disable temporarily
      failed += 0.0;
      exposure_time = 0.0;
    }
  }

  //
  // Thermal calculation
  //

  // Calculate the depth-dependent porosity
  // NOTE: The porosity does not change in time so this calculation only needs
  //       to be done once, at the beginning of the simulation.
  auto porosity = porosity0_;
  if (porosity_from_file_.size() > 0) {
    porosity = interpolateVectors(
        z_above_mean_sea_level_, porosity_from_file_, height);
  }
  porosity_(cell, pt) = porosity;

  // A boundary cell (this is a hack): porosity = -1.0 (set in input deck)
  bool const b_cell = porosity < 0.0;

  // Calculate melting temperature
  auto sal = salinity_base_;  // should come from chemical part of model
  if (salinity_.size() > 0) {
    sal = interpolateVectors(z_above_mean_sea_level_, salinity_, height);
  }
  auto sal15          = std::sqrt(sal * sal * sal);
  auto pressure_fixed = 1.0;
  // Tmelt is in Kelvin
  auto Tmelt = -0.057 * sal + 0.00170523 * sal15 - 0.0002154996 * sal * sal -
               0.000753 / 10000.0 * pressure_fixed + 273.15;

  // Calculate temperature change
  auto dTemp = Tcurr - Told;
  if (delta_time > 0.0) {
    tdot_(cell, pt) = dTemp / delta_time;
  } else {
    tdot_(cell, pt) = 0.0;
  }

  // Calculate the freezing curve function df/dTemp
  // W term sets the width of the freezing curve.
  // Smaller W means steeper curve.
  // f(T) = 1 / (1 + e^(-W*(T-T0)))
  // New curve, formulated by Siddharth, which shifts the
  // freezing point to left or right:
  // f(T) = 1 / (1 + e^(-(8/W)((T-T0) + (b*W))))
  // W = true width of freezing curve (in Celsius)
  // b = shift to left or right (+ is left, - is right)
  ScalarT W = freeze_curve_width_;  // constant value
  if (freezing_curve_width_.size() > 0) {
    W = interpolateVectors(
        z_above_mean_sea_level_, freezing_curve_width_, height);
  }

  ScalarT const Tdiff = Tcurr - Tmelt;
  ScalarT const arg   = -(8.0 / W) * (Tdiff + (f_shift_ * W));
  ScalarT       icurr{1.0};
  ScalarT       dfdT{0.0};
  auto const    tol = 45.0;

  // Update freeze curve slope and ice saturation
  if (arg < -tol) {
    dfdT  = 0.0;
    icurr = 0.0;
  } else if (arg > tol) {
    dfdT  = 0.0;
    icurr = 1.0;
  } else {
    auto const    eps = minitensor::machine_epsilon<RealType>();
    ScalarT const et  = std::exp(arg);
    if (et < eps) {  // etp1 ~ 1.0
      dfdT  = -(W / 8.0) * et;
      icurr = 0.0;
    } else if (1.0 / et < eps) {  // etp1 ~ et
      dfdT  = -(W / 8.0) / et;
      icurr = 1.0 - 1.0 / et;
    } else {
      ScalarT const etp1 = et + 1.0;
      dfdT               = -(W / 8.0) * et / etp1 / etp1;
      icurr              = 1.0 - 1.0 / etp1;
    }
  }

  // Update the water saturation
  ScalarT wcurr = 1.0 - icurr;

  // Correct ice/water saturation if b_cell
  if (b_cell == true) {
    icurr = 0.0;
    wcurr = 0.0;
  }

  // Update the effective material density
  density_(cell, pt) =
      porosity * ((ice_density_ * icurr) + (water_density_ * wcurr));

  // Update the effective material heat capacity
  heat_capacity_(cell, pt) = porosity * ((ice_heat_capacity_ * icurr) +
                                         (water_heat_capacity_ * wcurr));

  // Update the effective material thermal conductivity
  thermal_cond_(cell, pt) = pow(ice_thermal_cond_, (icurr * porosity)) *
                            pow(water_thermal_cond_, (wcurr * porosity));

  // Update the material thermal inertia term
  thermal_inertia_(cell, pt) = (density_(cell, pt) * heat_capacity_(cell, pt)) -
                               (ice_density_ * latent_heat_ * dfdT);

  // Return values
  ice_saturation_(cell, pt)   = icurr;
  water_saturation_(cell, pt) = wcurr;

  //
  // Mechanical calculation
  //

  // Compute effective yield strength
  Y = icurr * Y;

  // fill local tensors
  F.fill(def_grad_, cell, pt, 0, 0);

  // Mechanical deformation gradient
  auto    Fm              = Tensor(F);
  ScalarT dtemp           = Tcurr - ref_temperature_;
  ScalarT thermal_stretch = std::exp(expansion_coeff_ * dtemp);
  Fm /= thermal_stretch;

  Tensor Fpn(num_dims_);

  for (int i{0}; i < num_dims_; ++i) {
    for (int j{0}; j < num_dims_; ++j) {
      Fpn(i, j) = ScalarT(Fp_old_(cell, pt, i, j));
    }
  }

  // compute trial state
  Tensor const  Fpinv = minitensor::inverse(Fpn);
  Tensor const  Cpinv = Fpinv * minitensor::transpose(Fpinv);
  Tensor const  be    = Jm23 * Fm * Cpinv * minitensor::transpose(Fm);
  Tensor        s     = mu * minitensor::dev(be);
  ScalarT const mubar = minitensor::trace(be) * mu / (num_dims_);

  // check yield condition
  ScalarT const smag = minitensor::norm(s);
  ScalarT const f =
      smag -
      SQ23 * (Y + K * eqps_old_(cell, pt) +
              sat_mod_ * (1.0 - std::exp(-sat_exp_ * eqps_old_(cell, pt))));

  RealType constexpr yield_tolerance = 1.0e-12;
  bool const yielded                 = f > yield_tolerance;

  if (yielded == true) {
    // Use minimization equivalent to return mapping
    using ValueT = typename Sacado::ValueType<ScalarT>::type;
    using NLS    = ACE_NLS<EvalT>;

    constexpr minitensor::Index nls_dim{NLS::DIMENSION};

    using MIN  = minitensor::Minimizer<ValueT, nls_dim>;
    using STEP = minitensor::NewtonStep<NLS, ValueT, nls_dim>;

    MIN  minimizer;
    STEP step;
    NLS  j2nls(sat_mod_, sat_exp_, eqps_old_(cell, pt), K, smag, mubar, Y);

    minitensor::Vector<ScalarT, nls_dim> x;

    x(0) = 0.0;

    LCM::MiniSolver<MIN, STEP, NLS, EvalT, nls_dim> mini_solver(
        minimizer, step, j2nls, x);

    ScalarT const alpha = eqps_old_(cell, pt) + SQ23 * x(0);
    ScalarT const H     = K * alpha + sat_mod_ * (1.0 - exp(-sat_exp_ * alpha));
    ScalarT const dgam  = x(0);

    // plastic direction
    Tensor const N = (1 / smag) * s;

    // update s
    s -= 2 * mubar * dgam * N;

    // update eqps
    eqps_(cell, pt) = alpha;

    // exponential map to get Fpnew
    Tensor const A     = dgam * N;
    Tensor const expA  = minitensor::exp(A);
    Tensor const Fpnew = expA * Fpn;

    for (int i{0}; i < num_dims_; ++i) {
      for (int j{0}; j < num_dims_; ++j) { Fp_(cell, pt, i, j) = Fpnew(i, j); }
    }
  } else {
    eqps_(cell, pt) = eqps_old_(cell, pt);

    for (int i{0}; i < num_dims_; ++i) {
      for (int j{0}; j < num_dims_; ++j) { Fp_(cell, pt, i, j) = Fpn(i, j); }
    }
  }

  // update yield surface
  yield_surf_(cell, pt) =
      Y + K * eqps_(cell, pt) +
      sat_mod_ * (1. - std::exp(-sat_exp_ * eqps_(cell, pt)));

  // compute pressure
  ScalarT const pressure = 0.5 * kappa * (J_(cell, pt) - 1. / (J_(cell, pt)));

  // compute stress
  sigma = pressure * I + s / J_(cell, pt);

  for (int i(0); i < num_dims_; ++i) {
    for (int j(0); j < num_dims_; ++j) {
      stress_(cell, pt, i, j) = sigma(i, j);
    }
  }

  //
  // Determine if critical stress is exceeded
  //
  if (yielded == true) failed += 1.0;

  //
  // Determine if kinematic failure occurred
  //
  auto const critical_angle = critical_angle_;
  if (critical_angle > 0.0) {
    auto const Fval   = Sacado::Value<decltype(F)>::eval(F);
    auto const Q      = minitensor::polar_rotation(Fval);
    auto       cosine = 0.5 * (minitensor::trace(Q) - 1.0);
    cosine            = cosine > 1.0 ? 1.0 : cosine;
    cosine            = cosine < -1.0 ? -1.0 : cosine;
    auto const theta  = std::acos(cosine);
    if (std::abs(theta) >= critical_angle) failed += 1.0;
  }

  return;
}

}  // namespace LCM

#include <fstream>
#include <iostream>
#include <mpi.h>
#include <xolotl/core/temperature/HeatEquation1DHandler.h>
#include <xolotl/core/temperature/HeatEquation2DHandler.h>
#include <xolotl/core/temperature/HeatEquation3DHandler.h>
#include <xolotl/factory/temperature/TemperatureHandlerFactory.h>
#include <xolotl/core/temperature/TemperatureHandler.h>
#include <xolotl/core/temperature/TemperatureProfileHandler.h>
#include <xolotl/core/temperature/TemperatureGradientHandler.h>
#include <xolotl/util/MathUtils.h>

namespace xolotl {
namespace factory {
namespace temperature {

static std::shared_ptr<core::temperature::ITemperatureHandler> theTemperatureHandler;

// Create the desired type of handler registry.
bool initializeTempHandler(const options::Options &opts) {
	// Get the current process ID
	int procId;
	MPI_Comm_rank(MPI_COMM_WORLD, &procId);

	bool ret = true;

	if (opts.useConstTemperatureHandlers()
			&& opts.useTemperatureProfileHandlers()) {
		// Only print the error message once when running in parallel
		if (procId == 0) {
			// A constant temperature value AND a temperature profile cannot both be given.
			throw std::string(
					"\nA constant temperature value AND a temperature file cannot both be given.");
		}
	} else if (opts.useConstTemperatureHandlers()) {
		auto temp = opts.getConstTemperature();
		// Check if we want a temperature gradient
		double bulkTemp = opts.getBulkTemperature();
		if (util::equal(bulkTemp, 0.0)) {
			// we are to use a constant temperature handler
			theTemperatureHandler = std::make_shared<
					core::temperature::TemperatureHandler>(temp);
		} else {
			// Use a temperature gradient
			theTemperatureHandler = std::make_shared<
					core::temperature::TemperatureGradientHandler>(temp, bulkTemp);
		}
	} else if (opts.useTemperatureProfileHandlers()) {
		auto tempFileName = opts.getTempProfileFilename();
		theTemperatureHandler = std::make_shared<
				core::temperature::TemperatureProfileHandler>(tempFileName);
	} else if (opts.useHeatEquationHandlers()) {
		if (util::equal(opts.getConstTemperature(), 0.0)) {
			// We are to use a constant temperature handler because the flux is 0.0
			theTemperatureHandler = std::make_shared<
					core::temperature::TemperatureHandler>(
					opts.getBulkTemperature());
		} else {
			// Actually using the heat equation
			// Switch on the dimension
			switch (opts.getDimensionNumber()) {
			case 1:
				theTemperatureHandler = std::make_shared<
						core::temperature::HeatEquation1DHandler>(
						opts.getConstTemperature(),
						opts.getBulkTemperature());
				break;
			case 2:
				theTemperatureHandler = std::make_shared<
						core::temperature::HeatEquation2DHandler>(
						opts.getConstTemperature(),
						opts.getBulkTemperature());
				break;
			case 3:
				theTemperatureHandler = std::make_shared<
						core::temperature::HeatEquation3DHandler>(
						opts.getConstTemperature(),
						opts.getBulkTemperature());
				break;
			default:
				// The asked dimension is not good (e.g. -1, 4)
				throw std::string(
						"\nxolotlFactory: Bad dimension for the heat equation handler.");
			}

			// Set the heat coefficient which depends on the material
			auto problemType = opts.getMaterial();
			// PSI case
			if (problemType == "W100" || problemType == "W110"
					|| problemType == "W111" || problemType == "W211"
					|| problemType == "TRIDYN") {
				theTemperatureHandler->setHeatCoefficient(
						core::tungstenHeatCoefficient);
				theTemperatureHandler->setHeatConductivity(
						core::tungstenHeatConductivity);
			}
			// NE case
			else if (problemType == "Fuel") {
				theTemperatureHandler->setHeatCoefficient(
						core::uo2HeatCoefficient);
				theTemperatureHandler->setHeatConductivity(
						core::uo2HeatConductivity);
			}
			// Fe case
			else if (problemType == "Fe") {
				theTemperatureHandler->setHeatCoefficient(
						core::feHeatCoefficient);
				theTemperatureHandler->setHeatConductivity(
						core::feHeatConductivity);
			}
		}
	} else {
		// Only print the error message once when running in parallel
		if (procId == 0) {
			std::cerr
					<< "\nWarning: Temperature information has not been given.  Defaulting to constant"
							" temperature = 1000K " << std::endl;
		}
		auto temp = opts.getConstTemperature();
		// we are to use a constant temperature handler
		theTemperatureHandler =
				std::make_shared<core::temperature::TemperatureHandler>(temp);
	}

	return ret;
}

// Provide access to our handler registry.
std::shared_ptr<core::temperature::ITemperatureHandler> getTemperatureHandler() {
	if (!theTemperatureHandler) {
		// We have not yet been initialized.
		throw std::string("\nxolotlFactory temperature handler requested but "
				"it has not been initialized.");
	}
	return theTemperatureHandler;
}

} // end namespace temperature
} // end namespace factory
} // end namespace xolotl

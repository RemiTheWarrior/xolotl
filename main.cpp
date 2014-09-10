/**
 * Main.c, currently only able to load clusters
 */
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <Reactant.h>
#include <PSIClusterNetworkLoader.h>
#include <PetscSolver.h>
#include <mpi.h>
#include <MPIUtils.h>
#include <Options.h>
#include <MaterialHandlerFactory.h>
#include <TemperatureHandlerFactory.h>
#include <HandlerRegistryFactory.h>
#include <VizHandlerRegistryFactory.h>
#include <HardwareQuantities.h>
#include <HDF5NetworkLoader.h>
#include <IVizHandlerRegistry.h>
#include <ctime>

using namespace std;
using std::shared_ptr;

//! This operation prints the start message
void printStartMessage() {
	std::cout << "Starting Xolotl Plasma-Surface Interactions Simulator" << std::endl;
	// TODO! Print copyright message
	// Print date and time
	std::time_t currentTime = std::time(NULL);
	std::cout << std::asctime(std::localtime(&currentTime)); // << std::endl;
}

std::vector<xolotlPerf::HardwareQuantities> declareHWcounters() {

	// Indicate we want to monitor some important hardware counters.
	std::vector<xolotlPerf::HardwareQuantities> hwq;

	hwq.push_back(xolotlPerf::FP_OPS);
	hwq.push_back(xolotlPerf::L1_CACHE_MISS);

	return hwq;
}

bool initMaterial(Options &options) {

	bool materialInitOK = xolotlSolver::initializeMaterial(options);
	if (!materialInitOK) {
		std::cerr << "Unable to initialize requested material.  Aborting"
				<< std::endl;
		return EXIT_FAILURE;
	} else
		return materialInitOK;
}

bool initTemp(Options &options) {

	bool tempInitOK = xolotlSolver::initializeTempHandler(options);
	if (!tempInitOK) {
		std::cerr << "Unable to initialize requested temperature.  Aborting"
				<< std::endl;
		return EXIT_FAILURE;
	} else
		return tempInitOK;
}

bool initPerf(bool opts, std::vector<xolotlPerf::HardwareQuantities> hwq) {

	bool perfInitOK = xolotlPerf::initialize(opts, hwq);
	if (!perfInitOK) {
		std::cerr
				<< "Unable to initialize requested performance data infrastructure.  Aborting"
				<< std::endl;
		return EXIT_FAILURE;
	} else
		return perfInitOK;
}

bool initViz(bool opts) {

	bool vizInitOK = xolotlViz::initialize(opts);
	if (!vizInitOK) {
		std::cerr
				<< "Unable to initialize requested visualization infrastructure. "
				<< "Aborting" << std::endl;
		return EXIT_FAILURE;
	} else
		return vizInitOK;
}

std::shared_ptr<xolotlSolver::PetscSolver> setUpSolver(
		std::shared_ptr<xolotlPerf::IHandlerRegistry> handlerRegistry, int argc,
		char **argv) {
	// Setup the solver
	auto solverInitTimer = handlerRegistry->getTimer("initSolver");
	solverInitTimer->start();
	std::shared_ptr<xolotlSolver::PetscSolver> solver = std::make_shared<
			xolotlSolver::PetscSolver>(handlerRegistry);
	solver->setCommandLineOptions(argc, argv);
	solver->initialize();
	solverInitTimer->stop();

	return solver;
}

void launchPetscSolver(std::shared_ptr<xolotlSolver::PetscSolver> solver,
		std::shared_ptr<xolotlPerf::IHandlerRegistry> handlerRegistry,
		std::shared_ptr<xolotlSolver::IFluxHandler> materialHandler,
		std::shared_ptr<xolotlSolver::ITemperatureHandler> tempHandler) {

	// Launch the PetscSolver
	auto solverTimer = handlerRegistry->getTimer("solve");
	solverTimer->start();
	solver->solve(materialHandler, tempHandler);
	solverTimer->stop();
}

std::shared_ptr<PSIClusterNetworkLoader> setUpNetworkLoader(int rank,
		MPI_Comm comm, std::string networkFilename,
		std::shared_ptr<xolotlPerf::IHandlerRegistry> registry) {

	// Create a HDF5NetworkLoader
	std::shared_ptr<HDF5NetworkLoader> networkLoader;
	networkLoader = std::make_shared<HDF5NetworkLoader>(registry);
	// Give the networkFilename to the network loader
	networkLoader->setFilename(networkFilename);

	return networkLoader;
}

//! Main program
int main(int argc, char **argv) {

	// Local Declarations
	int rank;

	// Check the command line arguments.
	// Skip the executable name before parsing.
	argc -= 1; // one for the executable name
	argv += 1; // one for the executable name
	Options opts;
	opts.readParams(argc, argv);
	if (!opts.shouldRun()) {
		return opts.getExitCode();
	}

	// Skip the name of the parameter file that was just used.
	// The arguments should be empty now.
	argc -= 1;
	argv += 1;

	// Extract the argument for the file name
	std::string networkFilename = opts.getNetworkFilename();
	assert(!networkFilename.empty());

	try {
		// Set up our performance data infrastructure.
		// Indicate we want to monitor some important hardware counters.
		auto hwq = declareHWcounters();
		auto perfInitOK = initPerf(opts.usePerfStandardHandlers(), hwq);

		// Initialize MPI. We do this instead of leaving it to some
		// other package (e.g., PETSc), because we want to avoid problems
		// with overlapping Timer scopes.
		MPI_Init(&argc, &argv);

		// Get the MPI rank
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		if (rank == 0) {
			// Print the start message
			printStartMessage();
		}

		// Set up the material infrastructure that is used to calculate flux
		auto materialInitOK = initMaterial(opts);
		// Set up the temperature infrastructure
		auto tempInitOK = initTemp(opts);
		// Set up the visualization infrastructure.
		auto vizInitOK = initViz(opts.useVizStandardHandlers());

		// Access the material handler registry to get the material
		auto materialHandler = xolotlSolver::getMaterialHandler();
		// Access the temperature handler registry to get the temperature
		auto tempHandler = xolotlSolver::getTemperatureHandler(opts);

		// Access our performance handler registry to obtain a Timer
		// measuring the runtime of the entire program.
		// NOTE: these long template types could be replaced with 'auto'
		auto handlerRegistry = xolotlPerf::getHandlerRegistry();
		auto totalTimer = handlerRegistry->getTimer("total");
		totalTimer->start();

		// Setup the solver
		auto solver = setUpSolver(handlerRegistry, opts.getPetscArgc(),
				opts.getPetscArgv());

		// Load the network
		auto networkLoadTimer = handlerRegistry->getTimer("loadNetwork");
		networkLoadTimer->start();

		// Set up the network loader
		auto networkLoader = setUpNetworkLoader(rank, MPI_COMM_WORLD,
				networkFilename, handlerRegistry);

		// Give the network loader to PETSc as input
		solver->setNetworkLoader(networkLoader);
		networkLoadTimer->stop();

		// Launch the PetscSolver
		launchPetscSolver(solver, handlerRegistry, materialHandler,
				tempHandler);

		// Finalize our use of the solver.
		auto solverFinalizeTimer = handlerRegistry->getTimer("solverFinalize");
		solverFinalizeTimer->start();
		solver->finalize();
		solverFinalizeTimer->stop();
		totalTimer->stop();

		// Report the performance data about the run we just completed
		// TODO Currently, this call writes EventCounter data to the
		// given stream, but Timer and any hardware counter data is
		// written by the underlying timing library to files, one per process.
		if (rank == 0) {
			handlerRegistry->dump(std::cout);
		}

	} catch (std::string & error) {
		std::cout << error << std::endl;
		std::cout << "Aborting." << std::endl;
		return EXIT_FAILURE;
	}

	// finalize our use of MPI
	MPI_Finalize();

	// Uncomment if GPTL was built with pmpi disabled
	// Output performance data if pmpi is disabled in GPTL
	// Access the handler registry to output performance data
	auto handlerRegistry = xolotlPerf::getHandlerRegistry();
	handlerRegistry->dump(rank);

	return EXIT_SUCCESS;
}

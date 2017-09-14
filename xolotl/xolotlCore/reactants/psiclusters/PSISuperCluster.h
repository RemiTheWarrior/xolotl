#ifndef PSISUPERCLUSTER_H
#define PSISUPERCLUSTER_H

// Includes
#include <string>
#include <unordered_map>
#include <Constants.h>
#include "PSICluster.h"
#include "IntegerRange.h"



// We use std::unordered_map for quick lookup of info about 
// reactions we participate in.
// The C++ standard library defines a std::hash for keys
// that are a single pointer, but not for pairs of pointers,
// so we define our own here.  To improve readability,
// we define a concise name for type of a pair of IReactant pointers
// that we use as keys.
// TODO should this be moved "upward," e.g., into IReactant.h?
namespace xolotlCore {
using ReactantAddrPair = std::pair<IReactant*, IReactant*>;
} // namespace xolotlCore

namespace std {

template<>
struct hash<xolotlCore::ReactantAddrPair> {
    size_t operator()(const xolotlCore::ReactantAddrPair& pr) const {
        // Idea for implementation taken from 
        // https://www.sultanik.com/blog/HashingPointers.
        auto sum = reinterpret_cast<uintptr_t>(pr.first) +
                    reinterpret_cast<uintptr_t>(pr.second);
        // Ensure result will fit in size_t
#if SIZE_MAX < UINTPTR_MAX
        sum %= SIZE_MAX;
#endif // SIZE_MAX < UINTPTR_MAX
        return sum;
    }
};

} // namespace std



namespace xolotlCore {
/**
 *  A cluster gathering the average properties of many HeV clusters.
 */
class PSISuperCluster: public PSICluster {

private:
    static
    std::string buildName(double nHe, double nV) {
        std::stringstream nameStream;
        nameStream << "He_" << nHe << "V_" << nV;
        return nameStream.str();
    }

protected:

    struct ReactingInfoBase {

		/**
		 * The first cluster in the pair
		 */
		PSICluster * first;

		/**
		 * The reaction/dissociation constant associated to this
		 * reaction or dissociation
		 */
		const double& kConstant;

		//! The constructor
		ReactingInfoBase(PSICluster * firstPtr, Reaction * reaction) :
				first(firstPtr),
                kConstant(reaction->kConstant) {

		}

        /**
         * Default and copy constructors, disallowed.
         */
        ReactingInfoBase() = delete;
        ReactingInfoBase(const ReactingInfoBase& other) = delete;
    };

    struct ReactingPairBase : public ReactingInfoBase {

		/**
		 * The second cluster in the pair
		 */
		PSICluster * second;

		//! The constructor
		ReactingPairBase(PSICluster * firstPtr,
				    PSICluster * secondPtr, Reaction * reaction) :
            ReactingInfoBase(firstPtr, reaction),
            second(secondPtr) {

		}

        /**
         * Default and copy constructors, disallowed.
         */
        ReactingPairBase() = delete;
        ReactingPairBase(const ReactingPairBase& other) = delete;
    };


    struct ProductionCoefficientBase {

		/**
		 * All the coefficient needed to compute each element
		 * The first number represent the momentum of A, the second of B
		 * in A + B -> C
		 *
		 * The third number represent which momentum we are computing.
		 *
		 * 0 -> l0
		 * 1 -> He
		 * 2 -> V
		 */
		double a000;
		double a001;
		double a002;
		double a100;
		double a101;
		double a102;
		double a200;
		double a201;
		double a202;
		double a010;
		double a011;
		double a012;
		double a020;
		double a021;
		double a022;
		double a110;
		double a111;
		double a112;
		double a120;
		double a121;
		double a122;
		double a210;
		double a211;
		double a212;
		double a220;
		double a221;
		double a222;

		//! The constructor
		ProductionCoefficientBase() :
                a000(0.0), a001(0.0), a002(0.0),
                a100(0.0), a101(0.0), a102(0.0),
                a200(0.0), a201(0.0), a202(0.0),
                a010(0.0), a011(0.0), a012(0.0),
                a020(0.0), a021(0.0), a022(0.0),
                a110(0.0), a111(0.0), a112(0.0),
                a120(0.0), a121(0.0), a122(0.0),
                a210(0.0), a211(0.0), a212(0.0),
                a220(0.0), a221(0.0), a222(0.0) {
		}

        /**
         * Copy constructor, disallowed.
         */
        ProductionCoefficientBase(const ProductionCoefficientBase& other) = delete;
    };

	/**
	 * This is a protected class that is used to implement the flux calculations
	 * for two body production reactions.
	 *
	 * The constants are stored along the clusters taking part in the
	 * reaction or dissociation for faster computation because they only change
	 * when the temperature change. k is computed when setTemperature() is called.
	 */
	struct SuperClusterProductionPair : public ReactingPairBase, public ProductionCoefficientBase {

        /**
         * Nice name for key type in map of key to production pair.
         */
        using KeyType = ReactantAddrPair;

		//! The constructor
		SuperClusterProductionPair(PSICluster * firstPtr,
				    PSICluster * secondPtr, Reaction * reaction) :
            ReactingPairBase(firstPtr, secondPtr, reaction),
            ProductionCoefficientBase() {

		}

        /**
         * Default and copy constructors, deleted to enforce constructing
         * using reactants.
         */
        SuperClusterProductionPair() = delete;
        SuperClusterProductionPair(const SuperClusterProductionPair& other) = delete;
	};

    /**
     * Concise name for type of map of SuperClusterProductionPairs.
     */
    using ProductionPairMap = std::unordered_map<SuperClusterProductionPair::KeyType, SuperClusterProductionPair>;


    /**
     * Info about a cluster we combine with.
     */
	struct SuperClusterCombiningCluster : public ReactingInfoBase, public ProductionCoefficientBase {

        /**
         * Concise name for type of keys in map of keys to 
         * combining cluster info.
         */
        using KeyType = IReactant*;

		//! The constructor
		SuperClusterCombiningCluster(PSICluster* firstPtr, Reaction* reaction) :
            ReactingInfoBase(firstPtr, reaction),
            ProductionCoefficientBase() {

		}

        /**
         * Default and copy construtors, deleted to enforce constructing
         * using reactants.
         */
        SuperClusterCombiningCluster() = delete;
        SuperClusterCombiningCluster(const SuperClusterCombiningCluster& other) = delete;
	};

    /**
     * Concise name for type of map of SuperClusterCombiningClusters.
     */
    using CombiningClusterMap = std::unordered_map<SuperClusterCombiningCluster::KeyType, SuperClusterCombiningCluster>;


	/**
	 * This is a protected class that is used to implement the flux calculations
	 * for two dissociation reactions.
	 *
	 * The constants are stored along the clusters taking part in the
	 * reaction or dissociation for faster computation because they only change
	 * when the temperature change. k is computed when setTemperature() is called.
	 */
	struct SuperClusterDissociationPair : public ReactingPairBase {

        /**
         * Concise name for type of key into map of dissociation pairs.
         */
        using KeyType = ReactantAddrPair;

		/**
		 * All the coefficient needed to compute each element
		 * The first number represent the momentum of A
		 * in A -> B + C
		 *
		 * The second number represent which momentum we are computing.
		 *
		 * 0 -> l0
		 * 1 -> He
		 * 2 -> V
		 */
		double a00;
		double a01;
		double a02;
		double a10;
		double a11;
		double a12;
		double a20;
		double a21;
		double a22;

		//! The constructor
		SuperClusterDissociationPair(PSICluster * firstPtr,
				    PSICluster * secondPtr, Reaction * reaction) :
            ReactingPairBase(firstPtr, secondPtr, reaction),
                a00(0.0), a01(0.0), a02(0.0), 
                a10(0.0), a11(0.0), a12(0.0),
                a20(0.0), a21(0.0), a22(0.0) {

		}

        /**
         * Default and copy constructors, disallowed.
         */
        SuperClusterDissociationPair() = delete;
        SuperClusterDissociationPair(const SuperClusterDissociationPair& other) = delete;
	};

    /**
     * Concise name for type of map of SuperClusterDissociationPairs.
     */
    using DissociationPairMap = std::unordered_map<SuperClusterDissociationPair::KeyType, SuperClusterDissociationPair>;

private:

	//! The mean number of helium atoms in this cluster.
	double numHe;

	//! The mean number of atomic vacancies in this cluster.
	double numV;

	//! The total number of clusters gathered in this super cluster.
	int nTot;

	//! The width in the helium direction.
	int sectionHeWidth;

	//! The width in the vacancy direction.
	int sectionVWidth;

    /**
     * Bounds on number of He atoms represented by this cluster.
     */
    IntegerRange<IReactant::SizeType> heBounds;

    /**
     * Bounds on number of vacancies represented by this cluster.
     */
    IntegerRange<IReactant::SizeType> vBounds;

	//! The 0th order momentum (mean).
	double l0;

	//! The first order momentum in the helium direction.
	double l1He;

	//! The first order momentum in the vacancy direction.
	double l1V;

	//! The dispersion in the group in the helium direction.
	double dispersionHe;

	//! The dispersion in the group in the vacancy direction.
	double dispersionV;

	//! The list of optimized effective reacting pairs.
    ProductionPairMap effReactingList;

	//! The list of optimized effective combining pairs.
    CombiningClusterMap effCombiningList;

	//! The list of optimized effective dissociating pairs.
    DissociationPairMap effDissociatingList;

	//! The list of optimized effective emission pairs.
    DissociationPairMap effEmissionList;

	/**
	 * The helium momentum flux.
	 */
	double heMomentumFlux;

	/**
	 * The vacancy momentum flux.
	 */
	double vMomentumFlux;


public:


    /**
     * Default constructor, deleted because we require info to construct.
     */
    PSISuperCluster() = delete;


	/**
	 * The constructor. All SuperClusters must be initialized with its
	 * composition.
	 *
	 * @param numHe The mean number of helium atoms in this cluster
	 * @param numV The mean number of vacancies in this cluster
	 * @param nTot The total number of clusters in this cluster
	 * @param heWidth The width of this super cluster in the helium direction
	 * @param vWidth The width of this super cluster in the vacancy direction
	 * @param radius The mean radius
	 * @param energy The mean formation energy
	 * @param registry The performance handler registry
	 */
	PSISuperCluster(double numHe, double numV, int nTot, int heWidth,
			int vWidth,
            IReactionNetwork& _network,
            std::shared_ptr<xolotlPerf::IHandlerRegistry> registry);

	/**
	 * Copy constructor, deleted to prevent use.
	 */
	PSISuperCluster(PSISuperCluster &other) = delete;

	//! Destructor
	~PSISuperCluster() {
	}

	/**
	 * Create a production pair associated with the given reaction.
	 * Create the connectivity.
	 *
	 * @param reaction The reaction creating this cluster.
	 * @param a Helium number.
	 * @param b Vacancy number.
	 * @param c Helium number.
	 * @param d Vacancy number.
	 */
	void createProduction(std::shared_ptr<ProductionReaction> reaction, 
            int a = 0, int b = 0, int c = 0, int d = 0) override;

	/**
	 * Create a combination associated with the given reaction.
	 * Create the connectivity.
	 *
	 * @param reaction The reaction where this cluster takes part.
	 * @param a Helium number.
	 * @param b Vacancy number.
	 */
	void createCombination(ProductionReaction& reaction,
                            int a = 0, int b = 0) override;

	/**
	 * Create a dissociation pair associated with the given reaction.
	 * Create the connectivity.
	 *
	 * @param reaction The reaction creating this cluster.
	 * @param a Helium number.
	 * @param b Vacancy number.
	 * @param c Helium number.
	 * @param d Vacancy number.
	 */
	void createDissociation(std::shared_ptr<DissociationReaction> reaction,
			int a = 0, int b = 0, int c = 0, int d = 0) override;

	/**
	 * Create an emission pair associated with the given reaction.
	 * Create the connectivity.
	 *
	 * @param reaction The reaction where this cluster emits.
	 * @param a Helium number.
	 * @param b Vacancy number.
	 * @param c Helium number.
	 * @param d Vacancy number.
	 */
	void createEmission(std::shared_ptr<DissociationReaction> reaction, int a =
			0, int b = 0, int c = 0, int d = 0) override;

	/**
	 * This operation returns true to signify that this cluster is a mixture of
	 * He and V.
	 *
	 * @return True if mixed
	 */
	virtual bool isMixed() const override {
		return true;
	}

	/**
	 * Set the HeV vector and compute different parameters
	 */
	void setHeVVector(std::vector<std::pair<int, int> > vec);

	/**
	 * This operation returns the current concentration.
	 *
	 * @param distHe The helium distance in the group
	 * @param distV The vacancy distance in the group
	 * @return The concentration of this reactant
	 */
	double getConcentration(double distHe, double distV) const {
		return l0 + (distHe * l1He) + (distV * l1V);
	}

	/**
	 * This operation returns the first helium momentum.
	 *
	 * @return The momentum
	 */
	double getHeMomentum() const {
		return l1He;
	}

	/**
	 * This operation returns the first vacancy momentum.
	 *
	 * @return The momentum
	 */
	double getVMomentum() const {
		return l1V;
	}

	/**
	 * This operation returns the current total concentration of clusters in the group.

	 * @return The concentration
	 */
	double getTotalConcentration() const;

	/**
	 * This operation returns the current total concentration of helium in the group.

	 * @return The concentration
	 */
	double getTotalHeliumConcentration() const;

	/**
	 * This operation returns the current total concentration of vacancies in the group.

	 * @return The concentration
	 */
	double getTotalVacancyConcentration() const;

	/**
	 * This operation returns the distance to the mean.
	 *
	 * @param he The number of helium
	 * @return The distance to the mean number of helium in the group
	 */
	double getHeDistance(int he) const {
		return (sectionHeWidth == 1) ?
				0.0 : 2.0 * (he - numHe) / (sectionHeWidth - 1.0);
	}

	/**
	 * This operation returns the distance to the mean.
	 *
	 * @param he The number of vacancy
	 * @return The distance to the mean number of vacancy in the group
	 */
	double getVDistance(int v) const {
		return (sectionVWidth == 1) ?
				0.0 : 2.0 * (v - numV) / (sectionVWidth - 1.0);
	}

	/**
	 * This operation sets the zeroth order momentum.
	 *
	 * @param mom The momentum
	 */
	void setZerothMomentum(double mom) {
		l0 = mom;
	}

	/**
	 * This operation sets the first order momentum in the helium direction.
	 *
	 * @param mom The momentum
	 */
	void setHeMomentum(double mom) {
		l1He = mom;
	}

	/**
	 * This operation sets the first order momentum in the vacancy direction.
	 *
	 * @param mom The momentum
	 */
	void setVMomentum(double mom) {
		l1V = mom;
	}

	/**
	 * This operation reset the connectivity sets based on the information
	 * in the production and dissociation vectors.
	 */
	void resetConnectivities();

	/**
	 * This operation returns the total flux of this cluster in the
	 * current network.
	 *
	 * @return The total change in flux for this cluster due to all
	 * reactions
	 */
	double getTotalFlux() {

		// Initialize the fluxes
		heMomentumFlux = 0.0;
		vMomentumFlux = 0.0;

		// Compute the fluxes.
		return getProductionFlux() - getCombinationFlux()
				+ getDissociationFlux() - getEmissionFlux();
	}

	/**
	 * This operation returns the total change in this cluster due to
	 * other clusters dissociating into it. Compute the contributions to
	 * the momentum fluxes at the same time.
	 *
	 * @return The flux due to dissociation of other clusters
	 */
	double getDissociationFlux();

	/**
	 * This operation returns the total change in this cluster due its
	 * own dissociation. Compute the contributions to
	 * the momentum fluxes at the same time.
	 *
	 * @return The flux due to its dissociation
	 */
	double getEmissionFlux();

	/**
	 * This operation returns the total change in this cluster due to
	 * the production of this cluster by other clusters. Compute the contributions to
	 * the momentum fluxes at the same time.
	 *
	 * @return The flux due to this cluster being produced
	 */
	double getProductionFlux();

	/**
	 * This operation returns the total change in this cluster due to
	 * the combination of this cluster with others. Compute the contributions to
	 * the momentum fluxes at the same time.
	 *
	 * @return The flux due to this cluster combining with other clusters
	 */
	double getCombinationFlux();

	/**
	 * This operation returns the total change for its helium momentum.
	 *
	 * @return The momentum flux
	 */
	double getHeMomentumFlux() const {
		return heMomentumFlux;
	}

	/**
	 * This operation returns the total change for its vacancy momentum.
	 *
	 * @return The momentum flux
	 */
	double getVMomentumFlux() const {
		return vMomentumFlux;
	}

	/**
	 * This operation works as getPartialDerivatives above, but instead of
	 * returning a vector that it creates it fills a vector that is passed to
	 * it by the caller. This allows the caller to optimize the amount of
	 * memory allocations to just one if they are accessing the partial
	 * derivatives many times.
	 *
	 * @param the vector that should be filled with the partial derivatives
	 * for this reactant where index zero corresponds to the first reactant in
	 * the list returned by the ReactionNetwork::getAll() operation. The size of
	 * the vector should be equal to ReactionNetwork::size().
	 *
	 */
	void getPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives due to production
	 * reactions.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted. This vector should have a length equal to the size of the
	 * network.
	 */
	void getProductionPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives due to combination
	 * reactions.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted. This vector should have a length equal to the size of the
	 * network.
	 */
	void getCombinationPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives due to dissociation of
	 * other clusters into this one.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted. This vector should have a length equal to the size of the
	 * network.
	 */
	void getDissociationPartialDerivatives(
			std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives due to emission
	 * reactions.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted. This vector should have a length equal to the size of the
	 * network.
	 */
	void getEmissionPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives for the helium momentum.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted.
	 */
	void getHeMomentPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * This operation computes the partial derivatives for the vacancy momentum.
	 *
	 * @param partials The vector into which the partial derivatives should be
	 * inserted.
	 */
	void getVMomentPartialDerivatives(std::vector<double> & partials) const;

	/**
	 * Returns the average number of vacancies.
	 *
	 * @return The average number of vacancies
	 */
	double getNumV() const {
		return numV;
	}

	/**
	 * Returns the number of clusters contained.
	 *
	 * @return The number of clusters
	 */
	double getNTot() const {
		return nTot;
	}

    /**
     * Access bounds on number of He atoms represented by this cluster.
     */
    // TODO do we want to make this generic by taking a type parameter?
    const IntegerRange<IReactant::SizeType>& getHeBounds() const {
        return heBounds;
    }
    
    /**
     * Access bounds on number of vacancies represented by this cluster.
     */
    const IntegerRange<IReactant::SizeType>& getVBounds() const {
        return vBounds;
    }

	/**
     * Detect if given number of He and V are in this cluster's group.
	 *
     * @param _nHe number of He of interest.
     * @param _nV number of V of interest
	 * @return True if _nHe and _nV is contained in our super cluster.
	 */
	bool isIn(IReactant::SizeType _nHe, IReactant::SizeType _nV) const {

        return heBounds.contains(_nHe) and vBounds.contains(_nV);
	}

};
//end class PSISuperCluster

} /* end namespace xolotlCore */
#endif

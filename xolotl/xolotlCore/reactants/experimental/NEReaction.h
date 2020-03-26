#pragma once

#include <experimental/NETraits.h>

namespace xolotlCore
{
namespace experimental
{
class NEReactionNetwork;

class NEProductionReaction :
    public ProductionReaction<NEReactionNetwork, NEProductionReaction>
{
public:
    using Superclass =
        ProductionReaction<NEReactionNetwork, NEProductionReaction>;

    using Superclass::Superclass;
};

class NEDissociationReaction :
    public DissociationReaction<NEReactionNetwork, NEDissociationReaction>
{
public:
    using Superclass =
        DissociationReaction<NEReactionNetwork, NEDissociationReaction>;

    using Superclass::Superclass;

    KOKKOS_INLINE_FUNCTION
    double
    computeBindingEnergy()
    {
        auto cl = this->_clusterData.getCluster(this->_reactant);
        auto prod1 = this->_clusterData.getCluster(this->_products[0]);
        auto prod2 = this->_clusterData.getCluster(this->_products[1]);
        return prod1.getFormationEnergy() + prod2.getFormationEnergy() -
            cl.getFormationEnergy();
    }
};
}
}
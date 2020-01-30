#pragma once

#include <experimental/ReactionNetwork.h>
#include <experimental/NEReaction.h>
#include <experimental/NETraits.h>

namespace xolotlCore
{
namespace experimental
{
namespace detail
{
class NEReactionValidator;
}

class NEReactionNetwork : public ReactionNetwork<NEReactionNetwork>
{
    friend class ReactionNetwork<NEReactionNetwork>;
    friend class detail::ReactionNetworkWorker<NEReactionNetwork>;

public:
    using Superclass = ReactionNetwork<NEReactionNetwork>;
    using Subpaving = typename Superclass::Subpaving;
    using ReactionType = typename Superclass::ReactionType;
    using Composition = typename Superclass::Composition;
    using Species = typename Superclass::Species;
    static constexpr auto invalid = plsm::invalid<std::size_t>;

    using Superclass::Superclass;

private:
    double
    checkLatticeParameter(double latticeParameter)
    {
        if (latticeParameter <= 0.0) {
            return uraniumDioxydeLatticeConstant;
        }
        return latticeParameter;
    }

    double checkImpurityRadius(double impurityRadius)
    {
        if (impurityRadius <= 0.0) {
            return xenonRadius;
        }
        return impurityRadius;
    }

    detail::NEReactionValidator
    getReactionValidator() const noexcept;
};

namespace detail
{
class NEReactionValidator
{
public:
    using Network = NEReactionNetwork;
    using Subpaving = typename Network::Subpaving;
    using ClusterSet = typename Network::ReactionType::ClusterSet;

    KOKKOS_INLINE_FUNCTION
    void
    operator()(std::size_t i, std::size_t j, const Subpaving& subpaving,
        const UpperTriangle<Kokkos::pair<ClusterSet, ClusterSet> >& prodSet,
        const UpperTriangle<Kokkos::pair<ClusterSet, ClusterSet> >& dissSet) const;
};
}
}
}

#include <experimental/NEClusterGenerator.h>
#include <experimental/NEReactionNetwork.inl>
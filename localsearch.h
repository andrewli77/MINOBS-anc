#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H 

#include <boost/dynamic_bitset.hpp>
#include "instance.h"
#include "ordering.h"
#include "pivotresult.h"
#include "searchresult.h"
#include "population.h"
#include "resultregister.h"
#include "swapresult.h"
#include "fastpivotresult.h"
#include "types.h"

enum class Neighbours {
  SWAP,
  INSERT
};

enum class CrossoverType {
  CX,
  OB,
  RK
};

enum class SelectType {
  FIRSTV1,
  FIRSTV2,
  BEST,
  HYBRID,
  OLDHYBRID
};

class LocalSearch {
  public:
    LocalSearch(Instance &instance);
    const ParentSet &bestParent(const Ordering &ordering, const Types::Bitset pred, int idx) const;
    const ParentSet &bestParentVar(const Types::Bitset pred, const Variable &v) const;
    const ParentSet *bestParentVarWithParent(const Types::Bitset pred, const Variable &a, const Variable &b, const Types::Score orig) const;
    Types::Bitset getPred(const Ordering &ordering, int idx) const;
    Types::Score getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const;

    int numConstraintsSatisfied(const std::vector<int> &parents) const;
    bool hasDipath(const std::vector<int> &parents, int x, int y) const;
    Types::Score modifiedDAGScore(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const;
    SearchResult hillClimb(const Ordering &ordering);
      Types::Score findBestScoreRange(const Ordering &o, int start, int end);
    SearchResult genetic(float cutoffTime, int INIT_POPULATION_SIZE, int NUM_CROSSOVERS, int NUM_MUTATIONS, int MUTATION_POWER, int DIV_LOOKAHEAD, int NUM_KEEP, float DIV_TOLERANCE, CrossoverType crossoverType, int greediness, Types::Score opt, ResultRegister &rr);
    void checkSolution(const Ordering &o);
    bool consistentWithAncestral(const Ordering &ordering) const;
  private:
    Instance &instance;
};

#endif /* LOCALSEARCH_H */

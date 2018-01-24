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

    bool hasDipath(const std::vector<int> &parents, int x, int y) const;
    void dfs(std::vector<int> *G, int node, int i, bool des, int *visited) const;
    Types::Bitset csat(const std::vector<Ancestral> &constraints, const ParentSet &p, const ParentSet &oldP, const Variable &var) const;
    Types::Score modifiedDAGScore(const Ordering &ordering, std::vector<int> &parents, Types::Score score) const;
    SwapResult findBestScoreSwap(const Ordering &ordering, int i, const std::vector<int> &parents, Types::Bitset &pred);
    PivotResult getBestInsert(const Ordering &ordering, int pivot, Types::Score initScore) const;
    FastPivotResult getBestInsertFast(const Ordering &ordering, int pivot, Types::Score initScore, const std::vector<int> &parents, const std::vector<Types::Score> &scores);
    SearchResult hillClimb(const Ordering &ordering);
    SearchResult hillClimb(const Ordering &ordering, float timeLimit, ResultRegister &rr);
      Types::Score findBestScoreRange(const Ordering &o, int start, int end);
    SearchResult genetic(float cutoffTime, int INIT_POPULATION_SIZE, int NUM_CROSSOVERS, int NUM_MUTATIONS, int MUTATION_POWER, int DIV_LOOKAHEAD, int NUM_KEEP, float DIV_TOLERANCE, CrossoverType crossoverType, int greediness, Types::Score opt, ResultRegister &rr);
    FastPivotResult getInsertScore(Ordering o, int i, int j, Types::Score initScore, std::vector<int> parents, std::vector<Types::Score> scores);
    void checkSolution(const Ordering &o);
    bool consistentWithAncestral(const Ordering &ordering) const;
  private:
    Instance &instance;
};

#endif /* LOCALSEARCH_H */

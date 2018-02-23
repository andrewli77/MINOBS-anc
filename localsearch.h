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
    ~LocalSearch();
    const ParentSet &bestParent(const Ordering &ordering, const Types::Bitset pred, int idx);
    const ParentSet &bestParentVar(const Types::Bitset pred, const Variable &v);
    Types::Score getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores, std::vector<int> &unconstrainedParents);

    int numConstraintsSatisfied(const std::vector<int> &parents);
    int numConstraintsSatisfied(const std::vector<int> &newParents, bool **ancestor, bool **descendant, bool *satisfied, int cur);
    bool hasDipath(const std::vector<int> &parents, int x, int y);
    bool hasDipathWithMemo(const std::vector<int> &parents, int x, int y, int **memo);
    void alloc_2d(bool **&ancestor, bool **&descendant, bool *&satisfied);
    void dealloc_2d(bool **&ancestor, bool **&descendant, bool *&satisfied);
    void computeAncestralGraph(const std::vector<int> &parents, bool **ancestor, bool **descendant, bool *satisfied);

    Types::Score modifiedDAGScore(const Ordering &ordering, const std::vector<int> &parents);
    Types::Score modifiedDAGScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores);

    void bestSwapBackward(int pivot, Ordering o, const std::vector<int> &parents, Ordering &bestOrdering, std::vector<int> &bestParents, Types::Score &bestSc);
    void bestSwapForward(int pivot, Ordering o, const std::vector<int> &parents, Ordering &bestOrdering, std::vector<int> &bestParents, Types::Score &bestSc);
    SearchResult hillClimb(const Ordering &ordering);
    SearchResult genetic(float cutoffTime, int INIT_POPULATION_SIZE, int NUM_CROSSOVERS, int NUM_MUTATIONS, int MUTATION_POWER, int DIV_LOOKAHEAD, int NUM_KEEP, float DIV_TOLERANCE, CrossoverType crossoverType, int greediness, Types::Score opt, ResultRegister &rr);
    void checkSolution(const Ordering &o);
    bool consistentWithAncestral(const Ordering &ordering);
    std::pair<int, int> constraintRange(const Ordering &ordering);
    bool consistentWithOrdering(const Ordering &o, const std::vector<int> &parents);
    Types::Score getBestScore(const Ordering &ordering);
    void printModelString(const std::vector<int> &parents);
  private:
    Instance &instance;
    static int climbs;
    std::list< std::pair<int,int> > allParents;
    bool **ancestor, **descendant, *satisfied;
};

#endif /* LOCALSEARCH_H */

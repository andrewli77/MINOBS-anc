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
    LocalSearch(const Instance &instance);
    const ParentSet &bestParent(const Ordering &ordering, const Types::Bitset pred, int idx) const;
    const ParentSet &bestParentVar(const Types::Bitset pred, const Variable &v) const;
    const ParentSet *bestParentVarWithParent(const Types::Bitset pred, const Variable &a, const Variable &b, const Types::Score orig) const;
    Types::Bitset getPred(const Ordering &ordering, int idx) const;
    Types::Score getBestScore(const Ordering &ordering) const;
    Types::Score getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const;
    SwapResult findBestScoreSwap(const Ordering &ordering, int i, const std::vector<int> &parents, Types::Bitset &pred);
    PivotResult getBestInsert(const Ordering &ordering, int pivot, Types::Score initScore) const;
    FastPivotResult getBestInsertFast(const Ordering &ordering, int pivot, Types::Score initScore, const std::vector<int> &parents, const std::vector<Types::Score> &scores);
    SearchResult makeResult(const Ordering &ordering) const;
    SearchResult hillClimb(const Ordering &ordering);
    SearchResult hillClimb(const Ordering &ordering, float timeLimit, ResultRegister &rr);
    SearchResult ILS(const Ordering &ordering, int MAX_PERTURBS, int IMPROVE_THRESHHOLD, int PERTURB_FACTOR, float updateTolerance, ResultRegister &rr, float timeLimit, Types::Score opt);
    SearchResult tabuSearch(const Ordering &ordering, float timeLimit, int listSize, int softThreshold, ResultRegister &rr);
    SearchResult tabuSearchWithNRestarts(float timeLimit, int listSize, int softThreshold, ResultRegister &rr, Types::Score opt);
    SearchResult hillClimbingWithNRestarts(int numRestarts, ResultRegister &rr) ;
    SearchResult ILSWithNRestarts(float timeLimit, int greediness, int MAX_PERTURBS, int IMPROVE_THRESHHOLD, int PERTURB_FACTOR, float updateTolerance, ResultRegister &rr, Types::Score opt);
    Types::Score findBestScoreRange(const Ordering &o, int start, int end);
    SearchResult simulatedAnnealing(double initTemp, int numSteps, float decay, float timeLimit, Types::Score opt, Neighbours neighbour, ResultRegister &rr);
    SearchResult simulatedAnnealingStepsSwap(Ordering &o, double initTemp, int maxSteps, float decay, float timeLimit, ResultRegister &rr);
    SearchResult simulatedAnnealingStepsInsert(Ordering &o, double initTemp, int maxSteps, float decay, float timeLimit, ResultRegister &rr);
    std::vector<int> bestParentIds(const Ordering &ordering);
    Ordering depthSort(const Ordering &ordering);
    SearchResult genetic(float cutoffTime, int INIT_POPULATION_SIZE, int NUM_CROSSOVERS, int NUM_MUTATIONS, int MUTATION_POWER, int DIV_LOOKAHEAD, int NUM_KEEP, float DIV_TOLERANCE, CrossoverType crossoverType, int greediness, Types::Score opt, ResultRegister &rr);
    int getDepth(int m, const std::vector<int> &depth, const Ordering &o, const ParentSet &parent);
    SearchResult kollerSearch(Ordering &o, int listSize, float timeLimit, ResultRegister &rr);
    SearchResult kollerSearchRestarts(int listSize, float timeLimit, Types::Score opt, ResultRegister &rr);
    SearchResult hillClimbBestImprove(Ordering ordering, float cutoff, ResultRegister &rr);
    SearchResult hillClimbHybridImprove(Ordering ordering, float cutoffTime, ResultRegister &rr);
    SearchResult hillClimbOldHybridImprove(Ordering ordering, float cutoffTime, ResultRegister &rr);
    SearchResult hillClimbFirstImproveV1(Ordering ordering,float cutoffTime, ResultRegister &rr);
    PivotResult  getBestInsertWithHook(const Ordering &ordering, int pivot, Types::Score initScore, std::vector<std::vector<Types::Score>> &hook) const;
    SearchResult hillClimbFirstImproveV2(Ordering ordering, float cutoffTime, ResultRegister &rr);
    SearchResult hillClimbWithRestartsProbe(SelectType type, int numRuns, float cutoffTime, ResultRegister &rr, int greediness = -1);
    SearchResult kollerSearchV2(Ordering &o, int listSize, float timeLimit, ResultRegister &rr);
    FastPivotResult getInsertScore(Ordering o, int i, int j, Types::Score initScore, std::vector<int> parents, std::vector<Types::Score> scores);
    int depth(int n, const std::vector<int> &parents) const;
    void checkSolution(const Ordering &o);
  private:
    const Instance &instance;
};

#endif /* LOCALSEARCH_H */

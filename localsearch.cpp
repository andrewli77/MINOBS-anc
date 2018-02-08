#include "localsearch.h"
#include "debug.h"
#include <numeric>
#include <utility>
#include <deque>
#include <cmath>
#include "tabulist.h"
#include "util.h"
#include "movetabulist.h"
#include "swaptabulist.h"

LocalSearch::LocalSearch(Instance &instance) : instance(instance) { 
}

const Types::Score INF = 223372036854775807LL;
const Types::Score PENALTY = 100000000000000LL;

int LocalSearch::climbs = 0;

const ParentSet &LocalSearch::bestParent(const Ordering &ordering, const Types::Bitset pred, int idx) const {
  int current = ordering.get(idx);
  const Variable &v = instance.getVar(current);
  return bestParentVar(pred, v);
}

const ParentSet &LocalSearch::bestParentVar(const Types::Bitset pred, const Variable &v) const {
  int numParents = v.numParents();
  for (int i = 0; i < numParents; i++) {
    const ParentSet &p = v.getParent(i);
    if (p.subsetOf(pred)) {
      return p;
    }
  }
  //DBG("PARENT SET NOT FOUND");
  return v.getParent(0); //Should never happen in THeory
}


// Sketchy to use pointeres but we have to use null.. it's possible there is no Var at all.
const ParentSet *LocalSearch::bestParentVarWithParent(const Types::Bitset pred, const Variable &a, const Variable &b, const Types::Score orig) const {
  //const std::vector<int> &candidates = a.parentsWithVarId(b.getId());
  auto candidates_iter = a.parentsWithVarId(b.getId());
  if (candidates_iter == a.parentsWithVar.end()) return NULL;
  const std::vector<int> &candidates = candidates_iter->second;
  int n = candidates.size();
  for (int i = 0; i < n; i++) {
    const ParentSet &p = a.getParent(candidates[i]);
    if (p.getScore() >= orig) break;
    if (p.subsetOf(pred)) {
      return &p;
    }
  }
  //DBG("PARENT SET NOT FOUND");
  return NULL; //Coult happen
}


Types::Bitset LocalSearch::getPred(const Ordering &ordering, int idx) const {
  int n = instance.getN();
  Types::Bitset pred(n, 0);
  for (int i = 0; i < idx; i++) {
    pred[ordering.get(i)] = 1;
  }
  return pred;
}


bool LocalSearch::consistentWithAncestral(const Ordering &ordering) const {
  int n = instance.getN(), m = instance.getM();
  int pos[n];
  for (int i=0; i < n; i++) {
    pos[ordering.get(i)] = i;
  }

  for (int i=0; i < m; i++) {
    int x = instance.getAncestral(i).first, y = instance.getAncestral(i).second;
    if (pos[x] > pos[y]) {
      return false;
    }
  }

  return true;
}

// New code
Types::Score LocalSearch::getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores, std::vector<int> &unconstrainedParents) const {
  int n = instance.getN(), m = instance.getM();

  if (!consistentWithAncestral(ordering)) {
    return INF;
  }

  Types::Bitset pred(n, 0);
  Types::Score score = 0;
  for (int i = 0; i < n; i++) {
    const ParentSet &p = bestParent(ordering, pred, i);
    parents[ordering.get(i)] = p.getId();
    scores[ordering.get(i)] = p.getScore();
    unconstrainedParents[ordering.get(i)] = p.getId();
    score += p.getScore();
    pred[ordering.get(i)] = 1;
  }

  if (m == 0) {
    return score;
  } else {
    return modifiedDAGScoreWithParents(ordering, parents, scores);
  }
}

// CAN BE OPTIMIZED
bool LocalSearch::hasDipath(const std::vector<int> &parents, int x, int y) const {
  if (x == y) {
    return true;
  }

  const Variable &yVar = instance.getVar(y);
  const ParentSet &p = yVar.getParent(parents[y]);
  const std::vector<int> &pars = p.getParentsVec();

  for (int i=0; i < pars.size(); i++) {
    if (hasDipath(parents, x, pars[i])) {
      return true;
    }
  }

  return false;
}

int LocalSearch::numConstraintsSatisfied(const std::vector<int> &parents) const {
  int n = instance.getN(), m = instance.getM(), count = 0;

  for (int i=0; i < m; i++) {
    const Ancestral &cons = instance.getAncestral(i);
    if (hasDipath(parents, cons.first, cons.second)) {
      count++;
    }
  }

  return count;
}

// GREEDY HILL-CLIMBING METHOD (First Strict Improvement)
// Optimize solution by number of constraints satisfied, using score as a tiebreaker.
Types::Score LocalSearch::modifiedDAGScore(const Ordering &ordering, const std::vector<int> &parents) const {
  int n = instance.getN(), m = instance.getM();
  int curNumSat = numConstraintsSatisfied(parents);

  if (curNumSat == m) {
    // Compute the final score of the graph.

    Types::Score finalSc = 0LL;
    for (int i=0; i<n; i++) {
      const Variable &v = instance.getVar(i);
      finalSc += v.getParent(parents[i]).getScore();
    }
    return finalSc;
  }


  Types::Bitset pred[n];

  std::vector<int> bestGraph = parents;


  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }

  while(true) {
    climbs++;

    // Consider all feasible parent sets

    int bestVar, bestParent;

    bool foundImproving = false;

    std::list< std::pair<int, int> >  &allParents = instance.getParentList();

    for (auto it = allParents.begin(); it != allParents.end(); it++) {
      int cur = it->first, par = it->second;
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
        int oldPar = bestGraph[cur];
        bestGraph[cur] = par;

        int numSat = numConstraintsSatisfied(bestGraph);
        Types::Score sc = p.getScore();

        bestGraph[cur] = oldPar;

        if (numSat > curNumSat) {
          bestVar = cur;
          bestParent = par;
          foundImproving = true;
          break;
        }
      }
    }

    if (!foundImproving) {
      break;
    }

    bestGraph[bestVar] = bestParent;
    curNumSat = numConstraintsSatisfied(bestGraph);

    if (curNumSat == m) {
      break;
    }
  }

  Types::Score finalSc = 0LL;
  for (int i=0; i<n; i++) {
      const Variable &v = instance.getVar(i);
      finalSc += v.getParent(bestGraph[i]).getScore();
  }

  // Add a penalty for each broken constraint.
  finalSc += PENALTY * (m - curNumSat);
  return finalSc;
}

// GREEDY HILL-CLIMBING METHOD (First Strict Improvement)
// Optimize solution by number of constraints satisfied, using score as a tiebreaker.
Types::Score LocalSearch::modifiedDAGScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const {
  int n = instance.getN(), m = instance.getM();
  int curNumSat = numConstraintsSatisfied(parents);

  if (curNumSat == m) {
    // Compute the final score of the graph.

    Types::Score finalSc = 0LL;
    for (int i=0; i<n; i++) {
      finalSc += scores[i];
    }
    return finalSc;
  }


  Types::Bitset pred[n];

  std::vector<int> bestGraph = parents;


  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }

  while(true) {
    climbs++;

    // Consider all feasible parent sets

    int bestVar, bestParent;

    bool foundImproving = false;

    std::list< std::pair<int, int> >  &allParents = instance.getParentList();

    for (auto it = allParents.begin(); it != allParents.end(); it++) {
      int cur = it->first, par = it->second;
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
        int oldPar = bestGraph[cur];
        bestGraph[cur] = par;

        int numSat = numConstraintsSatisfied(bestGraph);
        Types::Score sc = p.getScore();

        bestGraph[cur] = oldPar;

        if (numSat > curNumSat) {
          bestVar = cur;
          bestParent = par;
          foundImproving = true;
          break;
        }
      }
    }

    if (!foundImproving) {
      break;
    }

    bestGraph[bestVar] = bestParent;
    curNumSat = numConstraintsSatisfied(bestGraph);

    if (curNumSat == m) {
      break;
    }
  }

  Types::Score finalSc = 0LL;
  for (int i=0; i<n; i++) {
      const Variable &v = instance.getVar(i);
      scores[i] = v.getParent(bestGraph[i]).getScore();
      finalSc += scores[i];
  }

  parents = bestGraph;

  // Add a penalty for each broken constraint.
  finalSc += PENALTY * (m - curNumSat);
  return finalSc;
}

Types::Score LocalSearch::findBestScoreRange(const Ordering &o, int start, int end) {
  int n = instance.getN();
  Types::Score curScore = 0;
  Types::Bitset used(n, 0);
  for (int i = 0; i < start; i++) {
    used[o.get(i)] = 1;
  }
  for (int i = start; i <= end; i++) {
    const ParentSet &cur = bestParent(o, used, i);
    curScore += cur.getScore();
    used[o.get(i)] = 1;
  }
  return curScore;
}

void LocalSearch::bestSwapForward(
  int pivot,
  Ordering o,
  const std::vector<int> &parents,
  Ordering &bestOrdering,
  std::vector<int> &bestParents, 
  Types::Score &bestSc
) const
{
  int n = instance.getN();
  std::vector<int> tmpParents = parents;
  Types::Bitset pred(n, 0);

  for (int i = 0; i < pivot; i++) {
    pred[o.get(i)] = 1;
  }

  for (int j = pivot; j < n-1; j++) {
    // First check that the swap results in a valid ordering.
    // It is valid iff O[j] -> O[j+1] is NOT an ancestral constraint.
    // Furthermore, if O[j] -> O[j+1] IS an ancestral constraint all further forward swaps are invalid.

    if (instance.isConstraint(o.get(j), o.get(j+1))) {
      break;
    }

    pred[o.get(j+1)] = 1;

    // Check if O_{j+1} still has a feasible parent set (feasible iff it does not contain O_j)
    const Variable &v = instance.getVar(o.get(j+1));
    const ParentSet &ps = v.getParent(tmpParents[o.get(j+1)]);

    
    if (ps.hasElement(o.get(j))) {
      // Replace the current parent set of O_{j+1} with some valid replacement.
      // Note that here pred[o.get(j)] = 0.
      tmpParents[o.get(j+1)] = bestParentVar(pred, v).getId();
    }

    o.swap(j, j+1);

    // Find the optimal new parent set for X_j.
    // We need only consider those that contain X_{j+1}.

    const Variable &other = instance.getVar(o.get(j+1));

    int nParents = other.numParents();
    for (int l = 0; l < nParents; l++) {
      const ParentSet &par = other.getParent(l);

      if (par.getScore() >= other.getParent(tmpParents[o.get(j+1)]).getScore()) {
        break;
      }

      if (par.hasElement(o.get(j)) && par.subsetOf(pred)) {
        tmpParents[o.get(j+1)] = par.getId();
        break;
      }
    }

    Types::Score sc = modifiedDAGScore(o, tmpParents);
    if (sc < bestSc) {
      bestSc = sc;
      bestParents = tmpParents;
      bestOrdering = o;
    }
  }
}

void LocalSearch::bestSwapBackward(
  int pivot,
  Ordering o,
  const std::vector<int> &parents,
  Ordering &bestOrdering,
  std::vector<int> &bestParents, 
  Types::Score &bestSc
) const
{
  int n = instance.getN();
  std::vector<int> tmpParents = parents;

  Types::Bitset pred(n, 0);

  for (int i = 0; i < pivot; i++) {
    pred[o.get(i)] = 1;
  }


  for (int j = pivot-1; j >= 0; j--) {
    // First check that the swap results in a valid ordering.
    // It is valid iff O[j] -> O[j+1] is NOT an ancestral constraint.
    // Furthermore, if O[j] -> O[j+1] IS an ancestral constraint all further forward swaps are invalid.


    if (instance.isConstraint(o.get(j), o.get(j+1))) {
      break;
    }


    pred[o.get(j)] = 0;

    // Check if O_{j+1} still has a feasible parent set (feasible iff it does not contain O_j)
    const Variable &v = instance.getVar(o.get(j+1));
    const ParentSet &ps = v.getParent(tmpParents[o.get(j+1)]);

    if (ps.hasElement(o.get(j))) {
      // Replace the current parent set of O_{j+1} with some valid replacement.
      // Note that here pred[o.get(j)] = 0.
      tmpParents[o.get(j+1)] = bestParentVar(pred, v).getId();
    }


    o.swap(j, j+1);

    pred[o.get(j)] = 1;

    // Find the optimal new parent set for X_j.
    // We need only consider those that contain X_{j+1}.
    const Variable &other = instance.getVar(o.get(j+1));

    int nParents = other.numParents();
    for (int l = 0; l < nParents; l++) {
      const ParentSet &par = other.getParent(l);

      if (par.getScore() >= other.getParent(tmpParents[o.get(j+1)]).getScore()) {
        break;
      }

      if (par.hasElement(o.get(j)) && par.subsetOf(pred)) {
        tmpParents[o.get(j+1)] = par.getId();
        break;
      }
    }

    pred[o.get(j)] = 0;

    Types::Score sc = modifiedDAGScore(o, tmpParents);

    if (sc < bestSc) {
      bestSc = sc;
      bestParents = tmpParents;
      bestOrdering = o;
    }
  }
}

SearchResult LocalSearch::hillClimb(const Ordering &ordering) {
  bool improving = false;
  int n = instance.getN();
  std::vector<int> parents(n), unconstrainedParents(n);
  std::vector<Types::Score> scores(n);
  int steps = 0;
  std::vector<int> positions(n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores, unconstrainedParents);

  std::iota(positions.begin(), positions.end(), 0);


  if (curScore >= PENALTY) {
    return SearchResult(curScore, cur);
  }

  DBG("Inits: " << cur);
  do {
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int s = 0; s < n && !improving; s++) {
      int pivot = positions[s];


      std::vector<int> bestUnconstrainedParents(n);
      Ordering bestOrdering;
      Types::Score bestSc = INF;

      bestSwapForward(pivot, cur, unconstrainedParents, bestOrdering, bestUnconstrainedParents, bestSc);
      bestSwapBackward(pivot, cur, unconstrainedParents, bestOrdering, bestUnconstrainedParents, bestSc);

      if (bestSc < curScore) {
        steps += 1;
        improving = true;
        cur = bestOrdering;
        curScore = bestSc;
        unconstrainedParents = bestUnconstrainedParents;
      }

      /*
      FastPivotResult result = getBestInsertFast(cur, pivot, curScore, parents, scores);
      if (result.getScore() < curScore) {
        steps += 1;
        improving = true;
        cur.insert(pivot, result.getSwapIdx());
        parents = result.getParents();
        scores = result.getScores();
        curScore = result.getScore();
      }
      */
    }
    DBG("Cur Score: " << curScore);
  } while(improving);
  DBG("Total Steps: " << steps);

  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::genetic(float cutoffTime, int INIT_POPULATION_SIZE, int NUM_CROSSOVERS, int NUM_MUTATIONS,
    int MUTATION_POWER, int DIV_LOOKAHEAD, int NUM_KEEP, float DIV_TOLERANCE, CrossoverType crossoverType, int greediness, Types::Score opt, ResultRegister &rr) {
  int n = instance.getN();
  SearchResult best(Types::SCORE_MAX, Ordering(n));
  std::deque<Types::Score> fitnesses;
  Population population(*this);
  int numGenerations = 1;
  std::cout << "Time: " << rr.check() << " Generating initial population" << std::endl;
  for (int i = 0; i < INIT_POPULATION_SIZE; i++) {
    SearchResult o;
    if (greediness == -1) {
      o = hillClimb(Ordering::randomOrdering(instance));
    } else {
      o = hillClimb(Ordering::greedyOrdering(instance, greediness));
    }
  /*
   * Added to show a solution early.
   */
    std::cout << "Time: " << rr.check() <<  " i = " << i << " The score is: " << o.getScore() << std::endl;
    std::vector<int> parents(n);
    std::vector<Types::Score> scores(n);

    rr.record(o.getScore(), o.getOrdering());
    population.addSpecimen(o);
  }
  std::cout << "Done generating initial population" << std::endl;

  do {
    //std::cout << "Time: " << rr.check() << " Starting generation " << numGenerations << std::endl;
    //DBG(population);
    std::vector<SearchResult> offspring;
    population.addCrossovers(NUM_CROSSOVERS, crossoverType, offspring);
    //DBG(population);
    population.mutate(NUM_MUTATIONS, MUTATION_POWER, offspring);
    //DBG(population);
    population.append(offspring);
    population.filterBest(INIT_POPULATION_SIZE);
    DBG(population);
    Types::Score fitness = population.getAverageFitness();
    fitnesses.push_back(fitness);
    if (fitnesses.size() > DIV_LOOKAHEAD) {
      Types::Score oldFitness = fitnesses.front();
      fitnesses.pop_front();
      float change = std::abs(((float)fitness-(float)oldFitness)/(float)oldFitness);
      if (change < DIV_TOLERANCE && DIV_TOLERANCE != -1) {
        DBG("Diversification Step. Change: " << change << " Old: " << oldFitness << " New: " << fitness);
        population.diversify(NUM_KEEP, instance);
        fitnesses.clear();
      }
    }
    DBG("Fitness: " << population.getAverageFitness());
    SearchResult curBest = population.getSpecimen(0);
    Types::Score curScore = curBest.getScore();
    if (curScore < best.getScore()) {
      std::cout << "Time: " << rr.check() <<  " The best score at this iteration is: " << curScore << std::endl;
      rr.record(curBest.getScore(), curBest.getOrdering());
      best = curBest;
    }
    numGenerations++;
  } while (rr.check() < cutoffTime);
  std::cout << "Generations: " << numGenerations << std::endl;
  return best;
}

void LocalSearch::checkSolution(const Ordering &o) {
  int n = instance.getN(), m = instance.getM();
  std::vector<int> parents(n), unconstrainedParents(n);
  std::vector<Types::Score> scores(n);
  long long sc = getBestScoreWithParents(o, parents, scores, unconstrainedParents);
  long long scoreFromScores = 0;
  long long scoreFromParents = 0;
  std::vector<int> inverse(n);
  for (int i = 0; i < n; i++) {
    inverse[o.get(i)] = i;
  }
  bool valid = true;
  for (int i = 0; i < n; i++) {
    int var = o.get(i);
    const Variable &v = instance.getVar(var);
    const ParentSet &p = v.getParent(parents[var]);
    const std::vector<int> &parentVars = p.getParentsVec();
    std::string parentsStr = "";
    bool before = true;
    for (int j = 0; j < parentVars.size(); j++) {
      parentsStr += std::to_string(parentVars[j]) + " ";
      before = before && (inverse[parentVars[j]] < i);
    }
    valid &= before;
    scoreFromParents += p.getScore();
    scoreFromScores += scores[var];
    std::cout << "Ordering[" << i << "]\t= "<< var << "\tScore:\t" << scores[var] << "\tParents:\t{ " << parentsStr << "}\tValid: " << before << std::endl;
  }

  std::cout << "Total Score: " << scoreFromScores << " " << scoreFromParents << " " << sc << std::endl;
  std::string validStr = valid ? "Good" : "Bad";
  std::cout << "Validity Check: " << validStr << std::endl;

  if (numConstraintsSatisfied(parents) == m) {
    std::cout << "Ancestral constraints check: Good" << std::endl;
  } else {
    std::cout << "Ancestral constraints check: Bad" << std::endl;
  }

  std::cout << "Climbs: " << climbs << std::endl;
}

bool LocalSearch::consistentWithOrdering(const Ordering &o, const std::vector<int> &parents) const {
  int n = instance.getN();
  Types::Bitset pred(n, 0);

  for (int i = 0; i < n; i++) {
    int cur = o.get(i);
    const Variable &v = instance.getVar(cur);
    const ParentSet &par = v.getParent(parents[cur]);

    if (!par.subsetOf(pred)) {
      return false;
    }

    pred[cur] = 1;
  }

  return true;
}

Types::Score LocalSearch::getBestScore(const Ordering &ordering) const {
  int n = instance.getN(), m = instance.getM();

  if (!consistentWithAncestral(ordering)) {
    return INF;
  }

  Types::Bitset pred(n, 0);
  Types::Score score = 0;
  for (int i = 0; i < n; i++) {
    const ParentSet &p = bestParent(ordering, pred, i);
    score += p.getScore();
    pred[ordering.get(i)] = 1;
  }

  return score;
}
#include "localsearch.h"
#include "debug.h"
#include <numeric>
#include <utility>
#include <string>
#include <deque>
#include <fstream>
#include <stack>
#include <chrono>
#include <cmath>
#include "tabulist.h"
#include "util.h"
#include "movetabulist.h"
#include "swaptabulist.h"

LocalSearch::LocalSearch(Instance &instance) : instance(instance) { 
  allParents = instance.getParentList();
  alloc_2d(ancestor, descendant, satisfied);
}

LocalSearch::~LocalSearch() {
  dealloc_2d(ancestor, descendant, satisfied);
}

int LocalSearch::climbs = 0;

const ParentSet &LocalSearch::bestParent(const Ordering &ordering, const Types::Bitset &pred, int idx) {
  int current = ordering.get(idx);
  const Variable &v = instance.getVar(current);
  return bestParentVar(pred, v);
}

const ParentSet &LocalSearch::bestParentVar(const Types::Bitset &pred, const Variable &v) {
  int numParents = v.numParents();
  for (int i = 0; i < numParents; i++) {
    const ParentSet &p = v.getParent(i);
    if (p.subsetOf(pred)) {
      return p;
    }
  }

  assert(false);
  return v.getParent(0); //Should never happen in THeory
}

bool LocalSearch::consistentWithAncestral(const Ordering &ordering) {
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
Types::Score LocalSearch::getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores, std::vector<int> &unconstrainedParents) {
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
    if (score < optimalScore) {
      optimalScore = score;
      optimalOrdering = ordering;
      globalOptimum = parents;
      optimalScores = scores;
    }

    return score;
  } else {
    return modifiedDAGScoreWithParents(ordering, parents, scores);
  }
}

void LocalSearch::alloc_2d(bool **&ancestor, bool **&descendant, bool *&satisfied) {
  int n = instance.getN(), m = instance.getM();
  ancestor = new bool* [n];
  descendant = new bool* [n];
  satisfied = new bool[m];

  for (int i = 0; i < n; i++) {
    ancestor[i] = new bool[m];
    descendant[i] = new bool[m];
  }
}


void LocalSearch::dealloc_2d(bool **&ancestor, bool **&descendant, bool *&satisfied) {
  int n = instance.getN(), m = instance.getM();

  for (int i = 0; i < n; i++) {
    delete[] ancestor[i];
    delete[] descendant[i];
  }

  delete[] ancestor;
  delete[] descendant;
  delete[] satisfied;
}

bool LocalSearch::hasDipath(const std::vector<int> &parents, int x, int y) {
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


bool LocalSearch::hasDipathWithOrdering(const std::vector<int> &parents, int x, int y, const std::vector<int> &positions) {
  if (x == y) {
    return true;
  }

  if (positions[y] < positions[x]) {
    return false;
  }

  const Variable &yVar = instance.getVar(y);
  const ParentSet &p = yVar.getParent(parents[y]);
  const std::vector<int> &pars = p.getParentsVec();

  for (int i=0; i < pars.size(); i++) {
    if (hasDipathWithOrdering(parents, x, pars[i], positions)) {
      return true;
    }
  }

  return false;
}

int LocalSearch::numConstraintsSatisfied(const std::vector<int> &parents) {
  int n = instance.getN(), m = instance.getM(), count = 0;

  for (int i=0; i < m; i++) {
    const Ancestral &cons = instance.getAncestral(i);
    if (hasDipath(parents, cons.first, cons.second)) {
      count++;
    }
  }

  return count;
}

int LocalSearch::numConstraintsSatisfied(const std::vector<int> &parents, const std::vector<int> &positions) {
  int n = instance.getN(), m = instance.getM(), count = 0;

  for (int i=0; i < m; i++) {
    const Ancestral &cons = instance.getAncestral(i);
    if (hasDipathWithOrdering(parents, cons.first, cons.second, positions)) {
      count++;
    }
  }

  return count;
}

int LocalSearch::numConstraintsSatisfied(const std::vector<int> &newParents, bool **ancestor, bool **descendant, bool *satisfied, int cur, const std::vector<int> &positions) {
  int n = instance.getN(), m = instance.getM(), count = 0, par = newParents[cur];

  const Variable &var = instance.getVar(cur);
  const ParentSet &p = var.getParent(par); 

  for (int i = 0; i < m; i++) {

    // Check if a satisfied constraint is now unsatisfied
    if (satisfied[i]) {
      if (!(positions[instance.getAncestral(i).first] <= positions[cur] && positions[cur] <= positions[instance.getAncestral(i).second])) {
        count++;
      } else if (ancestor[cur][i] && descendant[cur][i]) {
        // Maybe do a check here later for if cur has a parent who has X_i has an ancestor
        const std::vector<int> &curPars = p.getParentsVec();
        bool connected = false;
        for (int j = 0; j < curPars.size(); j++) {
          if (ancestor[curPars[j]][i]) {
            connected = true;
            break;
          }
        }

        if (connected) {
          count++;
        } else if (hasDipathWithOrdering(newParents, instance.getAncestral(i).first, instance.getAncestral(i).second, positions)) {
          count++;
        }
      } else {
        count++;
      }
    } 

    // Check if an unsatisfied constraint is now satisfied.
    else {
      if (positions[instance.getAncestral(i).first] <= positions[cur] && positions[cur] <= positions[instance.getAncestral(i).second] && descendant[cur][i]) {
        const std::vector<int> &curPars = p.getParentsVec();
        bool connected = false;
        for (int j = 0; j < curPars.size(); j++) {
          if (ancestor[curPars[j]][i]) {
            connected = true;
            break;
          }
        }

        if (connected) {
          count++;
        }
      }
    }
  }
  
  return count;
}


bool LocalSearch::improving(const std::vector<int> &newParents, bool **ancestor, bool **descendant, bool *satisfied, int cur, const std::vector<int> &positions, int curNumSat, int oldPar, int &numSat) {

  int n = instance.getN(), m = instance.getM(), count = 0, par = newParents[cur];

  const Variable &var = instance.getVar(cur);
  const ParentSet &p = var.getParent(par); 
  const ParentSet &oldP = var.getParent(oldPar); 

  for (int i = 0; i < m; i++) {

    // Check if an unsatisfied constraint is now satisfied.
    if (!satisfied[i]) {
      if (positions[instance.getAncestral(i).first] <= positions[cur] && positions[cur] <= positions[instance.getAncestral(i).second] && descendant[cur][i]) {
        const std::vector<int> &curPars = p.getParentsVec();
        bool connected = false;
        for (int j = 0; j < curPars.size(); j++) {
          if (ancestor[curPars[j]][i]) {
            connected = true;
            break;
          }
        }

        if (connected) {
          count++;
        }
      }
    } else {
      count ++;
    }
  }

  // Count now is a theoretical upper bound on the number of satisfied constraints.

  for (int i = 0; i < m; i++) {
    // Check if a satisfied constraint is now unsatisfied

    if (count < curNumSat ||
      (count == curNumSat && p.getScore() > oldP.getScore()) || 
      (count == curNumSat && p.getScore() == oldP.getScore() && p.size() >= oldP.size())) {
      numSat = 0;
      return false;
    }

    if (satisfied[i]) {
      if (ancestor[cur][i] && descendant[cur][i]) {

        if (!(hasDipathWithOrdering(newParents, instance.getAncestral(i).first, instance.getAncestral(i).second, positions))) {
          count--;
        }
      }
    } 
  }
  
  numSat = count;

  return (count > curNumSat) ||
      (count == curNumSat && p.getScore() < oldP.getScore()) ||
      (count == curNumSat && p.getScore() == oldP.getScore() && p.size() < oldP.size());

}


void LocalSearch::computeAncestralGraph(const std::vector<int> &parents, bool **ancestor, bool **descendant, bool *satisfied, const std::vector<int> &positions) {
  int n = instance.getN(), m = instance.getM();

  for (int j = 0; j < m; j++) {
    satisfied[j] = false;
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      ancestor[i][j] = hasDipathWithOrdering(parents, instance.getAncestral(j).first, i, positions);
      descendant[i][j] = hasDipathWithOrdering(parents, i, instance.getAncestral(j).second, positions);
      if (ancestor[i][j] && descendant[i][j]) {
        satisfied[j] = true;
      }
    }
  }
}


Types::Score LocalSearch::modifiedDAGScore(const Ordering &ordering, const std::vector<int> &parents) {
  int n = instance.getN(), m = instance.getM();
  std::vector<int> bestGraph(parents);


  std::vector<int> positions(n);

  for (int i = 0; i < n; i++) {
    positions[ordering.get(i)] = i;
  }
  computeAncestralGraph(bestGraph, ancestor, descendant, satisfied, positions);
  int curNumSat = 0;

  for (int i = 0; i < m; i++) {
    curNumSat += (satisfied[i]);
  }

  std::vector<Types::Bitset> pred(n);

  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }
  
  int lastRandomWalk[n];

  for (int i = 0; i < n; i++) {
    lastRandomWalk[i] = -tabuTenure-1;
  }



  int iters = 0;
  while(true) {

    bool foundImproving = false;

    for (int i = 0; i < allParents.size(); i++) {
      int cur = allParents[i].first, par = allParents[i].second;
      int oldPar = bestGraph[cur];
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      const ParentSet &oldP = var.getParent(oldPar);
      Types::Score sc = p.getScore();
      
      if (curNumSat == m && sc > oldP.getScore()) {
        continue;
      }

      if (par != bestGraph[cur] && (iters - lastRandomWalk[cur] > tabuTenure) && p.subsetOf(pred[cur])) {
        bestGraph[cur] = par;
        int numSat;
        bool improv = improving(bestGraph, ancestor, descendant, satisfied, cur, positions, curNumSat, oldPar, numSat);
        bestGraph[cur] = oldPar;

        if (improv) {
          bestGraph[cur] = par;
          curNumSat = numSat;
          foundImproving = true;
          break;
        }
      }
    }

    if (!foundImproving) {
      // Take a random walk.
      if ((double)rand() / RAND_MAX < walkProb) {

        Types::Score finalSc = 0LL;
        for (int i=0; i<n; i++) {
          const Variable &v = instance.getVar(i);
          finalSc += v.getParent(bestGraph[i]).getScore();
        }

        // Add a penalty for each broken constraint.
        finalSc += PENALTY * (m - curNumSat);

        if (finalSc < optimalScore) {
          optimalScore = finalSc;
          optimalOrdering = ordering;
          globalOptimum = bestGraph;

          for (int i = 0; i < n; i++) {
            optimalScores[i] = instance.getVar(i).getParent(bestGraph[i]).getScore();
          }
        }

        while (true) {
          int i = rand() % allParents.size();

          int cur = allParents[i].first, par = allParents[i].second;
          const Variable &var = instance.getVar(cur);
          const ParentSet &p = var.getParent(par);

          if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
            lastRandomWalk[cur] = iters;
            bestGraph[cur] = par;
            curNumSat = numConstraintsSatisfied(bestGraph, ancestor, descendant, satisfied, cur, positions);
            foundImproving = true;
            break;
          }
        }
      } else {
        break;
      } 
    }

    computeAncestralGraph(bestGraph, ancestor, descendant, satisfied, positions);
    iters++;
  }




  Types::Score finalSc = 0LL;
  for (int i=0; i<n; i++) {
      const Variable &v = instance.getVar(i);
      finalSc += v.getParent(bestGraph[i]).getScore();
  }

  // Add a penalty for each broken constraint.
  finalSc += PENALTY * (m - curNumSat);

  if (finalSc < optimalScore) {
    optimalScore = finalSc;
    optimalOrdering = ordering;
    globalOptimum = bestGraph;

    for (int i = 0; i < n; i++) {
      optimalScores[i] = instance.getVar(i).getParent(bestGraph[i]).getScore();
    }
  }

  return finalSc;
}


// GREEDY HILL-CLIMBING METHOD (First Improvement)
Types::Score LocalSearch::modifiedDAGScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) {
  int n = instance.getN(), m = instance.getM();
  std::vector<int> bestGraph(parents);

  std::vector<int> positions(n);

  for (int i = 0; i < n; i++) {
    positions[ordering.get(i)] = i;
  }

  computeAncestralGraph(bestGraph, ancestor, descendant, satisfied, positions);

  int curNumSat = 0;

  for (int i = 0; i < m; i++) {
    curNumSat += (satisfied[i]);
  }

  std::vector<Types::Bitset> pred(n);
  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }

  int lastRandomWalk[n];

  for (int i = 0; i < n; i++) {
    lastRandomWalk[i] = -tabuTenure-1;
  }

  int iters = 0;
  while(true) {

    bool foundImproving = false;

    for (int i = 0; i < allParents.size(); i++) {
      int cur = allParents[i].first, par = allParents[i].second;
      int oldPar = bestGraph[cur];
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      const ParentSet &oldP = var.getParent(oldPar);
      Types::Score sc = p.getScore();
      

      if (curNumSat == m && sc > oldP.getScore()) {
        continue;
      }

      if (par != bestGraph[cur] && (iters - lastRandomWalk[cur] > tabuTenure) && p.subsetOf(pred[cur])) {
        bestGraph[cur] = par;
        int numSat;
        bool improv = improving(bestGraph, ancestor, descendant, satisfied, cur, positions, curNumSat, oldPar, numSat);
        bestGraph[cur] = oldPar;


        if (improv) {
          bestGraph[cur] = par;
          curNumSat = numSat;
          foundImproving = true;
          break;
        }
      }
    }

    if (!foundImproving) {
      // Take a random walk.
      if ((double)rand() / RAND_MAX < walkProb) {

        Types::Score finalSc = 0LL;
        for (int i=0; i<n; i++) {
            const Variable &v = instance.getVar(i);
            scores[i] = v.getParent(bestGraph[i]).getScore();
            finalSc += scores[i];
        }

        // Add a penalty for each broken constraint.
        finalSc += PENALTY * (m - curNumSat);

        if (finalSc < optimalScore) {
          optimalScore = finalSc;
          optimalScores = scores;
          optimalOrdering = ordering;
          globalOptimum = bestGraph;
        }

        while (true) {
          int i = rand() % allParents.size();

          int cur = allParents[i].first, par = allParents[i].second;
          const Variable &var = instance.getVar(cur);
          const ParentSet &p = var.getParent(par);

          if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
            lastRandomWalk[cur] = iters;
            bestGraph[cur] = par;
            curNumSat = numConstraintsSatisfied(bestGraph, ancestor, descendant, satisfied, cur, positions);
            foundImproving = true;
            break;
          }
        }
      } else {
        break;
      } 
    }

    computeAncestralGraph(bestGraph, ancestor, descendant, satisfied, positions);
    iters++;
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

  if (finalSc < optimalScore) {
    optimalScore = finalSc;
    optimalScores = scores;
    optimalOrdering = ordering;
    globalOptimum = bestGraph;
  }

  return finalSc;
}


// Return the parent of node that satisfies the most ancestral constraints.
// Score is used to tiebreak.
int LocalSearch::bestConstrainedParent(std::vector<int> &parents, int node, const Types::Bitset &pred, const std::vector<int> &positions) {
  int n = instance.getN();
  int bestNumSat = -1, bestSize, bestParent;
  Types::Score bestSc;

  const Variable &var = instance.getVar(node);

  for (int i = 0; i < var.numParents(); i++) {
    const ParentSet &ps = var.getParent(i);

    if (ps.subsetOf(pred)) {
      int oldPar = parents[node];
      parents[node] = i;
      int numSat = numConstraintsSatisfied(parents, positions);
      parents[node] = oldPar;

      if (numSat > bestNumSat ||
         (numSat == bestNumSat && ps.getScore() < bestSc) || 
         (numSat == bestNumSat && ps.getScore() == bestSc && ps.size() < bestSize)) {
        bestNumSat = numSat;
        bestSc = ps.getScore();
        bestSize = ps.size();
        bestParent = i;
      }
    }
  }

  return bestParent;
}

void LocalSearch::bestSwapForward(
  int pivot,
  Ordering o,
  const std::vector<int> &parents,
  Ordering &bestOrdering,
  std::vector<int> &bestParents, 
  Types::Score &bestSc
)
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
      tmpParents[o.get(j+1)] = bestParentVar(pred, v).getId(); //bestConstrainedParent(tmpParents, o.get(j+1), pred, positions);
    }

    o.swap(j, j+1);

    std::vector<Types::Score> scores(n); // We don't actually use this.
    Types::Score sc = modifiedDAGScoreWithParents(o, tmpParents, scores);
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
)
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
      tmpParents[o.get(j+1)] = bestParentVar(pred, v).getId(); //bestConstrainedParent(tmpParents, o.get(j+1), pred, positions);
    }

    o.swap(j, j+1);

    pred[o.get(j)] = 0;

    std::vector<Types::Score> scores(n); // We don't actually use this.
    Types::Score sc = modifiedDAGScoreWithParents(o, tmpParents, scores);

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

    do {
      o = hillClimb(Ordering::randomOrdering(instance));
    } while (o.getScore() >= PENALTY);


    std::cout << "Time: " << rr.check() <<  " i = " << i << " The score is: " << o.getScore() << std::endl;
    std::vector<int> parents(n);
    std::vector<Types::Score> scores(n);

    rr.record(o.getScore(), o.getOrdering());
    population.addSpecimen(o);
  }
  std::cout << "Done generating initial population" << std::endl;

  do {

    Types::Score initialSc = optimalScore;

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


    if (optimalScore < initialSc) {
      walkProb = std::max(0.0, walkProb - 0.025);
    } else {
      walkProb = std::min(0.25, walkProb + 0.05);
    }

    std::cout << "Finished generation: " << numGenerations << std::endl;

  } while (numGenerations < 5);
  std::cout << "Generations: " << numGenerations << std::endl;
  return best;
}

void LocalSearch::checkSolution() {
  int n = instance.getN(), m = instance.getM();

  const Ordering &o = optimalOrdering;
  const std::vector<int> &parents = globalOptimum;
  const std::vector<Types::Score> scores = optimalScores;

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

  std::cout << "Total Score: " << scoreFromScores << " " << scoreFromParents << " " << optimalScore << std::endl;
  std::string validStr = valid ? "Good" : "Bad";
  std::cout << "Validity Check: " << validStr << std::endl;

  bool ancestralValid = (numConstraintsSatisfied(parents) == m);
  if (ancestralValid) {
    std::cout << "Ancestral constraints check: Good" << std::endl;
  } else {
    std::cout << "Ancestral constraints check: Bad" << std::endl;
  }

  std::cout << "Num constraints satisfied: " << numConstraintsSatisfied(parents) << std::endl;

  std::cout << "Climbs: " << climbs << std::endl;


  printModelString(parents, (ancestralValid && valid && (scoreFromParents == scoreFromScores && scoreFromScores == optimalScore)), optimalScore);
  std::cout << "Time: " << total_time << std::endl;
}

bool LocalSearch::consistentWithOrdering(const Ordering &o, const std::vector<int> &parents) {
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

Types::Score LocalSearch::getBestScore(const Ordering &ordering) {
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

void LocalSearch::printModelString(const std::vector<int> &parents, bool valid, Types::Score score) {
  int n = instance.getN();
  std::ifstream file;
  std::ofstream outF;

  if (instance.getFileName().find("asia") != std::string::npos) {
    file = std::ifstream("data/mappings/asia.mapping");
    outF.open("data/asia_results", std::ios_base::app);
  } else if (instance.getFileName().find("alarm") != std::string::npos) {
    file = std::ifstream("data/mappings/alarm.mapping");
    outF.open("data/alarm_results", std::ios_base::app);
  } else if (instance.getFileName().find("hailfinder") != std::string::npos) {
    file = std::ifstream("data/mappings/hailfinder.mapping");
    outF.open("data/hailfinder_results", std::ios_base::app);
  } else if (instance.getFileName().find("child") != std::string::npos) {
    file = std::ifstream("data/mappings/child.mapping");
    outF.open("data/child_results", std::ios_base::app);
  } else if (instance.getFileName().find("sachs") != std::string::npos) {
    file = std::ifstream("data/mappings/sachs.mapping");
    outF.open("data/sachs_results", std::ios_base::app);
  } else if (instance.getFileName().find("insurance") != std::string::npos) {
    file = std::ifstream("data/mappings/insurance.mapping");
    outF.open("data/insurance_results", std::ios_base::app);
  } else {
    std::cout << "No suitable mapping found!" << std::endl;
    exit(0);
  }


  std::vector<std::string> vars(n);

  for (int i=0; i < n; i++) {
    getline(file, vars[i]);
  }


  // Print the network.
  outF << instance.getM() << std::endl;

  if (!valid) {
    outF << "INVALID" << std::endl;
  }

  for (int i = 0; i < n; i++) {
    outF << "[" + vars[i];

    const Variable &varI = instance.getVar(i);
    const ParentSet &par = varI.getParent(parents[i]);
    const std::vector<int> parVec = par.getParentsVec();

    if (parVec.size() != 0) {
      outF << "|";
    }

    for (int j = 0; j < parVec.size(); j++) {
      outF << vars[parVec[j]];
      if (j < parVec.size() - 1) {
        outF << ":";
      }
    }

    outF << "]";
  }

  outF << std::endl;

  outF << score << std::endl;
}
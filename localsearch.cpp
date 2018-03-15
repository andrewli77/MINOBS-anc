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
Types::Score LocalSearch::getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) {
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
    score += p.getScore();
    pred[ordering.get(i)] = 1;
  }

  if (m == 0) {

/*
    if (((double) (score - 205907722839)) / 205907722839 < 0.0005) {
      std::cout << "Printed" << std::endl;
      printModelString(parents, true, score);
    }
*/

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
  std::vector<int> parents(n), initialDAG(n);
  std::vector<Types::Score> scores(n);
  int steps = 0;
  std::vector<int> positions(n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);
  initialDAG = parents;


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

      std::vector<int> bestDAG(n);
      Ordering bestOrdering;
      Types::Score bestSc = INF;

      bestSwapForward(pivot, cur, initialDAG, bestOrdering, bestDAG, bestSc);
      bestSwapBackward(pivot, cur, initialDAG, bestOrdering, bestDAG, bestSc);

      if (bestSc < curScore) {
        steps += 1;
        improving = true;
        cur = bestOrdering;
        curScore = bestSc;
        initialDAG = bestDAG;
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
    population.mutate(NUM_MUTATIONS, MUTATION_POWER, offspring, instance);
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
    //std::cout << "Fitness: " << population.getAverageFitness() << std::endl;
    SearchResult curBest = population.getSpecimen(0);
    Types::Score curScore = curBest.getScore();
    if (curScore < best.getScore()) {
      std::cout << "Time: " << rr.check() <<  " The best score at this iteration is: " << curScore << std::endl;
      rr.record(curBest.getScore(), curBest.getOrdering());
      best = curBest;
    }
    numGenerations++;


    if (optimalScore < initialSc) {
      walkProb = std::max(0.05, walkProb - 0.025);
    } else {
      walkProb = std::min(0.20, walkProb + 0.025);
    }

    std::cout << "Finished generation: " << numGenerations << std::endl;

  } while (numGenerations < 20);
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
  } else if (instance.getFileName().find("water") != std::string::npos) {
    file = std::ifstream("data/mappings/water.mapping");
    outF.open("data/water_results", std::ios_base::app);
  }

  else {
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


void LocalSearch::getScoreOfTrueBN() {
  int n = instance.getN(), m = instance.getM();

  
  std::vector< std::vector<int> > trueBN(n);
  std::vector< int > trueParents(n);


  const int arr0[] = {};
  const int arr1[] = {0};
  const int arr2[] = {};
  const int arr3[] = {2};
  const int arr4[] = {2};
  const int arr5[] = {1, 3};
  const int arr6[] = {5};
  const int arr7[] = {4, 5};

  trueBN[0] = std::vector<int>(arr0, arr0 + sizeof(arr0) / sizeof(int));
  trueBN[1] = std::vector<int>(arr1, arr1 + sizeof(arr1) / sizeof(int));
  trueBN[2] = std::vector<int>(arr2, arr2 + sizeof(arr2) / sizeof(int));
  trueBN[3] = std::vector<int>(arr3, arr3 + sizeof(arr3) / sizeof(int));
  trueBN[4] = std::vector<int>(arr4, arr4 + sizeof(arr4) / sizeof(int));
  trueBN[5] = std::vector<int>(arr5, arr5 + sizeof(arr5) / sizeof(int));
  trueBN[6] = std::vector<int>(arr6, arr6 + sizeof(arr6) / sizeof(int));
  trueBN[7] = std::vector<int>(arr7, arr7 + sizeof(arr7) / sizeof(int));
/*
  // sachs
  const int arr8[] = {};
  const int arr9[] = {};
  const int arr6[] = {9};
  const int arr7[] = {8};
  const int arr2[] = {7, 8};
  const int arr4[] = {7, 8};
  const int arr5[] = {6, 9};
  const int arr10[] = {7, 8};
  const int arr3[] = {7, 8, 10};
  const int arr1[] = {3, 7};
  const int arr0[] = {1, 7};
*/
/*
  // child
  const int arr0[] =  {};
  const int arr11[] =  {0};
  const int arr14[] =  {11};
  const int arr15[] =  {11};
  const int arr16[] =  {11};
  const int arr17[] =  {11};
  const int arr18[] =  {11};
  const int arr19[] =  {11};
  const int arr1[] =  {15, 16};
  const int arr2[] =  {16, 17};
  const int arr3[] =  {17};
  const int arr4[] =  {17, 18};
  const int arr5[] =  {17, 19};
  const int arr6[] =  {14};
  const int arr13[] =  {11, 19};
  const int arr7[] =  {1, 2};
  const int arr8[] =  {2};
  const int arr9[] =  {3};
  const int arr10[] =  {4};
  const int arr12[] =  {5};
*/

/*
  // insurance
    const int arr1[] = {};
    const int arr10[] = {};
    const int arr2[] = {1};
    const int arr0[] = {1, 2};
    const int arr3[] = {1, 2};
    const int arr21[] = {2};
    const int arr4[] = {2, 3};
    const int arr8[] = {2, 3};
    const int arr13[] = {1, 3};
    const int arr17[] = {2, 3};
    const int arr18[] = {2, 3};
    const int arr6[] = {4, 8};
    const int arr11[] = {4, 8};
    const int arr12[] = {1, 13};
    const int arr16[] = {4, 8, 10};
    const int arr24[] = {4, 8};
    const int arr9[] = {3, 12};
    const int arr15[] = {16, 17, 18};
    const int arr23[] = {6, 24};
    const int arr26[] = {3, 12};
    const int arr7[] = {9, 10, 11};
    const int arr5[] = {6, 7};
    const int arr20[] = {6, 7};
    const int arr22[] = {1, 7, 23};
    const int arr25[] = {7};
    const int arr14[] = {5, 15, 16};
    const int arr19[] = {14, 20};
/*
  // water
  const int arr0[] = {};
  const int arr1[] = {};
  const int arr2[] = {};
  const int arr3[] = {};
  const int arr4[] = {};
  const int arr5[] = {};
  const int arr6[] = {};
  const int arr7[] = {};
  const int arr8[] = {0};
  const int arr9[] = {1};
  const int arr10[] = {0,1,2,4,5 };
  const int arr11[] = {1,3,6 };
  const int arr12[] = {2,4,7};
  const int arr13[] = {2,5,7 };
  const int arr14[] = {3, 6 };
  const int arr15[] = {4,5,6,7 };
  const int arr16[] = {8 };
  const int arr17[] = {9 };
  const int arr18[] = {8,9,10,12,13};
  const int arr19[] = {9,11,14 };
  const int arr20[] = {10, 12,15 };
  const int arr21[] = {10, 13, 15 };
  const int arr22[] = {11, 14 };
  const int arr23[] = {12, 13, 14, 15 };
  const int arr24[] = {16 };
  const int arr25[] = {17 };
  const int arr26[] = {16, 17, 18, 20, 21 };
  const int arr27[] = {17, 19, 22 };
  const int arr28[] = {18, 20, 23 };
  const int arr29[] = {18, 21, 23 };
  const int arr30[] = {19, 22 };
  const int arr31[] = {20, 21, 22, 23};

  trueBN[0] = std::vector<int>(arr0, arr0 + sizeof(arr0) / sizeof(int));
  trueBN[1] = std::vector<int>(arr1, arr1 + sizeof(arr1) / sizeof(int));
  trueBN[2] = std::vector<int>(arr2, arr2 + sizeof(arr2) / sizeof(int));
  trueBN[3] = std::vector<int>(arr3, arr3 + sizeof(arr3) / sizeof(int));
  trueBN[4] = std::vector<int>(arr4, arr4 + sizeof(arr4) / sizeof(int));
  trueBN[5] = std::vector<int>(arr5, arr5 + sizeof(arr5) / sizeof(int));
  trueBN[6] = std::vector<int>(arr6, arr6 + sizeof(arr6) / sizeof(int));
  trueBN[7] = std::vector<int>(arr7, arr7 + sizeof(arr7) / sizeof(int));
  trueBN[8] = std::vector<int>(arr8, arr8 + sizeof(arr8) / sizeof(int));
  trueBN[9] = std::vector<int>(arr9, arr9 + sizeof(arr9) / sizeof(int));
  trueBN[10] = std::vector<int>(arr10, arr10 + sizeof(arr10) / sizeof(int));
  trueBN[11] = std::vector<int>(arr11, arr11 + sizeof(arr11) / sizeof(int));
  trueBN[12] = std::vector<int>(arr12, arr12 + sizeof(arr12) / sizeof(int));
  trueBN[13] = std::vector<int>(arr13, arr13 + sizeof(arr13) / sizeof(int));
  trueBN[14] = std::vector<int>(arr14, arr14 + sizeof(arr14) / sizeof(int));
  trueBN[15] = std::vector<int>(arr15, arr15 + sizeof(arr15) / sizeof(int));
  trueBN[16] = std::vector<int>(arr16, arr16 + sizeof(arr16) / sizeof(int));
  trueBN[17] = std::vector<int>(arr17, arr17 + sizeof(arr17) / sizeof(int));
  trueBN[18] = std::vector<int>(arr18, arr18 + sizeof(arr18) / sizeof(int));
  trueBN[19] = std::vector<int>(arr19, arr19 + sizeof(arr19) / sizeof(int));
  trueBN[20] = std::vector<int>(arr20, arr20 + sizeof(arr20) / sizeof(int));
  trueBN[21] = std::vector<int>(arr21, arr21 + sizeof(arr21) / sizeof(int));
  trueBN[22] = std::vector<int>(arr22, arr22 + sizeof(arr22) / sizeof(int));
  trueBN[23] = std::vector<int>(arr23, arr23 + sizeof(arr23) / sizeof(int));
  trueBN[24] = std::vector<int>(arr24, arr24 + sizeof(arr24) / sizeof(int));
  trueBN[25] = std::vector<int>(arr25, arr25 + sizeof(arr25) / sizeof(int));
  trueBN[26] = std::vector<int>(arr26, arr26 + sizeof(arr26) / sizeof(int));
  trueBN[27] = std::vector<int>(arr27, arr27 + sizeof(arr27) / sizeof(int));
  trueBN[28] = std::vector<int>(arr28, arr28 + sizeof(arr28) / sizeof(int));
  trueBN[29] = std::vector<int>(arr29, arr29 + sizeof(arr29) / sizeof(int));
  trueBN[30] = std::vector<int>(arr30, arr30 + sizeof(arr30) / sizeof(int));
  trueBN[31] = std::vector<int>(arr31, arr31 + sizeof(arr31) / sizeof(int));
*/

  Types::Score totalScore = 0;
  for (int i = 0; i < n; i++) {
    const Variable &var = instance.getVar(i);

    bool foundParent = false;
    for (int j = 0; j < var.numParents(); j++) {
      const ParentSet &parSet = var.getParent(j);
      const std::vector<int> &ps = parSet.getParentsVec();

      if (ps.size() == trueBN[i].size() && std::equal(ps.begin(), ps.end(), trueBN[i].begin())) {

        std::cout << "Node " << i << ": ";
        for (int k = 0; k < trueBN[i].size(); k++) {
          std::cout << trueBN[i][k] << " ";
        }
        std::cout << std::endl;
        totalScore += parSet.getScore();
        foundParent = true;
        trueParents[i] = j;
        break;
      }
    }

    if (!foundParent) {
      std::cout << "No parent found for: " << i << std::endl;
    }
    assert(foundParent);
  }

  std::cout << "Score of true BN: " << totalScore << std::endl;

  bool ancestralValid = (numConstraintsSatisfied(trueParents) == m);
  if (ancestralValid) {
    std::cout << "Ancestral constraints check: Good" << std::endl;
  } else {
    std::cout << "Ancestral constraints check: Bad" << std::endl;
  }

  std::cout << "Num constraints satisfied: " << numConstraintsSatisfied(trueParents) << std::endl;
}
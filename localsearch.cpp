#include "localsearch.h"
#include "debug.h"
#include <numeric>
#include <utility>
#include <string>
#include <deque>
#include <fstream>
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

const Types::Score INF = 223372036854775807LL;
const Types::Score PENALTY = 100000000000000LL;

int LocalSearch::climbs = 0;

const ParentSet &LocalSearch::bestParent(const Ordering &ordering, const Types::Bitset pred, int idx) {
  int current = ordering.get(idx);
  const Variable &v = instance.getVar(current);
  return bestParentVar(pred, v);
}

const ParentSet &LocalSearch::bestParentVar(const Types::Bitset pred, const Variable &v) {
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


// CAN BE OPTIMIZED
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

bool LocalSearch::hasDipathWithMemo(const std::vector<int> &parents, int x, int y, int **memo) {
  if (x == y) {
    return true;
  }

  if (memo[x][y] == 1) {
    return true;
  } else if (memo[x][y] == -1) {
    return false;
  }

  const Variable &yVar = instance.getVar(y);
  const ParentSet &p = yVar.getParent(parents[y]);
  const std::vector<int> &pars = p.getParentsVec();

  for (int i=0; i < pars.size(); i++) {
    if (hasDipath(parents, x, pars[i])) {
      memo[x][y] = 1;
      return true;
    }
  }

  memo[x][y] = -1;
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

int LocalSearch::numConstraintsSatisfied(const std::vector<int> &newParents, bool **ancestor, bool **descendant, bool *satisfied, int cur) {
  int n = instance.getN(), m = instance.getM(), count = 0, par = newParents[cur];

  const ParentSet &p = instance.getVar(cur).getParent(par); 

  for (int i = 0; i < m; i++) {

    // Check if a satisfied constraint is now unsatisfied
    if (satisfied[i]) {
      if (ancestor[cur][i] && descendant[cur][i]) {
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
        } else if (hasDipath(newParents, instance.getAncestral(i).first, instance.getAncestral(i).second)) {
          count++;
        }
      } else {
        count++;
      }
    } 

    // Check if an unsatisfied constraint is now satisfied.
    else {
      if (descendant[cur][i]) {
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

void LocalSearch::computeAncestralGraph(const std::vector<int> &parents, bool **ancestor, bool **descendant, bool *satisfied) {
  int n = instance.getN(), m = instance.getM();

  for (int j = 0; j < m; j++) {
    satisfied[j] = false;
  }

  int **memo = new int*[n];

  for (int i = 0; i < n; i++) {
    memo[i] = new int[n](); // Value initialized to 0.
  }

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      ancestor[i][j] = hasDipathWithMemo(parents, instance.getAncestral(j).first, i, memo);
      descendant[i][j] = hasDipathWithMemo(parents, i, instance.getAncestral(j).second, memo);
      if (ancestor[i][j] && descendant[i][j]) {
        satisfied[j] = true;
      }
    }
  }

  for (int i = 0; i < n; i++) {
    delete[] memo[i];
  }
  delete[] memo;
}


Types::Score LocalSearch::modifiedDAGScore(const Ordering &ordering, const std::vector<int> &parents) {
  int n = instance.getN(), m = instance.getM();
  std::vector<int> bestGraph(parents);
  computeAncestralGraph(bestGraph, ancestor, descendant, satisfied);
  int curNumSat = 0;

  for (int i = 0; i < m; i++) {
    curNumSat += (satisfied[i]);
  }

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
  

  

  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }

  int climbs = 0;
  while(true) {
    climbs++;

    // Consider all feasible parent sets

    int bestVar, bestParent;

    bool foundImproving = false;

    for (auto it = allParents.begin(); it != allParents.end(); ++it) {
      int cur = it->first, par = it->second;
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      Types::Score sc = p.getScore();
      int oldPar = bestGraph[cur];

      if (curNumSat == m && sc >= var.getParent(oldPar).getScore()) {
        continue;
      }

      if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
        bestGraph[cur] = par;
        int numSat = numConstraintsSatisfied(bestGraph, ancestor, descendant, satisfied, cur);
        bestGraph[cur] = oldPar;
        if (numSat > curNumSat || (numSat == curNumSat && sc < var.getParent(oldPar).getScore())) {
          bestVar = cur;
          bestParent = par;
          bestGraph[bestVar] = bestParent;
          curNumSat = numSat;
          foundImproving = true;

          if ((double)rand() / RAND_MAX < 0.01) {
            // Transpose.
            if (it != allParents.begin()) {
              auto it2 = it;
              --it2;
              std::iter_swap(it, it2);

              //assert(!(it->first == cur && it->second == par));
              //assert(it2->first == cur);
            }
          }

          break;
        }
      }
    }

    if (!foundImproving) {
      break;
    }
    computeAncestralGraph(bestGraph, ancestor, descendant, satisfied);
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


// GREEDY HILL-CLIMBING METHOD (First Improvement)
Types::Score LocalSearch::modifiedDAGScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) {
  int n = instance.getN(), m = instance.getM();
  std::vector<int> bestGraph(parents);
  computeAncestralGraph(bestGraph, ancestor, descendant, satisfied);
  int curNumSat = 0;

  for (int i = 0; i < m; i++) {
    curNumSat += (satisfied[i]);
  }

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
  Types::Bitset curPred(n, 0);

  for (int i = 0; i < n; i++) {
    pred[ordering.get(i)] = curPred;
    curPred[ordering.get(i)] = 1;
  }

  int climbs = 0;
  while(true) {
    climbs++;

    // Consider all feasible parent sets

    int bestVar, bestParent;

    bool foundImproving = false;

    for (auto it = allParents.begin(); it != allParents.end(); ++it) {
      int cur = it->first, par = it->second;
      const Variable &var = instance.getVar(cur);
      const ParentSet &p = var.getParent(par);
      Types::Score sc = p.getScore();
      int oldPar = bestGraph[cur];

      if (curNumSat == m && sc >= var.getParent(oldPar).getScore()) {
        continue;
      }

      if (par != bestGraph[cur] && p.subsetOf(pred[cur])) {
        bestGraph[cur] = par;
        int numSat = numConstraintsSatisfied(bestGraph, ancestor, descendant, satisfied, cur);
        bestGraph[cur] = oldPar;
        if (numSat > curNumSat || (numSat == curNumSat && sc < var.getParent(oldPar).getScore())) {
          bestVar = cur;
          bestParent = par;
          bestGraph[bestVar] = bestParent;
          curNumSat = numSat;
          foundImproving = true;

          if ((double)rand() / RAND_MAX < 0.01) {
            // Transpose.
            if (it != allParents.begin()) {
              auto it2 = it;
              --it2;
              std::iter_swap(it, it2);

              //assert(!(it->first == cur && it->second == par));
              //assert(it2->first == cur);
            }
          }
          break;
        }
      }
    }

    if (!foundImproving) {
      break;
    }
    computeAncestralGraph(bestGraph, ancestor, descendant, satisfied);
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

  instance.sortAllParents();

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

  std::cout << "Num constraints satisfied: " << numConstraintsSatisfied(parents) << std::endl;

  std::cout << "Climbs: " << climbs << std::endl;


  printModelString(parents);
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


void LocalSearch::printModelString(const std::vector<int> &parents) {
  int n = instance.getN();
  std::ifstream file;

  if (instance.getFileName().find("asia") != std::string::npos) {
    file = std::ifstream("data/mappings/asia.mapping");
  } else if (instance.getFileName().find("alarm") != std::string::npos) {
    file = std::ifstream("data/mappings/alarm.mapping");
  } else if (instance.getFileName().find("hailfinder") != std::string::npos) {
    file = std::ifstream("data/mappings/hailfinder.mapping");
  } else if (instance.getFileName().find("child") != std::string::npos) {
    file = std::ifstream("data/mappings/child.mapping");
  } else {
    std::cout << "No suitable mapping found!" << std::endl;
    exit(0);
  }


  std::vector<std::string> vars(n);

  for (int i=0; i < n; i++) {
    getline(file, vars[i]);
  }


  // Print the network.

  std::cout << "Model string: ";

  for (int i = 0; i < n; i++) {
    std::cout << "[" + vars[i];

    const Variable &varI = instance.getVar(i);
    const ParentSet &par = varI.getParent(parents[i]);
    const std::vector<int> parVec = par.getParentsVec();

    if (parVec.size() != 0) {
      std::cout << "|";
    }

    for (int j = 0; j < parVec.size(); j++) {
      std::cout << vars[parVec[j]];
      if (j < parVec.size() - 1) {
        std::cout << ":";
      }
    }

    std::cout << "]";
  }

  std::cout << std::endl;
}
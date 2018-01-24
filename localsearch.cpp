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
Types::Score LocalSearch::getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const {

  if (!consistentWithAncestral(ordering)) {
    return 223372036854775807LL;
  }

  int n = instance.getN();
  Types::Bitset pred(n, 0);
  Types::Score score = 0;
  for (int i = 0; i < n; i++) {
    const ParentSet &p = bestParent(ordering, pred, i);
    score += p.getScore();
    parents[ordering.get(i)] = p.getId();
    scores[ordering.get(i)] = p.getScore();
    pred[ordering.get(i)] = 1;
  }

  modifiedDAGScore(ordering, parents, score);

  return score;
}


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


// i-th constraint
// if des is true, fill the descendants property, else fill the ancestors property
void LocalSearch::dfs(std::vector<int> *G, int node, int i, bool des, int *visited) const {
  Variable &var = instance.getVar(node);

  if (visited[node]) {
    return;
  } else {
    visited[node] = true;
  }

  if (des) {
    var.addDescendant(i);
  } else {
    var.addAncestor(i);
  }

  for (int j = 0; j < G[node].size(); j++) {
    dfs(G, G[node][j], i, des, visited);
  }
}

// Given a set of constraints, a (feasible) parent set p and it's corresponding child node,
// return the set of constraints satisfied by replacing the current parent set of var with p.
Types::Bitset LocalSearch::csat(const std::vector<Ancestral> &constraints, const ParentSet &p, const ParentSet &oldP, const Variable &var) const {
  Types::Bitset sat(constraints.size(), 0);

  for (int w=0; w < instance.getN(); w++) {
    if (p.hasElement(w) && !oldP.hasElement(w)) {
      const std::vector<int> &anc = instance.getVar(w).getAncestors();
      for (int j = 0; j < anc.size(); j++) {
        if (var.hasDescendant(anc[j])) {
          sat[anc[j]] = 1;
        }
      }
    }
  }

  return sat;
}


Types::Score LocalSearch::modifiedDAGScore(const Ordering &ordering, std::vector<int> &parents, Types::Score score) const {
  int n = instance.getN(), m = instance.getM();

  // Compute the set of unsatisfied ancestral constraints.
  std::vector<Ancestral> constraints;
  std::vector<int> X, Y; // X is the set of nodes on the LHS of a constraint
                         // Y is the set of nodes on the RHS of a constraint

  for (int i=0; i < m; i++) {
    const Ancestral &cons = instance.getAncestral(i);
    if (!hasDipath(parents, cons.first, cons.second)) {
      constraints.push_back(cons);
      std::cout << "Constraint not satisfied: " << cons.first << " " << cons.second << std::endl;
      X.push_back(cons.first);
      Y.push_back(cons.second);
    }
  }

  m = constraints.size();

  if (m == 0) {
    return score;
  }


  // Compute the ancestors/descendants properties for each vertex 
  std::vector<int> graph[n], rgraph[n]; // The arcs and reverse arcs of G

  for (int i=0; i < n; i++) {
    Variable &var = instance.getVar(i);
    const ParentSet &p =  var.getParent(parents[i]);
    const std::vector<int> &pars = p.getParentsVec();
    for (int j=0; j < pars.size(); j++) {
      graph[pars[j]].push_back(i);
      rgraph[i].push_back(pars[j]);
    }

    var.clearAncestry();
  }

  int visited[n];
  for (int i=0; i < n; i++) visited[i] = false;

  for (int i=0; i < m; i++) {
    dfs(graph, X[i], i, false, visited);
  }

  for (int i=0; i < n; i++) visited[i] = false;
  for (int i=0; i < m; i++) {
    dfs(rgraph, Y[i], i, true, visited);
  }


  // Find all feasible parent sets and their associated scores.
  // A parent set for v is *feasible* iff p respects the ordering and is a 
  // strict superset of the current parent set for v.
  Types::Bitset pred(n, 0);

  for (int i=0; i < n; i++) {
    int cur = ordering.get(i);
    const Variable &var = instance.getVar(cur);

    if (var.numDescendants() != 0) {
      const ParentSet &curPar = var.getParent(parents[cur]);
      Types::Bitset cp(n, 0);

      assert(curPar.subsetOf(pred));

      for (int j=0; j < curPar.size(); j++) {
        cp[curPar.getParentsVec()[j]] = 1;
      }

      for (int j=0; j < var.numParents(); j++) {
        const ParentSet &p = var.getParent(j);

        if (p.size() > curPar.size() && p.supersetOf(cp) && p.subsetOf(pred)) {
          // p is feasible
          std::cout << "FEASIBLE" << std::endl;
          // Compute the constraints satisfied by p
          Types::Bitset satisfied = csat(constraints, p, curPar, var);

          std::cout << satisfied << " " << " Score: " << p.getScore() - curPar.getScore() << std::endl;
        } 
      }
    }
    pred[ordering.get(i)] = 1;
  }

  return score;

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

// A particularily efficient implementation of recalculating swap score
SwapResult LocalSearch::findBestScoreSwap(
const Ordering &ordering, int i, const std::vector<int> &parents, Types::Bitset &pred)
{
  int n = instance.getN();
  int j = i + 1;
  Types::Score curScore = 0;
  int aVarId = ordering.get(i);
  int bVarId = ordering.get(j);
  const Variable &a = instance.getVar(aVarId);
  const Variable &b = instance.getVar(bVarId);
  const ParentSet &b_0 = b.getParent(parents[bVarId]);
  const ParentSet &a_0 = a.getParent(parents[aVarId]);
  int aNewParentSetId = -1;
  int bNewParentSetId = -1;
  Types::Score newBScore = -1LL;
  Types::Score newAScore = -1LL;
  if (b_0.hasElement(aVarId)) {
    
    const ParentSet &bNew = bestParentVar(pred, b);
    newBScore = bNew.getScore();
    bNewParentSetId = bNew.getId();
    //DBG("Collision detected, found new parent set " << bNew.getId() << " for " << bVarId);
  } else {
    //DBG("No collision detected, reusing old parent set " << b_0.getId() << " for " << bVarId);
    newBScore = b_0.getScore();
    bNewParentSetId = b_0.getId();
  }
  pred[bVarId] = 1;

  if (a_0.getId() != 0) {
    const ParentSet *aNew = bestParentVarWithParent(pred, a, b, a_0.getScore());
    if (aNew == NULL || aNew->getScore() > a_0.getScore()) {
      //DBG("No new parent sets or none improving for " << aVarId);
      newAScore = a_0.getScore();
      aNewParentSetId = a_0.getId();
    } else {
      //DBG("Found improving parent set " << aNew->getId() << " including " << bVarId << " for " << aVarId);
      newAScore = aNew->getScore();
      aNewParentSetId = aNew->getId();
    }
  } else {
    //DBG("Optimal parent for " << aVarId << " already found.");
    newAScore = a_0.getScore();
    aNewParentSetId = a_0.getId();
  }
  //DBG("A Var ID");
  pred[bVarId] = 0;

  return SwapResult(newBScore, newAScore, bNewParentSetId, aNewParentSetId);
}

FastPivotResult LocalSearch::getInsertScore(Ordering o, int pivot, int dest, Types::Score initScore, std::vector<int> parents, std::vector<Types::Score> scores) {
  int n = instance.getN();
  Types::Score curScore = initScore;
  Types::Bitset pred = getPred(o, pivot);
  if (pivot < dest) {
    for (int i = pivot; i + 1 < dest; i++) {
      Types::Score oldScore = scores[o.get(i)] + scores[o.get(i+1)];
      SwapResult sr = findBestScoreSwap(o, i, parents, pred);
      o.swap(i, i+1);
      pred[o.get(i)] = 1;
      std::pair<Types::Score, Types::Score> newParentScores = sr.getScores();
      scores[o.get(i)] = newParentScores.first;
      scores[o.get(i + 1)] = newParentScores.second;
      std::pair<int, int> newParentSets = sr.getParentSets();
      parents[o.get(i)] = newParentSets.first;
      parents[o.get(i + 1)] = newParentSets.second;
      Types::Score newScore = sr.getScore();
      curScore += newScore - oldScore;
    }
  } else {
    for (int i = pivot - 1; i >= dest; i--) {
      pred[o.get(i)] = 0;
      Types::Score oldScore = scores[o.get(i)] + scores[o.get(i+1)];
      SwapResult sr = findBestScoreSwap(o, i, parents, pred);
      o.swap(i, i+1);
      std::pair<Types::Score, Types::Score> newParentScores = sr.getScores();
      scores[o.get(i)] = newParentScores.first;
      scores[o.get(i + 1)] = newParentScores.second;
      std::pair<int, int> newParentSets = sr.getParentSets();
      parents[o.get(i)] = newParentSets.first;
      parents[o.get(i + 1)] = newParentSets.second;
      Types::Score newScore = sr.getScore();
      curScore += newScore - oldScore;
    }
  }
  return FastPivotResult(curScore, -1, o, parents, scores);
}


// This code....
FastPivotResult LocalSearch::getBestInsertFast(const Ordering &ordering, int pivot, Types::Score initScore, const std::vector<int> &parents, const std::vector<Types::Score> &scores) {
  //DBG("START");
  int n = instance.getN();
  Types::Bitset forwardPred = getPred(ordering, pivot);
  Types::Bitset backwardPred(forwardPred);
  /*
  if (pivot > 0) {
    backwardPred[ordering.get(pivot - 1)] = 0;
  }
  */
  std::vector<int> forwardParents(parents);
  std::vector<int> backwardParents(parents);
  std::vector<Types::Score> forwardScores(scores);
  std::vector<Types::Score> backwardScores(scores);
  std::vector<std::pair<Types::Score, int>> firstScore;
  firstScore.resize(n, std::pair<Types::Score, int>(-1, -1));
  Types::Score curScore = initScore;
  Types::Score bestScore = initScore;
  int bestPivot = -1;
  Ordering forwardModified(ordering);
  Ordering backwardModified(ordering);
  //DBG("CURRENT ORDERING: " << ordering << " PIVOT: " << pivot);
  //DBG("FORWARD");
  for (int i = pivot; i + 1 < n; i++) {
    //DBG("ON PIVOT " << i);
    //DBG("Current Pred: " << forwardPred);
    for (int i = 0; i < n; i++) {
      //DBG("Var idx: " << i << " Var: " << forwardModified.get(i) << " Parent: " << forwardParents[forwardModified.get(i)] << " Scores: " << forwardScores[forwardModified.get(i)]);
    }
    SwapResult sr = findBestScoreSwap(forwardModified, i, forwardParents, forwardPred);
    Types::Score oldScore = forwardScores[forwardModified.get(i)] + forwardScores[forwardModified.get(i+1)];
    //DBG("Var(i): " << forwardModified.get(i) <<" Var(i+1): " << forwardModified.get(i+1) << " StoredScore(i): " << forwardScores[forwardModified.get(i)] << " StoreScore(i+1) " << forwardScores[forwardModified.get(i+1)]);
    //DBG("Swap Result: " << sr);
    forwardModified.swap(i, i+1);
    forwardPred[forwardModified.get(i)] = 1;
    std::pair<Types::Score, Types::Score> newParentScores = sr.getScores();
    //DBG("new first " << newParentScores.first << " new second " << newParentScores.second);
    forwardScores[forwardModified.get(i)] = newParentScores.first;
    forwardScores[forwardModified.get(i + 1)] = newParentScores.second;
    std::pair<int, int> newParentSets = sr.getParentSets();
    forwardParents[forwardModified.get(i)] = newParentSets.first;
    forwardParents[forwardModified.get(i + 1)] = newParentSets.second;
    firstScore[i + 1] = std::make_pair(newParentScores.second, newParentSets.second);
    
    Types::Score newScore = sr.getScore();
    //DBG("OldScore: " << oldScore << " New Scores: " << newScore);
    curScore += newScore - oldScore;
    if (curScore < bestScore) {
      bestScore = curScore;
      bestPivot = i + 1;
    }
    //DBG("New Score: " << curScore);
  }
  curScore = initScore;
  //DBG("BACKWARD");
  for (int i = pivot - 1; i >= 0; i--) {
    //DBG("ON PIVOT " << i);
    //DBG("Current Pred: " << backwardPred);
    for (int i = 0; i < n; i++) {
      //DBG("Var idx: " << i << " Var: " << backwardModified.get(i) << " Parent: " << backwardParents[backwardModified.get(i)] << " Scores: " << backwardScores[backwardModified.get(i)]);
    }
    backwardPred[backwardModified.get(i)] = 0;
    SwapResult sr = findBestScoreSwap(backwardModified, i, backwardParents, backwardPred);
    Types::Score oldScore = backwardScores[backwardModified.get(i)] + backwardScores[backwardModified.get(i+1)];
    backwardModified.swap(i, i+1);
    /*
    if (i - 1 >= 0) {
      backwardPred[backwardModified.get(i - 1)] = 0;
    }*/
    std::pair<Types::Score, Types::Score> newParentScores = sr.getScores();
    backwardScores[backwardModified.get(i)] = newParentScores.first;
    backwardScores[backwardModified.get(i + 1)] = newParentScores.second;
    std::pair<int, int> newParentSets = sr.getParentSets();
    backwardParents[backwardModified.get(i)] = newParentSets.first;
    backwardParents[backwardModified.get(i + 1)] = newParentSets.second;
    firstScore[i] = std::make_pair(newParentScores.first, newParentSets.first);
    Types::Score newScore = sr.getScore();
    curScore += newScore - oldScore;
    
    if (curScore < bestScore) {
      bestScore = curScore;
      bestPivot = i;
      //DBG("Found New Best " << bestScore);
    }
  }

  
  std::vector<int> newParents(parents);
  std::vector<Types::Score> newScores(scores);
  Ordering modified(ordering);
  if (bestPivot != -1 ) {
    modified.insert(pivot, bestPivot);
    if (bestPivot < pivot) {
      for (int k = pivot; k > bestPivot; k--) {
        int varId = modified.get(k);
        newParents[varId] = backwardParents[varId];
        newScores[varId] = backwardScores[varId];
      }
      int pivotVarId = modified.get(bestPivot);
      newParents[pivotVarId] = firstScore[bestPivot].second;
      newScores[pivotVarId] = firstScore[bestPivot].first;
    } else if (bestPivot > pivot) {
      for (int k = pivot; k < bestPivot; k++) {
        int varId = modified.get(k);
        newParents[varId] = forwardParents[varId];
        newScores[varId] = forwardScores[varId];
      }
      int pivotVarId = modified.get(bestPivot);
      newParents[pivotVarId] = firstScore[bestPivot].second;
      newScores[pivotVarId] = firstScore[bestPivot].first;
    }
    return FastPivotResult(bestScore, bestPivot, modified, newParents, newScores);
  } else {
    return FastPivotResult(bestScore, bestPivot, modified, newParents, newScores);
  }
}



PivotResult LocalSearch::getBestInsert(const Ordering &ordering, int pivot, Types::Score initScore) const {
  Types::Bitset forwardPred = getPred(ordering, pivot);
  Types::Bitset backwardPred(forwardPred);
  Types::Score curScore = initScore;
  Types::Score bestScore = Types::SCORE_MAX;
  std::vector<int> best;
  int bestPivot = -1;
  Ordering modified(ordering);
  int n = instance.getN();
  int i = pivot;
  int j = i + 1;
  for (; j < n; i++, j++) {
    //DBG(modified << " " << forwardPred << " " << curScore);
    Types::Score oldIScore = bestParent(modified, forwardPred, i).getScore();
    forwardPred[modified.get(i)] = 1;
    //DBG(modified << " " << forwardPred);
    Types::Score oldJScore = bestParent(modified, forwardPred, j).getScore();
    forwardPred[modified.get(i)] = 0;
    Types::Score combinedOldScore = oldIScore + oldJScore;
    //DBG(combinedOldScore);
    modified.swap(i, j);
    const ParentSet &iParent = bestParent(modified, forwardPred, i);
    forwardPred[modified.get(i)] = 1;
    const ParentSet &jParent = bestParent(modified, forwardPred, j);
    Types::Score newIScore = iParent.getScore();
    Types::Score newJScore = jParent.getScore();
    Types::Score combinedNewScore = newIScore + newJScore;
    curScore += combinedNewScore - combinedOldScore;
    //DBG(combinedNewScore);
    //DBG("CUR: " << curScore << " ordering: " << modified);
    if (curScore < bestScore) {
      bestScore = curScore;
      bestPivot = j;
    }
  }
  // TODO: old scores for here
  curScore = initScore;
  modified = ordering;
  i = pivot;
  j = i - 1;
  for (; j >= 0; i--, j--) {
    //DBG(modified << " " << backwardPred << " " << curScore);
    Types::Score oldIScore = bestParent(modified, backwardPred, i).getScore();
    backwardPred[modified.get(j)] = 0;
    //DBG(modified << " " << backwardPred);
    Types::Score oldJScore = bestParent(modified, backwardPred, j).getScore();
    backwardPred[modified.get(j)] = 1;
    Types::Score combinedOldScore = oldIScore + oldJScore;
    //DBG(combinedOldScore);
    modified.swap(i, j);
    backwardPred[modified.get(i)] = 0;
    backwardPred[modified.get(j)] = 1;
    const ParentSet &iParent = bestParent(modified, backwardPred, i);
    backwardPred[modified.get(j)] = 0;
    const ParentSet &jParent = bestParent(modified, backwardPred, j);
    Types::Score newIScore = iParent.getScore();
    Types::Score newJScore = jParent.getScore();
    Types::Score combinedNewScore = newIScore + newJScore;
    curScore += combinedNewScore - combinedOldScore;
    //DBG(combinedNewScore);
    //DBG("CUR: " << curScore << " ordering: " << modified);
    if (curScore < bestScore) {
      bestScore = curScore;
      bestPivot = j;
    }
  }
  
  modified = ordering;
  if (bestPivot != -1) {
    modified.insert(pivot, bestPivot);
  }
  PivotResult ret(bestScore, bestPivot, modified);
  return ret;
}

SearchResult LocalSearch::hillClimb(const Ordering &ordering) {
  bool improving = false;
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  int steps = 0;
  std::vector<int> positions(n);
  Ordering cur(ordering);

  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);
  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur);
  do {
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int s = 0; s < n && !improving; s++) {
      int pivot = positions[s];
      FastPivotResult result = getBestInsertFast(cur, pivot, curScore, parents, scores);
      if (result.getScore() < curScore) {
        steps += 1;
        improving = true;
        cur.insert(pivot, result.getSwapIdx());
        parents = result.getParents();
        scores = result.getScores();
        curScore = result.getScore();
      }
    }
    DBG("Cur Score: " << curScore);
  } while(improving);
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::hillClimb(const Ordering &ordering, float timeLimit, ResultRegister &rr) {
  bool improving = false;
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  int steps = 0;
  std::vector<int> positions(n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);
  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int s = 0; s < n && !improving; s++) {
      int pivot = positions[s];
      FastPivotResult result = getBestInsertFast(cur, pivot, curScore, parents, scores);
      if (result.getScore() < curScore) {
        steps += 1;
        improving = true;
        cur.insert(pivot, result.getSwapIdx());
        parents = result.getParents();
        scores = result.getScores();
        curScore = result.getScore();
      }
    }
    DBG("Cur Score: " << curScore);
    rr.record(curScore, cur);
  } while(improving && (rr.check() <= timeLimit));
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
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  getBestScoreWithParents(o, parents, scores);
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

  std::cout << "Total Score: " << scoreFromScores << " " << scoreFromParents << std::endl;
  std::string validStr = valid ? "Good" : "Bad";
  std::cout << "Validity Check: " << validStr << std::endl;

  modifiedDAGScore(o, parents, scoreFromScores);
}

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

LocalSearch::LocalSearch(const Instance &instance) : instance(instance) { 
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

Types::Score LocalSearch::getBestScore(const Ordering &ordering) const {
  int n = instance.getN();
  Types::Bitset pred(n, 0);
  Types::Score score = 0;
  for (int i = 0; i < n; i++) {
    const ParentSet &p = bestParent(ordering, pred, i);
    score += p.getScore();
    pred[ordering.get(i)] = 1;
  }
  return score;
}

// New code
Types::Score LocalSearch::getBestScoreWithParents(const Ordering &ordering, std::vector<int> &parents, std::vector<Types::Score> &scores) const {
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

SearchResult LocalSearch::simulatedAnnealing(double initTemp, int numSteps, float decay, float timeLimit, Types::Score opt, Neighbours neighbour, ResultRegister &rr) {
  int n = instance.getN();
  Types::Score bestScore = Types::SCORE_MAX;
  rr.set();
  SearchResult best(bestScore, Ordering(n));
  do {
    Ordering o = Ordering::greedyOrdering(instance);
    SearchResult sr(Types::SCORE_MAX, o);
    if (neighbour == Neighbours::INSERT) {
      sr = simulatedAnnealingStepsInsert(o, initTemp, numSteps, decay, timeLimit, rr);
    } else {
      sr = simulatedAnnealingStepsSwap(o, initTemp, numSteps, decay, timeLimit, rr);
    }
    if (sr.getScore() < best.getScore()) {
      best = sr;
    }
  } while (!Util::isOpt(best, opt) && rr.check() < timeLimit);

  return best;
}

SearchResult LocalSearch::simulatedAnnealingStepsSwap(Ordering &o, double initTemp, int maxSteps, float decay, float timeLimit, ResultRegister &rr) {
  int numSteps = 0;
  int n = instance.getN();
  Types::Score curScore = getBestScore(o);
  double temp = initTemp;
  Ordering current(o);
  DBG("Anneal(" << initTemp << ", " << maxSteps << ", " << decay << ")");
  while (numSteps < maxSteps && rr.check() < timeLimit) {
    //DBG(curScore);
    bool accept = false;
    std::pair<int, int> indices = Util::getUniquePair(n);
    int i = indices.first;
    int j = indices.second;
    Ordering inserted(current);
    inserted.swap(i, j);
    if (i > j) {
      std::swap(i, j);
    }
    Types::Score cost_0 = findBestScoreRange(current, i, j);
    Types::Score cost = findBestScoreRange(inserted, i, j);
    Types::Score delta = cost - cost_0;
    if (delta < 0) {
      accept = true;
    } else {
      double pAccept = pow(2.716, (double) -delta / temp);
      double r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
      accept = r <= pAccept;
      if (r <= pAccept) {
        //DBG("acceping worse step (" << i << ", " << j << ") OldScore: " << cost_0 << " New: " << cost << " Paccept: " << pAccept);
      }
    }
    if (accept) {
      current = inserted;
      curScore += cost - cost_0;
    }
    numSteps += 1;
    temp *= decay;
  }
  DBG(getBestScore(current));
  return SearchResult(getBestScore(current), current);
}

SearchResult LocalSearch::simulatedAnnealingStepsInsert(Ordering &o, double initTemp, int maxSteps, float decay, float timeLimit, ResultRegister &rr) {
  int numSteps = 0;
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  Ordering current(o);
  Types::Score curScore = getBestScoreWithParents(current, parents, scores);
  double temp = initTemp;
  DBG("Anneal(" << initTemp << ", " << maxSteps << ", " << decay << ")");
  while (numSteps < maxSteps && rr.check() < timeLimit) {
    //DBG(curScore);
    bool accept = false;
    std::pair<int, int> indices = Util::getUniquePair(n);
    int i = indices.first;
    int j = indices.second;
    FastPivotResult newResult = getInsertScore(current, i, j, curScore, parents, scores);
    Types::Score delta = newResult.getScore()  - curScore;
    if (delta < 0) {
      accept = true;
    } else {
      double pAccept = pow(2.716, (double) -delta / temp);
      double r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
      accept = r <= pAccept;
      if (r <= pAccept) {
        //DBG("acceping worse step (" << i << ", " << j << ") OldScore: " << cost_0 << " New: " << cost << " Paccept: " << pAccept);
      }
    }
    if (accept) {
      current = newResult.getOrdering();
      curScore = newResult.getScore();
      parents = newResult.getParents();
      scores = newResult.getScores();
    }
    numSteps += 1;
    temp *= decay;
  }
  DBG(getBestScore(current));
  return SearchResult(getBestScore(current), current);
}

SearchResult LocalSearch::kollerSearch(Ordering &o, int listSize, float timeLimit, ResultRegister &rr) {
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  int maxNonImprovingSteps = listSize;
  int nonImprovingSteps = 0;
  Ordering current(o);
  Types::Score curScore = getBestScoreWithParents(current, parents, scores);

  Ordering bestSeenOrdering(o);
  Types::Score bestSeenOrderingScore = curScore;
  SwapTabuList stl(listSize);
  //DBG("TEST");
  //while (rr.check() < timeLimit && nonImprovingSteps < maxNonImprovingSteps) { // Out for now
  while (rr.check() < timeLimit && nonImprovingSteps < maxNonImprovingSteps) {
    Types::Bitset pred(n, 0);
    DBG("Cur Score: " << curScore);
    Types::Score bestDelta = Types::SCORE_MAX;
    int bestSwap = -1;
    SwapResult bestSwapResult(Types::SCORE_MAX, Types::SCORE_MAX, -1, -1);
    std::vector<int> plateauMoves;
    std::vector<SwapResult> plateauResults;
    for (int i = 0; i < n - 1; i++) {
      if (stl.contains(current.get(i), current.get(i+1))) {
        continue;
      }
      Types::Score cost_0 = scores[current.get(i)] + scores[current.get(i+1)];
      SwapResult sr = findBestScoreSwap(current, i, parents, pred);
      Types::Score cost = sr.getScore();
      Types::Score delta = cost - cost_0;
      //DBG(sr);
      //DBG("Orig Cost: " << cost_0 << " New: " << cost);
      //DBG("Delta(" << i << "): " << delta);
      //DBG(pred);
      pred[current.get(i)] = 1; //NEW
      if (delta < bestDelta) {
        bestSwap = i;
        bestSwapResult = sr;
        bestDelta = delta;
      }
      if (delta == 0) {
        plateauMoves.push_back(i);
        plateauResults.push_back(sr);
      }
    }
    if (bestSwap != -1) {
      if (bestDelta == 0) {
        int plateauIdx = rand() % plateauMoves.size();
        bestSwap = plateauMoves[plateauIdx];
        bestSwapResult = plateauResults[plateauIdx];
      }
      current.swap(bestSwap, bestSwap + 1);
      curScore += bestDelta;
      stl.add(current.get(bestSwap), current.get(bestSwap + 1));
      if (curScore < bestSeenOrderingScore) {
        rr.record(curScore, current);
        bestSeenOrderingScore = curScore;
        bestSeenOrdering = current;
      }
      std::pair<Types::Score, Types::Score> newParentScores = bestSwapResult.getScores();
      scores[current.get(bestSwap)] = newParentScores.first;
      scores[current.get(bestSwap + 1)] = newParentScores.second;
      std::pair<int, int> newParentSets = bestSwapResult.getParentSets();
      parents[current.get(bestSwap)] = newParentSets.first;
      parents[current.get(bestSwap + 1)] = newParentSets.second;
      
    } else {
      break;
    }
    if (bestDelta >= 0) {
      ++nonImprovingSteps;
    } else {
      nonImprovingSteps = 0;
    }
    //DBG(current);
    //stl.print();
  }
  DBG("DONE");
  return SearchResult(getBestScore(bestSeenOrdering), bestSeenOrdering);
}

SearchResult LocalSearch::kollerSearchV2(Ordering &o, int listSize, float timeLimit, ResultRegister &rr) {
  int n = instance.getN();
  Types::Score curScore = getBestScore(o);
  Ordering current(o);
  int maxNonImprovingSteps = listSize;
  int nonImprovingSteps = 0;
  TabuList tl(listSize);
  Ordering bestSeenOrdering(o);
  Types::Score bestSeenOrderingScore = curScore;
  while (rr.check() < timeLimit && nonImprovingSteps < maxNonImprovingSteps) {
    DBG("Cur Score: " << curScore);
    Types::Score bestDelta = Types::SCORE_MAX;
    int bestSwap = -1;
    for (int i = 0; i < n - 1; i++) {

      Types::Score cost_0 = findBestScoreRange(current, i, i+1);
      Ordering swapped(current);
      if (tl.contains(swapped)) continue;
      swapped.swap(i, i+1);
      Types::Score cost = findBestScoreRange(swapped, i, i+1);
      Types::Score delta = cost - cost_0;
      //DBG("Delta(" << i << "): " << delta);
      if (delta < bestDelta) {
        bestSwap = i;
        bestDelta = delta;
      }
    }
    if (bestSwap != -1) {
      current.swap(bestSwap, bestSwap + 1);
      tl.add(current);
      curScore += bestDelta;
      if (curScore < bestSeenOrderingScore) {
        bestSeenOrderingScore = curScore;
        bestSeenOrdering = current;
      }
    }
    if (bestDelta >= 0) {
      ++nonImprovingSteps;
    } else {
      nonImprovingSteps = 0;
    }
    //DBG(current);
    //stl.print();
  }
  return SearchResult(getBestScore(bestSeenOrdering), bestSeenOrdering);
}
SearchResult LocalSearch::kollerSearchRestarts(int listSize, float timeLimit, Types::Score opt, ResultRegister &rr) {
   int n = instance.getN();
   Types::Score bestScore = Types::SCORE_MAX;
   SearchResult best(bestScore, Ordering(n));
   do {
     Ordering o = Ordering::randomOrdering(instance);
     SearchResult cur = kollerSearch(o, listSize, timeLimit, rr);
     DBG(cur);
     if (cur.getScore() < best.getScore()) {
       best = cur;
     }
   } while (!Util::isOpt(best, opt) && rr.check() < timeLimit);
   return best; 
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

SearchResult LocalSearch::tabuSearch(const Ordering &ordering, float timeLimit, int listSize, int softThreshold, ResultRegister &rr)  {
  int stepsSinceImprovement = 0;
  int n = instance.getN();
  int steps = 0;
  std::vector<int> positions(n);
  MoveTabuList tabuList(listSize, n);
  Types::Score curScore = getBestScore(ordering);
  Ordering cur(ordering);
  std::iota(positions.begin(), positions.end(), 0);
  Types::Score bestSeenScore = Types::SCORE_MAX;
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    std::random_shuffle(positions.begin(), positions.end());
    Types::Score bestScore = Types::SCORE_MAX;
    int bestPivot = -1;
    int bestLocation = -1;
    for (int s = 0; s < n; s++) {
      int pivot = positions[s];
      PivotResult result = getBestInsert(cur, pivot, curScore);
      if (result.getScore() != curScore && result.getScore() < bestScore && !tabuList.contains(cur.get(pivot))) {
        bestScore = result.getScore();
        bestPivot = pivot;
        bestLocation = result.getSwapIdx();
      }
    }
    if (bestPivot != -1) {
      int movingNum = cur.get(bestPivot);
      cur.insert(bestPivot, bestLocation);
      tabuList.add(movingNum, bestLocation);
      if (bestScore < bestSeenScore) {
        stepsSinceImprovement = 0;
        bestSeenScore = bestScore;
      } else {
        stepsSinceImprovement += 1;
      }
      curScore = bestScore;
    } else {
      break;
    }
    DBG("Cur Score: " << curScore);
  } while(stepsSinceImprovement < softThreshold && rr.check() <= timeLimit);
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

 SearchResult LocalSearch::tabuSearchWithNRestarts(float timeLimit, int listSize, int softThreshold, ResultRegister &rr, Types::Score opt) {
   int n = instance.getN();
   Types::Score bestScore = Types::SCORE_MAX;
   SearchResult best(bestScore, Ordering(n));
   do {
     Ordering o = Ordering::greedyOrdering(instance);
     SearchResult cur = tabuSearch(o, timeLimit, listSize, softThreshold, rr);
     if (cur.getScore() < best.getScore()) {
       best = cur;
     }
   } while (!Util::isOpt(best, opt) && rr.check() < timeLimit);
   return best; 
 }

SearchResult LocalSearch::hillClimbingWithNRestarts(int numRestarts, ResultRegister &rr) {
  int n = instance.getN();
  Types::Score bestScore = Types::SCORE_MAX;
  SearchResult best(bestScore, Ordering(n));
  rr.set();
  for (int i = 0; i < numRestarts; i++) {
    Ordering o = Ordering::greedyOrdering(instance);
    SearchResult cur = hillClimb(o);
    if (cur.getScore() < best.getScore()) {
      rr.record(cur.getScore(), cur.getOrdering());
      best = cur;
    }
  }
  return best;
}

SearchResult LocalSearch::ILSWithNRestarts(float timeLimit, int greediness, int MAX_PERTURBS, int IMPROVE_THRESHHOLD, int PERTURB_FACTOR, float updateTolerance, ResultRegister &rr, Types::Score opt) {
  int n = instance.getN();
  Types::Score bestScore = Types::SCORE_MAX;
  SearchResult best(bestScore, Ordering(n));

  do {
    Ordering o = Ordering::randomOrdering(instance);
    SearchResult cur = ILS(o, MAX_PERTURBS, IMPROVE_THRESHHOLD, PERTURB_FACTOR, updateTolerance, rr, timeLimit, opt);
    if (cur.getScore() < best.getScore()) {
      best = cur;
    }
  } while (!Util::isOpt(best, opt) && rr.check() < timeLimit);
  return best;
}

SearchResult LocalSearch::ILS(const Ordering &ordering, int MAX_PERTURBS, int IMPROVE_THRESHHOLD, int PERTURB_FACTOR, float updateTolerance, ResultRegister &rr, float timeLimit, Types::Score opt) {
  DBG("ILS(" << MAX_PERTURBS << ", " << IMPROVE_THRESHHOLD << ")");
  SearchResult s = hillClimb(ordering, timeLimit, rr);
  int numPerturbs = 0;
  int timeSinceLastImprovement = 0;
  while (numPerturbs < MAX_PERTURBS && timeSinceLastImprovement < IMPROVE_THRESHHOLD) {
    Ordering perturbed(s.getOrdering());
    perturbed.perturb(PERTURB_FACTOR);
    SearchResult climbed = hillClimb(perturbed, timeLimit, rr);
    if ((1.0-updateTolerance)*(float)climbed.getScore() < s.getScore()) {
      DBG("Tolerance: "<< (1.0-updateTolerance)*(double)climbed.getScore() );
      timeSinceLastImprovement = 0;
      s = climbed;
      numPerturbs += 1;
    } else {
      timeSinceLastImprovement += 1;
    }
    if (climbed.getScore() < rr.getBest()) {
      rr.record(climbed.getScore(), climbed.getOrdering());
    }
    if (rr.check() > timeLimit || Util::isOpt(s, opt)) {
      return s;
    }
  }
  DBG("NUM PERTURBS: " << numPerturbs);
  return s;
}

std::vector<int> LocalSearch::bestParentIds(const Ordering &ordering) {
  int n = instance.getN();
  Types::Bitset pred(n, 0);
  std::vector<int> parents(0);
  for (int i = 0; i < n; i++) {
    parents.push_back(bestParent(ordering, pred, i).getId());
    pred[ordering.get(i)] = 1;
  }
  return parents;
}

SearchResult LocalSearch::makeResult(const Ordering &ordering) const {
  int n = instance.getN();
  Types::Bitset pred(n, 0);
  std::vector<int> parents(0);
  Types::Score score = 0;
  for (int i = 0; i < n; i++) {
    score += bestParent(ordering, pred, i).getScore();
    pred[ordering.get(i)] = 1;
  }
  return SearchResult(score, ordering);
}

Ordering LocalSearch::depthSort(const Ordering &ordering) {
  std::vector<int> parentIds = bestParentIds(ordering);
  int n = instance.getN();
  std::vector<int> depth(n);
  for (int i = 0; i < n; i++) {
    const Variable &v = instance.getVar(ordering.get(i));
    ParentSet p = v.getParent(parentIds[i]);
    int d = getDepth(i, depth, ordering, p);
    depth[i] = d;
    DBG("Depth: " << d);

  }
  std::vector<std::pair<int, int>> depth_var;
  for (int i = 0; i < n; i++) {
    depth_var.push_back(std::make_pair(depth[i], ordering.get(i)));
  }
  std::sort(depth_var.begin(), depth_var.end());
  Ordering ret(n);
  for (int i = 0; i < n; i++) {
    ret.set(i, depth_var[i].second);
  }
  return ret;
}

int LocalSearch::getDepth(int m, const std::vector<int> &depth, const Ordering &ordering, const ParentSet &parent) {
  int n = instance.getN();
  std::vector<int> inDepth(n, n-1);

  for (int i = 0; i < m; i++) {
    inDepth[ordering.get(i)] = depth[i];
  }
  int d = 0;
  for (int i = 0; i < n; i++) {
    if (parent.hasElement(i)) {
      if (d < inDepth[i] + 1) {
        d = inDepth[i] + 1;
      }
    }
  }
  return d;
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

SearchResult LocalSearch::hillClimbBestImprove(Ordering ordering, float cutoffTime, ResultRegister &rr) {
  
  int n = instance.getN();
  int steps = 0;
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    Types::Score bestScore = Types::SCORE_MAX;
    int bestPivot = -1;
    int bestLocation = -1;
    std::vector<int> bestParents;
    std::vector<Types::Score> bestScores;
    for (int s = 0; s < n; s++) {
      int pivot = s;
      FastPivotResult result = getBestInsertFast(cur, pivot, curScore, parents, scores);
      if (result.getScore() < bestScore) {
        bestParents = result.getParents();
        bestScores = result.getScores();
        bestPivot = s;
        bestLocation = result.getSwapIdx();
        bestScore = result.getScore();
      }
    }
    if (bestScore < curScore) {
      steps++;
      cur.insert(bestPivot, bestLocation);
      curScore = bestScore;
      parents = bestParents;
      scores = bestScores;
    } else {
      break;
    }
    DBG("Cur Score: " << curScore);
  } while(rr.check() <= cutoffTime);
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::hillClimbOldHybridImprove(Ordering ordering, float cutoffTime, ResultRegister &rr) {
  bool improving = false;
  int n = instance.getN();
  int steps = 0;
  std::vector<int> positions(n);
  Types::Score curScore = getBestScore(ordering);
  Ordering cur(ordering);
  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int s = 0; s < n && !improving; s++) {
      int pivot = positions[s];
      //DBG("checking pivot " << pivot);
      PivotResult result = getBestInsert(cur, pivot, curScore);
      if (result.getScore() < curScore) {
        steps += 1;
        //DBG("Inserting " << pivot << " to " << result.getSwapIdx());
        improving = true;
        cur.insert(pivot, result.getSwapIdx());
        curScore = result.getScore();
      }
    }
    DBG("Cur Score: " << curScore);
    rr.record(curScore, cur);
  } while(improving && (rr.check() <= cutoffTime));
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::hillClimbHybridImprove(Ordering ordering, float cutoffTime, ResultRegister &rr) {
  bool improving = false;
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);
  int steps = 0;
  std::vector<int> positions(n);
  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    //for (int i = 0; i < n; i++) {
    //  DBG("INdex: " << i << " Variable: " << cur.get(i) << " Parent Set: " << parents[cur.get(i)] << " Score: " << scores[cur.get(i)]);
    //}
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int s = 0; s < n && !improving; s++) {
      int pivot = positions[s];
      //DBG("checking pivot " << pivot);
      FastPivotResult result = getBestInsertFast(cur, pivot, curScore, parents, scores);
      if (result.getScore() < curScore) {
        steps += 1;
        improving = true;
        //DBG("Inserting " << pivot << " to " << result.getSwapIdx());
        cur.insert(pivot, result.getSwapIdx());
        parents = result.getParents();
        scores = result.getScores();
        curScore = result.getScore();

      }
    }
    DBG("Cur Score: " << curScore);
  } while(improving && (rr.check() <= cutoffTime));
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::hillClimbFirstImproveV1(Ordering ordering, float cutoffTime, ResultRegister &rr) {
  bool improving = false;
  int n = instance.getN();
  std::vector<int> parents(n);
  std::vector<Types::Score> scores(n);
  
  int steps = 0;
  std::vector<int> positions(n*n);
  Ordering cur(ordering);
  Types::Score curScore = getBestScoreWithParents(cur, parents, scores);

  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int i = 0; i < n*n && !improving; i++) {
      int s = positions[i]/n;
      int t = positions[i]%n;
      if (s==t) continue;
      FastPivotResult newResult = getInsertScore(cur, s, t, curScore, parents, scores);
      if (newResult.getScore() < curScore) {
        curScore = newResult.getScore();
        cur = newResult.getOrdering();
        scores = newResult.getScores();
        parents = newResult.getParents();
        steps += 1;
        improving = true;
      }
    }
    DBG("Cur Score: " << curScore);
  } while(improving && (rr.check() <= cutoffTime));
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}


PivotResult LocalSearch::getBestInsertWithHook(const Ordering &ordering, int pivot, Types::Score initScore, std::vector<std::vector<Types::Score>> &hook) const {
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
    Types::Score oldIScore = bestParent(modified, forwardPred, i).getScore();
    forwardPred[modified.get(i)] = 1;
    Types::Score oldJScore = bestParent(modified, forwardPred, j).getScore();
    forwardPred[modified.get(i)] = 0;
    Types::Score combinedOldScore = oldIScore + oldJScore;
    modified.swap(i, j);
    const ParentSet &iParent = bestParent(modified, forwardPred, i);
    forwardPred[modified.get(i)] = 1;
    const ParentSet &jParent = bestParent(modified, forwardPred, j);
    Types::Score newIScore = iParent.getScore();
    Types::Score newJScore = jParent.getScore();
    Types::Score combinedNewScore = newIScore + newJScore;
    curScore += combinedNewScore - combinedOldScore;
    hook[pivot][j] = curScore;
    if (curScore < bestScore) {
      bestScore = curScore;
      bestPivot = j;
    }
  }
  curScore = initScore;
  modified = ordering;
  i = pivot;
  j = i - 1;
  for (; j >= 0; i--, j--) {
    Types::Score oldIScore = bestParent(modified, backwardPred, i).getScore();
    backwardPred[modified.get(j)] = 0;
    Types::Score oldJScore = bestParent(modified, backwardPred, j).getScore();
    backwardPred[modified.get(j)] = 1;
    Types::Score combinedOldScore = oldIScore + oldJScore;
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
    //DBG("CUR: " << curScore << " ordering: " << modified);
    hook[pivot][j] = curScore;
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

SearchResult LocalSearch::hillClimbFirstImproveV2(Ordering ordering, float cutoffTime, ResultRegister &rr) {
  bool improving = false;
  int n = instance.getN();
  int steps = 0;
  std::vector<std::vector<Types::Score>> hook;
  hook.resize(n, std::vector<Types::Score>(n, -1));
  Types::Score curScore = getBestScore(ordering);

  std::vector<int> positions(n*n);
  Ordering cur(ordering);
  std::iota(positions.begin(), positions.end(), 0);
  DBG("Inits: " << cur << " Time: " << rr.check());
  do {
    for (int i = 0; i < n; i++) {
      getBestInsertWithHook(cur, i, curScore, hook);
    }
    improving = false;
    std::random_shuffle(positions.begin(), positions.end());
    for (int i = 0; i < n*n && !improving; i++) {
      int s = positions[i]/n;
      int t = positions[i]%n;
      if (s==t) continue;
      Types::Score score = hook[s][t];
      if (score < curScore) {
        curScore = score;
        cur.insert(s, t);
        improving = true;
        steps += 1;
      }
    }
    DBG("Cur Score: " << curScore);
    rr.record(curScore, cur);
  } while(improving && (rr.check() <= cutoffTime));
  DBG("Total Steps: " << steps);
  return SearchResult(curScore, cur);
}

SearchResult LocalSearch::hillClimbWithRestartsProbe(SelectType type, int numRuns, float cutoffTime, ResultRegister &rr, int greediness) {
   int n = instance.getN();
   Types::Score bestScore = Types::SCORE_MAX;
   SearchResult best(bestScore, Ordering(n));
   int runs = 0;
   do {
     rr.set();
     Ordering o(n);
     if (greediness == -1) {
       o = Ordering::randomOrdering(instance);
     } else {
       o = Ordering::greedyOrdering(instance, greediness);
     }
     SearchResult cur(bestScore, o);
     if (type == SelectType::FIRSTV1) {
       cur = hillClimbFirstImproveV1(o, cutoffTime, rr);
     } else if (type == SelectType::FIRSTV2) {
       cur = hillClimbFirstImproveV2(o, cutoffTime, rr);
     } else if (type == SelectType::BEST) {
       cur = hillClimbBestImprove(o, cutoffTime, rr);
     } else if (type == SelectType::HYBRID) {
       cur = hillClimbHybridImprove(o, cutoffTime, rr);
     } else if (type == SelectType::OLDHYBRID) {
       cur = hillClimbOldHybridImprove(o, cutoffTime, rr);
     }
     rr.record(cur.getScore(), cur.getOrdering());
     if (cur.getScore() < best.getScore()) {
       best = cur;
     }
     runs += 1;
   } while (rr.check() < cutoffTime && runs < numRuns);
   return best; 
}

int LocalSearch::depth(int n, const std::vector<int> &parents) const{
  int depths[n];
  for (int i=0; i<n; i++) {
    depths[i] = 1;
  }

  int maxDepth = 1;

  for (int it=0; it<=2*n; it++) {
    for (int i=0; i<n;i++) {
      const Variable &v = instance.getVar(i);
      const ParentSet &p = v.getParent(parents[i]);
      const std::vector<int> &parentVars = p.getParentsVec();

      for (int j=0; j<parentVars.size(); j++) {
        depths[i] = std::max(depths[i], depths[parentVars[j]] + 1);
        maxDepth = std::max(maxDepth, depths[i]);
      }
    }
  }

  for (int i=0; i<n; i++) {
    std::cout << "Var " << i << " depth: " << depths[i] << std::endl;
  }

  return maxDepth;
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
  std::cout << "Depth of Optimal DAG: " << depth(n, parents) << std::endl;
  std::cout << "Total Score: " << scoreFromScores << " " << scoreFromParents << std::endl;
  std::string validStr = valid ? "Good" : "Bad";
  std::cout << "Validity Check: " << validStr << std::endl;
  std::cout << o << std::endl;
}

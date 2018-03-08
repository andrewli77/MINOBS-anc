#include "instance.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "debug.h"
#include<boost/dynamic_bitset.hpp>

Instance::Instance(std::string fileName, std::string constraintsFileName) {
  const int SCORE_SCALE = -1000000;
  int countParents = 0, pruned = 0;
  std::ifstream file(fileName);
  std::ifstream constraintsFile(constraintsFileName);
  this->fileName = fileName;

  if (file.is_open()) {
    file >> n;


    orderConstraints = std::vector<std::vector<bool>>(n);

    for (int i = 0; i < n; i++) {
      orderConstraints[i].resize(n);

      for (int j = 0; j < n; j++) {
        orderConstraints[i][j] = false;
      }
    }

    // Read ancestral constraints.

    constraintsFile >> m;
    ancestralConstraints.resize(m);
    for (int i=0; i < m; i++) {
      int a, b;
      constraintsFile >> a >> b;
      ancestralConstraints[i] = std::make_pair(a, b);
      orderConstraints[a][b] = true; // A basic hash
    }

    // like Bellman-Ford to generate all possible inferred constraints.
    for (int iters = 0; iters < n; iters++) {
      bool improvement = false;

      for (int i = 0; i < n; i++) {
        for (int j = i+1; j < n; j++) {
          if (orderConstraints[i][j]) {
            for (int k = j+1; k < n; k++) {
              if (orderConstraints[j][k]) {
                if (! orderConstraints[i][k]) {
                  improvement = true;
                  orderConstraints[i][k] = true;
                } 
              }
            }
          }
        }
      }

      if (!improvement) break;
    }

    // Read parent set scores.
    vars.resize(n);
    for (int i = 0; i < n; i++) {
      DBG("Working on var: " << i);
      int varId;
      int numParents;
      file >> varId >> numParents;

      Variable v(varId, numParents, n);

      for (int j = 0; j < numParents; j++) {
        double doubleScore;
        int parentSize;
        Types::Bitset set(n, 0);
        std::vector<int> parentsVec;
        file >> doubleScore >> parentSize;
        Types::Score score = (Types::Score)(doubleScore * SCORE_SCALE);

        for (int k = 0; k < parentSize; k++) {
          int parentVar;
          file >> parentVar;
          set[parentVar] = 1;
          parentsVec.push_back(parentVar);
        }

        ParentSet parentSet(score, set, varId, j, parentsVec);
        v.addParentSet(parentSet);
      }

      countParents += numParents;

      v.setNumParents(numParents);
      v.parentSort();
      v.resetParentIds();
      vars[varId] = v;
    }
  } else {
    throw "Could not open file";
  }

  std::cout << countParents << std::endl;

  pruned += pruneParentSetsLossless();
  pruned += pruneParentSetsHeuristic();

  // Initialize the allParentSets array
  for (int i = 0; i < n; i++) {
    const Variable &v = getVar(i);
    for (int j=0; j < v.numParents(); j++) {
      allParentSets.push_back(std::make_pair(i, j));  
    }
  }

  std::cout << "Number of candidate parents: " << allParentSets.size() << std::endl;
  std::cout << "Pruned parent sets: " << pruned << std::endl;
  std::cout << "Number of constraints: " << m << std::endl;
  std::cout << "Pruning factor: " << pruneFactor() << std::endl;
  sortAllParents();
}

int Instance::pruneParentSetsLossless() {
  int pruned = 0;

  for (int i = 0; i < n; i++) {
    Variable &var = getVar(i);
    std::vector<ParentSet> validParents;

    for (int j = 0; j < var.numParents(); j++) {
      if (!canPruneParentLossless(i, j)) {
        validParents.push_back(var.getParent(j));
      } else {
        pruned++;
      }
    }

    var.clearParentSets();

    for (int l = 0; l < validParents.size(); l++) {
      var.addParentSet(validParents[l]);
    }

    var.setNumParents(validParents.size());
    var.parentSort();
    var.resetParentIds();


    
  }
  
  return pruned;
}


bool Instance::canPruneParentLossless(int node, int j) {
  // Pruning rules-------------
  // 1) If X--->Y, P is a parent set of X containing Y, P can be eliminated.
  // 2) If X--->Y, P is a parent set of Y and each W in P precedes X in every ordering, P can be eliminated.
  // 3) If P \subset P' are parent sets for Y and 2*sc(P) < sc(P'), prune P'. This is a heuristic.

  const Variable &var = getVar(node);
  const ParentSet &ps = var.getParent(j);
  const std::vector<int> &parents = ps.getParentsVec();

  // Rule 1
  for (int i = 0; i < parents.size(); i++) {
    if (orderConstraints[node][parents[i]]) {
      return true;
    }
  }


  // Rule 2
  for (int x = 0; x < n; x++) {
    if (orderConstraints[x][node]) {
      bool allPrecedes = true;

      for (int i = 0; i < parents.size(); i++) {
        if (!orderConstraints[parents[i]][x]) {
          allPrecedes = false;
          break;
        }
      }

      if (allPrecedes) {
        return true;
      }
    }
  }

  return false;
}


int Instance::pruneParentSetsHeuristic() {
  int pruned = 0;

  for (int i = 0; i < n; i++) {
    Variable &var = getVar(i);
    std::vector<ParentSet> validParents;

    for (int j = 0; j < var.numParents(); j++) {
      if (!canPruneParentHeuristic(i, j)) {
        validParents.push_back(var.getParent(j));
      } else {
        pruned++;
      }
    }

    var.clearParentSets();

    for (int l = 0; l < validParents.size(); l++) {
      var.addParentSet(validParents[l]);
    }

    var.setNumParents(validParents.size());
    var.parentSort();
    var.resetParentIds();
  }
  
  return pruned;
}


double Instance::pruneFactor() const {
  const double omegaFactor = 1;

  return 1 + omegaFactor * m / (n * (n-1));
}

bool Instance::canPruneParentHeuristic(int node, int j) {
  // Pruning rules-------------
  // 1) If X--->Y, P is a parent set of X containing Y, P can be eliminated.
  // 2) If X--->Y, P is a parent set of Y and each W in P precedes X in every ordering, P can be eliminated.
  // 3) If P \subset P' are parent sets for Y and 2*sc(P) < sc(P'), prune P'. This is a heuristic.

  const Variable &var = getVar(node);
  const ParentSet &ps = var.getParent(j);
  const std::vector<int> &parents = ps.getParentsVec();

  // Rule 3
  Types::Bitset pred(n, 0);
  for (int i = 0; i < parents.size(); i++) {
    pred[parents[i]] = 1;
  }

  for (int i = 0; i < var.numParents(); i++) {
    const ParentSet &other = var.getParent(i);

    if (i != j && pruneFactor() * other.getScore() < ps.getScore() && other.subsetOf(pred)) {
      return true;
    }
  }

  return false;
}


void Instance::sortAllParents() {
  std::sort(allParentSets.begin(), allParentSets.end(), [&](const std::pair<int,int> a, const std::pair<int,int> b) -> bool {
      const Variable &aVar = getVar(a.first), &bVar = getVar(b.first);
      return (aVar.getParent(a.second).getScore() < bVar.getParent(b.second).getScore()
        || (aVar.getParent(a.second).getScore() == bVar.getParent(b.second).getScore() && 
                aVar.getParent(a.second).size() < bVar.getParent(b.second).size()));
    });
}

std::vector< std::pair<int, int> > &Instance::getParentList() {
  return allParentSets;
}

int Instance::getN() const {
  return n;
}

int Instance::getM() const {
  return m;
}

std::string Instance::getFileName() const {
  return fileName;
}

bool Instance::isConstraint(int a, int b) const {
  return (orderConstraints[a][b]);
}

Variable &Instance::getVar(int i) {
  return vars[i];
}

const Ancestral &Instance::getAncestral(int i) const {
  return ancestralConstraints[i];
}

std::ostream& operator<<(std::ostream &os, const Instance& I) {
  os << "Printing Instance: " << std::endl;
  for (int i = 0; i < I.n; i++) {
    os << I.vars[i] << std::endl;
  }
  return os;
}


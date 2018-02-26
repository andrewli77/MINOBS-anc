#include "instance.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "debug.h"
#include<boost/dynamic_bitset.hpp>

Instance::Instance(std::string fileName, std::string constraintsFileName) {
  const int SCORE_SCALE = -1000000;
    int countParents = 0;
  std::ifstream file(fileName);
  std::ifstream constraintsFile(constraintsFileName);
  this->fileName = fileName;

  if (file.is_open()) {
    file >> n;
    vars.resize(n);
    for (int i = 0; i < n; i++) {
      DBG("Working on var: " << i);
      int varId;
      int numParents;
      file >> varId >> numParents;
      countParents += numParents;
      DBG(numParents);
      Variable v(numParents, varId, n);

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
      v.parentSort();
      v.resetParentIds();
      v.initParentsWithVar();
      vars[varId] = v;
    }
    constraintsFile >> m;
    ancestralConstraints.resize(m);
    for (int i=0; i < m; i++) {
      int a, b;
      constraintsFile >> a >> b;
      ancestralConstraints[i] = std::make_pair(a, b);
      orderConstraints.insert(a * n + b); // A basic hash
    }
  } else {
    throw "Could not open file";
  }

  // Initialize the allParentSets array
  for (int i = 0; i < n; i++) {
    const Variable &v = getVar(i);
    for (int j=0; j < v.numParents(); j++) {
      allParentSets.push_back(std::make_pair(i, j));
    }
  }

  std::cout << "Number of candidate parents: " << allParentSets.size() << std::endl;

  sortAllParents();

  DBG("Read in " << countParents << " parent sets.");
}

void Instance::sortAllParents() {
  sort(allParentSets.begin(), allParentSets.end(), [&](std::pair<int,int> a, std::pair<int,int> b) {
      const Variable &aVar = getVar(a.first), &bVar = getVar(b.first);
      return aVar.getParent(a.second).getScore() < bVar.getParent(b.second).getScore()
        || (aVar.getParent(a.second).getScore() == bVar.getParent(b.second).getScore() && 
                aVar.getParent(a.second).size() < bVar.getParent(b.second).getScore());
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
  return (orderConstraints.count(a * n + b));
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


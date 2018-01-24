#include "variable.h"
#include "debug.h"
Variable::Variable(int numParents, int varId, int n) :
  parentsWithVar(), nParents(numParents),  varId(varId) { }

Variable::Variable() { };

void Variable::addParentSet(ParentSet parentSet) {
  parents.push_back(parentSet);
}

void Variable::addDescendant(int i) {
  descendants.push_back(i);
}

int Variable::numDescendants() const {
  return descendants.size();
}

void Variable::addAncestor(int i) {
  ancestors.push_back(i);
}

void Variable::clearAncestry() {
  ancestors.clear();
  descendants.clear();
}

int Variable::numAncestors() const {
  return ancestors.size();
}

const ParentSet &Variable::getParent(int i) const {
  return parents[i];
}

void Variable::parentSort() {
  std::sort(parents.begin(), parents.end(), [](ParentSet a, ParentSet b) {
    return a.getScore() < b.getScore();
  });
}

int Variable::numParents() const {
  return nParents;
}

std::ostream& operator<<(std::ostream &os, const Variable& v) {
  os << "Parents of Variable " << v.varId << ":" << std::endl;
  for (int i = 0; i < v.nParents; i++) {
    os << "Id: " << i << " " << v.parents[i] << std::endl;
  }
  return os;
}

void Variable::resetParentIds() {
  for (int j = 0; j < nParents; j++) {
    parents[j].setId(j);
  }
}

void Variable::initParentsWithVar() {
  int n = numParents();
  for (int i = 0; i < n; i++) {
    const std::vector<int> &parentsVec = getParent(i).getParentsVec();
    int m = parentsVec.size();
    for (int j = 0; j < m; j++) {
      int parentVarId = parentsVec[j];
      parentsWithVar[parentVarId].push_back(getParent(i).getId()); // Should just be i, but just in case)
    }
  }
}

std::unordered_map<int, std::vector<int>>::const_iterator Variable::parentsWithVarId(int i)  const {
  return parentsWithVar.find(i);
}

int Variable::getId() const {
  return varId;
}

const std::vector<int> &Variable::getAncestors() const {
  return ancestors;
}

const std::vector<int> &Variable::getDescendants() const {
  return descendants;
}

bool Variable::hasDescendant(int i) const {
  for (int j=0; j < descendants.size(); j++) {
    if (descendants[j] == i) return true;
  }
  return false;
}
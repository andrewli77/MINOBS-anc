#include "variable.h"
#include "debug.h"
Variable::Variable(int varId, int n) :
  parentsWithVar(), varId(varId) { }

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

void Variable::setNumParents(int n) {
  nParents = n;
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
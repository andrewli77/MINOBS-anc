#ifndef VARIABLE_H
#define VARIABLE_H 

#include"parentset.h"
#include<unordered_map>


class Variable {
  public:
    Variable(int varId, int n);
    Variable();
    void addParentSet(ParentSet parentSet);
    void clearParentSets();
    int numParents() const;
    void setNumParents(int n);
    const ParentSet &getParent(int i) const;
    void parentSort();
    friend std::ostream& operator<<(std::ostream &os, const Variable& v);
    void resetParentIds();
    int getId() const;
    std::unordered_map<int, std::vector<int>> parentsWithVar;

  private:
    int nParents;
    std::vector<ParentSet> parents;
    int varId;
};

#endif /* VARIABLE_H */
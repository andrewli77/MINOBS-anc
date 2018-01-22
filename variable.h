#ifndef VARIABLE_H
#define VARIABLE_H 

#include"parentset.h"
#include<unordered_map>


class Variable {
  public:
    Variable(int numParents, int varId, int n);
    Variable();
    void addParentSet(ParentSet parentSet);
    int numParents() const;
    const ParentSet &getParent(int i) const;
    void parentSort();
    friend std::ostream& operator<<(std::ostream &os, const Variable& v);
    void resetParentIds();
    void initParentsWithVar();
    std::unordered_map<int, std::vector<int>>::const_iterator parentsWithVarId(int i) const;
    int getId() const;
    std::unordered_map<int, std::vector<int>> parentsWithVar;
  private:
    int nParents;
    std::vector<ParentSet> parents;
    int varId;
};

#endif /* VARIABLE_H */
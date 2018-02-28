#ifndef VARIABLE_H
#define VARIABLE_H 

#include"parentset.h"
#include<unordered_map>


class Variable {
  public:
    Variable(int varId, int n);
    Variable();
    void addParentSet(ParentSet parentSet);
    void addDescendant(int i); 
    int numDescendants() const;
    void addAncestor(int i);
    int numAncestors() const;
    void clearAncestry();
    int numParents() const;
    void setNumParents(int n);
    const ParentSet &getParent(int i) const;
    const std::vector<int> &getAncestors() const;
    const std::vector<int> &getDescendants() const;
    bool hasDescendant(int i) const;
    void parentSort();
    friend std::ostream& operator<<(std::ostream &os, const Variable& v);
    void resetParentIds();
    int getId() const;
    std::unordered_map<int, std::vector<int>> parentsWithVar;

  private:
    int nParents;
    std::vector<ParentSet> parents;
    std::vector<int> descendants, ancestors;
    int varId;
};

#endif /* VARIABLE_H */
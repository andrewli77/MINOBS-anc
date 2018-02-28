#ifndef INSTANCE_H
#define INSTANCE_H 

#include <vector>
#include <list>
#include "variable.h"
#include "types.h"

typedef std::pair<int, int> Ancestral; // X --> Y is (X, Y)

class Instance {
  public:
    Instance(std::string fileName, std::string constraintsFileName);
    int getN() const;
    int getM() const;
    std::string getFileName() const;
    bool isConstraint(int a, int b) const;
    bool canPruneParent(int node, const std::vector<int> &parents) const;

    void sortAllParents();
    std::vector< std::pair<int, int> > &getParentList();
    int numParents() const;

    Variable &getVar(int i);
    const Ancestral &getAncestral(int i) const;


    friend std::ostream& operator<<(std::ostream &os, const Instance& I);

  private:
    int n, m;
    std::vector< std::pair<int,int> > allParentSets;
    std::vector<Variable> vars;
    std::vector<Ancestral> ancestralConstraints;
    std::vector<std::vector<bool>> orderConstraints;
    std::string fileName;
};

#endif /* INSTANCE_H */

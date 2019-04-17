#pragma once 
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <set>
#include <sstream>
#include <string>
#include <iostream>

/* Under Construction!!! */
#define ILP_LE "<="
#define ILP_GE ">="
#define ILP_GT ">"
#define ILP_LT "<"
#define ILP_EQ "=="
#define ILP_NE "!="
#define ILP_AS "="
#define ILP_PL "+"
#define ILP_SB "-"
#define ILP_DV "/"
#define ILP_MP "*"

struct ILPConstraint;

struct ILPValue {
    ILPValue() : tag(UNINITIALIZED) {}
    ILPValue(int val) : tag(CONSTANT), constant_value(val) {}
    ILPValue(llvm::StringRef val) : tag(VARIABLE), variable_name(val) {}
    enum {CONSTANT, VARIABLE, CONSTRAINT, UNINITIALIZED} tag;
    // Constant
    int constant_value;
    // Variable
    llvm::StringRef variable_name;
    friend std::ostream& operator<<(std::ostream& os, const ILPValue val);
};

std::ostream& operator<<(std::ostream& os, const ILPValue val) {
    if (val.tag == ILPValue::CONSTANT) os << val.constant_value;
    else if (val.tag == ILPValue::VARIABLE) os << val.variable_name.str();
    else os << "(NULL)";
    return os;
}

struct ILPConstraint {
    ILPConstraint() {}
    ILPConstraint(std::string op, ILPValue v1, ILPValue v2) {
        this->op = op;
        this->v1 = v1;
        this->v2 = v2;
    }

    std::string op;
    ILPValue v1;
    ILPValue v2;
};

/*
 *
 * Used to pass ILP expressions.
 *
 */
struct ILPSolver {
    ILPSolver() {
        
    }
    
    void add_constraint(ILPConstraint constraint) {
        constraints.push_back(constraint);
    }
    
    std::string printILP() {
        // TODO: Iterate over vector of constraints, find all ILPValue with 'VARIABLE' tag, print that out
        // Variables need to be printed first as '-var1 -var2 ... -varN;'
        // Then print out all constraints recursively
        // Constraints need to be printed out as 'var1 op var2;'
        // You can then run lp_solve on this; I'll do this later!
        std::set<llvm::StringRef> variables;
        std::stringstream str;
        for (ILPConstraint& constraint : constraints) {
            if (constraint.v1.tag == ILPValue::VARIABLE) {
                variables.insert(constraint.v1.variable_name);
            }
            if (constraint.v2.tag == ILPValue::VARIABLE) {
                variables.insert(constraint.v2.variable_name);
            }
        }
        for (auto variable : variables) {
            str << "-" << variable.str() << " ";
        }
        str << ";\n";
        
        // TODO: Need to print out constraints...
        for (ILPConstraint& constraint : constraints) {
            str << constraint.v1 << " " << constraint.op << " " << constraint.v2 << ";\n";
        }

        return str.str();
    }
     
    std::vector<ILPConstraint> constraints;
};



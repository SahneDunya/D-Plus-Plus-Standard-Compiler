#include "AST.h"
#include "../lexer/Token.h" // tokenTypeToString için
#include <iostream>

namespace dppc {
namespace ast {

// Genel ASTNode dump implementasyonu (varsayılan)
void ASTNode::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ASTNode (Type: " << typeid(*this).name() << ", Loc: " << loc << ")\n";
}

// İfade düğümlerinin dump implementasyonları
void IntegerExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "IntegerExpr (" << Value << ", Loc: " << loc << ")\n";
}

void FloatingExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "FloatingExpr (" << Value << ", Loc: " << loc << ")\n";
}

void StringExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "StringExpr (\"" << Value << "\", Loc: " << loc << ")\n";
}

void BooleanExprAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "BooleanExpr (" << (Value ? "true" : "false") << ", Loc: " << loc << ")\n";
}

void IdentifierExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "IdentifierExpr (" << Name << ", Loc: " << loc << ")\n";
}

void BinaryOpExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "BinaryOpExpr (" << lexer::tokenTypeToString(Op) << ", Loc: " << loc << ")\n";
    Left->dump(indent + 1);
    Right->dump(indent + 1);
}

void UnaryOpExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "UnaryOpExpr (" << lexer::tokenTypeToString(Op) << ", Loc: " << loc << ")\n";
    Operand->dump(indent + 1);
}

void CallExprAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "CallExpr (Callee: " << CalleeName << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Args:\n";
     for (const auto& arg : Args) {
         arg->dump(indent + 2);
     }
}

void MemberAccessExprAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "MemberAccessExpr (Member: " << MemberName << ", PtrAccess: " << (IsPointerAccess ? "true" : "false") << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Base:\n";
     Base->dump(indent + 2);
}

void ArrayAccessExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ArrayAccessExpr (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Base:\n";
    Base->dump(indent + 2);
    printIndent(indent + 1);
    std::cerr << "Index:\n";
    Index->dump(indent + 2);
}

void NewExprAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "NewExpr (Type: " << TypeName << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Args:\n";
     for (const auto& arg : Args) {
         arg->dump(indent + 2);
     }
}


// D++'a Özgü İfadelerin dump implementasyonları

void BorrowExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "BorrowExpr (" << (IsMutable ? "&mut" : "&") << ", Loc: " << loc << ")\n";
    Operand->dump(indent + 1);
}

void DereferenceExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "DereferenceExpr (*, Loc: " << loc << ")\n";
    Operand->dump(indent + 1);
}

void MatchExprAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "MatchExpr (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Value:\n";
    Value->dump(indent + 2);
    printIndent(indent + 1);
    std::cerr << "Arms:\n";
    for (const auto& arm : Arms) {
        printIndent(indent + 2);
        std::cerr << "MatchArm:\n";
        printIndent(indent + 3);
        std::cerr << "Pattern:\n";
        arm.Pattern->dump(indent + 4);
        if (arm.Guard) {
            printIndent(indent + 3);
            std::cerr << "Guard:\n";
            arm.Guard->dump(indent + 4);
        }
        printIndent(indent + 3);
        std::cerr << "Body:\n";
        arm.Body->dump(indent + 4);
    }
}

// --- Desen (Pattern) düğümlerinin dump implementasyonları ---

void PatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "PatternAST (Type: " << typeid(*this).name() << ", Loc: " << loc << ")\n";
}

void WildcardPatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "WildcardPattern (_ , Loc: " << loc << ")\n";
}

void IdentifierPatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "IdentifierPattern (" << (IsMutable ? "mut " : "") << Name << ", Loc: " << loc << ")\n";
}

void LiteralPatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "LiteralPattern (Loc: " << loc << ")\n";
     Literal->dump(indent + 1);
}

void StructDeconstructionPatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "StructDeconstructionPattern (Type: " << TypeName << ", IgnoreExtra: " << (IgnoreExtraFields ? "true" : "false") << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Fields:\n";
     for (const auto& field : Fields) {
         printIndent(indent + 2);
         std::cerr << "FieldMatch (Name: " << field.FieldName << "):\n";
         field.Binding->dump(indent + 3);
     }
}

void EnumVariantPatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "EnumVariantPattern (Enum: " << EnumName << ", Variant: " << VariantName << ", Loc: " << loc << ")\n";
     if (!Args.empty()) {
         printIndent(indent + 1);
         std::cerr << "Args:\n";
         for (const auto& arg : Args) {
             arg->dump(indent + 2);
         }
     }
}

void TuplePatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "TuplePattern (Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Elements:\n";
     for (const auto& elem : Elements) {
         elem->dump(indent + 2);
     }
}

void ReferencePatternAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "ReferencePattern (" << (IsMutable ? "&mut" : "&") << ", Loc: " << loc << ")\n";
     SubPattern->dump(indent + 1);
}


// --- Deyim (Statement) düğümlerinin dump implementasyonları ---

void StmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "StmtAST (Type: " << typeid(*this).name() << ", Loc: " << loc << ")\n";
}

void ExprStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ExprStmt (Loc: " << loc << ")\n";
    Expr->dump(indent + 1);
}

void BlockStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "BlockStmt (Loc: " << loc << ")\n";
    for (const auto& stmt : Statements) {
        stmt->dump(indent + 1);
    }
}

void IfStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "IfStmt (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Condition:\n";
    Condition->dump(indent + 2);
    printIndent(indent + 1);
    std::cerr << "Then:\n";
    ThenBlock->dump(indent + 2);
    if (ElseBlock) {
        printIndent(indent + 1);
        std::cerr << "Else:\n";
        ElseBlock->dump(indent + 2);
    }
}

void WhileStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "WhileStmt (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Condition:\n";
    Condition->dump(indent + 2);
    printIndent(indent + 1);
    std::cerr << "Body:\n";
    Body->dump(indent + 2);
}

void ForStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ForStmt (Loc: " << loc << ")\n";
    if (Init) { printIndent(indent + 1); std::cerr << "Init:\n"; Init->dump(indent + 2); }
    if (Condition) { printIndent(indent + 1); std::cerr << "Condition:\n"; Condition->dump(indent + 2); }
    if (LoopEnd) { printIndent(indent + 1); std::cerr << "LoopEnd:\n"; LoopEnd->dump(indent + 2); }
    printIndent(indent + 1); std::cerr << "Body:\n";
    Body->dump(indent + 2);
}

void ReturnStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ReturnStmt (Loc: " << loc << ")\n";
    if (ReturnValue) {
        ReturnValue->dump(indent + 1);
    } else {
        printIndent(indent + 1);
        std::cerr << "<void>\n";
    }
}

void BreakStmtAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "BreakStmt (Loc: " << loc << ")\n";
}

void ContinueStmtAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "ContinueStmt (Loc: " << loc << ")\n";
}

// D++'a Özgü Deyimlerin dump implementasyonları

void LetStmtAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "LetStmt (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Pattern:\n";
    Pattern->dump(indent + 2);
    if (Type) {
        printIndent(indent + 1);
        std::cerr << "Type:\n";
        Type->dump(indent + 2);
    }
    if (Initializer) {
        printIndent(indent + 1);
        std::cerr << "Initializer:\n";
        Initializer->dump(indent + 2);
    }
}

// --- Bildirim (Declaration) düğümlerinin dump implementasyonları ---

void DeclAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "DeclAST (Type: " << typeid(*this).name() << ", Loc: " << loc << ")\n";
}

void ParameterAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "Parameter (" << Name << ", Loc: " << loc << ")\n";
    Type->dump(indent + 1);
}

void FunctionDeclAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "FunctionDecl (Name: " << Name << ", Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Parameters:\n";
    for (const auto& param : Params) {
        param->dump(indent + 2);
    }
    printIndent(indent + 1);
    std::cerr << "ReturnType:\n";
    ReturnType->dump(indent + 2);
    if (Body) {
        printIndent(indent + 1);
        std::cerr << "Body:\n";
        Body->dump(indent + 2);
    }
}

void StructFieldAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "StructField (Name: " << Name << ", Loc: " << loc << ")\n";
    Type->dump(indent + 1);
}

void StructDeclAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "StructDecl (Name: " << Name << ", Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Fields:\n";
    for (const auto& field : Fields) {
        field->dump(indent + 2);
    }
}

void TraitDeclAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "TraitDecl (Name: " << Name << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Required Functions:\n";
     for (const auto& func : RequiredFunctions) {
         func->dump(indent + 2);
     }
}

void ImplDeclAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "ImplDecl (ForType: " << ForTypeName << ", Trait: " << (TraitName.empty() ? "<none>" : TraitName) << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Methods:\n";
     for (const auto& method : Methods) {
         method->dump(indent + 2);
     }
}

void EnumVariantAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "EnumVariant (Name: " << Name << ", Loc: " << loc << ")\n";
     // İlgili tipler veya değerler varsa dump edilebilir
}

void EnumDeclAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "EnumDecl (Name: " << Name << ", Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Variants:\n";
     for (const auto& variant : Variants) {
         variant->dump(indent + 2);
     }
}


// --- Tip (Type) düğümlerinin dump implementasyonları ---

void TypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "TypeAST (Type: " << typeid(*this).name() << ", Loc: " << loc << ")\n";
}

void PrimitiveTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "PrimitiveType (" << lexer::tokenTypeToString(TypeToken) << ", Loc: " << loc << ")\n";
}

void IdentifierTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "IdentifierType (" << Name << ", Loc: " << loc << ")\n";
}

void PointerTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "PointerType (*, Loc: " << loc << ")\n";
    BaseType->dump(indent + 1);
}

void ReferenceTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ReferenceType (" << (IsMutable ? "&mut" : "&") << ", Loc: " << loc << ")\n";
    BaseType->dump(indent + 1);
}

void ArrayTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ArrayType (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Element Type:\n";
    ElementType->dump(indent + 2);
    if (Size) {
         printIndent(indent + 1);
         std::cerr << "Size:\n";
         Size->dump(indent + 2);
    }
}

void TupleTypeAST::dump(int indent) const {
     printIndent(indent);
     std::cerr << "TupleType (Loc: " << loc << ")\n";
     printIndent(indent + 1);
     std::cerr << "Elements:\n";
     for (const auto& elem : ElementTypes) {
         elem->dump(indent + 2);
     }
}


void OptionTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "OptionType (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Inner Type:\n";
    InnerType->dump(indent + 2);
}

void ResultTypeAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "ResultType (Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Ok Type:\n";
    OkType->dump(indent + 2);
    printIndent(indent + 1);
    std::cerr << "Err Type:\n";
    ErrType->dump(indent + 2);
}

// --- Kök Düğüm dump implementasyonu ---

void SourceFileAST::dump(int indent) const {
    printIndent(indent);
    std::cerr << "SourceFile (Filename: " << Filename << ", Loc: " << loc << ")\n";
    printIndent(indent + 1);
    std::cerr << "Declarations:\n";
    for (const auto& decl : Declarations) {
        decl->dump(indent + 2);
    }
}


} // namespace ast
} // namespace dppc

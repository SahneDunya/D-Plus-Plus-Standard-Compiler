#ifndef DPP_COMPILER_SEMA_SEMANTICANALYZER_H
#define DPP_COMPILER_SEMA_SEMANTICANALYZER_H

#include "../parser/AST.h"          // AST düğümleri için
#include "../utils/ErrorReporter.h" // Hata raporlama için
#include "SymbolTable.h"            // Sembol tablosu için (SymbolTable.h dosyasının var olduğu varsayılır)
#include "TypeSystem.h"             // Tip sistemi için (TypeSystem.h dosyasının var olduğu varsayılır)
#include "../lexer/Token.h" // TokenType için

#include <memory>
#include <vector>

// İleri bildirimler
namespace dppc::ast {
    struct SourceFileAST;
    struct DeclAST;
    struct StmtAST;
    struct ExprAST;
    struct TypeAST;
    struct PatternAST;
    struct LetStmtAST;
    struct MatchExprAST;
    struct BorrowExprAST;
    struct DereferenceExprAST;
    struct FunctionDeclAST;
    // ... diğer AST düğümleri
}

namespace dppc::sema {

// Semantik Analizci sınıfı
class SemanticAnalyzer {
private:
    ErrorReporter& Reporter; // Hata raporlama nesnesi
    SymbolTable& Symbols;    // Sembol tablosu
    TypeSystem& Types;       // Tip sistemi

    // Semantik analiz sırasında ihtiyaç duyulabilecek durum bilgileri
    SymbolTable::Scope* CurrentScope; // Şu anki kapsam
    Type* CurrentFunctionReturnType; // İçinde bulunulan fonksiyonun dönüş tipi (return kontrolü için)
    // Diğer durumlar: Döngü içinde miyiz (break/continue kontrolü), mutable blokta mıyız vb.

    // Yardımcı Fonksiyonlar (AST düğümlerini analiz eden rekürsif fonksiyonlar)
    // Her fonksiyon, ilgili AST düğümünü alır, anlamsal kontrolleri yapar ve
    // gerekirse AST düğümüne analiz sonucunda elde edilen bilgileri ekler (örn: resolvedType).

    // Genel analiz fonksiyonları
    void analyzeDecl(ast::DeclAST* decl);
    void analyzeStmt(ast::StmtAST* stmt);
    // analyzeExpr fonksiyonu, ifadenin çözümlenmiş tipini döndürür.
    // AST düğümünün kendi içinde resolvedType alanı olması daha yaygın bir yaklaşımdır.
    Type* analyzeExpr(ast::ExprAST* expr);
    Type* analyzeType(ast::TypeAST* typeNode); // TypeAST düğümünü alıp Type* nesnesi döndürür
    void analyzePattern(ast::PatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope); // Pattern analizi ve değişken bağlama

    // Spesifik bildirim analiz fonksiyonları
    void analyzeFunctionDecl(ast::FunctionDeclAST* funcDecl);
    void analyzeStructDecl(ast::StructDeclAST* structDecl);
    void analyzeTraitDecl(ast::TraitDeclAST* traitDecl); // D++'a özgü
    void analyzeImplDecl(ast::ImplDeclAST* implDecl);   // D++'a özgü
    void analyzeEnumDecl(ast::EnumDeclAST* enumDecl);

    // Spesifik deyim analiz fonksiyonları
    void analyzeBlockStmt(ast::BlockStmtAST* blockStmt);
    void analyzeIfStmt(ast::IfStmtAST* ifStmt);
    void analyzeWhileStmt(ast::WhileStmtAST* whileStmt);
    void analyzeForStmt(ast::ForStmtAST* forStmt);
    void analyzeReturnStmt(ast::ReturnStmtAST* returnStmt);
    void analyzeBreakStmt(ast::BreakStmtAST* breakStmt); // Döngü kontrolü gerekebilir
    void analyzeContinueStmt(ast::ContinueStmtAST* continueStmt); // Döngü kontrolü gerekebilir
    void analyzeLetStmt(ast::LetStmtAST* letStmt); // D++'a özgü

    // Spesifik ifade analiz fonksiyonları
    Type* analyzeIntegerExpr(ast::IntegerExprAST* expr);
    Type* analyzeFloatingExpr(ast::FloatingExprAST* expr);
    Type* analyzeStringExpr(ast::StringExprAST* expr);
    Type* analyzeBooleanExpr(ast::BooleanExprAST* expr);
    Type* analyzeIdentifierExpr(ast::IdentifierExprAST* expr);
    Type* analyzeBinaryOpExpr(ast::BinaryOpExprAST* expr);
    Type* analyzeUnaryOpExpr(ast::UnaryOpExprAST* expr);
    Type* analyzeCallExpr(ast::CallExprAST* expr);
    Type* analyzeMemberAccessExpr(ast::MemberAccessExprAST* expr);
    Type* analyzeArrayAccessExpr(ast::ArrayAccessExprAST* expr);
    Type* analyzeNewExpr(ast::NewExprAST* expr);
    Type* analyzeMatchExpr(ast::MatchExprAST* expr); // D++'a özgü
    Type* analyzeBorrowExpr(ast::BorrowExprAST* expr); // D++'a özgü
    Type* analyzeDereferenceExpr(ast::DereferenceExprAST* expr); // D++'a özgü

    // Spesifik pattern analiz fonksiyonları (analyzePattern tarafından çağrılır)
    void analyzeWildcardPattern(ast::WildcardPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeIdentifierPattern(ast::IdentifierPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeLiteralPattern(ast::LiteralPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeStructDeconstructionPattern(ast::StructDeconstructionPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeEnumVariantPattern(ast::EnumVariantPatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeTuplePattern(ast::TuplePatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    void analyzeReferencePattern(ast::ReferencePatternAST* pattern, Type* targetType, SymbolTable::Scope* bindingScope);
    // TODO: analyzeRangePattern

    // Sembol Tablosu ve Tip Sistemi ile etkileşim
    SymbolTable::Symbol* lookupSymbol(const std::string& name, const SourceLocation& loc); // İsim çözümleme
    void declareSymbol(const std::string& name, Symbol::SymbolKind kind, Type* type, bool isMutable, const SourceLocation& loc); // Sembol tablosuna ekle
    Type* getResolvedType(ast::TypeAST* typeNode); // TypeAST düğümünden çözümlenmiş Type* nesnesini al
    Type* getCommonType(Type* type1, Type* type2, const SourceLocation& loc); // İki tipin ortak paydasını bul
    bool isAssignable(Type* destType, Type* srcType); // srcType, destType'a atanabilir mi?
    bool isConvertible(Type* fromType, Type* toType); // fromType, toType'a dönüştürülebilir mi?

    // D++'a Özgü Analiz Yardımcıları (Ownership, Borrowing, Lifetimes, Traits)
    // Bu fonksiyonlar oldukça karmaşık olacaktır ve SymbolTable'daki sembollerin durumunu (Owned, Borrowed vb.) güncellemeyi ve kuralları kontrol etmeyi içerir.
    void checkOwnershipRules(ast::ExprAST* expr, Type* exprType); // İfade sonucu üzerinde sahiplik kurallarını kontrol et (move, copy)
    void checkBorrowRules(ast::BorrowExprAST* borrowExpr, Type* operandType); // Ödünç alma kurallarını kontrol et (&, &mut)
    void checkDereferenceRules(ast::DereferenceExprAST* derefExpr, Type* operandType); // Dereference kurallarını kontrol et (*)
    void checkMutability(ast::ExprAST* targetExpr, bool isAssignment); // Atama veya mutable işlemde mutability'yi kontrol et
    void analyzeLifetimes(ast::ASTNode* node); // Yaşam sürelerini analiz et ve doğrula (karmaşık bir konu)
    void resolveTraitsAndMethods(ast::CallExprAST* callExpr, Type* baseType); // Metot çağrısı veya trait çözümü

    // Pattern Matching yardımcıları
    void checkPatternValidity(ast::PatternAST* pattern, Type* targetType); // Pattern'ın eşleşilen tip ile uyumluluğunu kontrol et
    void checkPatternExhaustiveness(ast::MatchExprAST* matchExpr, Type* matchedType); // Match kollarının tüm durumları kapsayıp kapsamadığını kontrol et (Oldukça zor bir görev)
     void bindPatternVariables(ast::PatternAST* pattern, Type* targetType, SymbolTable::Scope* scope); // Pattern içindeki değişkenleri scope'a bağla (analyzePattern içinde yapılıyor)


public:
    // Constructor
    SemanticAnalyzer(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types);

    // Ana analiz fonksiyonu
    bool analyze(ast::SourceFileAST& root); // Başarılıysa true, hata varsa false döndürür
};

} // namespace sema
} // namespace dppc

#endif // DPP_COMPILER_SEMA_SEMANTICANALYZER_H

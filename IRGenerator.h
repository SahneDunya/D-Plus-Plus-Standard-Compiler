#ifndef DPP_COMPILER_IRGEN_IRGENERATOR_H
#define DPP_COMPILER_IRGEN_IRGENERATOR_H

#include "../utils/ErrorReporter.h" // Hata raporlama için
#include "../sema/SymbolTable.h"    // Sembol tablosu için
#include "../sema/TypeSystem.h"     // Tip sistemi için
#include "../parser/AST.h"          // AST düğümleri için

// LLVM Başlıkları
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/APInt.h"    // Integer sabitler için
#include "llvm/ADT/APFloat.h"  // Floating point sabitler için


#include <string>
#include <vector>
#include <unordered_map>
#include <memory> // std::unique_ptr için

// İleri bildirimler
namespace dppc::ast {
    struct SourceFileAST;
    struct DeclAST;
    struct StmtAST;
    struct ExprAST;
    struct TypeAST; // LLVM tipine çevrilmesi gereken TypeAST düğümleri
    struct FunctionDeclAST;
    struct StructDeclAST;
    // ... diğer ilgili AST düğümleri
}

namespace dppc::sema {
    class Type; // D++ Tip nesneleri
    class Symbol; // Sembol nesneleri
}


namespace dppc {
namespace irgen {

// IR Generator sınıfı
class IRGenerator {
private:
    ErrorReporter& Reporter; // Hata raporlama
    SymbolTable& Symbols;    // Sembol tablosu (D++ sembollerini LLVM değerlerine eşlemek için)
    TypeSystem& Types;       // Tip sistemi (D++ tiplerini LLVM tiplerine çevirmek için)

    // LLVM Altyapı Nesneleri
    // LLVMContext, LLVM'in temel veri yapılarını yönetir. Genellikle tekil (singleton) olur.
    // Module, derleme birimini temsil eder ve fonksiyonları, global değişkenleri içerir.
    // IRBuilder, LLVM talimatlarını kolayca oluşturmak için kullanılır.
    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::Module> Module;
    std::unique_ptr<llvm::IRBuilder<>> Builder;

    // D++ Tiplerini LLVM Tiplerine Eşleme
    // Semantik analiz sırasında çözümlenmiş D++ Type* pointer'larını LLVM llvm::Type* pointer'larına eşler.
    std::unordered_map<sema::Type*, llvm::Type*> DppToLLVMTypes;

    // D++ Sembollerini LLVM Değerlerine Eşleme
    // Değişkenler için AllocaInst* pointer'larını, fonksiyonlar için Function* pointer'larını saklar.
    // Symbol yapısına llvm::Value* pointer'ı eklemek daha yaygın bir yaklaşımdır,
    // bu sayede Symbol* pointer'ını aldığınız her yerde LLVM değeri de elde edebilirsiniz.
     llvm::Value* is base for Function*, AllocaInst*, Constant* etc.
     std::unordered_map<sema::Symbol*, llvm::Value*> DppSymbolToLLVMValue;


    // Kontrol Akışı Yönetimi
    // Break/Continue gibi ifadelerin hedefleyeceği BasicBlock'ları takip etmek için (döngüler vb.)
     std::vector<llvm::BasicBlock*> LoopExitBlocks;
     std::vector<llvm::BasicBlock*> LoopContinueBlocks;


    // Yardımcı Fonksiyonlar (AST düğümlerini IR'a çeviren rekürsif fonksiyonlar)

    // AST düğüm türüne göre IR üretimini yönlendiren genel fonksiyonlar
    void generateDecl(ast::DeclAST* decl);
    // Stmt ve Expr, ürettikleri LLVM BasicBlock veya Value*'ı döndürür.
    // generateStmt, BasicBlock* veya void döndürebilir (eğer terminatör içermiyorsa).
    // generateExpr, LLVM Value* döndürür.
    void generateStmt(ast::StmtAST* stmt);
    llvm::Value* generateExpr(ast::ExprAST* expr);

    // D++ Tipini LLVM Tipine çevirir
    llvm::Type* generateType(sema::Type* dppType);

    // Spesifik Bildirim IR Üretim Fonksiyonları
    void generateFunctionDecl(ast::FunctionDeclAST* funcDecl);
    // generateStructDecl, generateEnumDecl, generateTraitDecl gibi fonksiyonlar
    // genellikle sadece TypeSystem'daki Type* nesnesinin LLVM karşılığını generateType içinde hazırlar.
    // Kod üretimi gerekiyorsa (örn: struct literal veya metodlar), bu generateExpr/generateStmt içinde olur.

    // Spesifik Deyim IR Üretim Fonksiyonları
    void generateBlockStmt(ast::BlockStmtAST* blockStmt);
    void generateIfStmt(ast::IfStmtAST* ifStmt);
    void generateWhileStmt(ast::WhileStmtAST* whileStmt);
    void generateForStmt(ast::ForStmtAST* forStmt);
    void generateReturnStmt(ast::ReturnStmtAST* returnStmt);
    void generateBreakStmt(ast::BreakStmtAST* breakStmt);
    void generateContinueStmt(ast::ContinueStmtAST* continueStmt);
    void generateLetStmt(ast::LetStmtAST* letStmt); // D++'a özgü
    // TODO: Diğer deyim türleri

    // Spesifik İfade IR Üretim Fonksiyonları (Hepsi llvm::Value* döndürür)
    llvm::Value* generateIntegerExpr(ast::IntegerExprAST* expr);
    llvm::Value* generateFloatingExpr(ast::FloatingExprAST* expr);
    llvm::Value* generateStringExpr(ast::StringExprAST* expr);
    llvm::Value* generateBooleanExpr(ast::BooleanExprAST* expr);
    llvm::Value* generateIdentifierExpr(ast::IdentifierExprAST* expr);
    llvm::Value* generateBinaryOpExpr(ast::BinaryOpExprAST* expr);
    llvm::Value* generateUnaryOpExpr(ast::UnaryOpExprAST* expr); // Tekli -, !, gibi operatörler
    llvm::Value* generateCallExpr(ast::CallExprAST* expr);
    llvm::Value* generateMemberAccessExpr(ast::MemberAccessExprAST* expr); // . veya ->
    llvm::Value* generateArrayAccessExpr(ast::ArrayAccessExprAST* expr); // []
    llvm::Value* generateNewExpr(ast::NewExprAST* expr); // new
    llvm::Value* generateMatchExpr(ast::MatchExprAST* expr); // D++'a özgü
    llvm::Value* generateBorrowExpr(ast::BorrowExprAST* expr); // & veya &mut
    llvm::Value* generateDereferenceExpr(ast::DereferenceExprAST* expr); // *
    // TODO: Diğer ifade türleri (literallerin hepsi, array literal, struct literal, tuple literal vb.)


    // LLVM Yönetimi ve Yardımcıları
    llvm::BasicBlock* createBasicBlock(const std::string& name = "entry");
    void setInsertPoint(llvm::BasicBlock* block);
    llvm::BasicBlock* getCurrentBlock() const;
    llvm::Function* getCurrentFunction();

    // Değişken için stack alanı ayırır ve Symbol'e LLVM Value* bilgisini kaydeder
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, llvm::Type* varType, const std::string& varName, sema::Symbol* symbol = nullptr);

    // Helper: Sayısal tipler için LLVM tipini al
    llvm::Type* getLLVMIntegerType(sema::Type* dppType);
    llvm::Type* getLLVMFloatingPointType(sema::Type* dppType);

    // Helper: Bir ifade sonucunu belirtilen LLVM tipine dönüştürür (gerekirse)
    llvm::Value* createTypeCast(llvm::Value* value, llvm::Type* targetType);


public:
    // Constructor
    // TargetTriple, main.cpp veya CompilerDriver tarafından belirlenir (örn: "riscv64-unknown-elf")
    IRGenerator(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types, const std::string& moduleName, const std::string& targetTriple);

    // Yıkıcı (Context, Module, Builder unique_ptr'lar sayesinde otomatik temizlenir)
    ~IRGenerator() = default;

    // AST'nin tamamını IR'a çeviren ana fonksiyon
    std::unique_ptr<llvm::Module> generate(ast::SourceFileAST& root);

    // LLVM Modülünü doğrulama
    bool verifyModule();
};

} // namespace irgen
} // namespace dppc

#endif // DPP_COMPILER_IRGEN_IRGENERATOR_H

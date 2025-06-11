#ifndef DPP_COMPILER_SEMA_BORROWCHECKER_H
#define DPP_COMPILER_SEMA_BORROWCHECKER_H

#include "../utils/ErrorReporter.h" // Hata raporlama için
#include "SymbolTable.h"            // Sembol tablosu için
#include "TypeSystem.h"             // Tip sistemi için
#include "../parser/AST.h"          // AST düğümleri için

#include <vector>
#include <string>

// İleri bildirimler
namespace dppc::ast {
    struct ExprAST;
    struct StmtAST;
    struct DeclAST;
    struct BinaryOpExprAST;
    struct UnaryOpExprAST;
    struct CallExprAST;
    struct MemberAccessExprAST;
    struct ArrayAccessExprAST;
    struct BorrowExprAST;
    struct DereferenceExprAST;
    struct LetStmtAST;
    // ... ilgili diğer AST düğümleri
}

namespace dppc::sema {

// Yaşam Süresi (Lifetime) Temsili (Basit bir başlangıç)
// Tam bir lifetime sistemi çok daha karmaşıktır ve analiz gerektirir.
// Şimdilik sadece scope'lar veya manuel annotation'lar gibi basit lifetime referansları olabilir.
// Daha sonra daha sofistike bir temsil ile değiştirilebilir.
// Örneğin, her Scope'un bir lifetime'ı olabilir ve referanslar bu lifetime'a bağlanabilir.
struct Lifetime {
    // unique ID, isim (örn: 'a, 'static), bağlı olduğu scope pointer'ı vb.
    std::string Name; // Debug veya isimlendirilmiş lifetime'lar için

    Lifetime(std::string name = "<anon>") : Name(std::move(name)) {}

    // Debug için
    friend std::ostream& operator<<(std::ostream& os, const Lifetime& lt) {
        os << "'" << lt.Name;
        return os;
    }
};


// Borrow Checker sınıfı
// SemanticAnalyzer tarafından AST traversalı sırasında kullanılır.
class BorrowChecker {
private:
    ErrorReporter& Reporter; // Hata raporlama
    SymbolTable& Symbols;    // Sembol tablosu (Symbol state'lerini günceller)
    TypeSystem& Types;       // Tip sistemi (isCopyType vb. kontrolü için)

    // Borrow Checker durumu (SemanticAnalyzer'dan gelen veya kendi tuttuğu)
    // Sembol Tablosu zaten Symbol'lerin state'lerini tutar.
    // Aktif borrow'ları ve bunların etki alanlarını (lifetime) takip etmek gerekebilir.
    // Bu, oldukça karmaşık bir veri yapısı gerektirir.
    // Basit bir başlangıç için, Symbol'deki borrowCount ve state yeterli olabilir.


    // Yardımcı Fonksiyonlar (Kuralları kontrol eden ve hataları raporlayan)

    // Bir sembolün kullanılabilir olup olmadığını kontrol eder (moved, dropped, invalid borrow).
    bool checkSymbolUsability(Symbol* symbol, const SourceLocation& loc, const std::string& usageDescription);

    // Bir ifadenin sonucunun (value) move edilebilir/kopyalanabilir olduğunu kontrol eder.
    // Eğer move ediliyorsa, kaynak sembolün durumunu günceller.
    bool checkMoveOrCopy(ast::ExprAST* sourceExpr, Type* sourceType, const SourceLocation& usageLoc);

    // Bir sembole shared borrow (&) yapıldığında kuralları kontrol eder.
    // Kaynak sembolün durumunu günceller (borrow count artar). Bir Lifetime döndürebilir.
     bool checkSharedBorrow(ast::ExprAST* sourceExpr, Type* sourceType, Lifetime& outLifetime); // outLifetime'a lifetime'ı yazar

    // Bir sembole mutable borrow (&mut) yapıldığında kuralları kontrol eder.
    // Kaynak sembolün durumunu günceller (state = BorrowedMutable). Bir Lifetime döndürebilir.
    // bool checkMutableBorrow(ast::ExprAST* sourceExpr, Type* sourceType, Lifetime& outLifetime);

    // Bir referansın (veya pointer'ın) dereference edildiğinde (*ptr, *ref) kuralları kontrol eder.
    // Referansın/pointer'ın geçerli olduğundan ve işaret ettiği veriye erişimin kurallara uygun olduğundan emin olur.
    bool checkDereference(ast::DereferenceExprAST* derefExpr, Type* operandType);


    // Bir atama işleminin kurallarını kontrol eder (target = value).
    // Target'ın mutable lvalue olduğunu, value'nun atanabilir olduğunu ve sahiplik/borrowing kurallarını doğrular.
    bool checkAssignment(ast::ExprAST* targetExpr, Type* targetType, ast::ExprAST* valueExpr, Type* valueType);

    // Fonksiyon çağrısı argümanları için sahiplik/borrowing kurallarını kontrol eder.
    // Argümanların nasıl geçirildiğine (by value/move, shared ref, mutable ref) göre kontroller yapar.
    bool checkFunctionCallArguments(ast::CallExprAST* callExpr, FunctionType* calleeType);

    // Return deyimi için sahiplik/borrowing kurallarını kontrol eder (dönüş değerinin move edilmesi).
    bool checkReturnStatement(ast::ReturnStmtAST* returnStmt, Type* returnedType, Type* functionReturnType);


    // Yaşam süresi analizi yardımcıları (oldukça karmaşık)
    // Bu fonksiyonlar genellikle ayrı bir analiz pass'i veya daha entegre bir sistem gerektirir.
    // Basitlik için şimdilik sadece placeholder'lar.
     void assignLifetimes(ast::ASTNode* node); // AST traversalı sırasında lifetime'ları ata/infer et
     void checkLifetimeValidity(ast::ExprAST* useExpr, Type* typeWithLifetime); // Bir referansın kullanıldığı noktada geçerli olup olmadığını kontrol et

    // Symbol durumu güncelleme yardımcıları (SymbolTable'a delege edilebilir)
     void setSymbolState(Symbol* symbol, OwnershipState state);
     void incrementBorrowCount(Symbol* symbol);
     void decrementBorrowCount(Symbol* symbol);


public:
    // Constructor
    BorrowChecker(ErrorReporter& reporter, SymbolTable& symbols, TypeSystem& types);

    // Borrow Checker'ın ana giriş noktası (SemanticAnalyzer tarafından çağrılır)
    // Genellikle SemanticAnalyzer'ın analyze() metodunun içinde, AST gezilirken kullanılır.
    // analyzeDecl, analyzeStmt, analyzeExpr fonksiyonlarından çağrılan kontroller içerir.
    // Örneğin, analyzeExpr içinde bir BinaryOpExpr (atama) veya UnaryOpExpr (& veya *) ile karşılaşılınca.

    // Semantik analiz sırasında çağrılacak spesifik kontrol fonksiyonları
    // Bu fonksiyonlar, ilgili AST düğümünü ve analiz edilmiş tip bilgisini alır.

    void onVariableDeclaration(ast::LetStmtAST* letStmt, Symbol* symbol); // Let deyimi analiz edildiğinde
    void onAssignment(ast::BinaryOpExprAST* assignExpr, Type* targetType, Type* valueType); // Atama (=) analiz edildiğinde
    void onVariableUse(ast::IdentifierExprAST* idExpr, Symbol* symbol); // Değişken kullanıldığında (okuma)
    void onBorrowExpression(ast::BorrowExprAST* borrowExpr, Type* operandType); // & veya &mut ifadesi analiz edildiğinde
    void onDereferenceExpression(ast::DereferenceExprAST* derefExpr, Type* operandType); // * ifadesi analiz edildiğinde
    void onFunctionCall(ast::CallExprAST* callExpr, FunctionType* calleeType); // Fonksiyon çağrısı analiz edildiğinde
    void onReturnStatement(ast::ReturnStmtAST* returnStmt, Type* returnedType, Type* functionReturnType); // Return deyimi analiz edildiğinde
    void onScopeExit(SymbolTable::Scope* scope); // Kapsamdan çıkıldığında (Owned değerleri düşürmek, borrowları sonlandırmak)

    // TODO: Diğer durumlar: Alan erişimi (. , ->), Dizi erişimi ([]), Match (pattern binding), New, vb.

    // Analiz sonunda çağrılabilecek fonksiyonlar (örn: Kalan borrow var mı?)
     void finalizeCheck();
};

} // namespace sema
} // namespace dppc

#endif // DPP_COMPILER_SEMA_BORROWCHECKER_H

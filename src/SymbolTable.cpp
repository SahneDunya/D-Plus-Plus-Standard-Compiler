#include "SymbolTable.h"
#include "../parser/AST.h" // ASTNode kullanılıyorsa dahil edilmeli
#include "TypeSystem.h" // Type* kullanılıyorsa dahil edilmeli

#include <iostream> // Debug için

namespace dppc {
namespace sema {

// Yardımcı fonksiyonlar implementasyonu
const char* SymbolTable::symbolKindToString(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::Variable: return "Variable";
        case SymbolKind::Function: return "Function";
        case SymbolKind::Type: return "Type";
        case SymbolKind::Struct: return "Struct";
        case SymbolKind::Enum: return "Enum";
        case SymbolKind::EnumVariant: return "EnumVariant";
        case SymbolKind::Trait: return "Trait";
        case SymbolKind::Parameter: return "Parameter";
        case SymbolKind::Field: return "Field";
        case SymbolKind::Method: return "Method";
        // ... diğer türler
        default: return "UnknownKind";
    }
}

const char* SymbolTable::ownershipStateToString(OwnershipState state) {
    switch (state) {
        case OwnershipState::Owned: return "Owned";
        case OwnershipState::Moved: return "Moved";
        case OwnershipState::BorrowedShared: return "BorrowedShared";
        case OwnershipState::BorrowedMutable: return "BorrowedMutable";
        case OwnershipState::ImmutableBorrowed: return "ImmutableBorrowed";
        case OwnershipState::Dropped: return "Dropped";
        case OwnershipState::Uninitialized: return "Uninitialized";
        default: return "UnknownState";
    }
}


// SymbolTable Constructor
SymbolTable::SymbolTable(/*ErrorReporter* reporter*/)
    : GlobalScope(nullptr), CurrentScope(nullptr)/*, Reporter(reporter)*/
{
    // Global kapsamı oluştur ve aktif kapsam yap
    GlobalScope = enterScope("global");
}

// SymbolTable Yıkıcı
SymbolTable::~SymbolTable() {
    // AllScopes vektörü, unique_ptr'lar sayesinde otomatik olarak tüm Scope nesnelerini temizler.
    // Sadece GlobalScope ve CurrentScope pointer'larını sıfırlayabiliriz.
    GlobalScope = nullptr;
    CurrentScope = nullptr;
}

// Yeni bir kapsam oluşturur ve aktif kapsam yapar.
Scope* SymbolTable::enterScope(const std::string& name) {
    // Yeni bir Scope nesnesi oluştur ve unique_ptr ile sahipliğini al
    auto newScope = std::make_unique<Scope>(CurrentScope, name);
    // Scope pointer'ını kaydetmeden önce unique_ptr'dan raw pointer al
    Scope* newScopePtr = newScope.get();
    // unique_ptr'ı vektöre taşı (sahiplik artık vektörde)
    AllScopes.push_back(std::move(newScope));

    // Yeni kapsamı aktif yap
    CurrentScope = newScopePtr;
     std::cerr << "DEBUG: Entered scope '" << name << "' (Total scopes: " << AllScopes.size() << ")" << std::endl; // Debug
    return CurrentScope;
}

// Aktif kapsamdan çıkar ve üst kapsamı aktif yapar.
Scope* SymbolTable::exitScope() {
    if (!CurrentScope || !CurrentScope->Parent) {
        // Global kapsamdan çıkmaya çalışmak bir hata olabilir
        // Veya main.cpp'deki son exitScope çağrısıdır.
         Reporter->reportError(SourceLocation(), "Global kapsamdan çıkmaya çalışılıyor.");
          std::cerr << "DEBUG: Attempted to exit global scope." << std::endl; // Debug
        return nullptr; // Global kapsamdan çıkılamaz veya zaten çıkıldı
    }

    // Üst kapsamı aktif yap
    Scope* oldScope = CurrentScope;
    CurrentScope = CurrentScope->Parent;
     // std::cerr << "DEBUG: Exited scope '" << oldScope->Name << "', back to '" << (CurrentScope ? CurrentScope->Name : "null") << "'" << std::endl; // Debug

    // Not: Çıkılan kapsam (oldScope) hala AllScopes vektöründe durmaktadır.
    // Eğer kapsamlar iş bittiğinde otomatik silinecekse, bu yeterlidir.
    // Eğer scope nesnelerinin kendisini silmek gerekiyorsa, AllScopes vektöründen unique_ptr'ı bulup çıkarmak gerekir, bu daha karmaşıktır.
    // std::vector<std::unique_ptr<Scope>> yapısı, SymbolTable yıkıldığında tüm scope'ları otomatik temizler.
    // Bu yeterlidir.

    return CurrentScope; // Yeni aktif kapsamı döndür
}

// Şu anki kapsama yeni bir sembol ekler.
Symbol* SymbolTable::addSymbol(const std::string& name, SymbolKind kind, Type* type, bool isMutable, const SourceLocation& loc, ast::ASTNode* declNode) {
    if (!CurrentScope) {
         Reporter->reportError(loc, "Sembol eklenemiyor: Aktif kapsam yok.");
        std::cerr << "ERROR: Cannot add symbol '" << name << "': No active scope." << std::endl; // Debug
        return nullptr; // Aktif kapsam yok
    }

    // Aynı isimde başka bir sembol zaten bu kapsamda varsa ekleme
    if (CurrentScope->lookup(name)) {
        // Bu hata Sema sınıfında raporlanmalı ve burada sadece eklemenin başarısız olduğu belirtilmeli.
         Reporter->reportError(loc, "'" + name + "' ismi zaten bu kapsamda tanımlanmış.");
         std::cerr << "DEBUG: Symbol '" << name << "' already exists in current scope." << std::endl; // Debug
        return nullptr; // Zaten var, eklenmedi
    }

    // Yeni Symbol nesnesi oluştur
    Symbol newSymbol(name, kind, type, isMutable, loc, declNode);

    // Sembolü mevcut kapsama ekle (Scope struct'ının addSymbol metodunu kullan)
     std::cerr << "DEBUG: Adding symbol '" << name << "' to scope '" << CurrentScope->Name << "'" << std::endl; // Debug
    auto [it, inserted] = CurrentScope->Symbols.emplace(name, newSymbol);

    if (inserted) {
        return &it->second; // Başarıyla eklenen sembolün pointer'ını döndür
    } else {
        // Bu durum yukarıdaki kontrolden sonra teorik olarak olmamalı, ama defensive programming.
         Reporter->reportError(loc, "Sembol eklenemedi: Dahili hata veya çift isim kontrolü başarısız.");
         std::cerr << "ERROR: Failed to add symbol '" << name << "' to scope '" << CurrentScope->Name << "' (unexpected)." << std::endl; // Debug
        return nullptr;
    }
}

// Sembolü şu anki kapsamdan başlayarak üst kapsamlara doğru arar.
Symbol* SymbolTable::lookup(const std::string& name) {
    Scope* scope = CurrentScope;
    while (scope) {
        if (Symbol* found = scope->lookup(name)) {
            return found; // Sembol bulundu
        }
        scope = scope->Parent; // Üst kapsama geç
    }
    return nullptr; // Sembol hiçbir kapsamda bulunamadı
}

// Sembolü sadece şu anki kapsamda arar.
Symbol* SymbolTable::lookupInCurrentScope(const std::string& name) {
    if (!CurrentScope) {
          std::cerr << "ERROR: Cannot lookup in current scope: No active scope." << std::endl; // Debug
        return nullptr;
    }
    return CurrentScope->lookup(name);
}


} // namespace sema
} // namespace dppc

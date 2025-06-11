#ifndef DPP_COMPILER_SEMA_SYMBOLTABLE_H
#define DPP_COMPILER_SEMA_SYMBOLTABLE_H

#include "../lexer/Token.h" // SourceLocation için
#include "TypeSystem.h"     // Type* için (TypeSystem.h dosyasının var olduğu varsayılır)
#include "../parser/AST.h" // ASTNode* için (İleri bildirim)

#include <string>
#include <vector>
#include <unordered_map> // Hızlı arama için hash map
#include <memory>        // std::unique_ptr için
#include <stdexcept>     // Hata yönetimi için

// İleri bildirimler
namespace dppc::ast { struct ASTNode; }
namespace dppc::sema { class Type; } // TypeSystem.h'dan gelecek Type base sınıfı

namespace dppc {
namespace sema {

// Sembolün Türleri
enum class SymbolKind {
    Variable,       // Değişken (lokal, global, parametre)
    Function,       // Fonksiyon
    Type,           // Tip (primitive, struct, enum, class, interface, alias)
    Struct,         // Yapı bildirimi
    Enum,           // Enum bildirimi
    EnumVariant,    // Enum varyantı (Match pattern'larında kullanılabilir)
    Trait,          // Trait bildirimi (D++'a özgü)
     Impl,        // Impl blokları genellikle Symbol olarak saklanmaz, Type'a bağlanır
    Parameter,      // Fonksiyon parametresi
    Field,          // Yapı/Sınıf/Union alanı
    Method,         // Metod (FunctionKind'ın bir alt türü veya ayrı)
     Module,      // Modül
     Constant,    // Sabit
     GenericParameter, // Jenerik tip parametresi
};

// Sembolün D++ Sahiplik Durumu
// Bir sembolün (değişken, parametre vb.) bellek üzerindeki durumu.
enum class OwnershipState {
    Owned,             // Sembol değeri kendi bellek alanına sahiptir.
    Moved,             // Sembol değeri başka yere taşındı, artık kullanılamaz.
    BorrowedShared,    // Sembol değeri shared borrow (&) tarafından kullanılıyor. Birden fazla olabilir.
    BorrowedMutable,   // Sembol değeri mutable borrow (&mut) tarafından kullanılıyor. Sadece bir tane olabilir.
    ImmutableBorrowed, // Sembol değeri immutable bir değişkenden shared borrow edildi (&). Asıl değişken immutable ise bu durum.
    Dropped,           // Sembol değeri kapsam dışına çıktı ve destruct edildi (destructor çağrıldı).
    Uninitialized,     // Sembol tanımlandı ama henüz değer atanmadı.
};


// Tek Bir Sembolün Bilgileri
struct Symbol {
    std::string Name;          // Sembolün adı
    SymbolKind Kind;           // Sembolün türü
    Type* type;                // Sembolün çözümlenmiş tipi (TypeSystem'dan bir pointer)
    bool isMutable;            // Değişken/Referans mutable mi?
    SourceLocation loc;        // Bildirildiği kaynak konumu
    OwnershipState state;      // D++ sahiplik durumu
    int borrowCount;           // Shared borrow sayısı (state == BorrowedShared iken geçerli)
    ast::ASTNode* declarationNode; // Bu sembolü tanımlayan AST düğümü (Pointer, sahiplik yok)
    // TODO: Lifetime bilgisi eklenebilir (Borrow'lar için)

    Symbol(std::string name, SymbolKind kind, Type* symType, bool mut, SourceLocation l, ast::ASTNode* declNode = nullptr)
        : Name(std::move(name)), Kind(kind), type(symType), isMutable(mut), loc(std::move(l)),
          state(OwnershipState::Uninitialized), // Başlangıçta uninitialized
          borrowCount(0),
          declarationNode(declNode) {}

    // Varsayılan kopyalama ve atama geçerli olmalı (pointer sahiplikleri SymbolTable tarafından yönetiliyor)
};


// Bir Kapsam (Scope)
struct Scope {
    std::unordered_map<std::string, Symbol> Symbols; // Bu kapsamda tanımlanan semboller
    Scope* Parent;                                  // Üst kapsamın pointer'ı (global scope için nullptr)
    std::string Name;                               // Kapsamın adı (debug için, örn: "global", "function: main", "block")

    Scope(Scope* parent = nullptr, std::string name = "")
        : Parent(parent), Name(std::move(name)) {}

    // Kopya ve atama operatörlerini sil (Scope'lar pointer tabanlı ağaç yapısında)
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    // Yıkıcı: Alt sembollerin std::string ve diğer üyeleri otomatik temizlenir
    ~Scope() = default;

    // Bu kapsamda sembol arar (üst kapsama bakmaz)
    Symbol* lookup(const std::string& name) {
        auto it = Symbols.find(name);
        if (it != Symbols.end()) {
            return &it->second; // Sembol bulundu, pointer'ını döndür
        }
        return nullptr; // Bulunamadı
    }

    // Bu kapsamda sembol ekler (kopya alır)
    // İsim zaten varsa false döner
    bool addSymbol(const Symbol& symbol) {
         // Emplace ile eklemeye çalış, isim zaten varsa eklenmez
        auto [it, inserted] = Symbols.emplace(symbol.Name, symbol);
        return inserted; // Başarıyla eklendiyse true, isim zaten varsa false
    }
};


// Sembol Tablosu Yöneticisi
class SymbolTable {
private:
    // Tüm kapsamları tutan vektör (sahiplik SymbolTable'da)
    // Bu vektör, kapsamların yaşam süresini yönetir.
    std::vector<std::unique_ptr<Scope>> AllScopes;
    Scope* GlobalScope;  // Global kapsam pointer'ı
    Scope* CurrentScope; // Şu anki aktif kapsam pointer'ı

    // Hata raporlama için (isteğe bağlı, Sema sınıfı da hata raporlayabilir)
     ErrorReporter* Reporter;


public:
    // Constructor
    SymbolTable(/*ErrorReporter* reporter = nullptr*/);

    // Yıkıcı: Tüm kapsamları temizler
    ~SymbolTable();

    // Kapsam Yönetimi
    // Yeni bir kapsam oluşturur ve aktif kapsam yapar.
    Scope* enterScope(const std::string& name = "");
    // Aktif kapsamdan çıkar ve üst kapsamı aktif yapar.
    // Çıkılan kapsamın pointer'ını döndürür (isteğe bağlı, eğer dışarıda işlenecekse)
    Scope* exitScope();
    // Şu anki aktif kapsamı döndürür
    Scope* getCurrentScope() const { return CurrentScope; }
    // Global kapsamı döndürür
    Scope* getGlobalScope() const { return GlobalScope; }


    // Sembol Yönetimi
    // Şu anki kapsama yeni bir sembol ekler.
    // İsim zaten o kapsamda varsa hata (veya false dönüş) vermelidir.
    // Symbol nesnesi burada oluşturulur ve kapsama kopyalanır/taşınır.
    // isMutable ve state varsayılan değerlerle başlatılabilir.
    // Döndürülen pointer, kapsama eklenen Symbol'ün kendisidir.
    Symbol* addSymbol(const std::string& name, SymbolKind kind, Type* type, bool isMutable, const SourceLocation& loc, ast::ASTNode* declNode = nullptr);

    // Sembolü şu anki kapsamdan başlayarak üst kapsamlara doğru arar.
    // Bulursa sembolün pointer'ını, bulamazsa nullptr döndürür.
    Symbol* lookup(const std::string& name);

    // Sembolü sadece şu anki kapsamda arar.
    // Bulursa sembolün pointer'ını, bulamazsa nullptr döndürür.
    Symbol* lookupInCurrentScope(const std::string& name);

    // Yardımcı fonksiyon: SymbolKind'ı stringe çevirme
    static const char* symbolKindToString(SymbolKind kind);
    // Yardımcı fonksiyon: OwnershipState'i stringe çevirme
    static const char* ownershipStateToString(OwnershipState state);
};

} // namespace sema
} // namespace dppc

#endif // DPP_COMPILER_SEMA_SYMBOLTABLE_H

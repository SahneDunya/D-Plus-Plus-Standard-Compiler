module symbol_table;

import std.stdio;
import std.array;
import dpp_types;     // D++ tür temsilcileri
import syntax_kind;   // Sembol türleri (SyntaxKind enum'ı)
import ast;           // Sembolün bildirildiği AST düğümüne referans için (isteğe bağlı)
import error_reporting; // Hata raporlama için

// Sembol tablosu girişi için yapı
struct Symbol {
    string name; // Sembolün adı
    DppType type; // Sembolün türü (değişken tipi, fonksiyon dönüş tipi vb.)
    SyntaxKind kind; // Sembolün türü (Variable, Function, Type vb.)

    // Semantik analiz veya backend için ek bilgiler
     ASTNode* declarationNode; // Sembolün bildirildiği AST düğümüne referans
     int offset; // Değişkenler için stack veya data segmentindeki offset bilgisi (backend için)
    bool isMutable; // Değişkenler için değiştirilebilir mi?
     bool isInitialized; // Değişken başlangıçta değer aldı mı?
}

// Tek bir kapsamdaki sembolleri tutan sembol tablosu
class SymbolTable {
    private Symbol[] symbols; // Bu kapsamdaki semboller
     SymbolTable* parent; // Üst kapsam bilgisi burada TUTULMAZ, ScopeManager'da yönetilir.

    this() {
        this.symbols = [];
    }

    // Bu sembol tablosuna yeni bir sembol ekler
    // Aynı isimde sembol varsa hata döndürür veya raporlar.
    bool addSymbol(Symbol symbol) {
        // Mevcut kapsamda aynı isimde sembol var mı kontrolü
        if (lookup(symbol.name)) { // Sadece mevcut tabloda ara
            stderr.writeln("Hata: Sembol '", symbol.name, "' bu kapsamda zaten mevcut.");
            // error_reporting.reportError(...);
            return false; // Ekleme başarısız
        }
        symbols ~= symbol;
        writeln("Sembol eklendi (tabloya): ", symbol.name, " (", symbol.type.name, ")");
        return true; // Ekleme başarılı
    }

    // Bu sembol tablosunda bir sembolü adına göre arar
    // Sadece bu tablonun içini kontrol eder.
    Symbol* lookup(string name) {
        foreach (ref symbol; symbols) {
            if (symbol.name == name) {
                return &symbol;
            }
        }
        return null; // Sembol bu tabloda bulunamadı
    }

    // Bu tablodaki tüm sembolleri döndürür (debugging veya başka amaçlar için)
    const(Symbol)[] getAllSymbols() const {
        return symbols;
    }

    // Sembol tablosunu temizleyen fonksiyon (eğer D'nin GC'si kullanılmıyorsa ve semboller pointer içeriyorsa)
     void destroy() {
         // Sembollerin içerdiği pointerları temizle (varsa)
         // symbols dizisi için belleği serbest bırak (D'nin dizileri genellikle GC'lidir)
     }
}
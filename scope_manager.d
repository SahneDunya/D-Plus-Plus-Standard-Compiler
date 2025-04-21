module scope_manager;

import std.stdio;
import std.array;
import symbol_table; // Symbol ve SymbolTable yapıları için
import error_reporting; // Hata raporlama için

// Kapsamları ve sembol tablolarını yöneten sınıf
class ScopeManager {
    private SymbolTable[] scopeStack; // SymbolTable pointerlarının stack'i

    this() {
        this.scopeStack = [];
        // Başlangıçta global kapsamı otomatik olarak oluştur
        enterScope();
    }

    // Yeni bir kapsam girildiğinde çağrılır
    // Stack'e yeni bir SymbolTable ekler.
    void enterScope() {
        SymbolTable* newScopeTable = new SymbolTable();
        scopeStack ~= newScopeTable;
        writeln("Kapsam girildi. Mevcut kapsam derinliği: ", scopeStack.length);
    }

    // Mevcut kapsamdan çıkıldığında çağrılır
    // Stack'ten en üstteki SymbolTable'ı çıkarır.
    void exitScope() {
        if (scopeStack.length <= 1) {
            // Hata: Global kapsamdan çıkılmaya çalışılıyor
            stderr.writeln("Hata: Global kapsamdan çıkılmaya çalışılıyor.");
             error_reporting.reportError(...);
            return;
        }
        SymbolTable* exitedScope = scopeStack.popBack();
        // Çıkan kapsamın sembol tablosunu temizle (eğer D'nin GC'si kullanılmıyorsa)
         exitedScope.destroy(); // SymbolTable'ın destroy fonksiyonunu çağır
        destroy(exitedScope); // Eğer SymbolTable class ve GC kullanıyorsak
        writeln("Kapsamdan çıkıldı. Mevcut kapsam derinliği: ", scopeStack.length);
    }

    // Mevcut (en üstteki) kapsamın sembol tablosunu döndürür
    SymbolTable* getCurrentScope() {
        if (scopeStack.length == 0) {
            // Bu durum normalde olmamalı, programın en az bir (global) kapsamı olmalı.
            stderr.writeln("Hata: Geçerli kapsam bulunamadı.");
             error_reporting.reportError(...);
            return null;
        }
        return scopeStack.back; // Stack'in en üstündeki SymbolTable pointer'ını döndür
    }

    // Mevcut kapsama yeni bir sembol ekler
    bool addSymbol(Symbol symbol) {
        SymbolTable* current = getCurrentScope();
        if (current) {
            return current.addSymbol(symbol); // SymbolTable'ın addSymbol metodunu çağır
        }
        return false; // Kapsam yoksa eklenemedi
    }

    // Bir sembolü mevcut kapsamdan başlayarak üst kapsamlara doğru arar
    // İsim çözümleme işlemi burada gerçekleşir.
    Symbol* resolveSymbol(string name) {
        // Mevcut kapsamdan başlayarak üst kapsamlara doğru dolaş
        for (int i = scopeStack.length - 1; i >= 0; --i) {
            SymbolTable* currentTable = scopeStack[i];
            Symbol* symbol = currentTable.lookup(name); // Sadece bu tabloda ara
            if (symbol) {
                // Sembol bulundu
                writeln("Sembol çözüldü: '", name, "' bulundu.");
                return symbol;
            }
        }

        // Sembol hiçbir kapsamda bulunamadı
        writeln("Sembol çözülemedi: '", name, "' bulunamadı.");
        return null;
    }

    // ScopeManager temizliği (eğer D'nin GC'si kullanılmıyorsa)
     void destroy() {
    //     // Stackteki tüm SymbolTable pointerlarını sil
         while (scopeStack.length > 0) {
             SymbolTable* table = scopeStack.popBack();
              table.destroy(); // SymbolTable'ın destroy fonksiyonunu çağır
             destroy(table); // Eğer SymbolTable class ve GC kullanıyorsak
         }
     }
}
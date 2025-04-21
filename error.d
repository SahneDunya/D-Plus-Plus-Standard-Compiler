module core.error;

import std.string; // string için
import std.stdio;    // writeln için (debugging)

// Tüm hata tiplerinin implemente etmesi beklenen temel hata interface'i (trait)
// Rust'taki std::error::Error trait'ine benzer.
// D dilinde interface'ler trait olarak kullanılabilir.
interface Error {
    // Hatanın kısa, insan tarafından okunabilir açıklamasını döndürür
    string description() const;

    // Hatanın nedenini (bir başka hata nesnesi) döndürür (isteğe bağlı)
    // Bu, hata zincirlerini oluşturmak için kullanılır.
     Error* cause() const { return null; } // Varsayılan implementasyon
}

// Basit bir örnek hata türü (core içinde kullanılabilir)
// Bu struct Error interface'ini implement ediyor.
struct SimpleError : Error {
    string message; // Hatanın mesajı

    // SimpleError oluşturan constructor
    this(string msg) {
        this.message = msg;
    }

    // Error interface'inin description metodunu implement et
    override string description() const {
        return message;
    }

    // toString metodu (debugging için)
    string toString() const {
        return format("SimpleError: %s", message);
    }
}

// Diğer yaygın hata türleri burada tanımlanabilir (örneğin, I/O hataları, ayrıştırma hataları vb.)
// veya ilgili modüller içinde tanımlanıp buradan re-export edilebilir.
 import io.error : IOError; // io modülündeki hata tipi

// Örnek: I/O işlemleri için hata türü

enum IOErrorKind {
    NotFound,
    PermissionDenied,
    ConnectionRefused,
    // ... Diğer I/O hata türleri
}

struct IOError : Error {
    IOErrorKind kind;
    string details; // Ek detaylar

    this(IOErrorKind kind, string details = "") {
        this.kind = kind;
        this.details = details;
    }

    override string description() const {
        return format("IOError(%s): %s", kind.to!string, details);
    }

    string toString() const {
        return description();
    }
}


// Error interface'ini implement eden herhangi bir türü temsil etmek için
// bir takma ad veya şablon (template) kullanılabilir.
// alias AnyError = Error; // Error interface'ini implement eden herhangi bir sınıf/struct pointer'ı
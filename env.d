module env;

import std.stdio;    // writeln için
import std.string;   // string işlemleri için
import std.array;    // Dizi işlemleri için
import core.option;  // Option<T> için
import core.error; // Hata tipleri için
// D'nin std.process modülü veya C environment API'leri kullanılabilir.
import std.process; // D'nin std.process modülünü kullanalım

// Belirtilen isimdeki ortam değişkeninin değerini döndürür.
// Değişken yoksa None döndürür.
Option!string var(string key) {
    try {
        // D'nin environment.get metodu kullanılabilir.
        // D'nin environment modülü std.process altındadır.
        string value = std.process.environment.get(key);
        return Option!string.Some(value);
    } catch (RangeError e) {
        // Ortam değişkeni yoksa RangeError fırlatılır (D'nin davranışı).
        return Option!string.None();
    } catch (Exception e) {
        // Diğer hatalar (izin vb.)
         stderr.writeln("Hata (Env): Ortam değişkeni okunurken beklenmeyen hata '", key, "': ", e.msg);
         error_reporting.reportError(...);
        // Bu durumda da None döndürebiliriz veya Result<Option!string, EnvError> yapabiliriz.
        return Option!string.None(); // Basitlik için None
    }
}

// Tüm ortam değişkenlerini (anahtar-değer çiftleri) döndüren bir iterator veya koleksiyon
// Map!(string, string) veya (string, string) tuple dizisi döndürebilir.
string[string] vars() {
    // D'nin environment.byRange() iteratoru veya .toHashMap() metodu kullanılabilir.
    return std.process.environment.toHashMap(); // D'nin toHashMap()
}

// Komut satırı argümanlarını döndüren dizi (ilk eleman genellikle programın adı)
string[] args() {
    // D'nin std.process.args array'i kullanılabilir.
    return std.process.args;
}

// ... Diğer ortam/argüman yönetimi fonksiyonları (setVar, unsetVar vb.)
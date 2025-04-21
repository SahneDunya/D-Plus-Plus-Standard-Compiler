module io.path;

import std.path;     // D'nin path modülü için
import std.string;   // string işlemleri için
import std.array;    // dizi işlemleri için
import core.result;  // Result<T, E> için
import core.error;   // Temel hata tipleri için (IOError kullanılabilir)
import core.option;  // Option<T> için


// Dosya sistemi yolunu temsil eden yapı
// Platforma özel formatı (örn: '/' vs '\') içerebilir veya normalize edilmiş bir temsil kullanabilir.
// D'nin std.path.PathString yapısı veya basitçe string kullanılabilir.
struct Path {
    private string inner; // Yolun string temsilini saklayalım

    // Yeni bir Path nesnesi oluşturur
    this(string path) {
        // Yol stringini normalize edebilirsiniz (platforma göre ayırma karakterlerini düzeltme vb.)
        // Basitlik için şimdilik sadece stringi saklayalım.
        this.inner = path;
        this.inner = std.path.canonicalize(path); veya platforma göre normalize et.
    }

    // Yolun string temsilini döndürür
    string toString() const {
        return inner;
    }

    // Yolun mutlak olup olmadığını kontrol eder
    bool isAbsolute() const {
        return std.path.isAbsolute(inner); // D'nin isAbsolute fonksiyonunu kullan
    }

    // Yolun göreceli olup olmadığını kontrol eder
    bool isRelative() const {
        return std.path.isRelative(inner); // D'nin isRelative fonksiyonunu kullan
    }

    // İki yol parçasını birleştirerek yeni bir Path oluşturur
    // result.join("to", "file.txt") -> "result/to/file.txt" (Linux/macOS) veya "result\to\file.txt" (Windows)
    Path join(Args...)(Args args) const {
        string[] components = [inner];
        foreach (arg; args) {
            components ~= text(arg); // Argümanları stringe çevir
        }
        // D'nin std.path.join fonksiyonunu kullan
        return Path(std.path.join(components));
    }

    // Yolun son bileşenini (dosya veya dizin adı) döndürür. Kök dizin için boş string dönebilir.
    Option!string fileName() const {
        string name = std.path.base(inner); // D'nin base fonksiyonu son bileşeni verir
        if (name.empty && inner != std.path.root) { // Eğer boşsa ve kök dizin değilse (hata durumu?)
            // Bu durum, yolun '/' veya '\' gibi ayırıcılarla bitmesi durumunda olabilir.
             return Option!string.None(); // Belki None döndürmek daha anlamlı
        }
         return Option!string.Some(name);
    }

    // Yolun dosya uzantısını döndürür. Yoksa None döndürür.
    Option!string extension() const {
        string ext = std.path.extension(inner); // D'nin extension fonksiyonunu kullan
        if (ext.empty) {
            return Option!string.None(); // Uzantı yoksa None
        }
        // D'nin extension fonksiyonu "." dahil uzantıyı verir. "." karakterini kaldırmak isteyebilirsiniz.
        return Option!string.Some(ext.chompLeft(".")); // Varsayım: chompLeft() fonksiyonu var
    }

    // Yolun bulunduğu üst dizini döndürür. Kök dizin için None dönebilir.
    Option!Path parent() const {
        string dir = std.path.dir(inner); // D'nin dir fonksiyonu üst dizini verir
        if (dir.empty) {
            // Bu durum ya kök dizin için ya da geçersiz bir yol için olabilir.
            return Option!Path.None();
        }
        return Option!Path.Some(Path(dir));
    }

    // Yolun var olup olmadığını kontrol etmek için io.file_io modülünü kullanabiliriz.

    bool exists() const {
        return io.file_io.fileExists(inner);
    }

    bool isFile() const {
         return io.file_io.isRegularFile(inner);
     }

    bool isDir() const {
         return io.file_io.isDirectory(inner);
     }

    // Yol normalizasyonu veya canonicalize işlemleri
    
    Result!(Path, IOError) canonicalize() const {
        try {
            string canonicalPath = std.path.canonicalize(inner); // D'nin canonicalize fonksiyonunu kullan
            return Result!(Path, IOError).Ok(Path(canonicalPath));
        } catch (Exception e) {
            // D'nin canonicalize hatalarını IOError'a çevir.
             stderr.writeln("Hata (Path): Canonicalize hatası '", inner, "': ", e.msg);
            // error_reporting.reportError(...);
             return Result!(Path, IOError).Err(IOError(IOError.Kind.OtherError, format("Canonicalize hatası '%s': %s", inner, e.msg)));
        }
    }

    // Diğer yol manipülasyon fonksiyonları (absolutize, relativeTo, normalize, startsWith, endsWith vb.)
}
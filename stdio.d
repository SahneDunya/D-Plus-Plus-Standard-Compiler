module io.stdio;

import io; // io.d'deki traitler ve IOError için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import std.stdio : stdin, stdout, stderr, File, FileMode; // D'nin standart akışları için
import std.string; // String işlemleri için
import std.conv;   // Tip dönüşümleri için
import std.array;  // Dizi işlemleri için

// D++'ın standart girdi akışı (stdin)
// io.Read trait'ini implemente edecek bir nesne
class StandardInput : Read {
    private File delegateFile; // D'nin stdin nesnesini tutalım

    this() {
        // D'nin stdin'i File türündedir ve Read trait'ini implemente eder gibi davranabiliriz.
        // Ancak D'nin File.readf/readText gibi yüksek seviye fonksiyonlarını doğrudan kullanmak daha kolay olabilir.
        // D'nin stdin nesnesine doğrudan erişim
        this.delegateFile = stdin;
    }

    // io.Read trait'ini implemente et
    override Result!(size_t, IOError) read(ubyte[] buffer) {
        try {
            // D'nin File.read fonksiyonunu kullanalım
            size_t bytesRead = delegateFile.read(buffer);
            return Result!(size_t, IOError).Ok(bytesRead);
        } catch (Exception e) { // D'nin I/O hataları genellikle Exception fırlatır
            stderr.writeln("Hata (Stdio): stdin okuma hatası: ", e.msg);
            // Exception'ı bir IOError'a çevirmemiz gerekir.
            return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg)); // Basit çeviri
        }
    }

    override Result!(void, IOError) readExact(ubyte[] buffer) {
        try {
            delegateFile.readExact(buffer); // D'nin readExact'i var
            return Result!(void, IOError).Ok(void); // () döndür
        } catch (Exception e) {
             stderr.writeln("Hata (Stdio): stdin tam okuma hatası: ", e.msg);
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }

    // ... Seek trait'ini implemente etmeyebilir (stdin genellikle konumlanamaz)
}

// D++'ın standart çıktı akışı (stdout)
// io.Write trait'ini implemente edecek bir nesne
class StandardOutput : Write {
     private File delegateFile; // D'nin stdout nesnesini tutalım

     this() {
         this.delegateFile = stdout;
     }

    // io.Write trait'ini implemente et
    override Result!(size_t, IOError) write(const(ubyte)[] buffer) {
        try {
            // D'nin File.write fonksiyonunu kullanalım
            size_t bytesWritten = delegateFile.write(buffer);
            return Result!(size_t, IOError).Ok(bytesWritten);
        } catch (Exception e) {
            stderr.writeln("Hata (Stdio): stdout yazma hatası: ", e.msg);
            return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(void, IOError) writeAll(const(ubyte)[] buffer) {
        try {
            delegateFile.write(buffer); // D'nin writeAll gibi bir fonksiyonu yok, write kullanıp kendimiz implemente edebiliriz.
            // Veya D'nin writeln/write gibi metin fonksiyonlarını kullanırız.
            // Eğer write(buffer) tüm buffer'ı yazmayı garanti etmiyorsa, döngü içinde yazmalıyız.
            return Result!(void, IOError).Ok(void);
        } catch (Exception e) {
             stderr.writeln("Hata (Stdio): stdout tam yazma hatası: ", e.msg);
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }

    override Result!(void, IOError) flush() {
        try {
            delegateFile.flush(); // D'nin File.flush'ı var
            return Result!(void, IOError).Ok(void);
        } catch (Exception e) {
             stderr.writeln("Hata (Stdio): stdout flush hatası: ", e.msg);
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }
}

// D++'ın standart hata akışı (stderr)
// io.Write trait'ini implemente edecek bir nesne
class StandardError : Write {
     private File delegateFile; // D'nin stderr nesnesini tutalım

     this() {
         this.delegateFile = stderr;
     }

    // io.Write trait'ini implemente et (StandardOutput ile benzer)
    override Result!(size_t, IOError) write(const(ubyte)[] buffer) {
        try {
            size_t bytesWritten = delegateFile.write(buffer);
            return Result!(size_t, IOError).Ok(bytesWritten);
        } catch (Exception e) {
            // stderr'e yazarken hata oluşursa ne yapmalı? Çok temel bir durum.
            // Belki de sadece bir loglama mekanizması kullanmak.
             stderr.writeln("KRİTİK HATA: stderr yazma hatası: ", e.msg);
             return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(void, IOError) writeAll(const(ubyte)[] buffer) {
         try {
            delegateFile.write(buffer);
            return Result!(void, IOError).Ok(void);
        } catch (Exception e) {
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }

    override Result!(void, IOError) flush() {
        try {
            delegateFile.flush();
            return Result!(void, IOError).Ok(void);
        } catch (Exception e) {
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }
}


// Standart akış nesneleri (Singleton gibi)
private StandardInput std_in_instance;
private StandardOutput std_out_instance;
private StandardError std_err_instance;

StandardInput* stdin() {
    if (!std_in_instance) std_in_instance = new StandardInput();
    return std_in_instance;
}

StandardOutput* stdout() {
    if (!std_out_instance) std_out_instance = new StandardOutput();
    return std_out_instance;
}

StandardError* stderr() {
    if (!std_err_instance) std_err_instance = new StandardError();
    return std_err_instance;
}


// Kullanıcı dostu yüksek seviye I/O fonksiyonları (String tabanlı)
// Bunlar yukarıdaki Read ve Write trait implementasyonlarını kullanacaktır.

// Konsola metin yazdırır (yeni satır ekler)
void println(Args...)(Args args) {
    // Argümanları stringe çevir ve birleştir
    string outputString = format(args); // varsayım: format fonksiyonu var
    // Stringi byte dizisine çevir (UTF-8 varsayalım)
    ubyte[] buffer = cast(ubyte[])outputString;

    // stdout akışını kullan
    auto writeResult = stdout().writeAll(buffer); // Tüm byte'ları yazmaya çalış
    // Hata durumunda ne yapmalı? Genellikle standart akış hataları kurtarılamaz.
    if (writeResult.isErr()) {
        // stderr'e hata mesajı yazmak mantıklı değil, belki loglama veya panic.
         stderr.writeln("stdout println hatası!");
    }
    // Yeni satır karakteri ekle
    stdout().writeAll(cast(ubyte[])"\n");
    stdout().flush(); // Tamponu boşalt
}

// Konsola metin yazdırır (yeni satır eklemez)
void print(Args...)(Args args) {
    string outputString = format(args);
    ubyte[] buffer = cast(ubyte[])outputString;
    auto writeResult = stdout().writeAll(buffer);
    if (writeResult.isErr()) { /* ... */ }
    stdout().flush(); // Tamponu boşalt
}

// Standart hataya metin yazdırır (yeni satır ekler)
void eprintln(Args...)(Args args) {
     string outputString = format(args);
    ubyte[] buffer = cast(ubyte[])outputString;
    auto writeResult = stderr().writeAll(buffer);
     if (writeResult.isErr()) { /* ... */ } // stderr'e yazma hatası çok kritik olabilir.
    stderr().writeAll(cast(ubyte[])"\n");
    stderr().flush();
}

// Konsoldan bir satır metin okur. Hata veya EOF durumunda None döndürür.
Option!string readln() {
    // D'nin File.readln fonksiyonu gibi bir şey kullanacağız.
    // D'nin readln'i null dönebilir (EOF).
    string line = stdin().delegateFile.readln();
    if (line is null) {
        // EOF'a ulaşıldı veya hata oluştu
        return Option!string.None();
    }
    // Okunan satırın sonundaki newline karakterini kaldırmak gerekebilir.
    return Option!string.Some(line.chomp()); // chomp() sondaki newline'ı kaldırır (varsayım)
}

// ... Diğer standart I/O fonksiyonları (scanf, getChar vb.)
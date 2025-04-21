module io.file_io;

import io; // io.d'deki traitler ve IOError için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import std.stdio : File, FileMode, FileException; // D'nin File ve FileMode enum'ları için
import std.file : exists, isFile, isDir, remove, mkdir, mkdirRec, dirEntries, readText, writeText; // D'nin dosya işlemleri için
import std.string; // string için
import std.conv;   // Tip dönüşümleri için
import std.array;  // Dizi işlemleri için

// D++'ta bir açık dosyayı temsil eden yapı
// io.Read, io.Write ve io.Seek traitlerini implemente eder.
class DppFile : Read, Write, Seek {
    private File delegateFile; // D'nin stdio.File nesnesini tutalım

    // Constructor (yalnızca 'open' fonksiyonu tarafından çağrılmalı)
    private this(File delegateFile) {
        this.delegateFile = delegateFile;
    }

    // Bir dosyayı belirli bir modda açar. Başarılı durumda Ok(DppFile), hata durumunda Err(IOError) döndürür.
    static Result!(DppFile*, IOError) open(string filePath, FileMode mode) { // D'nin FileMode enum'ını kullanalım şimdilik
        writeln("Dosya açılıyor: ", filePath, " (Mod: ", mode.to!string, ")");
        try {
            // D'nin File sınıfını kullan
            File delegateFile = File(filePath, mode);
            writeln("Dosya başarıyla açıldı.");
            // Yeni DppFile nesnesini oluştur ve Ok içinde döndür
            return Result!(DppFile*, IOError).Ok(new DppFile(delegateFile));
        } catch (FileException e) {
            // D'nin FileException'ını IOError'a çevir
            IOError ioError = mapFileExceptionToIOError(e); // Aşağıda tanımlanacak yardımcı fonksiyon
            stderr.writeln("Hata (FileIO): Dosya açılamadı '", filePath, "': ", ioError.description());
            // error_reporting.reportError(...);
            return Result!(DppFile*, IOError).Err(ioError);
        } catch (Exception e) {
             stderr.writeln("Hata (FileIO): Dosya açılırken beklenmeyen hata '", filePath, "': ", e.msg);
             return Result!(DppFile*, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }

    // Açık dosyayı kapatır
    // Başarılı veya hata durumunu belirtebilir (Result<void, IOError> döndürmek iyi olur).
    Result!(void, IOError) close() {
        if (delegateFile) {
            writeln("Dosya kapatılıyor...");
            try {
                delegateFile.close();
                 writeln("Dosya başarıyla kapatıldı.");
                // Eğer DppFile class ve GC kullanıyorsak destroy'a gerek yok.
                // Eğer DppFile struct ve manuel new/destroy kullanıyorsak burada destroy(this) gerekebilir.
                return Result!(void, IOError).Ok(void);
            } catch (FileException e) {
                IOError ioError = mapFileExceptionToIOError(e);
                stderr.writeln("Hata (FileIO): Dosya kapatılırken hata: ", ioError.description());
                // error_reporting.reportError(...);
                return Result!(void, IOError).Err(ioError);
            } catch (Exception e) {
                 stderr.writeln("Hata (FileIO): Dosya kapatılırken beklenmeyen hata: ", e.msg);
                 return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
             }
        }
         return Result!(void, IOError).Ok(void); // Zaten kapatılmışsa başarı kabul edebiliriz.
    }

    // io.Read trait'ini implemente et
    override Result!(size_t, IOError) read(ubyte[] buffer) {
        try {
            size_t bytesRead = delegateFile.read(buffer);
            return Result!(size_t, IOError).Ok(bytesRead);
        } catch (FileException e) {
             return Result!(size_t, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(void, IOError) readExact(ubyte[] buffer) {
        try {
            delegateFile.readExact(buffer);
            return Result!(void, IOError).Ok(void);
        } catch (FileException e) {
             return Result!(void, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    // io.Write trait'ini implemente et
    override Result!(size_t, IOError) write(const(ubyte)[] buffer) {
        try {
            size_t bytesWritten = delegateFile.write(buffer);
            return Result!(size_t, IOError).Ok(bytesWritten);
        } catch (FileException e) {
             return Result!(size_t, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(void, IOError) writeAll(const(ubyte)[] buffer) {
        try {
            delegateFile.write(buffer); // D'nin write(buffer) genellikle tüm buffer'ı yazar
            return Result!(void, IOError).Ok(void);
        } catch (FileException e) {
             return Result!(void, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(void, IOError) flush() {
        try {
            delegateFile.flush();
            return Result!(void, IOError).Ok(void);
        } catch (FileException e) {
             return Result!(void, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    // io.Seek trait'ini implemente et
    override Result!(size_t, IOError) seek(Seek.SeekFrom from, long offset) {
        try {
            File.SeekBase seekBase;
            // D'nin SeekBase enum'ına çevir
            final switch (from) {
                case Seek.SeekFrom.Start: seekBase = File.SeekBase.set; break;
                case Seek.SeekFrom.End: seekBase = File.SeekBase.end; break;
                case Seek.SeekFrom.Current: seekBase = File.SeekBase.cur; break;
            }
            // D'nin File.seek fonksiyonunu kullan
            delegateFile.seek(offset, seekBase);
            // D'nin File.tellp() veya tellg() fonksiyonu yeni konumu verir (isteğe bağlı)
            return Result!(size_t, IOError).Ok(cast(size_t)delegateFile.tell()); // Varsayım: tell() var ve size_t döndürür
        } catch (FileException e) {
             return Result!(size_t, IOError).Err(mapFileExceptionToIOError(e));
        } catch (Exception e) {
             return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
        }
    }

    override Result!(size_t, IOError) currentPosition() {
         try {
            return Result!(size_t, IOError).Ok(cast(size_t)delegateFile.tell()); // Varsayım: tell() var
         } catch (FileException e) {
              return Result!(size_t, IOError).Err(mapFileExceptionToIOError(e));
         } catch (Exception e) {
              return Result!(size_t, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
         }
    }


    // D'nin FileException'ını D++'ın IOError'ına çeviren yardımcı fonksiyon
    private IOError mapFileExceptionToIOError(FileException e) const {
        // FileException'ın içerdiği bilgiye göre uygun IOError.Kind belirle
        // Bu, platforma özgü hata kodlarına bakmayı gerektirebilir.
        // Basitlik için çoğu FileException'ı OtherError olarak çevirelim.
        // Belirli FileException türleri için özel çeviriler yapılabilir (örneğin, FileNotFoundException -> IOError.Kind.NotFound).
        if (e.msg.startsWith("No such file or directory")) return IOError(IOError.Kind.NotFound, e.msg);
        if (e.msg.startsWith("Permission denied")) return IOError(IOError.Kind.PermissionDenied, e.msg);
        // ... Diğer özel durumlar
        return IOError(IOError.Kind.OtherError, e.msg);
    }

    // ... Diğer DppFile metotları (size, sync, lock vb.)

    // Manuel bellek yönetimi için destructor (Eğer DppFile class ve GC kullanmıyorsak)
    // Eğer DppFile struct ve manuel new/destroy kullanıyorsak gerekebilir.
     ~this() {
        close(); // Nesne temizlendiğinde dosyayı kapat
     }
}

// Dosya sistemi işlemleri için fonksiyonlar (Genellikle dosya yolu üzerinde çalışırlar)

// Dosyanın mevcut olup olmadığını kontrol eder
bool fileExists(string filePath) {
    return exists(filePath); // D'nin exists fonksiyonunu kullan
}

// Yolun bir dosya olup olmadığını kontrol eder
bool isRegularFile(string filePath) {
    return isFile(filePath); // D'nin isFile fonksiyonunu kullan
}

// Yolun bir dizin olup olmadığını kontrol eder
bool isDirectory(string filePath) {
    return isDir(filePath); // D'nin isDir fonksiyonunu kullan
}


// Bir dosyadan tüm içeriği metin olarak okur. Başarılı durumda Ok(string), hata durumunda Err(IOError) döndürür.
Result!(string, IOError) readAllText(string filePath) {
    writeln("Dosya okunuyor (metin): ", filePath);
    try {
        // D'nin readText fonksiyonunu kullan
        string content = readText(filePath);
        writeln("Dosya başarıyla okundu.");
        return Result!(string, IOError).Ok(content);
    } catch (FileException e) {
        IOError ioError = mapFileExceptionToIOError(e);
        stderr.writeln("Hata (FileIO): Dosya okunurken hata '", filePath, "': ", ioError.description());
        // error_reporting.reportError(...);
        return Result!(string, IOError).Err(ioError);
    } catch (Exception e) {
        stderr.writeln("Hata (FileIO): Dosya okunurken beklenmeyen hata '", filePath, "': ", e.msg);
        return Result!(string, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
    }
}

// Belirtilen içeriği bir dosyaya yazar. Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
Result!(void, IOError) writeAllText(string filePath, string content) {
     writeln("Dosyaya yazılıyor (metin): ", filePath);
    try {
        // D'nin writeText fonksiyonunu kullan
        writeText(filePath, content);
         writeln("Dosyaya başarıyla yazıldı.");
        return Result!(void, IOError).Ok(void);
    } catch (FileException e) {
         IOError ioError = mapFileExceptionToIOError(e);
         stderr.writeln("Hata (FileIO): Dosyaya yazılırken hata '", filePath, "': ", ioError.description());
        // error_reporting.reportError(...);
        return Result!(void, IOError).Err(ioError);
    } catch (Exception e) {
         stderr.writeln("Hata (FileIO): Dosyaya yazılırken beklenmeyen hata '", filePath, "': ", e.msg);
         return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
     }
}

// Dizin oluşturur. Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
Result!(void, IOError) createDirectory(string dirPath) {
     writeln("Dizin oluşturuluyor: ", dirPath);
    try {
        mkdir(dirPath); // D'nin mkdir fonksiyonunu kullan
         writeln("Dizin başarıyla oluşturuldu.");
        return Result!(void, IOError).Ok(void);
    } catch (FileException e) {
         IOError ioError = mapFileExceptionToIOError(e);
         stderr.writeln("Hata (FileIO): Dizin oluşturulurken hata '", dirPath, "': ", ioError.description());
        // error_reporting.reportError(...);
        return Result!(void, IOError).Err(ioError);
    } catch (Exception e) {
         stderr.writeln("Hata (FileIO): Dizin oluşturulurken beklenmeyen hata '", dirPath, "': ", e.msg);
         return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
     }
}

// Dizini ve üst dizinlerini recursive olarak oluşturur. Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
Result!(void, IOError) createDirectories(string dirPath) {
     writeln("Dizinler recursive oluşturuluyor: ", dirPath);
    try {
        mkdirRec(dirPath); // D'nin mkdirRec fonksiyonunu kullan
         writeln("Dizinler başarıyla oluşturuldu.");
        return Result!(void, IOError).Ok(void);
    } catch (FileException e) {
         IOError ioError = mapFileExceptionToIOError(e);
         stderr.writeln("Hata (FileIO): Dizinler oluşturulurken hata '", dirPath, "': ", ioError.description());
        // error_reporting.reportError(...);
        return Result!(void, IOError).Err(ioError);
    } catch (Exception e) {
         stderr.writeln("Hata (FileIO): Dizinler oluşturulurken beklenmeyen hata '", dirPath, "': ", e.msg);
         return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
     }
}


// Bir dosyayı veya boş dizini siler. Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
Result!(void, IOError) removePath(string path) {
     writeln("Dosya/Dizin siliniyor: ", path);
    try {
        remove(path); // D'nin remove fonksiyonunu kullan
         writeln("Dosya/Dizin başarıyla silindi.");
        return Result!(void, IOError).Ok(void);
    } catch (FileException e) {
         IOError ioError = mapFileExceptionToIOError(e);
         stderr.writeln("Hata (FileIO): Dosya/Dizin silinirken hata '", path, "': ", ioError.description());
        // error_reporting.reportError(...);
        return Result!(void, IOError).Err(ioError);
    } catch (Exception e) {
         stderr.writeln("Hata (FileIO): Dosya/Dizin silinirken beklenmeyen hata '", path, "': ", e.msg);
         return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
     }
}

// D'nin FileException'ını D++'ın IOError'ına çeviren yardımcı fonksiyon (private)
// Daha önce DppFile sınıfı içinde tanımlamıştık, burada modül seviyesinde tanımlayalım.
private IOError mapFileExceptionToIOError(FileException e) {
    // FileException'ın içerdiği bilgiye göre uygun IOError.Kind belirle
    if (e.msg.startsWith("No such file or directory")) return IOError(IOError.Kind.NotFound, e.msg);
    if (e.msg.startsWith("Permission denied")) return IOError(IOError.Kind.PermissionDenied, e.msg);
    if (e.msg.startsWith("Already exists")) return IOError(IOError.Kind.AlreadyExists, e.msg);
    // ... Diğer özel durumlar
    return IOError(IOError.Kind.OtherError, e.msg);
}


// ... Diğer dosya işlemleri (bayt olarak okuma/yazma, konumlanma, dosya bilgisi alma vb.)
// Dizin içeriğini listeleme gibi fonksiyonlar da buraya ait olabilir.

Result!(string[], IOError) readDir(string dirPath) {
    try {
        string[] entries;
        foreach (entry; std.file.dirEntries(dirPath, std.file.SpanMode.shallow)) {
            entries ~= entry.name;
        }
        return Result!(string[], IOError).Ok(entries);
    } catch (FileException e) {
        return Result!(string[], IOError).Err(mapFileExceptionToIOError(e));
    } catch (Exception e) {
        return Result!(string[], IOError).Err(IOError(IOError.Kind.OtherError, e.msg));
    }
}

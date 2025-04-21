module io;

import core.error; // Hata tipleri için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import std.stdio;   // Temel I/O fonksiyonları ve File, FileMode için (implementasyonlar için)

// I/O işlemleri için temel hata türü (core.error.Error'dan türetilmiş)
// Bu struct, farklı I/O hata türlerini (dosya bulunamadı, izin hatası vb.) içerebilir.
struct IOError : Error {
    // Farklı I/O hata türlerini temsil eden enum
    enum Kind {
        NotFound,          // Dosya veya yol bulunamadı
        PermissionDenied,  // İzin yetersizliği
        ConnectionRefused, // Ağ bağlantısı reddedildi
        Interrupted,       // İşlem kesintiye uğradı
        OtherError,        // Belirtilmemiş diğer hatalar
        WouldBlock,        // Non-blocking I/O'da işlem hemen tamamlanamadı
        UnexpectedEof,     // Beklenmeyen dosya sonu
        AlreadyExists,     // Oluşturulmaya çalışılan öğe zaten mevcut
        InvalidData,       // Geçersiz veri formatı
        // ... Diğer I/O hata türleri
    }

    Kind kind;             // Hatanın türü
    string message;        // Hatanın detaylı mesajı (isteğe bağlı)
    Error* source;      // Hatanın neden olduğu başka bir hata (hata zincirleri için)

    // Constructor
    this(Kind kind, string message = "") {
        this.kind = kind;
        this.message = message;
    }

    // Error interface'inin description metodunu implemente et
    override string description() const {
        return format("IOError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    // Debugging için stringe çevirme
    string toString() const {
        return description();
    }
}

// IOErrorKind enum değerini stringe çevirmek için yardımcı fonksiyon
string to!string(IOErrorKind kind) {
    final switch (kind) {
        case IOError.Kind.NotFound: return "NotFound";
        case IOError.Kind.PermissionDenied: return "PermissionDenied";
        case IOError.Kind.ConnectionRefused: return "ConnectionRefused";
        case IOError.Kind.Interrupted: return "Interrupted";
        case IOError.Kind.OtherError: return "OtherError";
        case IOError.Kind.WouldBlock: return "WouldBlock";
        case IOError.Kind.UnexpectedEof: return "UnexpectedEof";
        case IOError.Kind.AlreadyExists: return "AlreadyExists";
        case IOError.Kind.InvalidData: return "InvalidData";
    }
}


// Okunabilir veri kaynakları için temel trait (interface)
// Dosyalar, ağ akışları, stdin gibi kaynaklar bu trait'i implemente edebilir.
interface Read {
    // Kaynaktan belirli bir sayıda byte okumaya çalışır ve okunan byte sayısını döndürür.
    // Okunan byte'lar buffer'a yazılır. Buffer'ın boyutu okunabilecek maksimum byte sayısını belirler.
    // Başarılı durumda Ok(okunan_byte_sayısı), hata durumunda Err(IOError) döndürür.
    Result!(size_t, IOError) read(ubyte[] buffer);

    // Tampon dolana kadar okumaya çalışır veya EOF'a ulaşır.
    // Tüm tampon doldurulursa Ok(()), EOF'a ulaşılırsa Err(IOError.UnexpectedEof),
    // diğer hatalarda Err(IOError) döndürür.
    Result!(void, IOError) readExact(ubyte[] buffer);
}

// Yazılabilir veri hedefleri için temel trait (interface)
// Dosyalar, ağ akışları, stdout, stderr gibi hedefler bu trait'i implemente edebilir.
interface Write {
    // Hedefe belirli bir sayıda byte yazmaya çalışır ve yazılan byte sayısını döndürür.
    // Yazılacak byte'lar buffer'dan okunur.
    // Başarılı durumda Ok(yazılan_byte_sayısı), hata durumunda Err(IOError) döndürür.
    Result!(size_t, IOError) write(const(ubyte)[] buffer);

    // Tüm tamponun yazılmasını sağlar.
    // Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
    Result!(void, IOError) writeAll(const(ubyte)[] buffer);

    // Yazma tamponunu (buffer) temizler ve verinin hedefe fiziksel olarak yazılmasını sağlar.
    // Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
    Result!(void, IOError) flush();
}

// Konumlanabilir veri kaynakları/hedefleri için temel trait (interface)
// Dosyalar gibi rasgele erişimli kaynaklar bu trait'i implemente edebilir.
interface Seek {
    // Konumlanma başlangıç noktası
    enum SeekFrom {
        Start, // Akışın başından itibaren
        End,   // Akışın sonundan itibaren
        Current // Mevcut konumdan itibaren
    }

    // Akış içindeki konumu değiştirir.
    // from: Konumlanma başlangıç noktası.
    // offset: Başlangıç noktasından itibaren ofset (negatif olabilir).
    // Başarılı durumda Ok(yeni_konum), hata durumunda Err(IOError) döndürür.
    Result!(size_t, IOError) seek(SeekFrom from, long offset);

    // Akışın mevcut konumunu döndürür.
    Result!(size_t, IOError) currentPosition();
}


// Yaygın I/O bileşenlerini yeniden dışa aktar
public import io.stdio;    // stdio.d'deki her şeyi dışa aktar
public import io.file_io;  // file_io.d'deki her şeyi dışa aktar

// Belki de tamponlama ile ilgili temel sınıflar veya fonksiyonlar burada tanımlanabilir veya yeniden dışa aktarılabilir.
 public import io.buffer;
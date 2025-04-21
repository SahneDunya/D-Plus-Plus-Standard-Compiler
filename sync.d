module sync;

import core.error; // Temel hata tipleri için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import core.time;  // Zaman aşımı (timeout) için (condvar gibi)

// Senkronizasyon işlemleri için temel hata türü (core.error.Error'dan türetilmiş)
// Bu struct, farklı senkronizasyon hatalarını (kilitlenme, zaman aşımı vb.) içerebilir.
struct SyncError : Error {
    // Farklı senkronizasyon hata türlerini temsil eden enum
    enum Kind {
        Poisoned,      // Bir kilit, bir iş parçacığı panikledikten sonra kilitliyse (Rust benzeri)
        TimedOut,      // İşlem zaman aşımına uğradı
        OtherError,    // Belirtilmemiş diğer hatalar
        // ... Diğer senkronizasyon hata türleri
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
        return format("SyncError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    // Debugging için stringe çevirme
    string toString() const {
        return description();
    }
}

// SyncError.Kind enum değerini stringe çevirmek için yardımcı fonksiyon
string to!string(SyncError.Kind kind) {
    final switch (kind) {
        case SyncError.Kind.Poisoned: return "Poisoned";
        case SyncError.Kind.TimedOut: return "TimedOut";
        case SyncError.Kind.OtherError: return "OtherError";
    }
}


// Kilitlenebilir nesneler için temel trait (interface)
// Mutex ve RwLock gibi kilit tipleri bu traitleri implemente edebilir.
// Bu traitler, kilitlenme ve kilit açma arayüzünü sağlar.
interface Lock {
    // Kilidi alır (bloklayıcı). Başarılı durumda Ok(()), hata durumunda Err(SyncError) döndürür.
    // Genellikle guard nesnesi döndürülür, bu trait sadece temel arayüzü temsil ediyor olabilir.
    Result!(void, SyncError) acquire();

    // Kilidi serbest bırakır.
    void release();

    // Kilidi zaman aşımı ile almaya çalışır. Başarılı Ok(()), zaman aşımı Err(TimedOut), diğer Err(SyncError).
     Result!(void, SyncError) tryAcquireFor(core.time.Duration timeout);
}

interface ReadLock { // RwLock için okuma kilidi
    Result!(void, SyncError) acquireRead();
    void releaseRead();
     Result!(void, SyncError) tryAcquireReadFor(core.time.Duration timeout);
}

interface WriteLock { // RwLock için yazma kilidi
    Result!(void, SyncError) acquireWrite();
    void releaseWrite();
     Result!(void, SyncError) tryAcquireWriteFor(core.time.Duration timeout);
}


// Yaygın senkronizasyon ilkelciklerini yeniden dışa aktar
public import sync.mutex;   // mutex.d'deki her şeyi dışa aktar
public import sync.rwlock;    // rwlock.d'deki her şeyi dışa aktar
public import sync.condvar; // condvar.d'deki her şeyi dışa aktar

// Diğer senkronizasyon ile ilgili yardımcı fonksiyonlar veya tipler
// Örneğin, atomik tipler (std.concurrency.atomic gibi) veya senkronize blok makroları.
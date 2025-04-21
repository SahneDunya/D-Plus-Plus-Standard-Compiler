module sync.rwlock;

import std.stdio;    // writeln için
import std.string;   // format için
import core.result;  // Result<T, E> için
import core.option;  // Option<T> için
import sync;         // SyncError, ReadLock, WriteLock traitleri için
import std.concurrency; // D'nin threading/rwlock ilkelcikleri için
import core.time;    // Zaman aşımı (timeout) için

// Okuma/Yazma Kilidi (RwLock)
// T: RwLock'un koruduğu veri tipi
class RwLock(T) {
    // Altında yatan sistem RwLock nesnesi
     void* systemRwLockHandle; // Sistem RwLock tanıtıcısı
    // D'nin std.concurrency.RWLock gibi bir sınıfını kullanabiliriz.
    private std.concurrency.RWLock delegateRwLock; // D'nin RWLock'unu kullanalım

    private T data; // RwLock'un koruduğu veri
    private bool poisoned = false; // RwLock zehirlenmiş mi


    // Yeni bir RwLock oluşturur ve veriyi içerir.
    this(T data) {
        this.data = data;
        this.delegateRwLock = new std.concurrency.RWLock(); // D'nin RWLock'unu oluştur
    }

    // Okuma kilidini alır (bloklayıcı). Birden fazla okuyucu aynı anda kilit alabilir.
    // Başarılı durumda veriye erişim sağlayan bir RwLockReadGuard döndürür.
    Result!(RwLockReadGuard!T, SyncError) read() {
        writeln("RwLock okuma kilidi alınıyor...");
        // Altında yatan sistem RwLock okuma kilidini al
        delegateRwLock.lockRead(); // D'nin lockRead() metodu

        // Zehirlenme kontrolü (okuma kilidinde de yapılabilir ama genellikle yazmada daha kritiktir)
        if (this.poisoned) {
              delegateRwLock.unlockRead();
              return Result!(RwLockReadGuard!T, SyncError).Err(SyncError(SyncError.Kind.Poisoned, "RwLock zehirlenmiş."));
             writeln("Uyarı: RwLock zehirlenmiş (okuma kilidi alınıyor).");
        }

        writeln("RwLock okuma kilidi alındı.");
        // Kilidi serbest bırakacak bir guard nesnesi oluştur ve döndür
        return Result!(RwLockReadGuard!T, SyncError).Ok(RwLockReadGuard!T(this));
    }

    // Yazma kilidini alır (bloklayıcı). Sadece bir yazıcı aynı anda kilit alabilir ve başka okuyucu/yazıcı olmamalıdır.
    // Başarılı durumda veriye erişim sağlayan bir RwLockWriteGuard döndürür.
    Result!(RwLockWriteGuard!T, SyncError) write() {
        writeln("RwLock yazma kilidi alınıyor...");
        // Altında yatan sistem RwLock yazma kilidini al
        // Bu işlem bloklayıcıdır ve özel erişim gerektirir.
        delegateRwLock.lockWrite(); // D'nin lockWrite() metodu

        // Zehirlenme kontrolü
        if (this.poisoned) {
              delegateRwLock.unlockWrite();
              return Result!(RwLockWriteGuard!T, SyncError).Err(SyncError(SyncError.Kind.Poisoned, "RwLock zehirlenmiş."));
             writeln("Uyarı: RwLock zehirlenmiş (yazma kilidi alınıyor).");
        }

        writeln("RwLock yazma kilidi alındı.");
        // Kilidi serbest bırakacak bir guard nesnesi oluştur ve döndür
        return Result!(RwLockWriteGuard!T, SyncError).Ok(RwLockWriteGuard!T(this));
    }

    // Non-bloklayıcı okuma/yazma kilidi alma metotları (tryRead, tryWrite, tryReadFor, tryWriteFor)

    // RwLock'un zehirlenmiş olup olmadığını kontrol eder.
    bool isPoisoned() const {
        return this.poisoned;
    }


    // Okuma kilidini serbest bırakır. Guard tarafından çağrılmalıdır.
    private void unlockRead() {
        delegateRwLock.unlockRead(); // D'nin unlockRead() metodu
        writeln("RwLock okuma kilidi serbest bırakıldı.");
    }

    // Yazma kilidini serbest bırakır. Guard tarafından çağrılmalıdır.
    private void unlockWrite() {
        delegateRwLock.unlockWrite(); // D'nin unlockWrite() metodu
        writeln("RwLock yazma kilidi serbest bırakıldı.");
    }

    // Zehirlenme durumu yönetimi
    void __dpp_poison() {
        this.poisoned = true;
        writeln("RwLock zehirlendi!");
    }
    // ... Diğer RwLock metotları
}

// RwLockReadGuard yapısı (RAII) - Okuma kilidi koruyucusu
struct RwLockReadGuard(T) {
    private RwLock!T* rwLock; // Korunan RwLock'a işaretçi

    this(RwLock!T* rwLock) {
        this.rwLock = rwLock;
    }

    // Okunan veriye erişim sağlar (immutable)
    ref const(T) opDereference() const {
        return rwLock.data;
    }

     // . operatörü ile alanlara erişim sağlar (immutable)
    
    auto opDispatch(string name)() const {
        return mixin("rwLock.data." ~ name);
    }

    // Okuma kilidini serbest bırakır. Kapsam dışına çıktığında otomatik çağrılmalı.
    void unlock() {
        if (rwLock) {
            rwLock.unlockRead();
            rwLock = null;
        }
    }

     Kopyalama/atama devre dışı
     @disable this(this);
     @disable void opAssign(typeof(this));
}

// RwLockWriteGuard yapısı (RAII) - Yazma kilidi koruyucusu
struct RwLockWriteGuard(T) {
    private RwLock!T* rwLock; // Korunan RwLock'a işaretçi

    this(RwLock!T* rwLock) {
        this.rwLock = rwLock;
    }

    // Yazılan veriye erişim sağlar (mutable)
    ref T opDereference() {
        return rwLock.data;
    }

    // . operatörü ile alanlara erişim sağlar (mutable)
    
    auto opDispatch(string name)() {
        return mixin("rwLock.data." ~ name);
    }

    // Yazma kilidini serbest bırakır. Kapsam dışına çıktığında otomatik çağrılmalı.
    void unlock() {
        if (rwLock) {
            rwLock.unlockWrite();
            rwLock = null;
        }
    }

    // Kopyalama/atama devre dışı
     @disable this(this);
     @disable void opAssign(typeof(this));
}
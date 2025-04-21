module thread.atomic;

import std.stdio; // writeln için
// D'nin atomik ilkelcikleri için core.atomic modülü veya compiler built-in'leri kullanılabilir.
import core.atomic; // D'nin core.atomic modülünü kullanalım


// Bellek Sıralama (Memory Ordering) türleri
enum Ordering {
    Relaxed,     // En zayıf sıralama, sadece atomikliği garanti eder
    Acquire,     // Sonraki okumaları yeniden sıralamayı engeller
    Release,     // Önceki yazmaları yeniden sıralamayı engeller
    AcqRel,      // Hem Acquire hem Release
    SeqCst       // En güçlü sıralama (Sequential Consistency), tüm atomik işlemler küresel bir sıraya yerleştirilir
}

// Atomik Boolean tipi
struct AtomicBool {
    private core.atomic.Atomic!bool delegateAtomic; // D'nin Atomic!bool yapısı

    this(bool value = false) {
        this.delegateAtomic = core.atomic.Atomic!bool(value);
    }

    // Atomik olarak değeri oku (load)
    bool load(Ordering order) const {
        return delegateAtomic.load(order.toAtomicOrder()); // D'nin atomik sıralama enum'ına çevir
    }

    // Atomik olarak değeri yaz (store)
    void store(bool value, Ordering order) {
        delegateAtomic.store(value, order.toAtomicOrder());
    }

    // Atomik olarak değeri değiştir (swap) ve eski değeri döndür
    bool swap(bool value, Ordering order) {
        return delegateAtomic.swap(value, order.toAtomicOrder());
    }

    // Atomik olarak karşılaştır ve değiştir (compare-and-swap)
    // Eğer mevcut değer expected ile aynıysa, değeri new_value ile değiştirir.
    // Başarılı veya başarısız olduğunu belirtir ve mevcut değeri (swap sonrası veya öncesi) döndürür.
    // Result!(bool, bool) -> Ok(swap başarılı mı?), Err(mevcut değer) gibi
    bool compareAndSwap(bool expected, bool new_value, Ordering successOrder, Ordering failureOrder) {
         // D'nin cas fonksiyonu true/false döndürür (başarılı mı?)
         return delegateAtomic.cas(expected, new_value, successOrder.toAtomicOrder(), failureOrder.toAtomicOrder());
        // Gelişmiş versiyon mevcut değeri de döndürebilir.
    }
    // ... Diğer atomik bool işlemleri (and, or, xor)
}

// Atomik Tamsayı tipi (örn: AtomicI32, AtomicU64)
// T: Tamsayı tipi (int, uint, long, ulong, size_t vb.)
struct AtomicInt(T) {
    private core.atomic.Atomic!T delegateAtomic;

    this(T value = T.init) {
        this.delegateAtomic = core.atomic.Atomic!T(value);
    }

    // Atomik olarak değeri oku
    T load(Ordering order) const {
        return delegateAtomic.load(order.toAtomicOrder());
    }

    // Atomik olarak değeri yaz
    void store(T value, Ordering order) {
        delegateAtomic.store(value, order.toAtomicOrder());
    }

    // Atomik olarak değeri değiştir (swap) ve eski değeri döndür
    T swap(T value, Ordering order) {
        return delegateAtomic.swap(value, order.toAtomicOrder());
    }

    // Atomik olarak karşılaştır ve değiştir
    bool compareAndSwap(T expected, T new_value, Ordering successOrder, Ordering failureOrder) {
        return delegateAtomic.cas(expected, new_value, successOrder.toAtomicOrder(), failureOrder.toAtomicOrder());
    }

    // Atomik toplama (fetch-add) ve eski değeri döndür
    T fetchAdd(T value, Ordering order) {
        return delegateAtomic.fetchAdd(value, order.toAtomicOrder());
    }

    // Atomik çıkarma (fetch-sub) ve eski değeri döndür
    T fetchSub(T value, Ordering order) {
        return delegateAtomic.fetchSub(value, order.toAtomicOrder());
    }

    // ... Diğer atomik tamsayı işlemleri (fetchAnd, fetchOr, fetchXor)
}

// Atomik İşaretçi tipi (AtomicPtr)
// T: İşaret edilen tip
struct AtomicPtr(T) {
     private core.atomic.Atomic!(T*) delegateAtomic; // D'nin Atomic!(T*) yapısı

     this(T* ptr = null) {
         this.delegateAtomic = core.atomic.Atomic!(T*)(ptr);
     }

    // Atomik olarak işaretçiyi oku
     T* load(Ordering order) const {
         return delegateAtomic.load(order.toAtomicOrder());
     }

    // Atomik olarak işaretçiyi yaz
     void store(T* ptr, Ordering order) {
         delegateAtomic.store(ptr, order.toAtomicOrder());
     }

    // Atomik olarak işaretçiyi değiştir (swap) ve eski işaretçiyi döndür
     T* swap(T* ptr, Ordering order) {
         return delegateAtomic.swap(ptr, order.toAtomicOrder());
     }

    // Atomik olarak işaretçiyi karşılaştır ve değiştir
     bool compareAndSwap(T* expected, T* new_ptr, Ordering successOrder, Ordering failureOrder) {
         return delegateAtomic.cas(expected, new_ptr, successOrder.toAtomicOrder(), failureOrder.toAtomicOrder());
     }

     // ... Diğer atomik işaretçi işlemleri
}


// D++'ın Ordering enum'ını D'nin core.atomic.MemoryOrder enum'ına çeviren yardımcı fonksiyon
private core.atomic.MemoryOrder toAtomicOrder(Ordering order) {
    final switch (order) {
        case Ordering.Relaxed: return core.atomic.MemoryOrder.relaxed;
        case Ordering.Acquire: return core.atomic.MemoryOrder.acquire;
        case Ordering.Release: return core.atomic.MemoryOrder.release;
        case Ordering.AcqRel: return core.atomic.MemoryOrder.acqrel;
        case Ordering.SeqCst: return core.atomic.MemoryOrder.seqcst;
    }
}

// Atomik tipler için yaygın takma adlar
alias AtomicI8 = AtomicInt!byte;
alias AtomicU8 = AtomicInt!ubyte;
alias AtomicI16 = AtomicInt!short;
alias AtomicU16 = AtomicInt!ushort;
alias AtomicI32 = AtomicInt!int;
alias AtomicU32 = AtomicInt!uint;
alias AtomicI64 = AtomicInt!long;
alias AtomicU64 = AtomicInt!ulong;
alias AtomicSize = AtomicInt!size_t; // size_t platforma göre değişir


// Diğer atomik helper fonksiyonları veya tipler
// Örneğin, Fence komutları
 void fence(Ordering order);
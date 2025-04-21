module thread.channel;

import std.stdio;    // writeln için
import std.string;   // format için
import core.result;  // Result<T, E> için
import core.option;  // Option<T> için
import core.error;   // Temel hata tipleri için
import sync;         // Mutex ve Condvar için
import collections.vec_deque; // Dahili tampon olarak VecDeque kullanılabilir

// Kanal gönderme işlemleri için hata türü
struct SendError : Error {
    enum Kind {
        Disconnected, // Kanalın alıcı ucu kapatıldı veya düşürüldü
        Full,         // Kanal dolu (bounded channel)
        OtherError,   // Diğer hatalar
    }

    Kind kind;
    string message;

    this(Kind kind, string message = "") {
        this.kind = kind;
        this.message = message;
    }

    override string description() const {
        return format("SendError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    string toString() const {
        return description();
    }
}

string to!string(SendError.Kind kind) {
    final switch (kind) {
        case SendError.Kind.Disconnected: return "Disconnected";
        case SendError.Kind.Full: return "Full";
        case SendError.Kind.OtherError: return "OtherError";
    }
}

// Kanal alma işlemleri için hata türü
struct RecvError : Error {
    enum Kind {
        Disconnected, // Kanalın tüm gönderici uçları kapatıldı veya düşürüldü ve kanal boş
        Empty,        // Kanal boş (non-blocking recv)
        OtherError,   // Diğer hatalar
    }

    Kind kind;
    string message;

    this(Kind kind, string message = "") {
        this.kind = kind;
        this.message = message;
    }

    override string description() const {
        return format("RecvError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    string toString() const {
        return description();
    }
}

string to!string(RecvError.Kind kind) {
    final switch (kind) {
        case RecvError.Kind.Disconnected: return "Disconnected";
        case RecvError.Kind.Empty: return "Empty";
        case RecvError.Kind.OtherError: return "OtherError";
    }
}


// Kanalın dahili durumunu ve veriyi tutan yapı
private struct ChannelState(T) {
    // collections.vec_deque.VecDeque!T dataQueue; // Mesajları tutan kuyruk (bounded/unbounded için)
    // Şimdilik basit bir dinamik dizi kullanalım
    T[] dataQueue;

    size_t senderCount = 0; // Aktif gönderici sayısı
    size_t receiverCount = 0; // Aktif alıcı sayısı

    sync.mutex.Mutex!(void)* mutex; // Durumu koruyan mutex
    sync.condvar.Condvar* senderCondvar; // Göndericilerin beklemesi için (tampon doluysa)
    sync.condvar.Condvar* receiverCondvar; // Alıcıların beklemesi için (kanal boşsa)

    // Kanalın kapatılıp kapatılmadığı (tüm göndericiler veya alıcılar düştüğünde)
    bool disconnected = false;
    size_t capacity; // Kanal kapasitesi (bounded için)

    this() {
        this.dataQueue = [];
        this.mutex = new sync.mutex.Mutex!(void)(); // Mutex oluştur
        this.senderCondvar = new sync.condvar.Condvar(); // Condvar oluştur
        this.receiverCondvar = new sync.condvar.Condvar(); // Condvar oluştur
        this.capacity = 0; // Sınırsız (unbounded) kanal varsayalım
    }

    // Nesne temizlendiğinde kaynakları serbest bırak (eğer GC kullanmıyorsak veya explicit temizlik gerekiyorsa)
    
    ~this() {
        destroy(mutex);
        destroy(senderCondvar);
        destroy(receiverCondvar);
    }

}


// Kanalın gönderici ucu
struct Sender(T) {
    private ChannelState!T* state; // Paylaşılan kanal durumuna işaretçi

    // Constructor (only called by channel function)
    private this(ChannelState!T* state) {
        this.state = state;
        // Yeni bir gönderici eklendiğini duruma bildir
        auto guardResult = state.mutex.lock();
        if (guardResult.isOk()) {
            scope(exit) guardResult.unwrap().unlock(); // Kilidi serbest bırak (RAII)
            state.senderCount++;
        } else {
            // Mutex kilit hatası
            // panic! veya hata raporlama
            stderr.writeln("Hata (Channel): Sender oluşturulurken mutex kilit hatası!");
        }
    }

    // Kanal üzerinden bir değer gönderir.
    // Başarılı durumda Ok(()), hata durumunda Err(SendError) döndürür.
    // Bounded channel'larda tampon doluysa bloklayabilir.
    Result!(void, SendError) send(T value) {
        auto guardResult = state.mutex.lock();
        if (guardResult.isErr()) {
            return Result!(void, SendError).Err(SendError(SendError.Kind.OtherError, "Mutex kilit hatası: " ~ guardResult.unwrapErr().description()));
        }
        scope(exit) guardResult.unwrap().unlock(); // Kilidi serbest bırak

        // Kanal kapatılmış mı kontrol et (alıcı yoksa)
        if (state.disconnected || state.receiverCount == 0) {
            return Result!(void, SendError).Err(SendError(SendError.Kind.Disconnected));
        }

        // Bounded channel implementasyonu burada tampon doluluğunu kontrol eder ve bekler (Condvar kullanarak)
        
        while (state.dataQueue.len() == state.capacity) {
            state.senderCondvar.wait(guardResult.unwrap()); // Tampon boşalana kadar bekle (mutex guard'ı geçici serbest bırakır)
            // Beklemeden uyandıktan sonra zehirlenme veya kapatılma kontrolü
             if (state.disconnected) { ... }
        }

        // Değeri kuyruğa ekle
        state.dataQueue ~= value;
        writeln("Kanal gönderildi: ", text(value));

        // Bir alıcıyı uyandır (eğer bekleyen varsa)
        state.receiverCondvar.notifyOne();

        return Result!(void, SendError).Ok(void); // Başarılı
    }

    // Sender nesnesi düştüğünde (drop) kanal durumu güncellenir.
    // Bu, alıcıların kanalın kapatıldığını anlamasını sağlar.
    // D'nin struct destructorları veya Class destructorları kullanılabilir.
    
    ~this() { // Destructor
        auto guardResult = state.mutex.lock();
         if (guardResult.isOk()) {
            scope(exit) guardResult.unwrap().unlock();
            state.senderCount--;
            // Eğer aktif gönderici kalmadıysa, tüm alıcıları uyandır (kanal kapatıldı sinyali)
            if (state.senderCount == 0) {
                state.disconnected = true;
                state.receiverCondvar.notifyAll();
            }
        } else {
             // Mutex kilit hatası (destructor'da hata raporlama zor)
        }
        writeln("Sender düştü.");
    }
    
}

// Kanalın alıcı ucu
struct Receiver(T) {
    private ChannelState!T* state; // Paylaşılan kanal durumuna işaretçi

     // Constructor (only called by channel function)
    private this(ChannelState!T* state) {
        this.state = state;
         // Yeni bir alıcı eklendiğini duruma bildir
        auto guardResult = state.mutex.lock();
        if (guardResult.isOk()) {
            scope(exit) guardResult.unwrap().unlock();
            state.receiverCount++;
        } else {
            stderr.writeln("Hata (Channel): Receiver oluşturulurken mutex kilit hatası!");
        }
    }


    // Kanal üzerinden bir değer alır.
    // Başarılı durumda Ok(value), kanal boş ve tüm göndericiler düşmüşse Err(RecvError.Disconnected),
    // kanal boşsa (blocking recv) bekler, non-blocking recv'de boşsa Err(RecvError.Empty).
    Result!(T, RecvError) recv() {
        auto guardResult = state.mutex.lock();
        if (guardResult.isErr()) {
            return Result!(T, RecvError).Err(RecvError(RecvError.Kind.OtherError, "Mutex kilit hatası: " ~ guardResult.unwrapErr().description()));
        }
        scope(exit) guardResult.unwrap().unlock(); // Kilidi serbest bırak

        // Kanal boşsa bekle
        while (state.dataQueue.empty) {
            // Kanal boş ve tüm göndericiler düşmüşse, kanal kapatılmıştır.
            if (state.disconnected && state.dataQueue.empty) {
                return Result!(T, RecvError).Err(RecvError(RecvError.Kind.Disconnected));
            }
            // Kanal boşsa, bir mesaj gelene kadar bekle (Condvar kullanarak)
            state.receiverCondvar.wait(guardResult.unwrap()); // Bekle (mutex guard'ı geçici serbest bırakır)
             // Beklemeden uyandıktan sonra kapatılma kontrolü
        }

        // Kuyruktan değeri al (VecDeque.popFront() veya dizi popfromfront)
        // Basit dizi için: T value = state.dataQueue[0]; state.dataQueue.remove(0);
        T value = state.dataQueue.front; // Varsayım: front() metodu var
        state.dataQueue = state.dataQueue[1 .. $]; // Dizi dilimleme ile ilk elemanı at

        writeln("Kanal alındı: ", text(value));

        // Bir göndericiyi uyandır (eğer bekleyen varsa - bounded channel için)
        state.senderCondvar.notifyOne();

        return Result!(T, RecvError).Ok(value); // Başarılı
    }

    // Non-bloklayıcı alma metotları (tryRecv, recvTimeout)
    
    Option!T tryRecv() { ... } // Kanal boşsa None döndürür
    Result!(Option!T, RecvError) recvTimeout(core.time.Duration timeout); // Zaman aşımı ile bekleme
    


     // Receiver nesnesi düştüğünde (drop) kanal durumu güncellenir.
    
    ~this() { // Destructor
        auto guardResult = state.mutex.lock();
         if (guardResult.isOk()) {
            scope(exit) guardResult.unwrap().unlock();
            state.receiverCount--;
            // Eğer aktif alıcı kalmadıysa, kanal kapatıldı sinyali
            if (state.receiverCount == 0) {
                state.disconnected = true;
                state.senderCondvar.notifyAll(); // Göndericileri uyandır (tampon dolu olmasa bile)
            }
        } else {
             // Mutex kilit hatası
        }
        writeln("Receiver düştü.");
    }

}


// Kanal çifti oluşturan fonksiyon (gönderici ve alıcı)
// Başarılı durumda Ok((Sender<T>, Receiver<T>)), hata durumunda Err(SendError veya RecvError gibi bir hata) döndürür.
// Unbounded channel oluşturur
Result!(Tuple!(Sender!T, Receiver!T), Error) channel(T)() { // Error döndürelim genel hata için
     try {
        ChannelState!T* state = new ChannelState!T(); // Paylaşılan durum nesnesini oluştur

        Sender!T sender = Sender!T(state);   // Sender ucunu oluştur
        Receiver!T receiver = Receiver!T(state); // Receiver ucunu oluştur

        return Result!(Tuple!(Sender!T, Receiver!T), Error).Ok(tuple(sender, receiver));
     } catch (Exception e) {
         // Bellek tahsis hatası veya diğer hatalar
          stderr.writeln("Hata (Channel): Kanal oluşturma hatası: ", e.msg);
         return Result!(Tuple!(Sender!T, Receiver!T), Error).Err(SendError(SendError.Kind.OtherError, e.msg)); // Generic bir hata türüne çevir
     }
}

// Bounded channel oluşturan fonksiyon

Result!(Tuple!(Sender!T, Receiver!T), Error) boundedChannel(T)(size_t capacity) {
    // Channel fonksiyonuna benzer, ancak ChannelState'in capacity alanını ayarlar.
}

// Tuple!(Sender!T, Receiver!T) template argümanı için std.typecons.Tuple import edilmeli.
module net;

import core.error; // Temel hata tipleri için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import io;         // io.d'deki traitler (Read, Write) ağ akışları için kullanılabilir

// Ağ iletişimi işlemleri için temel hata türü (io.error.Error'dan türetilmiş)
// Bu struct, farklı ağ hatalarını (bağlantı reddedildi, adres kullanımda vb.) içerebilir.
struct NetError : Error {
    // Farklı ağ hata türlerini temsil eden enum
    enum Kind {
        ConnectionRefused, // Bağlantı hedef tarafından reddedildi
        ConnectionReset,   // Bağlantı hedef tarafından sıfırlandı
        ConnectionAborted, // Bağlantı iptal edildi
        NotFound,          // Adres veya ana bilgisayar bulunamadı
        PermissionDenied,  // İzin yetersizliği
        AddressInUse,      // Adres zaten kullanımda
        AddressNotAvailable,// Adres kullanılamıyor
        NotConnected,      // Bağlantı kurulu değil
        TimedOut,          // İşlem zaman aşımına uğradı
        WouldBlock,        // Non-blocking sokette işlem hemen tamamlanamadı
        OtherError,        // Belirtilmemiş diğer hatalar
        InvalidInput,      // Geçersiz girdi (örneğin, adres formatı)
        // ... Diğer ağ hata türleri
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
        return format("NetError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    // Debugging için stringe çevirme
    string toString() const {
        return description();
    }
}

// NetError.Kind enum değerini stringe çevirmek için yardımcı fonksiyon
string to!string(NetError.Kind kind) {
    final switch (kind) {
        case NetError.Kind.ConnectionRefused: return "ConnectionRefused";
        case NetError.Kind.ConnectionReset: return "ConnectionReset";
        case NetError.Kind.ConnectionAborted: return "ConnectionAborted";
        case NetError.Kind.NotFound: return "NotFound";
        case NetError.Kind.PermissionDenied: return "PermissionDenied";
        case NetError.Kind.AddressInUse: return "AddressInUse";
        case NetError.Kind.AddressNotAvailable: return "AddressNotAvailable";
        case NetError.Kind.NotConnected: return "NotConnected";
        case NetError.Kind.TimedOut: return "TimedOut";
        case NetError.Kind.WouldBlock: return "WouldBlock";
        case NetError.Kind.OtherError: return "OtherError";
        case NetError.Kind.InvalidInput: return "InvalidInput";
    }
}


// Soket adresine dönüştürülebilen tipler için trait (interface)
// String, (IPAddr, ushort) tuple gibi tipler bu trait'i implemente edebilir.
interface ToSocketAddrs {
    // Kendini SocketAddr listesine çözümler.
    // Başarılı durumda Ok(SocketAddr dizisi), hata durumunda Err(NetError) döndürür.
    Result!(SocketAddr[], NetError) toSocketAddrs() const;
}


// Yaygın ağ bileşenlerini yeniden dışa aktar
public import net.ip_address; // ip_address.d'deki her şeyi dışa aktar
public import net.tcp;        // tcp.d'deki her şeyi dışa aktar
public import net.udp;        // udp.d'deki her şeyi dışa aktar

// Belki de DNS çözünürlüğü gibi diğer yardımcı fonksiyonlar burada tanımlanabilir veya yeniden dışa aktarılabilir.
 Result!(IPAddr[], NetError) lookupHost(string hostname);
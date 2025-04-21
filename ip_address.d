module net.ip_address;

import std.stdio;   // writeln için
import std.string;  // string işlemleri için
import core.result; // Result<T, E> için
import net;         // NetError için
import std.conv;    // to!string, to!int için

// IPv4 adresini temsil eden yapı
struct IPv4Addr {
    ubyte[4] octets; // 4 baytlık oktetler (örneğin, 192.168.1.1 için [192, 168, 1, 1])

    // ulong asUlong; // Tek bir ulong olarak da saklanabilir

    // Yeni bir IPv4 adresi oluşturur
    this(ubyte o1, ubyte o2, ubyte o3, ubyte o4) {
        this.octets = [o1, o2, o3, o4];
    }

    // String'den IPv4 adresi ayrıştırır (örneğin "192.168.1.1")
    static Result!(IPv4Addr, NetError) parse(string ipString) {
        string[] parts = ipString.split(".");
        if (parts.length != 4) {
            return Result!(IPv4Addr, NetError).Err(NetError(NetError.Kind.InvalidInput, "IPv4 adresi 4 oktetten oluşmalı."));
        }
        ubyte[4] octets;
        foreach (i, part; parts) {
            try {
                int octet = part.to!int;
                if (octet < 0 || octet > 255) {
                    return Result!(IPv4Addr, NetError).Err(NetError(NetError.Kind.InvalidInput, format("Geçersiz oktet değeri: %s", octet)));
                }
                octets[i] = cast(ubyte)octet;
            } catch (Exception e) {
                return Result!(IPv4Addr, NetError).Err(NetError(NetError.Kind.InvalidInput, format("Oktet ayrıştırma hatası: %s", part)));
            }
        }
        return Result!(IPv4Addr, NetError).Ok(IPv4Addr(octets[0], octets[1], octets[2], octets[3]));
    }

    // Debugging için stringe çevirme
    string toString() const {
        return format("%s.%s.%s.%s", octets[0], octets[1], octets[2], octets[3]);
    }

    // ... Diğer IPv4 metotları (isLoopback, isPrivate, isGlobal vb.)
}

// IPv6 adresini temsil eden yapı (Daha karmaşık)
struct IPv6Addr {
    ushort[8] segments; // 8 tane 16-bitlik segment (örneğin, 2001:db8::1)

    // String'den IPv6 adresi ayrıştırır (karmaşık ayrıştırma gerekir)
     static Result!(IPv6Addr, NetError) parse(string ipString);

    // Debugging için stringe çevirme
     string toString() const;

    // ... Diğer IPv6 metotları
}

// IP adresini temsil eden enum veya union (IPv4 veya IPv6 olabilir)
enum IpAddr {
    IPv4(IPv4Addr),
    IPv6(IPv6Addr),
    // Veya union kullanarak: union { IPv4Addr v4; IPv6Addr v6; } kind; bool isV4;
}


// IPv4 soket adresini (IP + Port) temsil eden yapı
struct SocketAddrV4 {
    IPv4Addr ip; // IPv4 adresi
    ushort port; // Port numarası

    this(IPv4Addr ip, ushort port) {
        this.ip = ip;
        this.port = port;
    }

    // Debugging için stringe çevirme
    string toString() const {
        return format("%s:%s", ip.toString(), port);
    }

    // ... Diğer SocketAddrV4 metotları
}

// IPv6 soket adresini (IP + Port) temsil eden yapı
struct SocketAddrV6 {
    IPv6Addr ip;   // IPv6 adresi
    ushort port;   // Port numarası
    uint flowinfo; // Akış bilgisi (IPv6)
    uint scope_id; // Kapsam ID'si (IPv6)

    this(IPv6Addr ip, ushort port, uint flowinfo = 0, uint scope_id = 0) {
        this.ip = ip;
        this.port = port;
        this.flowinfo = flowinfo;
        this.scope_id = scope_id;
    }

    // Debugging için stringe çevirme
     string toString() const;

    // ... Diğer SocketAddrV6 metotları
}


// Soket adresini temsil eden enum veya union (IPv4 veya IPv6 olabilir)
enum SocketAddr {
    V4(SocketAddrV4),
    V6(SocketAddrV6),
    // Veya union kullanarak: union { SocketAddrV4 v4; SocketAddrV6 v6; } kind; bool isV4;
}

// String'den SocketAddr ayrıştırır (örneğin "127.0.0.1:8080" veya "[::1]:80")
static Result!(SocketAddr, NetError) parseSocketAddr(string addrString) {
    // Son ':' karakterini bul
    size_t lastColon = addrString.lastIndexOf(':');
    if (lastColon == string.npos) {
        return Result!(SocketAddr, NetError).Err(NetError(NetError.Kind.InvalidInput, "Soket adresi formatı 'ip:port' olmalı."));
    }

    string ipPart = addrString[0 .. lastColon];
    string portPart = addrString[lastColon + 1 .. $];

    // Port numarasını ayrıştır
    ushort port;
    try {
        uint portInt = portPart.to!uint;
        if (portInt > 65535) {
            return Result!(SocketAddr, NetError).Err(NetError(NetError.Kind.InvalidInput, format("Geçersiz port numarası: %s", portInt)));
        }
        port = cast(ushort)portInt;
    } catch (Exception e) {
        return Result!(SocketAddr, NetError).Err(NetError(NetError.Kind.InvalidInput, format("Port numarası ayrıştırma hatası: %s", portPart)));
    }

    // IP kısmını ayrıştırmaya çalış (önce IPv6, sonra IPv4)
    // IPv6 adresleri genellikle köşeli parantez içindedir ([::1]).
    if (ipPart.startsWith("[") && ipPart.endsWith("]")) {
        string ipv6String = ipPart[1 .. $ - 1];
        // IPv6 ayrıştırma fonksiyonunu çağır (implemente edilmeli)
         auto ipv6Result = IPv6Addr.parse(ipv6String);
         if (ipv6Result.isOk()) {
             return Result!(SocketAddr, NetError).Ok(SocketAddr.V6(SocketAddrV6(ipv6Result.unwrap(), port)));
         } else {
             return Result!(SocketAddr, NetError).Err(ipv6Result.unwrapErr());
         }
         stderr.writeln("Uyarı (IpAddress): IPv6 ayrıştırma implemente edilmedi."); // Placeholder
         return Result!(SocketAddr, NetError).Err(NetError(NetError.Kind.InvalidInput, "IPv6 ayrıştırma henüz desteklenmiyor."));
    } else {
        // IPv4 ayrıştırmaya çalış
        auto ipv4Result = IPv4Addr.parse(ipPart);
        if (ipv4Result.isOk()) {
            return Result!(SocketAddr, NetError).Ok(SocketAddr.V4(SocketAddrV4(ipv4Result.unwrap(), port)));
        } else {
             // IPv4 ayrıştırma başarısız olursa, hem IPv4 hem IPv6 ayrıştırmanın başarısız olduğunu belirten bir hata döndürün.
            return Result!(SocketAddr, NetError).Err(NetError(NetError.Kind.InvalidInput, format("Geçersiz IP adresi formatı: %s", ipPart)));
        }
    }
}


// Debugging için SocketAddr'ı stringe çevirme
string toString(SocketAddr addr) {
    final switch (addr) {
        case SocketAddr.V4(SocketAddrV4 v4): return v4.toString();
        case SocketAddr.V6(SocketAddrV6 v6): return v6.toString(); // IPv6 toString implemente edilmeli
    }
}


// ToSocketAddrs trait implementasyonları (örneğin string için)
// Bu implementasyonlar io.d'de tanımlanan trait'e referans vermelidir.

// string tipinin ToSocketAddrs traitini implemente etmesi
alias string.toSocketAddrs = parseSocketAddr; // String'in parseSocketAddr'a takma adı (basit örnek)

// (IPAddr, ushort) tuple'ının ToSocketAddrs traitini implemente etmesi
// Tuple'lar D++'ta özel bir yapı olmalı
 struct Tuple2!(IPAddr, ushort) { ... }
 bool opEquals(const(Object) other);
 size_t toHash();
 Result!(SocketAddr[], NetError) toSocketAddrs() const {
     auto ipAddr = this._0; // Tuple'ın ilk elemanı (IPAddr)
     auto port = this._1; // Tuple'ın ikinci elemanı (ushort)
    final switch (ipAddr) {
         case IpAddr.IPv4(IPv4Addr v4): return Result!(SocketAddr[], NetError).Ok([SocketAddr.V4(SocketAddrV4(v4, port))]);
         case IpAddr.IPv6(IPv6Addr v6): return Result!(SocketAddr[], NetError).Ok([SocketAddr.V6(SocketAddrV6(v6, port))]);
     }
 }
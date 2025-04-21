module time;

import std.stdio;    // writeln için
import std.string;   // format için
import core.result;  // Result<T, E> için
import core.option;  // Option<T> için
import core.error;   // Temel hata tipleri için
// D'nin zaman modülleri kullanılabilir.
import std.datetime; // D'nin zaman modülü

// Zaman işlemleri için hata türü (core.error.Error'dan türetilmiş)
struct TimeError : Error {
    enum Kind {
        ParsingError, // Zaman stringini ayrıştırma hatası
        OtherError,   // Diğer hatalar
    }

    Kind kind;
    string message;

    this(Kind kind, string message = "") {
        this.kind = kind;
        this.message = message;
    }

    override string description() const {
        return format("TimeError(%s): %s", kind.to!string, message.empty ? kind.to!string : message);
    }

    string toString() const {
        return description();
    }
}

string to!string(TimeError.Kind kind) {
    final switch (kind) {
        case TimeError.Kind.ParsingError: return "ParsingError";
        case TimeError.Kind.OtherError: return "OtherError";
    }
}


// Bir zaman noktasını temsil eden yapı (monotonic saat - belirli bir başlangıç noktasından geçen süre)
// Programın çalışmaya başlamasından veya sistemin açılmasından itibaren geçen süreyi ölçmek için kullanılır.
// Saat geri gitmez.
struct Instant {
    private std.datetime.SysTime innerTime; // D'nin SysTime yapısı

    // Şu anki zaman noktasını döndürür (sistem saatine göre)
    static Instant now() {
        return Instant(std.datetime.Clock.currTime(std.datetime.UTC)); // UTC veya yerel saat kullanılabilir
    }

    // İki Instant arasındaki süreyi (Duration) hesaplar
    Duration durationSince(Instant earlier) const {
        // D'nin SysTime'ları arasında çıkarma işlemi Duration döndürür
        return Duration(this.innerTime - earlier.innerTime); // Varsayım: Duration constructor'ı var
    }

    // Belirtilen Duration'ı bu Instant'a ekler
    Instant opBinary!"+"(Duration duration) const {
        return Instant(this.innerTime + duration.inner); // D'nin SysTime + Duration
    }

    // Belirtilen Duration'ı bu Instant'tan çıkarır
    Instant opBinary!"-"(Duration duration) const {
         return Instant(this.innerTime - duration.inner); // D'nin SysTime - Duration
    }

    // Başka bir Instant'tan bu Instant'ı çıkarır ve Duration döndürür
    Duration opBinary!"-"(Instant other) const {
         return Duration(this.innerTime - other.innerTime); // D'nin SysTime - SysTime
    }

    // ... Karşılaştırma operatörleri (<, <=, >, >=, ==, !=) opCmp, opEquals ile
     isAfter(other), isBefore(other)

    // Debugging için stringe çevirme
    string toString() const {
        return innerTime.toString(); // D'nin SysTime toString
    }
}

// Bir zaman aralığını temsil eden yapı (saniye, milisaniye vb.)
struct Duration {
    private std.datetime.Duration inner; // D'nin Duration yapısı

    // Belirtilen saniyeden Duration oluşturur
    static Duration fromSeconds(long seconds) {
        return Duration(std.datetime.seconds(seconds)); // D'nin seconds fonksiyonu
    }

    // Belirtilen milisaniyeden Duration oluşturur
    static Duration fromMilliseconds(long milliseconds) {
        return Duration(std.datetime.msecs(milliseconds)); // D'nin msecs fonksiyonu
    }

    // ... Diğer from... metotları (fromMinutes, fromHours, fromDays vb.)

    // Bu Duration'ı saniye olarak döndürür
    long asSeconds() const {
        return inner.total!"seconds"(); // D'nin total!"seconds"
    }

    // Bu Duration'ı milisaniye olarak döndürür
    long asMilliseconds() const {
        return inner.total!"msecs"(); // D'nin total!"msecs"
    }

    // ... Diğer as... metotları (asMinutes, asHours)

    // İki Duration'ı toplama
    Duration opBinary!"+"(Duration other) const {
        return Duration(this.inner + other.inner);
    }

    // Bir Duration'dan diğerini çıkarma
    Duration opBinary!"-"(Duration other) const {
        return Duration(this.inner - other.inner);
    }

    // Duration'ı bir sayıyla çarpma
    Duration opBinary!"*"(long factor) const {
        return Duration(this.inner * factor);
    }

    // Duration'ı bir sayıya bölme
    Duration opBinary!"/"(long divisor) const {
        return Duration(this.inner / divisor);
    }

     // ... Karşılaştırma operatörleri
     // Debugging için stringe çevirme
    string toString() const {
        return inner.toString(); // D'nin Duration toString
    }
}

// Takvimsel zaman (tarih ve saat dilimi)
// Bu daha karmaşık bir konudur ve ayrı bir CalendarTime veya DateTime yapısı gerektirebilir.
 struct DateTime { ... }
 Result!(DateTime, TimeError) parseDateTime(string timeString, string format);
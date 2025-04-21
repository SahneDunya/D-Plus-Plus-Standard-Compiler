module result_option_types;

import std.stdio;
import std.string;
import std.exception; // Hata durumları için (unwrap/expect gibi fonksiyonlarda)
import dpp_types;     // D++ tür temsilcileri (Result ve Option tiplerinin kendileri DppType sistemi içinde temsil edilecek)


// Option<T> Tipini temsil eden yapı
// T: İçerideki değerin tipi
struct DppOption(T) {
    // Hangi varyantın aktif olduğunu belirten discriminator
    private enum Variant {
        None,
        Some
    }
    private Variant activeVariant;

    // Değeri tutacak union. Sadece activeVariant == Some iken anlamlıdır.
    private union Value {
        T someValue;
        // None durumu için özel bir alan gerekmez, sadece activeVariant'a bakılır.
    }
    private Value data;


    // None varyantını oluşturan static factory fonksiyonu
    static DppOption!T None() {
        return DppOption!T(Variant.None);
    }

    // Some(value) varyantını oluşturan static factory fonksiyonu
    static DppOption!T Some(T value) {
        return DppOption!T(Variant.Some, value);
    }

    // Özel constructor (genellikle kullanıcı tarafından doğrudan kullanılmaz)
    private this(Variant variant, T value = T.init) {
        this.activeVariant = variant;
        if (variant == Variant.Some) {
            this.data.someValue = value;
        }
    }

    // Bu bir Some değeri mi?
    bool isSome() const {
        return activeVariant == Variant.Some;
    }

    // Bu bir None değeri mi?
    bool isNone() const {
        return activeVariant == Variant.None;
    }

    // İçindeki Some değerini döndürür. Eğer None ise çalışma zamanı hatası fırlatır.
    T unwrap() {
        if (isSome()) {
            return data.someValue;
        }
        throw new Exception("Option.unwrap(): Bir None değeri üzerinde unwrap çağrıldı.");
    }

    // İçindeki Some değerini döndürür. Eğer None ise belirtilen mesajla çalışma zamanı hatası fırlatır.
    T expect(string message) {
        if (isSome()) {
            return data.someValue;
        }
        throw new Exception("Option.expect(): " ~ message);
    }

    // Bu Option'ı bir Result'a çevirir. Eğer None ise, belirtilen hata değerini içeren Err döndürür.
    // Eğer Some ise, içindeki değeri içeren Ok döndürür.
    Result!(T, E) okOr!(E)(E errorValue) {
        if (isSome()) {
            return Result!(T, E).Ok(data.someValue);
        }
        return Result!(T, E).Err(errorValue);
    }

    // Debugging için stringe çevirme
    string toString() const {
        if (isSome()) {
            return format("Some(%s)", data.someValue);
        }
        return "None";
    }
}


// Result<T, E> Tipini temsil eden yapı
// T: Başarılı durumda dönen değerin tipi
// E: Hata durumunda dönen hata değerinin tipi
struct DppResult(T, E) {
    // Hangi varyantın aktif olduğunu belirten discriminator
    private enum Variant {
        Ok,
        Err
    }
    private Variant activeVariant;

    // Başarı veya hata değerini tutacak union.
    private union Value {
        T okValue;
        E errValue;
    }
    private Value data;


    // Ok(value) varyantını oluşturan static factory fonksiyonu
    static DppResult!(T, E) Ok(T value) {
        return DppResult!(T, E)(Variant.Ok, value);
    }

    // Err(errorValue) varyantını oluşturan static factory fonksiyonu
    static DppResult!(T, E) Err(E errorValue) {
        return DppResult!(T, E)(Variant.Err, errorValue);
    }

    // Özel constructor (genellikle kullanıcı tarafından doğrudan kullanılmaz)
    private this(Variant variant, auto value) { // auto kullanarak hem T hem E'yi kabul etsin
        this.activeVariant = variant;
        if (variant == Variant.Ok) {
            this.data.okValue = value;
        } else {
            this.data.errValue = value;
        }
    }

    // Bu bir Ok değeri mi?
    bool isOk() const {
        return activeVariant == Variant.Ok;
    }

    // Bu bir Err değeri mi?
    bool isErr() const {
        return activeVariant == Variant.Err;
    }

    // İçindeki Ok değerini döndürür. Eğer Err ise çalışma zamanı hatası fırlatır.
    T unwrap() {
        if (isOk()) {
            return data.okValue;
        }
        // Hata durumunda E'yi stringe çevirip hata mesajına ekleyelim (E'nin toString/text implementasyonu olmalı)
        throw new Exception(format("Result.unwrap(): Bir Err değeri üzerinde unwrap çağrıldı. Hata: %s", text(data.errValue)));
    }

    // İçindeki Ok değerini döndürür. Eğer Err ise belirtilen mesajla çalışma zamanı hatası fırlatır.
    T expect(string message) {
        if (isOk()) {
            return data.okValue;
        }
        throw new Exception(format("Result.expect(): %s. Hata: %s", message, text(data.errValue)));
    }

    // İçindeki Err değerini döndürür. Eğer Ok ise çalışma zamanı hatası fırlatır.
    E unwrapErr() {
        if (isErr()) {
            return data.errValue;
        }
        // Hata durumunda T'yi stringe çevirip hata mesajına ekleyelim (T'nin toString/text implementasyonu olmalı)
        throw new Exception(format("Result.unwrapErr(): Bir Ok değeri üzerinde unwrapErr çağrıldı. Değer: %s", text(data.okValue)));
    }

    // Bu Result'ı bir Option'a çevirir. Eğer Ok ise, içindeki değeri içeren Some döndürür.
    // Eğer Err ise, None döndürür (hata değeri kaybolur).
    DppOption!T ok() {
        if (isOk()) {
            return DppOption!T.Some(data.okValue);
        }
        return DppOption!T.None();
    }

    // Debugging için stringe çevirme
    string toString() const {
        if (isOk()) {
            return format("Ok(%s)", text(data.okValue));
        }
        return format("Err(%s)", text(data.errValue));
    }
}

// DppType sistemi içinde Result ve Option'ın temsil edilmesi
// Bu, type_system.d veya types.d içinde olmalıdır.
// Örneğin:

class ResultType : DppType {
    DppType* okType;  // Ok varyantının tipi
    DppType* errType; // Err varyantının tipi

    this(DppType* okType, DppType* errType) {
        super(DppTypeKind.Result, format("Result!(%s, %s)", okType.toString(), errType.toString()));
        this.okType = okType;
        this.errType = errType;
    }
    /// isExactly, toString vb. implementasyonları
}

class OptionType : DppType {
    DppType* someType; // Some varyantının tipi

    this(DppType* someType) {
        super(DppTypeKind.Option, format("Option!(%s)", someType.toString()));
        this.someType = someType;
    }
    // isExactly, toString vb. implementasyonları
}

// DppTypeKind enum'ına Result ve Option eklenecek
 enum DppTypeKind { ..., Result, Option, ... }

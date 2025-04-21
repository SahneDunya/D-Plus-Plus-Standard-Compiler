module core.option;

import std.stdio;    // writeln, stderr için
import std.string;   // format için
import std.exception; // throw new Exception için
import core.error; // Eğer None durumu için özel bir hata tipi kullanılıyorsa import edilebilir
import dpp_types;  // D++ tür sistemindeki OptionType temsili için (gerekirse)

// Option<T> Tipini temsil eden yapı
// T: İçerideki değerin tipi
struct Option(T) {
    // Hangi varyantın aktif olduğunu belirten discriminator
    private enum Variant {
        None,
        Some
    }
    private Variant activeVariant;

    // Değeri tutacak union. Sadece activeVariant == Some iken anlamlıdır.
    private union Value {
        T someValue;
        // None durumu için özel bir alana gerek yoktur, sadece activeVariant'a bakılır.
    }
    private Value data;


    // None varyantını oluşturan static factory fonksiyonu
    static Option!T None() {
        return Option!T(Variant.None);
    }

    // Some(value) varyantını oluşturan static factory fonksiyonu
    static Option!T Some(T value) {
        return Option!T(Variant.Some, value);
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

    // İçindeki Some değerine immutable referans döndürür. Eğer None ise çalışma zamanı hatası fırlatır.
    ref const(T) asRef() const {
        if (isSome()) {
            return data.someValue;
        }
        throw new Exception("Option.asRef(): Bir None değeri üzerinde asRef çağrıldı.");
    }

    // İçindeki Some değerine mutable referans döndürür. Eğer None ise çalışma zamanı hatası fırlatır.
    ref T asMut() {
        if (isSome()) {
            return data.someValue;
        }
        throw new Exception("Option.asMut(): Bir None değeri üzerinde asMut çağrıldı.");
    }


    // İçindeki Some değerini döndürür (sahipliği devreder). Eğer None ise çalışma zamanı hatası fırlatır.
    // D'nin struct'ları için 'move' semantiği varsayalım (kopyalama yerine).
    T unwrap() {
        if (isSome()) {
            return data.someValue; // Değeri döndür (move/copy)
        }
        throw new Exception("Option.unwrap(): Bir None değeri üzerinde unwrap çağrıldı.");
    }

    // İçindeki Some değerini döndürür. Eğer None ise belirtilen mesajla çalışma zamanı hatası fırlatır.
    T expect(string message) {
        if (isSome()) {
            return data.someValue; // Değeri döndür (move/copy)
        }
        throw new Exception("Option.expect(): " ~ message);
    }

    // Eğer Option Some(value) ise, verilen fonksiyona value'yu uygulayarak yeni bir Option döndürür.
    // Eğer None ise, None döndürür.
    // Func: T -> U (T alan ve U döndüren fonksiyon)
    Option!U map(alias Func, U)() {
        if (isSome()) {
            return Option!U.Some(Func(data.someValue));
        }
        return Option!U.None();
    }

    // Eğer Option Some(value) ise, value'yu verilen fonksiyona uygulayarak yeni bir Option döndürür.
    // Eğer None ise, None döndürür. Fonksiyonun kendisi bir Option döndürmelidir.
    // Func: T -> Option!U (T alan ve Option!U döndüren fonksiyon) - flatMap gibi
    Option!U andThen(alias Func, U)() {
        if (isSome()) {
            return Func(data.someValue);
        }
        return Option!U.None();
    }

    // Eğer Option None ise, verilen fonksiyona çağrı yaparak yeni bir Option döndürür.
    // Eğer Some ise, kendisini döndürür.
    // Func: () -> Option!T (Argüman almayan ve Option!T döndüren fonksiyon)
    Option!T orElse(alias Func)() {
        if (isNone()) {
            return Func();
        }
        return this;
    }

    // İçindeki Some değerini döndürür. Eğer None ise belirtilen varsayılan değeri döndürür.
    T unwrapOr(T defaultValue) {
        if (isSome()) {
            return data.someValue;
        }
        return defaultValue;
    }

    // İçindeki Some değerini döndürür. Eğer None ise verilen fonksiyona çağrı yaparak dönen değeri döndürür.
    // Func: () -> T (Argüman almayan ve T döndüren fonksiyon)
    T unwrapOrElse(alias Func)() {
        if (isSome()) {
            return data.someValue;
        }
        return Func();
    }

    // Bu Option'ı bir Result'a çevirir. Eğer None ise, belirtilen hata değerini içeren Err döndürür.
    // Eğer Some ise, içindeki değeri içeren Ok döndürür.
    // E: Hata tipinin türü
    Result!(T, E) okOr(E errorValue) {
        if (isSome()) {
            return Result!(T, E).Ok(data.someValue);
        }
        return Result!(T, E).Err(errorValue);
    }

    // Debugging için stringe çevirme
    string toString() const {
        if (isSome()) {
            return format("Some(%s)", text(data.someValue)); // text() ile T'yi stringe çevir
        }
        return "None";
    }
}

// Option için Some ve None helper fonksiyonları (Option!T.Some() ve Option!T.None() yerine kolaylık için)
// Genellikle prelude.d içinde re-export edilebilirler.
Option!T Some(T value) { return Option!T.Some(value); }
Option!T None(T)() { return Option!T.None(); } // T türü belirtilmeli
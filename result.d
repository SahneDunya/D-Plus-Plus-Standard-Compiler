module core.result;

import std.stdio;    // writeln, stderr için
import std.string;   // format için
import std.exception; // throw new Exception için
import core.error; // Hata tipi E için temel interface/class

// Result<T, E> Tipini temsil eden yapı
// T: Başarılı durumda dönen değerin tipi
// E: Hata durumunda dönen hata değerinin tipi
struct Result(T, E) {
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
    static Result!(T, E) Ok(T value) {
        return Result!(T, E)(Variant.Ok, value);
    }

    // Err(errorValue) varyantını oluşturan static factory fonksiyonu
    static Result!(T, E) Err(E errorValue) {
        return Result!(T, E)(Variant.Err, errorValue);
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

    // İçindeki Ok değerine immutable referans döndürür. Eğer Err ise çalışma zamanı hatası fırlatır.
    ref const(T) asRef() const {
        if (isOk()) {
            return data.okValue;
        }
        throw new Exception(format("Result.asRef(): Bir Err değeri üzerinde asRef çağrıldı. Hata: %s", text(data.errValue)));
    }

    // İçindeki Ok değerine mutable referans döndürür. Eğer Err ise çalışma zamanı hatası fırlatır.
    ref T asMut() {
        if (isOk()) {
            return data.okValue;
        }
        throw new Exception(format("Result.asMut(): Bir Err değeri üzerinde asMut çağrıldı. Hata: %s", text(data.errValue)));
    }


    // İçindeki Ok değerini döndürür (sahipliği devreder). Eğer Err ise çalışma zamanı hatası fırlatır.
    T unwrap() {
        if (isOk()) {
            return data.okValue; // Değeri döndür (move/copy)
        }
        // Hata durumunda E'yi stringe çevirip hata mesajına ekleyelim (E'nin toString/text implementasyonu olmalı)
        throw new Exception(format("Result.unwrap(): Bir Err değeri üzerinde unwrap çağrıldı. Hata: %s", text(data.errValue)));
    }

    // İçindeki Ok değerini döndürür. Eğer Err ise belirtilen mesajla çalışma zamanı hatası fırlatır.
    T expect(string message) {
        if (isOk()) {
            return data.okValue; // Değeri döndür (move/copy)
        }
        throw new Exception(format("Result.expect(): %s. Hata: %s", message, text(data.errValue)));
    }

    // İçindeki Err değerini döndürür (sahipliği devreder). Eğer Ok ise çalışma zamanı hatası fırlatır.
    E unwrapErr() {
        if (isErr()) {
            return data.errValue; // Hata değerini döndür (move/copy)
        }
        // Hata durumunda T'yi stringe çevirip hata mesajına ekleyelim (T'nin toString/text implementasyonu olmalı)
        throw new Exception(format("Result.unwrapErr(): Bir Ok değeri üzerinde unwrapErr çağrıldı. Değer: %s", text(data.okValue)));
    }

    // İçindeki Err değerini döndürür. Eğer Ok ise belirtilen mesajla çalışma zamanı hatası fırlatır.
    E expectErr(string message) {
        if (isErr()) {
            return data.errValue; // Hata değerini döndür (move/copy)
        }
         throw new Exception(format("Result.expectErr(): %s. Değer: %s", message, text(data.okValue)));
    }


    // Eğer Result Ok(value) ise, verilen fonksiyona value'yu uygulayarak yeni bir Result döndürür.
    // Eğer Err(error) ise, Err(error) döndürür (hata değişmez).
    // Func: T -> U (T alan ve U döndüren fonksiyon)
    Result!(U, E) map(alias Func, U)() {
        if (isOk()) {
            return Result!(U, E).Ok(Func(data.okValue));
        }
        return Result!(U, E).Err(data.errValue);
    }

     // Eğer Result Err(error) ise, verilen fonksiyona error'u uygulayarak yeni bir Result döndürür.
    // Eğer Ok(value) ise, Ok(value) döndürür (değer değişmez).
    // Func: E -> F (E alan ve F döndüren fonksiyon)
    Result!(T, F) mapErr(alias Func, F)() {
        if (isErr()) {
            return Result!(T, F).Err(Func(data.errValue));
        }
        return Result!(T, F).Ok(data.okValue);
    }


    // Eğer Result Ok(value) ise, value'yu verilen fonksiyona uygulayarak yeni bir Result döndürür.
    // Eğer Err(error) ise, Err(error) döndürür. Fonksiyonun kendisi bir Result döndürmelidir.
    // Func: T -> Result!U, E (T alan ve Result!U, E döndüren fonksiyon) - flatMap gibi
    Result!(U, E) andThen(alias Func, U)() {
        if (isOk()) {
            return Func(data.okValue);
        }
        return Result!(U, E).Err(data.errValue);
    }

     // Eğer Result Err(error) ise, error'u verilen fonksiyona uygulayarak yeni bir Result döndürür.
    // Eğer Ok(value) ise, Ok(value) döndürür. Fonksiyonun kendisi bir Result döndürmelidir.
    // Func: E -> Result!T, F (E alan ve Result!T, F döndüren fonksiyon)
    Result!(T, F) orElse(alias Func, F)() {
        if (isErr()) {
            return Func(data.errValue);
        }
        return Result!(T, F).Ok(data.okValue);
    }


    // İçindeki Ok değerini döndürür. Eğer Err ise belirtilen varsayılan değeri döndürür.
    T unwrapOr(T defaultValue) {
        if (isOk()) {
            return data.okValue;
        }
        return defaultValue;
    }

    // İçindeki Ok değerini döndürür. Eğer Err ise verilen fonksiyona çağrı yaparak dönen değeri döndürür.
    // Func: E -> T (E alan ve T döndüren fonksiyon)
    T unwrapOrElse(alias Func)() {
        if (isOk()) {
            return data.okValue;
        }
        return Func(data.errValue);
    }

    // Bu Result'ı bir Option'a çevirir. Eğer Ok ise, içindeki değeri içeren Some döndürür.
    // Eğer Err ise, None döndürür (hata değeri kaybolur).
    Option!T ok() {
        if (isOk()) {
            return Option!T.Some(data.okValue);
        }
        return Option!T.None();
    }

    // Bu Result'ı bir Option'a çevirir. Eğer Err ise, içindeki hata değerini içeren Some döndürür.
    // Eğer Ok ise, None döndürür (başarı değeri kaybolur).
    Option!E err() {
        if (isErr()) {
            return Option!E.Some(data.errValue);
        }
        return Option!E.None();
    }

    // Debugging için stringe çevirme
    string toString() const {
        if (isOk()) {
            return format("Ok(%s)", text(data.okValue)); // text() ile T'yi stringe çevir
        }
        return format("Err(%s)", text(data.errValue)); // text() ile E'yi stringe çevir
    }
}

// Result için Ok ve Err helper fonksiyonları (Result!T, E.Ok() ve Result!T, E.Err() yerine kolaylık için)
// Genellikle prelude.d içinde re-export edilebilirler.

Result!(T, E) Ok(T value) { return Result!(T, E).Ok(value); }
Result!(T, E) Err(E errorValue) { return Result!(T, E).Err(errorValue); }

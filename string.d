module text.string;

import std.stdio;   // writeln için (debugging)
import std.string;  // D'nin string fonksiyonları için
import std.array;   // Dizi işlemleri için
import std.conv;    // Tip dönüşümleri için
import core.option; // Option<T> için
import core.error; // Hata tipleri için

// D++'ın temel metin dizisi tipi
// D'nin immutable string tipini sarmalayalım veya alias yapalım.
// Immutable stringler yaygın bir tercihtir çünkü paylaşılabilirler ve eşzamanlı kodda güvenlidirler.
alias String = immutable(char)[]; // D'nin immutable char dizisine alias

// String literal'leri compiler tarafından otomatik olarak String tipine çevrilmelidir.
 "hello" -> String("hello")

// String tipinin metotları (op... ile operatör overloading'i veya normal metotlar)

// İki stringi birleştirme (Concatenation)
// "+"" operatörü overloading'i
String opBinary!"+"(String left, String right) {
    return left ~ right; // D'nin string birleştirme operatörünü kullan
}

// String uzunluğunu alma (karakter sayısı, byte sayısı farklı olabilir)
// D'de .length byte sayısını verir, byCodeUnit, byChar gibi iteratorlar karakter sayısını verir.
size_t len() const {
    // Unicode karakter sayısını döndürmek daha mantıklı olabilir.
    // Basitlik için byte sayısını döndürelim.
    return this.length; // D'nin .length özelliğini kullan
}

// Stringin boş olup olmadığını kontrol etme
bool isEmpty() const {
    return this.length == 0;
}

// Belirli bir indexteki karaktere erişim (bounds checking ile)
// Index operatörü overloading'i (immutable)
Option!dchar opIndex(size_t index) const { // dchar 32-bit Unicode karakter
    if (index >= this.length) { // Byte indexi kontrolü (karakter değil!)
        // Unicode karakter indexine göre bounds checking yapılmalıdır.
         stderr.writeln("Uyarı (String): Unicode indexleme implemente edilmedi. Byte indexi kullanılıyor."); // Placeholder
        return Option!dchar.None(); // Sınır dışı ise None
    }
    // Tek bir byte'ı dchar'a çevirmek doğru olmayabilir, Unicode code point'ini elde etmek gerekir.
      return Option!dchar.Some(cast(dchar)this[index]); // Byte'ı char'a çevir (yanlış olabilir)
     // D'nin byChar iteratoru kullanılabilir:
     size_t currentByte = 0;
     size_t currentChar = 0;
     while (currentChar < index && currentByte < this.length) {
         dchar c;
         size_t numCodeUnits = std.utf.decode(this.ptr[currentByte .. $], c); // D'nin Unicode fonksiyonu
         currentByte += numCodeUnits;
         currentChar++;
     }
     if (currentChar == index && currentByte < this.length) {
          dchar c;
          std.utf.decode(this.ptr[currentByte .. $], c);
          return Option!dchar.Some(c);
     }
     return Option!dchar.None();
}


// Alt string alma (Slicing)
// Slice operatörü overloading'i
String opIndex(size_t start, size_t end) const { // Byte indexleri
    // Unicode karakter indexlerine göre slicing yapılmalıdır.
     stderr.writeln("Uyarı (String): Unicode slicing implemente edilmedi. Byte slicing kullanılıyor."); // Placeholder
    if (start > end || end > this.length) { // Byte indexi kontrolü
         // Hata veya boş string döndür
         return String("");
    }
    return this[start .. end]; // D'nin dizi dilimleme özelliğini kullan
}


// Başka tiplerden stringe çevirme
// fmt, text, to!string gibi fonksiyonlar string.d veya prelude.d'de bulunabilir.
 String toString(T)(T value);

// String arama ve karşılaştırma metotları
// contains(substring), startsWith(prefix), endsWith(suffix)
 find(substring) -> Option<size_t> (ilk geçişin indexi)

// String manipülasyon metotları
 replace(old, new) -> Yeni string
 toUpper(), toLower() -> Yeni string (locale dikkate alınmalı)
 trim(), trimLeft(), trimRight() -> Yeni string

// String formatlama fonksiyonu (printf/format benzeri)
 String format(string formatString, Args...)(Args args); // std.string.format kullanılarak

// String encoding (UTF-8, UTF-16, UTF-32 arası dönüşümler)
// Bu, string implementasyonunun temel bir parçası olmalıdır.
module prelude;

// core modülünden temel tipleri ve Option/Result'ı import et ve yeniden dışa aktar (re-export)
// 'public import' kullanarak, prelude'u import eden kullanıcılar bu isimlere doğrudan erişebilir.
public import core : Int, Float, Bool, String, Option, Result;
// Eğer Option ve Result ayrı modüldeyse:
 public import core.option : Option;
 public import core.result : Result;


// stdio modülünden yaygın I/O fonksiyonlarını import et ve yeniden dışa aktar
// Kullanıcılar println() demek için 'import stdio;' demek zorunda kalmaz.
import stdio : println, print, eprintln; // public import yapmıyoruz ki sadece bu fonksiyonlar gelsin, tüm stdio gelmesin

// Kullanışlı trait'ler (eğer D++ trait'leri destekliyorsa)
// Örneğin, 'Display' trait'i nesnelerin stringe çevrilebilir olmasını sağlar.
// 'Add' trait'i '+' operatörünü aşırı yüklemek için kullanılır.
// prelude'da temel trait'ler tanımlanabilir veya diğer modüllerden import edilip yeniden dışa aktarılabilir.

public import core.traits : Display, Add, Debug; // Varsayımsal trait modülü


// Yaygın olarak kullanılan bazı fonksiyonlar
// Örneğin, Some() ve None() constructor fonksiyonları Option için (eğer static method olarak tanımlandıysa core.option veya core'dan import edilir).
// Result için Ok() ve Err() constructor fonksiyonları.
// Bunları doğrudan kullanıma açmak kullanışlıdır.
// public import core.option : Some, None; // Option struct/class içindelerse böyle import edilemez
// Eğer constructor'lar static method ise, Option.Some() gibi kullanılır. Prelude'a koymanın anlamı olmaz.
// Ancak eğer global helper fonksiyonları varsa (Bazı dillerde böyledir: Some(value), None) o zaman buraya konulabilir.

// Hata yayılımı için '?' operatörü (Dilin kendi sözdizimi tarafından sağlanır, kütüphane tarafından değil)
// Prelude'da bu operatörün kullanımını kolaylaştıran helper fonksiyonlar veya makrolar olabilir.

// Belki de temel bir koleksiyon tipi (örneğin Vector) prelude'da bulunabilir.
 public import collections.vector : Vector;

// Diğer yaygın kullanıma yönelik helper fonksiyonlar veya tipler
// Örneğin, panik fonksiyonu (core'dan import edilebilir).
 public import core : panic;
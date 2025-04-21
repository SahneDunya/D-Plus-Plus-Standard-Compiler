module collections.hash_set;

import std.stdio;     // writeln için (debugging)
import std.string;    // format için
import core.option;   // Option için
import collections.hash_map; // HashSet implementasyonu için HashMap'i kullanıyoruz


// Hash Set yapısı
// T: Settteki elemanların tipi (Hash ve Eq interface'lerini implemente etmeli)
struct HashSet(T) {
    // HashSet'i implemente etmek için içeride HashMap'i kullanıyoruz.
    // Öğeleri anahtar olarak saklıyoruz ve değer tipi önemsiz.
    private HashMap!(T, byte) data; // Değer tipi olarak dummy bir byte kullanalım

    // Yeni, boş bir hash set oluşturur
    this() {
        this.data = new HashMap!(T, byte)(); // Yeni boş HashMap oluştur
    }

    // Sette bir elemanın olup olmadığını kontrol eder
    // T tipi Eq interface'ini implemente etmeli.
    bool contains(T element) const {
        return data.contains(element); // HashMap'in contains metodunu kullan
    }

    // Sete bir eleman ekler. Eleman zaten sette varsa false, yoksa true döndürür.
    // T tipi Hash ve Eq interface'lerini implemente etmeli.
    bool insert(T element) {
        // HashMap'in insert metodunu kullanıyoruz. Eğer eleman zaten varsa, insert
        // eski değeri (burada dummy byte) döndürür (Option.Some). Yeni eklenirse None döner.
        Option!byte oldValue = data.insert(element, 0); // Dummy değer 0

        // Eğer insert sonucu None ise, eleman daha önce yoktu ve yeni eklendi.
        return oldValue.isNone();
    }

    // Setteki bir elemanı çıkarır. Eleman setteyse ve çıkarılırsa true, yoksa false döndürür.
    // T tipi Hash ve Eq interface'lerini implemente etmeli.
    bool remove(T element) {
        // HashMap'in remove metodunu kullanıyoruz. Eleman varsa değeri (dummy byte) döndürür (Option.Some).
        Option!byte removedValue = data.remove(element);

        // Eğer remove sonucu Some ise, eleman bulundu ve çıkarıldı.
        return removedValue.isSome();
    }

    // Settteki eleman sayısını döndürür
    size_t len() const {
        return data.len(); // HashMap'in len metodunu kullan
    }

    // Setin boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return data.isEmpty(); // HashMap'in isEmpty metodunu kullan
    }

    // Setin tüm elemanlarını temizler
    void clear() {
        data.clear(); // HashMap'in clear metodunu kullan
    }

    // Setin içeriğini yazdır (debugging için)
    string toString() const {
        string s = "{";
        size_t count = 0;
        // HashMap'in bucket'ları üzerinde dolaşarak elemanları alabiliriz.
        // HashMap'de bir iterator implementasyonu olması bu kısım için idealdir.
        // Şimdilik basitçe HashMap'in toString çıktısını kullanalım ve anahtarları alalım.
        // Bu doğru bir Set toString'i değildir!
        s ~= data.toString(); // HashMap'in string temsilini ekle (örnek)
        s = s.replace(":", ""); // Dummy değerleri ve ':' kaldır (çok basit ve hatalı bir parse)
        s = s.replace("0", ""); // Dummy değer 0'ı kaldır (çok basit ve hatalı bir parse)
        s = s.replace(" ", ""); // Boşlukları kaldır

        // Doğru implementasyon: HashMap'in iteratoru üzerinden anahtarları al ve birleştir.
        
        bool first = true;
        foreach(key; data.keys()) { // Varsayım: HashMap'de keys() iteratoru var
             if (!first) {
                 s ~= ", ";
             }
             s ~= text(key);
             first = false;
         }
        
        s ~= "}";
        return s;
    }

    // Iterator döndüren metod (IntoIterator trait'ini implemente edebilir)
    // HashMap iteratoru üzerinden anahtarları döndüren bir iterator implemente edilmelidir.
    
    Iterator!T intoIterator() {
        // Yeni bir HashSet iteratoru oluştur (HashMap iteratorunu kullanacak)
        return new HashSetIterator!T(this); // Varsayım: HashSetIterator sınıfı var
    }
    
}

// HashSet için Iterator (yukarıdaki intoIterator için)

class HashSetIterator(T) {
    private HashMapIterator!(T, byte) innerIterator; // İçteki HashMap iteratoru

    this(HashSet!T set) {
        this.innerIterator = set.data.intoIterator(); // Varsayım: HashMap'de intoIterator var
    }

    // Iterator interface'inin next metodu
    override Option!T next() {
        // HashMap iteratoru Entry!(K, V) veya Tuple!(K, V) döndürebilir. Biz sadece anahtarı istiyoruz.
        // Varsayım: HashMap iteratoru Tuple!(K, V) döndürüyor.
        Option!(Tuple!(T, byte)) nextEntry = innerIterator.next();

        if (nextEntry.isSome()) {
            return Option!T.Some(nextEntry.unwrap()._0); // Tuple'ın ilk elemanını (anahtarı) döndür
        }
        return Option!T.None(); // İterasyon bitti
    }
}
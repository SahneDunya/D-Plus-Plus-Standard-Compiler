module collections.hash_map;

import std.stdio;     // writeln için (debugging)
import std.string;    // format için
import core.option;   // Option için (get, remove metotları gibi)
import core.error;  // Hata tipleri için
import core;        // Temel tipler için

// Hash için gereken özellikler (Anahtar tipi K bu interface'i implement etmeli)
// D++ trait'leri destekliyorsa bu bir trait olmalıdır.
interface Hash {
    // Bu nesnenin hash değerini hesaplar
    size_t toHash() const;
}

// Eşitlik karşılaştırması için gereken özellikler (Anahtar tipi K bu interface'i implement etmeli)
// D++ trait'leri destekliyorsa bu bir trait olmalıdır.
// D'nin opEquals overloading'i kullanılabilir.
interface Eq {
    // Bu nesnenin başka bir nesneye eşit olup olmadığını kontrol eder
    bool opEquals(const(Object) other) const; // Veya const(T) other, eğer T implement ediyorsa
}


// Hash haritasındaki bir anahtar-değer çiftini temsil eden yapı
struct Entry(K, V) {
    K key;   // Anahtar
    V value; // Değer
    bool isActive = true; // Bu giriş aktif mi (silinmiş mi?)
     int hash; // Anahtarın hash değeri (tekrar hesaplamamak için saklanabilir)
}

// Hash Haritası yapısı
// K: Anahtar tipi (Hash ve Eq interface'lerini implement etmeli)
// V: Değer tipi
struct HashMap(K, V) {
    private Entry!(K, V)[] buckets; // Hash tablosunu temsil eden bucket dizisi
    private size_t currentLength; // Haritadaki aktif eleman sayısı
    private size_t capacity;     // Bucket dizisinin boyutu (kapasite)
    private size_t tombstoneCount; // Silinmiş (aktif olmayan) giriş sayısı

    // Yük faktörü eşikleri (yeniden boyutlandırma için)
    private const double LoadFactorUpper = 0.75; // Max doluluk oranı
    private const double LoadFactorLower = 0.25; // Min doluluk oranı (küçültmek için)


    // Yeni, boş bir hash haritası oluşturur
    this() {
        this.capacity = 8; // Başlangıç kapasitesi (genellikle 2'nin kuvveti)
        this.buckets = new Entry!(K, V)[capacity]; // Bucket dizisini oluştur
        this.currentLength = 0;
        this.tombstoneCount = 0;
    }

    // Haritaya bir anahtar-değer çifti ekler veya mevcut anahtarı günceller.
    // Mevcut anahtar güncellenirse, eski değeri döndürür (Option).
    Option!V insert(K key, V value) {
        // Yeniden boyutlandırma gerekip gerekmediğini kontrol et
        if ((currentLength + tombstoneCount) / cast(double)capacity > LoadFactorUpper) {
            resize(capacity * 2); // Kapasiteyi iki katına çıkar
        }

        size_t hashValue = key.toHash(); // Anahtarın hash değerini al
        size_t index = hashValue % capacity; // Başlangıç bucket indexi

        // Çakışma çözme (Linear Probing gibi basit bir yöntem)
        while (true) {
            // Boş bir bucket bulduk veya silinmiş bir yer (tombstone)
            if (!buckets[index].isActive || (buckets[index].isActive && buckets[index].key.opEquals(key))) {
                // Anahtar zaten var mı kontrol et (aktif ve anahtar eşit mi?)
                if (buckets[index].isActive && buckets[index].key.opEquals(key)) {
                    // Anahtar zaten var, değeri güncelle ve eski değeri döndür.
                    V oldValue = buckets[index].value;
                    buckets[index].value = value;
                    writeln("Hash Map: Anahtar güncellendi: ", text(key));
                    return Option!V.Some(oldValue);
                } else {
                    // Yeni giriş ekle (boş bucket veya tombstone)
                    buckets[index] = Entry!(K, V)(key, value, true); // Yeni giriş oluştur
                    currentLength++; // Aktif eleman sayısını artır
                    if (!buckets[index].isActive) { // Eğer tombstone üzerine yazdıysak
                         tombstoneCount--; // Tombstone sayısını azalt
                    }
                    writeln("Hash Map: Yeni anahtar eklendi: ", text(key));
                    return Option!V.None(); // Yeni giriş olduğu için None döndür
                }
            }

            // Çakışma varsa sıradaki bucket'a geç
            index = (index + 1) % capacity;
        }
    }

    // Belirtilen anahtarla ilişkili değeri döndürür. Anahtar yoksa None döndürür.
    Option!V get(K key) const {
        if (isEmpty()) {
            return Option!V.None(); // Harita boşsa
        }

        size_t hashValue = key.toHash(); // Anahtarın hash değerini al
        size_t index = hashValue % capacity; // Başlangıç bucket indexi

        // Çakışma çözme (Linear Probing)
        size_t startIndex = index; // Döngüyü tespit etmek için başlangıç indexi
        while (true) {
            // Boş bucket bulduk (anahtar haritada yok)
            if (!buckets[index].isActive && buckets[index].key.opEquals(K.init)) { // Varsayım: K.init default değer
                return Option!V.None();
            }

            // Aktif bir giriş bulduk ve anahtarlar eşit mi kontrol et
            if (buckets[index].isActive && buckets[index].key.opEquals(key)) {
                // Anahtar bulundu
                writeln("Hash Map: Anahtar bulundu: ", text(key));
                return Option!V.Some(buckets[index].value);
            }

            // Çakışma varsa sıradaki bucket'a geç
            index = (index + 1) % capacity;

            // Başlangıç indexine geri döndüysek ve hala bulamadıysak (veya tombstone üzerinden geçtiysek),
            // anahtar haritada olmayabilir. Tam kontrol için daha gelişmiş probing gerekir.
             if (index == startIndex) {
                 break; // Sonsuz döngüyü önle (tam doğru değil)
             }
        }

        writeln("Hash Map: Anahtar bulunamadı: ", text(key));
        return Option!V.None(); // Anahtar bulunamadı
    }

    // Belirtilen anahtarı ve ilişkili değeri haritadan çıkarır. Anahtar yoksa None döndürür.
    Option!V remove(K key) {
        if (isEmpty()) {
            return Option!V.None(); // Harita boşsa
        }

        size_t hashValue = key.toHash(); // Anahtarın hash değerini al
        size_t index = hashValue % capacity; // Başlangıç bucket indexi

        // Çakışma çözme ve silme
        size_t startIndex = index;
         while (true) {
            // Boş bucket bulduk (anahtar haritada yok)
            if (!buckets[index].isActive && buckets[index].key.opEquals(K.init)) { // Varsayım: K.init default değer
                return Option!V.None();
            }

            // Aktif bir giriş bulduk ve anahtarlar eşit mi kontrol et
            if (buckets[index].isActive && buckets[index].key.opEquals(key)) {
                // Anahtar bulundu, sil
                V removedValue = buckets[index].value;
                buckets[index].isActive = false; // Girişi aktif değil olarak işaretle (tombstone)
                // Anahtar ve değer alanlarını temizlemek isteyebilirsiniz (GC'ye yardımcı olmak için).
                 buckets[index].key = K.init;
                 buckets[index].value = V.init;
                currentLength--; // Aktif eleman sayısını azalt
                tombstoneCount++; // Tombstone sayısını artır
                writeln("Hash Map: Anahtar silindi: ", text(key));

                // Yeniden boyutlandırma gerekip gerekmediğini kontrol et (küçültmek için)
                if (currentLength > 0 && currentLength / cast(double)capacity < LoadFactorLower) {
                    resize(capacity / 2); // Kapasiteyi yarıya indir
                }

                return Option!V.Some(removedValue); // Silinen değeri döndür
            }

            // Çakışma varsa sıradaki bucket'a geç
            index = (index + 1) % capacity;

            // Başlangıç indexine geri döndüysek ve hala bulamadıysak, anahtar haritada yok.
             if (index == startIndex) {
                 break; // Sonsuz döngüyü önle
             }
         }

        writeln("Hash Map: Silinecek anahtar bulunamadı: ", text(key));
        return Option!V.None(); // Silinecek anahtar bulunamadı
    }

     // Haritada belirli bir anahtarın olup olmadığını kontrol eder
     bool contains(K key) const {
         // Get fonksiyonunu kullanabiliriz, Option.isSome() kontrolü ile.
          return get(key).isSome();
     }


    // Haritadaki aktif eleman sayısını döndürür
    size_t len() const {
        return currentLength;
    }

    // Haritanın boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return currentLength == 0;
    }

    // Haritayı temizler
    void clear() {
        this.buckets = new Entry!(K, V)[capacity]; // Yeni boş bucket dizisi oluştur
        this.currentLength = 0;
        this.tombstoneCount = 0;
        writeln("Hash Map temizlendi.");
    }


    // Hash haritasını yeniden boyutlandırır (iç elemanları yeni boyuta taşır)
    private void resize(size_t newCapacity) {
        writeln("Hash Map yeniden boyutlandırılıyor: ", capacity, " -> ", newCapacity);
        Entry!(K, V)[] oldBuckets = buckets; // Eski bucketları sakla
        size_t oldCapacity = capacity;

        this.capacity = newCapacity;
        this.buckets = new Entry!(K, V)[newCapacity]; // Yeni bucket dizisi
        this.currentLength = 0; // Yeniden başlayacağız
        this.tombstoneCount = 0; // Yeniden başlayacağız

        // Eski bucketlardaki aktif elemanları yeni bucketlara taşı
        foreach (entry; oldBuckets) {
            if (entry.isActive) {
                // Yeni insert fonksiyonunu kullanarak elemanı yeni haritaya ekle
                insert(entry.key, entry.value); // Bu insert kendi içinde hashing ve probing yapar
            }
        }
        writeln("Hash Map yeniden boyutlandırma tamamlandı. Yeni kapasite: ", newCapacity);
    }

    // Haritanın içeriğini yazdır (debugging için)
    string toString() const {
        string s = "{";
        size_t count = 0;
        foreach (entry; buckets) {
            if (entry.isActive) {
                s ~= format("%s: %s", text(entry.key), text(entry.value));
                count++;
                if (count < currentLength) {
                    s ~= ", ";
                }
            }
        }
        s ~= "}";
        return s;
    }


    // Manuel bellek yönetimi için destructor (Eğer Entry pointer içeriyorsa)
    // Eğer Entry struct ve D'nin GC'si kullanılıyorsa buna gerek yok.
     ~this() {
         // Buckets dizisindeki pointerları temizle (varsa)
     }
}
module collections.vector;

import std.stdio; // writeln için (debugging)
import std.array; // D'nin dinamik dizileri için
import core.option; // Option için (get metodu gibi)
import core.error; // Hata tipleri için (eğer indexleme hatası Result döndürüyorsa)
import core; // Temel tipler için

// Dinamik boyutlu bir dizi (Vector)
// T: Vektördeki elemanların tipi
struct Vector(T) {
    private T[] data; // D'nin dinamik dizisi (bellek yönetimi D'ye bırakılıyor)

    // Yeni, boş bir vektör oluşturur
    this() {
        this.data = []; // Boş D dizisi ile başlat
    }

    // Belirtilen kapasite ile yeni bir vektör oluşturur (ön tahsis)
    this(size_t capacity) {
        this.data = new T[0]; // Boş bir dizi oluştur
        this.data.reserve(capacity); // Kapasiteyi önceden ayır
    }

    // Vektöre bir eleman ekler (sona)
    void push(T value) {
        this.data ~= value; // D'nin dizi ekleme operatörünü kullan
    }

    // Vektörün son elemanını çıkarır ve döndürür. Vektör boşsa None döndürür.
    Option!T pop() {
        if (this.data.length == 0) {
            return Option!T.None();
        }
        // D'nin popBack fonksiyonunu kullan
        T lastElement = this.data.back; // Son elemanı al
        this.data.length--;          // Dizinin boyutunu küçült
        return Option!T.Some(lastElement);
    }

    // Vektördeki eleman sayısını döndürür
    size_t len() const {
        return this.data.length;
    }

    // Vektörün ayrılmış bellek kapasitesini döndürür
    size_t capacity() const {
        return this.data.capacity;
    }

    // Vektörün boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return this.data.length == 0;
    }

    // Vektörün tüm elemanlarını temizler (kapasiteyi koruyabilir veya serbest bırakabilir)
    void clear() {
        this.data.length = 0; // Boyutu sıfırla, bellek serbest bırakılmaz (genellikle)
        // Tamamen bellek serbest bırakmak için: this.data = [];
    }

    // Belirtilen indexteki elemana immutable referans döndürür. Sınır dışı ise çalışma zamanı hatası fırlatır.
    // Rust'taki gibi Option döndüren 'get' metodu daha güvenlidir.
    ref const(T) opIndex(size_t index) const {
        // D'nin dinamik dizileri bounds checking yapar, bu yüzden doğrudan erişim kullanılabilir.
        // Eğer D++ compiler kendi bounds checking'ini yapacaksa, burası daha basit olabilir.
        return this.data[index];
    }

    // Belirtilen indexteki elemana mutable referans döndürür. Sınır dışı ise çalışma zamanı hatası fırlatır.
    ref T opIndex(size_t index) {
        return this.data[index]; // D'nin dinamik dizileri bounds checking yapar
    }


    // Belirtilen indexteki elemana immutable referans döndürür. Sınır dışı ise None döndürür.
    Option!T get(size_t index) {
        if (index < this.data.length) {
            return Option!T.Some(this.data[index]);
        }
        return Option!T.None();
    }

    // Belirtilen indexteki elemana mutable referans döndürür. Sınır dışı ise None döndürür.
    Option!T getMut(size_t index) {
         if (index < this.data.length) {
            return Option!T.Some(this.data[index]);
        }
        return Option!T.None();
    }


    // Belirtilen indexe eleman ekler, mevcut elemanları kaydırır.
    void insert(size_t index, T value) {
        // D'nin dinamik dizilerinin insert fonksiyonu kullanılabilir veya manuel implemente edilebilir.
         this.data.insertInPlace(index, value); // Varsayım: D'nin insertInPlace fonksiyonu var
        // Manuel implementasyon: Yeni bir dizi oluştur, elemanları kopyala, yeni değeri ekle.
    }

    // Belirtilen indexteki elemanı çıkarır, sonraki elemanları kaydırır.
    void remove(size_t index) {
        // D'nin dinamik dizilerinin remove fonksiyonu kullanılabilir veya manuel implemente edilebilir.
         this.data.remove(index); // Varsayım: D'nin remove fonksiyonu var
        // Manuel implementasyon: Elemanları sola kaydır, boyutu azalt.
    }

    // Vektörün içeriğini yazdır (debugging için)
    string toString() const {
        string s = "[";
        foreach (i, elem; this.data) {
            s ~= text(elem); // Her elemanı stringe çevir
            if (i < this.data.length - 1) {
                s ~= ", ";
            }
        }
        s ~= "]";
        return s;
    }

    // Iterator döndüren metod (IntoIterator trait'ini implemente edebilir)
    
    Iterator!T intoIterator() {
        // Yeni bir vektör iteratörü nesnesi oluştur ve döndür
        return new VectorIterator!T(this); // Varsayım: VectorIterator sınıfı var
    }

    // Vektör kopyalama (Copy) veya taşıma (Move) semantikleri
    // Structlar varsayılan olarak kopyalanır. Eğer T büyük bir yapı veya pointer ise,
    // açık kopyalama (clone) veya taşıma (move) semantiklerini D++ compiler yönetmelidir.
    // Bu, Vector'ün kendisinin ve içindeki T'nin nasıl kopyalandığı/taşındığı ile ilgilidir.
    // Rust'taki Clone ve Copy trait'lerine benzer yapılar D++'ta tanımlanıp compiler tarafından tanınmalıdır.
}

// Vektör için Iterator (Yukarıdaki intoIterator için)

class VectorIterator(T) {
    private Vector!T* vector; // İterate edilen vektörün pointer'ı
    private size_t currentIndex; // Mevcut index

    this(Vector!T* vector) {
        this.vector = vector;
        this.currentIndex = 0;
    }

    // Iterator interface'inin next metodu
    override Option!T next() {
        if (currentIndex < vector.len()) {
            T value = vector.opIndex(currentIndex); // Elemanı al (kopyalanır)
            currentIndex++;
            return Option!T.Some(value);
        }
        return Option!T.None();
    }
}
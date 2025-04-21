module collections.vec_deque;

import std.stdio;   // writeln için (debugging)
import std.string;  // format için
import core.option; // Option için (pop metotları, get metotları gibi)
import core.error; // Hata tipleri için
import core;      // Temel tipler için
import std.exception; // Hata durumları için

// Vector Deque yapısı
// T: Deque'deki elemanların tipi
struct VecDeque(T) {
    private T[] buffer;     // Öğeleri saklayan döngüsel tampon (D'nin dinamik dizisi)
    private size_t head;    // Deque'nin başındaki elemanın buffer içindeki indexi
    private size_t tail;    // Deque'nin sonundan sonraki boş yerin buffer içindeki indexi
    private size_t currentLength; // Deque'deki eleman sayısı
    private size_t capacity; // Tamponun toplam kapasitesi

    // Yeni, boş bir vector deque oluşturur
    this() {
        this.capacity = 0; // Başlangıç kapasitesi 0
        this.buffer = [];  // Boş tampon
        this.head = 0;     // Başlangıç indexleri 0
        this.tail = 0;
        this.currentLength = 0; // Eleman sayısı 0
    }

    // Belirtilen kapasite ile yeni bir vector deque oluşturur (ön tahsis)
    this(size_t capacity) {
        // Kapasitenin en az 1 olması gerekir (circular buffer için)
        if (capacity == 0) capacity = 1;
        this.capacity = capacity;
        this.buffer = new T[capacity]; // Belirtilen kapasitede tampon oluştur
        this.head = 0;
        this.tail = 0;
        this.currentLength = 0;
    }

    // Deque'nin başına bir eleman ekler
    void pushFront(T value) {
        // Eğer tampon doluysa, yeniden boyutlandır
        if (currentLength == capacity) {
            resize(capacity * 2); // Kapasiteyi iki katına çıkar
        }

        // Head indexini bir geri kaydır (döngüsel olarak)
        head = (head == 0) ? capacity - 1 : head - 1;

        // Yeni elemanı head indexine yerleştir
        buffer[head] = value;
        currentLength++; // Eleman sayısını artır
    }

    // Deque'nin sonuna bir eleman ekler
    void pushBack(T value) {
         // Eğer tampon doluysa, yeniden boyutlandır
        if (currentLength == capacity) {
            resize(capacity * 2); // Kapasiteyi iki katına çıkar
        }

        // Yeni elemanı tail indexine yerleştir
        buffer[tail] = value;

        // Tail indexini bir ileri kaydır (döngüsel olarak)
        tail = (tail + 1) % capacity;
        currentLength++; // Eleman sayısını artır
    }

    // Deque'nin başından bir eleman çıkarır ve döndürür. Deque boşsa None döndürür.
    Option!T popFront() {
        if (isEmpty()) {
            return Option!T.None(); // Deque boşsa None
        }

        // Baş elemanın değerini al
        T value = buffer[head];

        // Head indexini bir ileri kaydır (döngüsel olarak)
        head = (head + 1) % capacity;
        currentLength--; // Eleman sayısını azalt

        // Yeniden boyutlandırma gerekip gerekmediğini kontrol et (küçültmek için)
        // Eğer çok boşaldıysa kapasiteyi azaltabiliriz.
        // Basitlik için bu örnekte küçültme implemente edilmedi.

        return Option!T.Some(value); // Çıkarılan değeri Some içinde döndür
    }

    // Deque'nin sonundan bir eleman çıkarır ve döndürür. Deque boşsa None döndürür.
    Option!T popBack() {
        if (isEmpty()) {
            return Option!T.None(); // Deque boşsa None
        }

        // Tail indexini bir geri kaydır (döngüsel olarak)
        tail = (tail == 0) ? capacity - 1 : tail - 1;

        // Son elemanın değerini al
        T value = buffer[tail];
        currentLength--; // Eleman sayısını azalt

        // Yeniden boyutlandırma (küçültme) kontrolü

        return Option!T.Some(value); // Çıkarılan değeri Some içinde döndür
    }

    // Deque'deki eleman sayısını döndürür
    size_t len() const {
        return currentLength;
    }

    // Tamponun ayrılmış bellek kapasitesini döndürür
    size_t capacity() const {
        return capacity;
    }

    // Deque'nin boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return currentLength == 0;
    }

    // Deque'nin tüm elemanlarını temizler (tamponu koruyabilir veya serbest bırakabilir)
    void clear() {
        // Öğeleri temizlemeye gerek yok, sadece head, tail ve length'i sıfırla
        head = 0;
        tail = 0;
        currentLength = 0;
        // Tamponun içeriği GC tarafından temizlenecektir (eğer T pointer içeriyorsa).
        writeln("VecDeque temizlendi.");
    }

    // Belirtilen indexteki elemana immutable referans döndürür. Index deque'ye göre (0..len-1).
    // Dahili buffer indexini hesaplamak için döngüsel aritmetik kullanılır.
    Option!T get(size_t index) const {
        if (index >= currentLength) {
            return Option!T.None(); // Sınır dışı
        }
        // Deque indexini buffer indexine çevir
        size_t bufferIndex = (head + index) % capacity;
        return Option!T.Some(buffer[bufferIndex]);
    }

    // Belirtilen indexteki elemana mutable referans döndürür. Index deque'ye göre (0..len-1).
    Option!T getMut(size_t index) {
         if (index >= currentLength) {
            return Option!T.None(); // Sınır dışı
        }
        size_t bufferIndex = (head + index) % capacity;
        return Option!T.Some(buffer[bufferIndex]);
    }


    // Deque'yi yeniden boyutlandırır (iç elemanları yeni boyuta taşır)
    private void resize(size_t newCapacity) {
        writeln("VecDeque yeniden boyutlandırılıyor: ", capacity, " -> ", newCapacity);
        // Yeni kapasitenin mevcut eleman sayısından az olmaması gerekir
        if (newCapacity < currentLength) {
            throw new Exception(format("VecDeque resize: Yeni kapasite (%s) mevcut eleman sayısından (%s) az.", newCapacity, currentLength));
        }

        T[] newBuffer = new T[newCapacity]; // Yeni tampon oluştur
        size_t oldCapacity = capacity;

        // Eski tampondaki elemanları yeni tampona kopyala
        // Başlangıçtan itibaren sırayla kopyalamak daha kolaydır.
        for (size_t i = 0; i < currentLength; ++i) {
            size_t oldBufferIndex = (head + i) % oldCapacity;
            newBuffer[i] = buffer[oldBufferIndex]; // Elemanları kopyala
        }

        this.buffer = newBuffer;    // Tamponu yeni tamponla değiştir
        this.capacity = newCapacity; // Kapasiteyi güncelle
        this.head = 0;               // Head artık 0'dır (yeni tamponda)
        this.tail = currentLength;   // Tail, eleman sayısına eşittir
        // currentLength değişmez

        writeln("VecDeque yeniden boyutlandırma tamamlandı. Yeni kapasite: ", newCapacity);
    }


     // Deque'nin içeriğini yazdır (debugging için)
    string toString() const {
        string s = "[";
        for (size_t i = 0; i < currentLength; ++i) {
             size_t bufferIndex = (head + i) % capacity;
             s ~= text(buffer[bufferIndex]); // Elemanı stringe çevir
             if (i < currentLength - 1) {
                 s ~= ", ";
             }
         }
        s ~= "]";
        return s;
    }

    // Iterator döndüren metod (IntoIterator trait'ini implemente edebilir)
    
    Iterator!T intoIterator() {
        // Yeni bir VecDeque iteratoru oluştur
        return new VecDequeIterator!T(this); // Varsayım: VecDequeIterator sınıfı var
    }
    
}

// Vector Deque için Iterator

class VecDequeIterator(T) {
    private VecDeque!T* deque; // İterate edilen deque'nin pointer'ı
    private size_t currentIndex; // Şu anki Deque indexi
    private size_t elementsLeft; // Kalan eleman sayısı

    this(VecDeque!T deque) {
        this.deque = deque;
        this.currentIndex = 0; // Deque indexi olarak başla
        this.elementsLeft = deque.len();
    }

    // Iterator interface'inin next metodu
    override Option!T next() {
        if (elementsLeft == 0) {
            return Option!T.None(); // İterasyon bitti
        }

        // Deque indexinden buffer indexini hesapla
        size_t bufferIndex = (deque.head + currentIndex) % deque.capacity;
        T value = deque.buffer[bufferIndex]; // Elemanı al (kopyalanır)

        currentIndex++;
        elementsLeft--;

        return Option!T.Some(value);
    }
}
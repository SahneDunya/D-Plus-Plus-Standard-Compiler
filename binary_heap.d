module collections.binary_heap;

import std.stdio;    // writeln için (debugging)
import std.string;   // format için
import core.option;  // Option için (pop, peek metotları gibi)
import collections.vector; // BinaryHeap implementasyonu için Vector'u kullanıyoruz
import core.error; // Hata tipleri için

// Sıralama için gereken özellikler (Eleman tipi T bu interface'i implemente etmeli)
// D++ trait'leri destekliyorsa bu bir trait olmalıdır.
interface Ord {
    // Bu nesneyi başka bir nesneyle karşılaştırır
    // Returns: < 0 eğer bu < diğer, 0 eğer bu == diğer, > 0 eğer bu > diğer
    int opCmp(const(Object) other) const; // Veya const(T) other
}


// İkili Yığın (Binary Heap) yapısı (Maksimum yığın - Max Heap)
// T: Yığındaki elemanların tipi (Ord interface'ini implemente etmeli)
struct BinaryHeap(T) {
    // İkili yığını implemente etmek için içeride Vector'u kullanıyoruz.
    // Kök 0 indexindedir. Bir düğümün indexi i ise, çocukları 2*i + 1 ve 2*i + 2'dir.
    // Bir çocuğun indexi j ise, ebeveyni floor((j - 1) / 2)'dir.
    private Vector!T data; // Yığın elemanlarını saklayan Vector

    // Yeni, boş bir ikili yığın oluşturur
    this() {
        this.data = new Vector!T(); // Yeni boş Vector oluştur
    }

    // Yığına bir eleman ekler
    void push(T value) {
        data.push(value); // Elemanı Vector'ün sonuna ekle
        siftUp(data.len() - 1); // Yığın özelliğini korumak için yukarı eleme yap
    }

    // Yığından en yüksek öncelikli elemanı (kökü) çıkarır ve döndürür. Yığın boşsa None döndürür.
    Option!T pop() {
        if (isEmpty()) {
            return Option!T.None(); // Yığın boşsa None
        }

        // En yüksek öncelikli eleman köktedir (index 0)
        T rootValue = data.opIndex(0); // Kök değeri al

        // Son elemanı köke taşı
        if (data.len() > 1) {
            T lastElement = data.pop().unwrap(); // Sondaki elemanı çıkar (Option.Some dönecektir)
            data.opIndex(0) = lastElement; // Kök yerine koy
            siftDown(0); // Yığın özelliğini korumak için aşağı eleme yap (köktən başla)
        } else {
            // Yığında tek eleman varsa, sadece onu çıkar
            data.pop(); // Tek elemanı çıkar
        }

        return Option!T.Some(rootValue); // Çıkarılan kök değerini döndür
    }

    // Yığındaki en yüksek öncelikli elemana (köke) bakar, çıkarmaz. Yığın boşsa None döndürür.
    Option!T peek() const {
        if (isEmpty()) {
            return Option!T.None(); // Yığın boşsa None
        }
        // Kök elemana bak
        return Option!T.Some(data.opIndex(0));
    }


    // Yığındaki eleman sayısını döndürür
    size_t len() const {
        return data.len(); // Vector'ün len metodunu kullan
    }

    // Yığının boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return data.isEmpty(); // Vector'ün isEmpty metodunu kullan
    }

    // Yığının tüm elemanlarını temizler
    void clear() {
        data.clear(); // Vector'ün clear metodunu kullan
    }


    // Bir elemanı yığın özelliğini koruyarak yukarı doğru taşır
    private void siftUp(size_t index) {
        size_t current = index;
        // Kök değilse ve ebeveyninden büyükse
        while (current > 0) {
            size_t parent = (current - 1) / 2;
            // Elemanları karşılaştır (T tipinin Ord interface'ini kullan)
            if (data.opIndex(current).opCmp(data.opIndex(parent)) > 0) { // current > parent
                // Elemanları yer değiştir
                swap(data.opIndex(current), data.opIndex(parent));
                current = parent; // Ebeveyn indexine geç
            } else {
                break; // Yığın özelliği sağlandı
            }
        }
    }

    // Bir elemanı yığın özelliğini koruyarak aşağı doğru taşır
    private void siftDown(size_t index) {
        size_t current = index;
        size_t leftChild = 2 * current + 1;
        size_t rightChild = 2 * current + 2;
        size_t largest = current; // En büyük elemanın indexi (başlangıçta kendisi)

        // Sol çocuğu kontrol et
        if (leftChild < data.len() && data.opIndex(leftChild).opCmp(data.opIndex(largest)) > 0) { // leftChild > largest
            largest = leftChild;
        }

        // Sağ çocuğu kontrol et
        if (rightChild < data.len() && data.opIndex(rightChild).opCmp(data.opIndex(largest)) > 0) { // rightChild > largest
            largest = rightChild;
        }

        // Eğer en büyük eleman mevcut elemanın kendisi değilse
        if (largest != current) {
            // Mevcut eleman ile en büyük çocuğu yer değiştir
            swap(data.opIndex(current), data.opIndex(largest));
            // Yer değiştirdiğimiz çocuğun olduğu yerden aşağı eleme yapmaya devam et
            siftDown(largest);
        }
        // Eğer largest == current ise, bu alt ağaçta yığın özelliği sağlanmıştır.
    }

    // İki referansın değerlerini yer değiştiren yardımcı fonksiyon
    private void swap(ref T a, ref T b) {
        T temp = a;
        a = b;
        b = temp;
    }


    // Yığının içeriğini yazdır (debugging için - sıralı olmayabilir)
    string toString() const {
        return data.toString(); // İçindeki Vector'ün string temsilini kullan
    }

    // Iterator döndüren metod (IntoIterator trait'ini implemente edebilir)
    // BinaryHeap iteratörleri genellikle elemanları sıralı olarak değil, yığın yapısına göre döndürür.
    // Sıralı erişim için elemanları pop etmeniz gerekir.
    
    Iterator!T intoIterator() {
        // Vector iteratorunu kullanabiliriz ama sıralı değildir.
        // return data.intoIterator();
        // Yığın yapısını koruyarak dolaşan özel bir iterator gerekebilir.
         stderr.writeln("Uyarı: BinaryHeap iteratoru implemente edilmedi.");
         return null; // Placeholder
    }
    
}

// Ord interface'ini implemente etmesi gereken T tipi için örnek (Integer gibi)

struct MyInteger : Ord {
    int value;
    this(int v) { this.value = v; }
    override int opCmp(const(Object) other) const {
        if (auto otherInt = cast(const(MyInteger))other) {
            return this.value - otherInt.value;
        }
        throw new Exception("Karşılaştırma uyumsuz tipi!"); // Hata yönetimi
    }
    string toString() const { return text(value); }
    // Eq interface'ini de implemente etmelidir (opEquals)
}
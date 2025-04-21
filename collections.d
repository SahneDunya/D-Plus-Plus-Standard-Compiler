module collections;

// Farklı koleksiyon modüllerini import et ve yaygın kullanılanları yeniden dışa aktar (re-export)
public import collections.vector : Vector;       // vector.d'deki Vector'u dışa aktar
public import collections.hash_map : HashMap; // hash_map.d'deki HashMap'i dışa aktar
public import collections.list : LinkedList; // list.d'deki LinkedList'i dışa aktar
// ... Diğer koleksiyon modülleri


// Tüm koleksiyonlar için geçerli olabilecek ortak trait'ler (eğer D++ trait'leri destekliyorsa)
// Bu trait'ler, farklı koleksiyon tiplerinin aynı arayüz üzerinden kullanılmasını sağlar.

// Bir koleksiyonun elemanları üzerinde döngü yapabilme yeteneği
interface IntoIterator(T) {
    // Bu koleksiyon için bir iterator nesnesi döndürür
    Iterator!T intoIterator();
}

// Indexleme yeteneği (immutable)
interface Index(Idx, Output) {
    // Belirtilen indexteki elemana immutable referans döndürür
    ref const(Output) opIndex(Idx index) const;
}

// Indexleme yeteneği (mutable)
interface IndexMut(Idx, Output) {
    // Belirtilen indexteki elemana mutable referans döndürür
    ref Output opIndex(Idx index);
}

// Koleksiyonun boyutunu alabilme
interface SizedCollection {
    // Koleksiyondaki eleman sayısını döndürür
    size_t len() const;
    // Koleksiyonun boş olup olmadığını kontrol eder
    bool isEmpty() const;
}

// ... Diğer ortak trait'ler (Extend, FromIterator vb.)
*/


// Ortak yardımcı fonksiyonlar veya tipler
// Örneğin, bir iteratör tanımı (eğer traitler kullanılıyorsa IntoIterator için gereklidir)

// Iterator nesnesini temsil eden temel interface/trait
interface Iterator(T) {
    // Bir sonraki elemanı döndürür (Eğer varsa Some(value), yoksa None)
    Option!T next(); // core.option'dan Option import edilmeli
}

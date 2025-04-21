module collections.list;

import std.stdio;     // writeln için (debugging)
import std.string;    // format için
import core.option;   // Option için (pop metotları gibi)
import core.error;  // Hata tipleri için

// Bağlı listedeki tek bir elemanı (düğümü) temsil eden sınıf
// Class kullanmak D'nin çöp toplamasını kullanmamızı kolaylaştırır.
class Node(T) {
    T value;         // Düğümün sakladığı değer
    Node!T* prev;    // Önceki düğüme işaretçi
    Node!T* next;    // Sonraki düğüme işaretçi

    this(T value) {
        this.value = value;
        this.prev = null; // Başlangıçta önceki düğüm yok
        this.next = null; // Başlangıçta sonraki düğüm yok
    }

    // Düğümün temizlenmesi (eğer manuel bellek yönetimi yapılıyorsa)
    // Eğer GC kullanılıyorsa bu explicit temizliğe gerek kalmaz.
     ~this() { // Destructor
        writeln("Node temizlendi: ", text(value));
     }
}


// Çift Bağlı Liste yapısı
// T: Liste elemanlarının tipi
struct LinkedList(T) {
    private Node!T* head; // Listenin başındaki düğüme işaretçi
    private Node!T* tail; // Listenin sonundaki düğüme işaretçi
    private size_t currentLength; // Listedeki eleman sayısı

    // Yeni, boş bir bağlı liste oluşturur
    this() {
        this.head = null; // Başlangıçta baş düğüm yok
        this.tail = null; // Başlangıçta son düğüm yok
        this.currentLength = 0; // Başlangıçta eleman sayısı 0
    }

    // Listenin başına bir eleman ekler
    void pushFront(T value) {
        Node!T* newNode = new Node!T(value); // Yeni düğüm oluştur

        if (isEmpty()) {
            // Liste boşsa, yeni düğüm hem baş hem son olur
            head = newNode;
            tail = newNode;
        } else {
            // Liste boş değilse, yeni düğümü başa ekle
            newNode.next = head; // Yeni düğümün next'i eski başı gösterir
            head.prev = newNode; // Eski başın prev'i yeni düğümü gösterir
            head = newNode;      // Baş artık yeni düğümdür
        }
        currentLength++; // Eleman sayısını artır
    }

    // Listenin sonuna bir eleman ekler
    void pushBack(T value) {
        Node!T* newNode = new Node!T(value); // Yeni düğüm oluştur

        if (isEmpty()) {
            // Liste boşsa, yeni düğüm hem baş hem son olur
            head = newNode;
            tail = newNode;
        } else {
            // Liste boş değilse, yeni düğümü sona ekle
            newNode.prev = tail; // Yeni düğümün prev'i eski sonu gösterir
            tail.next = newNode; // Eski sonun next'i yeni düğümü gösterir
            tail = newNode;      // Son artık yeni düğümdür
        }
        currentLength++; // Eleman sayısını artır
    }

    // Listenin başından bir eleman çıkarır ve döndürür. Liste boşsa None döndürür.
    Option!T popFront() {
        if (isEmpty()) {
            return Option!T.None(); // Liste boşsa None
        }

        Node!T* oldHead = head; // Eski baş düğümü al
        T value = oldHead.value; // Değeri sakla

        if (currentLength == 1) {
            // Listede tek eleman varsa, baş ve son null olur
            head = null;
            tail = null;
        } else {
            // Listede birden fazla eleman varsa, başı bir sonraki düğüme kaydır
            head = oldHead.next;
            head.prev = null; // Yeni başın prev'i null olur
        }
        currentLength--; // Eleman sayısını azalt

        // Eski baş düğümü temizle (eğer manuel bellek yönetimi yapılıyorsa)
        // Eğer GC kullanılıyorsa bu explicit temizliğe gerek kalmaz.
         destroy(oldHead);

        return Option!T.Some(value); // Çıkarılan değeri Some içinde döndür
    }

    // Listenin sonundan bir eleman çıkarır ve döndürür. Liste boşsa None döndürür.
    Option!T popBack() {
        if (isEmpty()) {
            return Option!T.None(); // Liste boşsa None
        }

        Node!T* oldTail = tail; // Eski son düğümü al
        T value = oldTail.value; // Değeri sakla

        if (currentLength == 1) {
            // Listede tek eleman varsa, baş ve son null olur
            head = null;
            tail = null;
        } else {
            // Listede birden fazla eleman varsa, sonu bir önceki düğüme kaydır
            tail = oldTail.prev;
            tail.next = null; // Yeni sonun next'i null olur
        }
        currentLength--; // Eleman sayısını azalt

        // Eski son düğümü temizle (eğer manuel bellek yönetimi yapılıyorsa)
        // destroy(oldTail);

        return Option!T.Some(value); // Çıkarılan değeri Some içinde döndür
    }

    // Listedeki eleman sayısını döndürür
    size_t len() const {
        return currentLength;
    }

    // Listenin boş olup olmadığını kontrol eder
    bool isEmpty() const {
        return currentLength == 0;
    }

    // Listenin tüm elemanlarını temizler (düğümleri siler)
    void clear() {
        Node!T* current = head;
        while (current) {
            Node!T* next = current.next;
            // Düğümü temizle (eğer manuel bellek yönetimi yapılıyorsa)
            // destroy(current);
            current = next;
        }
        head = null;
        tail = null;
        currentLength = 0;
         // GC kullanılıyorsa, head ve tail'i null yapmak düğümlere olan referansları koparır
         // ve GC onları temizleyecektir.
    }


    // Belirtilen indexteki elemana immutable referans döndürür. Index geçersizse hata veya Option.
    // Bağlı listelerde indexleme verimli değildir, bu yüzden dikkatli kullanılmalıdır.
    Option!T get(size_t index) const {
        if (index >= currentLength) {
            return Option!T.None(); // Sınır dışı
        }
        Node!T* current = head;
        for (size_t i = 0; i < index; ++i) {
            current = current.next;
        }
        return Option!T.Some(current.value);
    }

    // Belirtilen indexteki elemana mutable referans döndürür.
    Option!T getMut(size_t index) {
        if (index >= currentLength) {
            return Option!T.None(); // Sınır dışı
        }
        Node!T* current = head;
        for (size_t i = 0; i < index; ++i) {
            current = current.next;
        }
        return Option!T.Some(current.value);
    }


    // Vektörün içeriğini yazdır (debugging için)
    string toString() const {
        string s = "[";
        Node!T* current = head;
        size_t count = 0;
        while (current) {
            s ~= text(current.value); // Düğüm değerini stringe çevir
            count++;
            if (count < currentLength) {
                s ~= ", ";
            }
            current = current.next;
        }
        s ~= "]";
        return s;
    }

    // Iterator döndüren metod (IntoIterator trait'ini implemente edebilir)
    
    Iterator!T intoIterator() {
        // Yeni bir bağlı liste iteratörü nesnesi oluştur
        return new LinkedListIterator!T(this); // Varsayım: LinkedListIterator sınıfı var
    }
    

    // Manuel bellek yönetimi için destructor
    // Eğer Node class ve GC kullanıyorsak buna gerek yok, GC temizler.
    // Eğer Node struct ve manuel new/destroy kullanıyorsak gerekebilir.
     ~this() {
         clear(); // Liste silindiğinde düğümleri temizle
     }
}

// Çift bağlı liste için Iterator

class LinkedListIterator(T) {
    private Node!T* currentNode; // Şu anki düğüm
    private bool isDone; // İterasyon bitti mi?

    this(LinkedList!T list) {
        this.currentNode = list.head;
        this.isDone = list.isEmpty();
    }

    // Iterator interface'inin next metodu
    override Option!T next() {
        if (isDone || !currentNode) {
            isDone = true;
            return Option!T.None();
        }
        T value = currentNode.value; // Değeri al (kopyalanır)
        currentNode = currentNode.next;
        if (!currentNode) {
            isDone = true; // Son düğümdü
        }
        return Option!T.Some(value);
    }
}
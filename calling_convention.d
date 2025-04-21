module calling_convention;

import std.stdio;
import dpp_types; // D++ tür temsilcileri
import ir;        // Register türü için
import std.array;

// Desteklenen çağırma kuralı türleri (Hedef mimariye bağlı olarak değişir)
enum CallingConventionType {
    SystemV_AMD64, // Linux, macOS gibi sistemlerde x86-64 için yaygın
    Windows_x64,   // Windows'ta x86-64 için
    // ... Diğer mimariler ve işletim sistemleri için
    Simple_Stack_Based // Basit, örnek amaçlı stack tabanlı kural
}

// Bir çağırma kuralını tanımlayan sınıf
// Hedef mimariye ve işletim sistemine özgü bilgiler içerir.
class CallingConvention {
    CallingConventionType type;

    this(CallingConventionType type) {
        this.type = type;
    }

    // Bir fonksiyonun argümanlarının nasıl geçirileceğini belirler
    // parameterTypes: Fonksiyonun parametrelerinin tipleri
    // Returns: Argümanların nasıl geçirileceğini belirten bir yapı (Register, StackOffset vb.)
    // Gerçek implementasyonlar çok daha karmaşıktır.
    void determineArgumentPassing(const DppType*[] parameterTypes) const {
        writeln("Çağırma Kuralı (", type.to!string, "): Argüman geçirme belirleniyor...");
        // Bu fonksiyon, hangi argümanın hangi kayıttan geçeceğini veya stack'e kaç byte yerleştirileceğini belirler.
        // Örneğin (SystemV AMD64 için): İlk 6 tam sayı/pointer argüman RDI, RSI, RDX, RCX, R8, R9 kayıtlarından geçer.
        // Kalan argümanlar stack'e sağdan sola doğru yerleştirilir. Floating point argümanlar XMM kayıtlarından geçer.

        if (type == CallingConventionType.Simple_Stack_Based) {
            writeln("  Basit Stack Tabanlı Kural: Tüm argümanlar stack'e itilir.");
            // Implementasyon: Parametre tiplerinin boyutuna göre stack offsetleri belirlenir.
        } else if (type == CallingConventionType.SystemV_AMD64) {
             writeln("  SystemV AMD64 Kuralı: İlk argümanlar kayıtlardan, kalanı stack'ten geçer.");
             // Implementasyon: Kayıt atama ve stack offset belirleme mantığı buraya gelir.
         }
        // ... Diğer kurallar
    }

    // Bir fonksiyonun dönüş değerinin nasıl döndürüleceğini belirler
    // returnType: Fonksiyonun dönüş tipi
    // Returns: Dönüş değerinin nasıl alınacağını belirten bir yapı (Register, Bellek Konumu vb.)
    void determineReturnPassing(const DppType* returnType) const {
        writeln("Çağırma Kuralı (", type.to!string, "): Dönüş değeri geçirme belirleniyor...");
        // Örneğin (SystemV AMD64 için): Tam sayı/pointer dönüş değerleri RAX kaydından döner.
        // Floating point dönüş değerleri XMM0 kaydından döner. Büyük yapılar bellekten dönebilir.

         if (type == CallingConventionType.Simple_Stack_Based) {
            writeln("  Basit Stack Tabanlı Kural: Dönüş değeri stack'e itilir veya belirli bir stack slotuna konur.");
        } else if (type == CallingConventionType.SystemV_AMD64) {
             writeln("  SystemV AMD64 Kuralı: Dönüş tipi ve boyutuna göre kayıt veya bellek kullanılır.");
         }
        // ... Diğer kurallar
    }

    // Fonksiyon çağrısı sırasında kayıtların nasıl yönetileceğini belirler (Caller-save vs Callee-save)
    void determineRegisterUsage() const {
        writeln("Çağırma Kuralı (", type.to!string, "): Kayıt kullanım kuralları belirleniyor.");
        // Hangi kayıtların çağıran (caller) tarafından korunması gerektiği (callee-save),
        // hangilerinin çağrılan (callee) tarafından korunması gerektiği (caller-save).
        // Bu bilgi, backend'de kayıt tahsisi (register allocation) sırasında kullanılır.
    }

    // Diğer çağırma kuralı ile ilgili bilgiler (stack hizalaması, varargs işleme vb.)
}

// Çağırma kuralı tipini stringe çevirmek için yardımcı fonksiyon
string to!string(CallingConventionType type) {
    final switch (type) {
        case CallingConventionType.SystemV_AMD64: return "SystemV_AMD64";
        case CallingConventionType.Windows_x64: return "Windows_x64";
        case CallingConventionType.Simple_Stack_Based: return "Simple_Stack_Based";
    }
}


// Belirli bir hedef için (mimari, OS) çağırma kuralı instance'ını döndüren fonksiyon
// Derleyici yapılandırmasına göre doğru çağırma kuralını seçer.
// CompilerOptions veya hedef bilgisi parametre olarak alınabilir.
CallingConvention* getCallingConvention(CallingConventionType type) {
    // Hedefe göre uygun çağırma kuralı nesnesini oluştur veya döndür
    // Bu nesneler önceden oluşturulmuş singletonlar olabilir.
    writeln("Çağırma Kuralı alınıyor: ", type.to!string);
    return new CallingConvention(type); // Basitçe yeni bir nesne döndürelim
}
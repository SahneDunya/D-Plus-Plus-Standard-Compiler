module core;

import types; // D++ tür temsilcileri için (varsayımsal types.d'deki DppType ve alt sınıfları)
import mem;   // Temel bellek yönetimi fonksiyonları için (malloc, free gibi, eğer manuel yönetim expose ediliyorsa)

// D++'ın temel (primitive) türlerine takma adlar (alias) veya wrapper'lar
// Compiler tarafından doğrudan desteklenen türlere referans verir.
alias Int = DppType.IntType;     // types.d'deki IntType singleton'ına takma ad
alias Float = DppType.FloatType; // types.d'deki FloatType singleton'ına takma ad
alias Bool = DppType.BoolType;   // types.d'deki BoolType singleton'ına takma ad
alias Char = DppType.CharType;   // Eğer CharType varsa
alias Byte = DppType.ByteType;   // Eğer ByteType varsa
alias Void = DppType.VoidType;   // types.d'deki VoidType singleton'ına takma ad

// String tipi (genellikle primitive'den ayrı ele alınır)
alias String = DppType.StringType; // types.d'deki StringType singleton'ına takma ad

// Result ve Option tipleri (result_option_types.d'den alınabilir veya burada tanımlanabilir)
// Genellikle core'un bir parçası olarak düşünülürler.
// import result_option_types; // Eğer ayrı dosyadalarsa
 alias Result(T, E) = result_option_types.DppResult!(T, E);
 alias Option(T) = result_option_types.DppOption!T;

// Çekirdek seviye hatalar için temel bir hata türü (Result E tipi için kullanılabilir)
 struct CoreError { /* ... */ }

// Temel bellek yönetimi fonksiyonları (eğer dil manuel yönetimi expose ediyorsa)
// Compiler'ın çalışma zamanı tarafından sağlanan fonksiyonlara çağrı yapabilir.

void* allocate(size_t bytes) {
    // Compiler'ın veya çalışma zamanının tahsis fonksiyonunu çağır
     return __builtin_allocate(bytes); // Varsayımsal yerleşik fonksiyon
}

void deallocate(void* ptr) {
    // Compiler'ın veya çalışma zamanının serbest bırakma fonksiyonunu çağır
     __builtin_deallocate(ptr); // Varsayımsal yerleşik fonksiyon
}

// Diğer çok temel, çekirdek seviye fonksiyonlar veya yapılar
// Örneğin, panic! makrosunun çalışma zamanı implementasyonuna bir referans.
 void __dpp_panic(string message, string file, int line); // Compiler runtime tarafından sağlanan yerleşik fonksiyon
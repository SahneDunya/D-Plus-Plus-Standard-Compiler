#include "driver/CompilerDriver.h"
#include "utils/ErrorReporter.h"

// Gerekli LLVM başlık dosyaları
// Hedefleri ve arka uçları başlatmak için
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ManagedStatic.h" // llvm_shutdown için
#include "llvm/Support/CommandLine.h"   // LLVM'in komut satırı işleyicisi (isteğe bağlı ama önerilir)
#include "llvm/Support/InitLLVM.h"      // Daha modern LLVM init için (alternatif)

#include <iostream>
#include <string>
#include <vector>

// LLVM'in komut satırı argüman işleyicisini kullanmak için değişkenler tanımlayabiliriz
// (Bu, daha karmaşık argümanları yönetmeyi kolaylaştırır)
llvm::cl::opt<std::string> InputFilename(llvm::cl::Positional, llvm::cl::desc("<input file>"), llvm::cl::Required);

// Diğer komut satırı argümanları (örneğin, çıktı dosyası, optimizasyon seviyesi vb.)
 llvm::cl::opt<std::string> OutputFilename("o", llvm::cl::desc("Output filename"), llvm::cl::value_desc("filename"));
 llvm::cl::opt<int> OptLevel("O", llvm::cl::desc("Optimization level: 0-3"), llvm::cl::init(2));
// ... diğer seçenekler (verbose, hedef mimari vb.)

int main(int argc, char* argv[]) {
    // LLVM'in init fonksiyonunu çağırıyoruz. Bu, bazı dahili yapıları ayarlar.
    // Alternatif ve daha modern bir yol:
     llvm::InitLLVM X(argc, argv, "dppc\n"); // Program adı ve kısa açıklama
     return X.run([&]() { ... derleyici mantığı ... });

    // Temel argüman işleme veya LLVM'in komut satırı işleyicisini kullanma
    #if 0 // LLVM'in cl kütüphanesini kullanmıyorsanız
    if (argc < 2) {
        std::cerr << "Kullanım: " << argv[0] << " <kaynak_dosya.d++> [seçenekler]\n";
        return 1; // Hata durumunu belirt
    }
    std::string inputFile = argv[1]; // İlk argümanı kaynak dosya olarak al

    // Daha fazla argüman işleme burada yapılabilir
    #else // LLVM'in cl kütüphanesini kullanıyorsanız
    // LLVM komut satırı işleyicisini parse et
    // Bu, yukarıda tanımladığınız InputFilename ve diğer opt değişkenlerini doldurur.
    llvm::cl::ParseCommandLineOptions(argc, argv, "D++ Programlama Dili Derleyicisi\n");

    std::string inputFile = InputFilename; // cl::opt tarafından dolduruldu
    // Diğer argümanlara OutputFilename, OptLevel vb. üzerinden erişilebilir
    #endif


    // 1. LLVM Hedeflerini ve Arka Uçlarını Başlatma
    // Bu, derleyicinizin kod üreteceği mimarileri kullanıma hazırlar.
    // RISC-V'yi hedeflediğiniz için ilgili init fonksiyonlarını çağırıyoruz.
    // LLVM sürümüne göre bu fonksiyon isimleri veya ihtiyacınız olanlar değişebilir.
    // Genellikle hepsi birden veya sadece ihtiyacınız olanlar başlatılır.

    // Sadece spesifik RISC-V hedefini başlatmak için:
    LLVMInitializeRISCVTargetInfo();
    LLVMInitializeRISCVTarget();
    LLVMInitializeRISCVTargetMC();
    LLVMInitializeRISCVAsmPrinter(); // Assembly çıktısı için
     LLVMInitializeRISCVAsmParser(); // Assembly girdisi için (genellikle derleyici için gerekmez)

    // Ya da mevcut tüm hedefleri ve arka uçları başlatmak isterseniz:
     LLVMInitializeAllTargetInfos();
     LLVMInitializeAllTargets();
     LLVMInitializeAllTargetMCs();
     LLVMInitializeAllAsmPrinters();
     LLVMInitializeAllAsmParsers();
     LLVMInitializeAllDisassemblers();

    // 2. Hata Raporlayıcıyı Başlatma
    // Derleme sırasında oluşacak hataları kullanıcıya bildirmek için kullanılır.
    // errorReporter nesnesi hataları standart hata akımına (stderr) yazabilir
    // veya farklı bir raporlama mekanizması kullanabilir.
    ErrorReporter errorReporter;

    // 3. Derleyici Sürücüsünü Başlatma
    // CompilerDriver sınıfı, derleme işleminin adımlarını (lex, parse, sema, irgen, codegen)
    // yönetecek ana orkestratördür. Hata raporlayıcıyı ve hedef mimari bilgisini alır.
    // Hedef mimari triple bilgisi (örneğin "riscv64-unknown-elf"), LLVM'in hangi hedef için kod üreteceğini belirler.
    // Bu bilgiyi komut satırından veya yapılandırmadan alabilirsiniz.
    // Şimdilik RISC-V 64-bit için sabit bir değer kullanalım:
    std::string targetTriple = "riscv64-unknown-elf"; // Örnek RISC-V 64-bit hedef triple'ı
     std::string targetTriple = llvm::sys::getDefaultTargetTriple(); // Sistemin varsayılanını almak isterseniz

    CompilerDriver driver(errorReporter, targetTriple);

    // 4. Derleme İşlemini Başlatma
    // CompilerDriver sınıfının compile metodunu çağırarak tüm süreci başlatıyoruz.
    // Bu metodun derleme başarılı olursa 'true', başarısız olursa 'false' döndürmesini bekleriz.
    bool success = driver.compile(inputFile);

    // 5. Sonucu Değerlendirme ve Çıkış
    int exitCode = success ? 0 : 1; // Başarılıysa 0, değilse 1 döndür

    if (!success) {
         // Hata mesajları zaten ErrorReporter tarafından yazılmış olmalı
         std::cerr << "Derleme başarısız: " << inputFile << "\n";
    } else {
         // İsteğe bağlı: Başarılı mesajı
          std::cerr << "Derleme başarılı: " << inputFile << "\n";
    }

    // 6. LLVM Kaynaklarını Temizleme
    // LLVM'in dahili statik nesnelerini temizlemek için bu çağrı önemlidir.
    // Özellikle derleyici birden fazla kez çalışacaksa veya bellek sızıntılarını önlemek için.
    llvm::llvm_shutdown();

    return exitCode;
}

#include "CodeGen.h"

// LLVM Backend Başlıkları (Daha fazlası gerekebilir)
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h" // LLVMInitializeAll... fonksiyonları için
#include "llvm/Support/FileSystem.h"   // File operations için
#include "llvm/IR/LegacyPassManager.h" // Legacy PassManager için (basit kod üretimi)
#include "llvm/MC/TargetRegistry.h"   // TargetRegistry için (bazı versiyonlarda)


#include <iostream> // Debug için

namespace dppc {
namespace codegen {

// CodeGen Constructor
CodeGen::CodeGen(ErrorReporter& reporter, const std::string& targetTriple, const std::string& cpu, const std::string& features, OutputType outputKind)
    : Reporter(reporter), TargetTriple(targetTriple), CPU(cpu), Features(features), OutputKind(outputKind)
{
    // LLVM hedeflerini ve backend'lerini başlat
    // Bu fonksiyonlar genellikle uygulamanın başlangıcında bir kez çağrılmalıdır.
    // CompilerDriver veya main.cpp'de çağırmak daha temiz olabilir.
    // Burada çağırmak, bu bileşenin kendi kendine yetmesini sağlar ama birden fazla CodeGen nesnesi
    // oluşturulursa gereksiz yere tekrar başlatma yapabilir.
    // Güvenli bir başlangıç için burada çağıralım.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Hedef makineyi oluştur
    const llvm::Target* target = findTarget();
    if (!target) {
        TargetMach = nullptr; // Hata oluştu, TargetMach null kalır
        Reporter.reportError(SourceLocation(), "Kod üretimi: Hedef '" + TargetTriple + "' bulunamadı.");
        return;
    }

    TargetMach = createTargetMachine(target);

    if (!TargetMach) {
         Reporter.reportError(SourceLocation(), "Kod üretimi: Hedef makine oluşturulamadı.");
    }

     std::cerr << "DEBUG: CodeGen initialized for target " << TargetTriple << ", CPU " << CPU << ", Features " << Features << std::endl; // Debug
}

// Hedef üçlemesine göre LLVM Target'ı bulur
const llvm::Target* CodeGen::findTarget() {
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(TargetTriple, error);
    if (!target) {
        // Hata raporlama constructor'da yapılıyor
         Reporter.reportError(SourceLocation(), "Hedef üçlemesi bulunamadı: " + TargetTriple + ". Hata: " + error);
        return nullptr;
    }
    return target;
}

// LLVM Target'a göre TargetMachine oluşturur
std::unique_ptr<llvm::TargetMachine> CodeGen::createTargetMachine(const llvm::Target* target) {
    if (!target) return nullptr;

    llvm::TargetOptions options;
    // TargetOptions üzerinde çeşitli ayarlar yapılabilir (örn: PIC, TLS, FramePointer vb.)
     options.FloatABIType = llvm::FloatABI::Soft; // Örnek: Soft float ABI

    // Optimizasyon seviyesi (O0, O1, O2, O3)
    llvm::CodeGenOpt::Level optLevel = llvm::CodeGenOpt::None; // Şimdilik optimizasyon yok (O0)
    // Production derleyici için bu seviye dışarıdan ayarlanmalıdır.

    // Relocation Model (Reloc::PIC, Reloc::Static, Reloc::DynamicNoPIC vb.)
    // Kod modeli (CodeModel::Small, CodeModel::Kernel, CodeModel::Large vb.)
    auto relocModel = llvm::Reloc::PIC_; // Position-Independent Code
    auto codeModel = llvm::CodeModel::Small;

    return std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
        TargetTriple, // Hedef üçlemesi
        CPU,          // CPU adı
        Features,     // CPU özellikleri
        options,      // Hedef seçenekleri
        relocModel,   // Relocation model
        codeModel,    // Kod modeli
        optLevel      // Optimizasyon seviyesi
    ));
}

// Çıktı dosyası için raw_pwrite_stream oluşturur
std::unique_ptr<llvm::raw_pwrite_stream> CodeGen::createOutputStream(const std::string& filename) {
    std::error_code errorCode;
    // Çıktı dosyasını yazma modu (+ binary mode) ile aç
    auto os = std::make_unique<llvm::raw_file_ostream>(filename, errorCode, llvm::sys::fs::OF_None); // OF_None genellikle yeterlidir

    if (errorCode) {
        Reporter.reportError(SourceLocation(), "Çıktı dosyası açılamadı: '" + filename + "'. Hata: " + errorCode.message());
        return nullptr;
    }
    return os;
}

// LLVM Modülünü alıp belirtilen dosyaya makine kodu üretir
bool CodeGen::generate(llvm::Module& module, const std::string& outputFilename) {
    if (!TargetMach) {
        Reporter.reportError(SourceLocation(), "Kod üretimi: Hedef makine geçerli değil.");
        return false; // Hedef makine oluşturulamadı
    }

    // Çıktı dosyasını aç
    auto os = createOutputStream(outputFilename);
    if (!os) {
        return false; // Dosya açılamadı
    }

    // Çıktı türünü LLVM formatına çevir
    llvm::CodeGenFileType fileType = getLLVMFileType();

    // Legacy PassManager'ı kullan (daha basit bir yaklaşım)
    llvm::legacy::PassManager passManager;

    // Hedef makineye, çıktıyı belirtilen dosyaya yazacak gerekli pass'leri ekle
    // verify=false çünkü Module zaten IRGenerator'da doğrulanmıştır.
    if (TargetMach->addPassesToEmitFile(passManager, *os, fileType, true)) { // true: DisableVerify
        Reporter.reportError(SourceLocation(), "Kod üretimi: Hedef makine çıktı üretme pass'leri ekleyemiyor.");
        os-> հատError(); // Dosya akışındaki hataları kontrol et
        os->close();
        return false;
    }

    // PassManager'ı çalıştır (IR -> Makine kodu dönüşümü gerçekleşir)
    passManager.run(module);

    // Akışı boşalt ve kapat
    os->flush();
    os->close();

    // Dosya akışında hata olup olmadığını kontrol et
    if (os->has_error()) {
         Reporter.reportError(SourceLocation(), "Çıktı dosyasına yazarken hata oluştu.");
         return false;
    }


     std::cerr << "DEBUG: Code generated successfully to " << outputFilename << std::endl; // Debug
    return true; // Başarılı
}

// Çıktı türünü llvm::CodeGenFileType'a çevirir
llvm::CodeGenFileType CodeGen::getLLVMFileType() const {
    switch (OutputKind) {
        case OutputType::Assembly: return llvm::CodeGenFileType::CGFT_AssemblyFile;
        case OutputType::Object:   return llvm::CodeGenFileType::CGFT_ObjectFile;
        default:
            // Bilinmeyen tür, hata raporla ve varsayılan döndür
            Reporter.reportError(SourceLocation(), "Kod üretimi: Bilinmeyen çıktı türü.");
            return llvm::CodeGenFileType::CGFT_AssemblyFile; // Varsayılan
    }
}


} // namespace codegen
} // namespace dppc

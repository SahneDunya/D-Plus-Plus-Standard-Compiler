#ifndef DPP_COMPILER_CODEGEN_CODEGEN_H
#define DPP_COMPILER_CODEGEN_CODEGEN_H

#include "../utils/ErrorReporter.h" // Hata raporlama için

// LLVM Başlıkları (Backend için gerekli olanlar)
#include "llvm/IR/Module.h" // LLVM Module için
#include "llvm/Target/TargetMachine.h" // TargetMachine için
#include "llvm/Support/CodeGen.h" // CodeGenFileType için
#include "llvm/Support/raw_ostream.h" // raw_ostream için

#include <string>
#include <memory> // std::unique_ptr için

// İleri bildirimler
namespace llvm {
    class Target;
    class TargetOptions;
    class TargetMachine;
    class Module;
}

namespace dppc {
namespace codegen {

// Çıktı Dosyası Türleri
enum class OutputType {
    Assembly, // .s dosyası
    Object,   // .o dosyası
};

// Kod Üretici Sınıfı
class CodeGen {
private:
    ErrorReporter& Reporter; // Hata raporlama

    std::string TargetTriple; // Hedef üçlemesi (örn: "riscv64-unknown-elf")
    std::string CPU;          // Hedef CPU (örn: "generic")
    std::string Features;     // Hedef özellikler (örn: "+m")
    OutputType OutputKind;    // Üretilecek çıktı türü (Assembly veya Object)

    // LLVM Hedef Makine Nesnesi
    // Bu nesne, hedef mimariye özgü bilgileri ve kod üretim yeteneklerini içerir.
    std::unique_ptr<llvm::TargetMachine> TargetMach;

    // Yardımcı Fonksiyonlar
    // Hedef üçlemesine göre LLVM Target'ı bulur
    const llvm::Target* findTarget();
    // LLVM Target'a göre TargetMachine oluşturur
    std::unique_ptr<llvm::TargetMachine> createTargetMachine(const llvm::Target* target);
    // Çıktı dosyası için raw_pwrite_stream oluşturur
    std::unique_ptr<llvm::raw_pwrite_stream> createOutputStream(const std::string& filename);


public:
    // Constructor
    // TargetTriple, CPU, Features ve OutputKind genellikle komut satırı argümanlarından gelir.
    CodeGen(ErrorReporter& reporter, const std::string& targetTriple, const std::string& cpu, const std::string& features, OutputType outputKind);

    // Yıkıcı (TargetMach unique_ptr sayesinde otomatik temizlenir)
    ~CodeGen() = default;

    // LLVM Modülünü alıp belirtilen dosyaya makine kodu üretir
    // Başarılı olursa true, hata olursa false döner
    bool generate(llvm::Module& module, const std::string& outputFilename);

    // Çıktı türünü llvm::CodeGenFileType'a çevirir
    llvm::CodeGenFileType getLLVMFileType() const;
};

} // namespace codegen
} // namespace dppc

#endif // DPP_COMPILER_CODEGEN_CODEGEN_H

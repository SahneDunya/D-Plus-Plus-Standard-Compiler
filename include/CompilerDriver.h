#ifndef DPP_COMPILER_DRIVER_COMPILERDRIVER_H
#define DPP_COMPILER_DRIVER_COMPILERDRIVER_H

#include "../utils/ErrorReporter.h" // Hata raporlama için
#include "../lexer/Lexer.h"         // Lexer için
#include "../parser/Parser.h"       // Parser için
#include "../sema/SemanticAnalyzer.h"// Semantic Analyzer için
#include "../irgen/IRGenerator.h.h" // IR Generator için
#include "../codegen/CodeGen.h"     // Code Generator için
#include "../sema/SymbolTable.h"    // Symbol Table için
#include "../sema/TypeSystem.h"     // Type System için


#include <vector>
#include <string>
#include <memory> // std::unique_ptr için
#include <iostream> // Girdi/çıktı için (debug amaçlı)
#include <fstream> // Dosya okuma için

// LLVM Başlatma fonksiyonları için (genellikle sadece bir kez çağrılır)
// Bu başlıkları CodeGen.cpp yerine burada veya main.cpp'de çağırmak daha mantıklıdır.
#include <llvm/Support/TargetSelect.h>


namespace dppc {

// Derleyici Sürücüsü Sınıfı
class CompilerDriver {
private:
    // Komut satırı seçenekleri
    std::vector<std::string> InputFiles; // Giriş D++ kaynak dosyaları (.d++)
    std::string OutputFile;              // Çıktı dosyası (assembly, object veya executable)
    std::string TargetTriple;            // Hedef üçlemesi (varsayılan veya kullanıcıdan)
    std::string CPU;                     // Hedef CPU (varsayılan veya kullanıcıdan)
    std::string Features;                // Hedef özellikler (varsayılan veya kullanıcıdan)
    codegen::OutputType OutputKind;      // Üretilecek çıktı türü (Assembly, Object, Executable)
    int OptimizationLevel;               // Optimizasyon seviyesi (0, 1, 2, 3)

    ErrorReporter Reporter; // Hata raporlama nesnesi

    // Yardımcı Fonksiyonlar
    // Komut satırı argümanlarını ayrıştırır
    bool parseCommandLineArgs(int argc, char* argv[]);
    // Varsayılan hedef bilgilerini alır
    void getDefaultTarget();
    // Çıktı türünü string'den enum'a çevirir
    codegen::OutputType stringToOutputType(const std::string& typeStr);

    // Derleme aşamalarını çağıran fonksiyonlar (her dosya için)
    std::unique_ptr<dppc::parser::ast::SourceFileAST> runLexerAndParser(const std::string& filename);
    bool runSemanticAnalysis(dppc::parser::ast::SourceFileAST& ast, sema::SymbolTable& symbolTable, sema::TypeSystem& typeSystem);
    std::unique_ptr<llvm::Module> runIRGeneration(dppc::parser::ast::SourceFileAST& ast, sema::SymbolTable& symbolTable, sema::TypeSystem& typeSystem, const std::string& moduleName);
    bool runCodeGeneration(llvm::Module& module, const std::string& outputFilename);
    bool runLinker(const std::vector<std::string>& objectFiles, const std::string& executableName);

    // Geçici dosyaları takip etme
    std::vector<std::string> TemporaryFiles;
    void addTemporaryFile(const std::string& filepath);
    void cleanupTemporaryFiles();


public:
    // Constructor
    CompilerDriver();

    // Yıkıcı: Geçici dosyaları temizler
    ~CompilerDriver();

    // Derleme sürecini başlatan ana fonksiyon
    // Komut satırı argümanlarını alır ve tüm aşamaları çalıştırır.
    int run(int argc, char* argv[]);
};

} // namespace dppc

#endif // DPP_COMPILER_DRIVER_COMPILERDRIVER_H
